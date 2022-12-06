/*
 * Copyright (C) 2019 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Author: Sungbae Yoo <sungbae.yoo@samsung.com>
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3, or (at your option) any later
 * version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License at
 * <http://www.gnu.org/licenses/>.
 */

#include <functional>
#include <iostream>

#include "xcfi.h"
#include "preference.h"
#include "pass/icv.h"
#include "pass/armv8_3/icv-wrap.h"
#include "pass/armv8_3/icv-context.h"
#include "util/gimple.h"
#include "util/aarch64.h"

#define BUILTIN_FUNC_NAME "__builtin_pac_get_context"

namespace {

tree getGlobalPACTargetList() {
	tree targets = NULL_TREE;
	varpool_node *node;

	FOR_EACH_VARIABLE (node) {
		tree v = node->decl;

		if (is_vtable(v) || is_rtti(v))
			continue;

		tree initial = DECL_INITIAL(v);
		if (!initial || TREE_CLOBBER_P(initial))
			continue;

		if (TREE_CODE(initial) == INTEGER_CST ||
				TREE_CODE(initial) == STRING_CST)
			continue;

		std::string name = get_tree_name(v);
		if (name.find("__gthread_active_") == 0 ||
				name.find("__ksymtab_") == 0 ||
				name.find("__exitcall_") == 0 ||
				name.find("dm_dax_") == 0 ||
				name.find("_kbl_addr_") == 0)
			continue;

		if (PREF.isTargetBlacked(name)) {
			continue;
		}

		bool isMutable = true;

		// some sections might not want mutable variables
		if (decl_section_name(v) != NULL) {
			isMutable = false;
			std::string section = decl_section_name(v);
			if (section == ".discard.addressable" ||
					section == "\".discard.addressable\"") {
				continue;
			}
		}

		iterate_constructor(v, initial, [&v, &targets, isMutable]
											(tree var, tree val) {
			if (TREE_CODE(val) == NOP_EXPR) {
				val = TREE_OPERAND(val, 0);
			}
			if (TREE_CODE(val) == ADDR_EXPR) {
				val = TREE_OPERAND(val, 0);
			}
			if (TREE_CODE(val) == FUNCTION_DECL) {
				tree addr = build_fold_addr_expr(var);
				targets = chainon(targets, build_tree_list(NULL_TREE, addr));

				long long sig = getBuildtimeContext(val, var);
				tree context = build_int_cst(long_long_integer_type_node, sig);
				targets = chainon(targets, build_tree_list(NULL_TREE, context));

				//TODO : Only must add runtimeCtx that is also initialized
				//		If not, the exception must be handled
				tree runtimeCtx = getRuntimeContext(var);
				if (runtimeCtx == NULL) {
					 runtimeCtx = build_int_cst(ptr_type_node, 0);
				 }
				 targets = chainon(targets, build_tree_list(NULL_TREE, runtimeCtx));

				if (isMutable) {
					TREE_READONLY(v) = 0;
				}
			}
		});
	}

	return targets;
}

void pass_execution_cb(void *event_data, void *user_data)
{
	static bool isFirst = true;
	if (isFirst) {
		isFirst = false;

		varpool_node *node;
		FOR_EACH_VARIABLE (node) {
			tree v = node->decl;
			if (get_tree_name(v) == "__global_pac_list") {
				return;
			}
		}

		tree globals = getGlobalPACTargetList();
		if (list_length(globals)) {
			tree type = build_array_type_nelts(ptr_type_node, list_length(globals));
			tree initial = build_constructor_from_list(type, globals);
			TREE_CONSTANT(initial) = 1;
			TREE_STATIC(initial) = 1;

			tree var = add_global_variable(type, "__global_pac_list", initial);
			set_decl_section_name(var, ".pac.table");
			TREE_READONLY(var) = 1;
		}
	}
}

const attribute_spec user_attr = {
	"user",/* name */
	0,     /* minimum num of arguments */
	0,     /* maximum num of arguments */
	false, /* decl_required */
	false, /* type_required */
	false, /* function_type_required */
	NULL,  /* handler */
	false  /* affects_type_identity */
};

void register_attributes(void *event_data, void *user_data)
{
	registerBindAttribute(event_data, user_data);
	register_attribute(&user_attr);
}

const pass_data xcfi_armv8_3_icv_gimple_pass_data = {
	GIMPLE_PASS,        /* type */
	"xcfi-armv8_3_icv",       /* name */
	OPTGROUP_NONE,      /* optinfo_flags */
	TV_NONE,            /* tv_id */
	PROP_gimple_any,    /* properties_required */
	0,                  /* properties_provided */
	0,                  /* properties_destroyed */
	0,                  /* todo_flags_start */
	0                   /* todo_flags_finish */
};

class xcfi_armv8_3_icv_gimple_pass : public gimple_opt_pass {
protected:
	bool isBuiltinFuncCall(gimple *stmt);
	tree getFuncDecl(gimple *stmt);

	void wrapPACtoLocal(function * f);

	void handleAssign(gimple_stmt_iterator &it);
	void handleCall(gimple_stmt_iterator &it);
	void handleCond(gimple_stmt_iterator &it);

	void handleStatement(gimple_stmt_iterator &it);

public:
	xcfi_armv8_3_icv_gimple_pass(gcc::context * ctx);
	virtual xcfi_armv8_3_icv_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
};

xcfi_armv8_3_icv_gimple_pass::xcfi_armv8_3_icv_gimple_pass(gcc::context * ctx)
 : gimple_opt_pass(xcfi_armv8_3_icv_gimple_pass_data, ctx)
{
}

xcfi_armv8_3_icv_gimple_pass *xcfi_armv8_3_icv_gimple_pass::clone()
{
	return this;
}

bool xcfi_armv8_3_icv_gimple_pass::isBuiltinFuncCall(gimple *stmt)
{
	tree var = gimple_call_fn(stmt);
	if (var != NULL_TREE && TREE_CODE(var) == ADDR_EXPR) {
		tree func = TREE_OPERAND(var, 0);
		if (TREE_CODE(func) == FUNCTION_DECL) {
			std::string name = get_tree_name(func);
			if (name == BUILTIN_FUNC_NAME &&
					gimple_call_num_args(stmt) == 2) {
				return true;
			}
		}
	}
	return false;
}

tree xcfi_armv8_3_icv_gimple_pass::getFuncDecl(gimple *stmt)
{
	tree var = gimple_call_fn(stmt);
	if (var != NULL_TREE && TREE_CODE(var) == ADDR_EXPR) {
		tree func = TREE_OPERAND(var, 0);
		if (TREE_CODE(func) == FUNCTION_DECL) {
			return func;
		}
	}
	return NULL_TREE;
}

void xcfi_armv8_3_icv_gimple_pass::wrapPACtoLocal(function *func) {
	unsigned int i;
	tree v;

	basic_block bb = ENTRY_BLOCK_PTR_FOR_FN(func)->next_bb;
	gimple_stmt_iterator it = gsi_start_bb(bb);

	FOR_EACH_VEC_SAFE_ELT(func->local_decls, i, v) {
		tree initial = DECL_INITIAL(v);
		if (!initial || TREE_CLOBBER_P(initial))
			continue;

		if (TREE_CODE(initial) == INTEGER_CST ||
				TREE_CODE(initial) == STRING_CST)
			continue;

		bool isMutable = false;
		bool isStatic = TREE_STATIC(v);

		iterate_constructor(v, initial, [&it, &isMutable, &isStatic, this]
				(tree var, tree val) {
			if (TREE_CODE(val) == NOP_EXPR) {
				val = TREE_OPERAND(val, 0);
			}
			if (TREE_CODE(val) == ADDR_EXPR) {
				val = TREE_OPERAND(val, 0);
			}
			if (TREE_CODE(val) == FUNCTION_DECL) {
				if (!PREF.isPACBlacked(get_tree_name(val))) {
					if (isStatic) {
						//Avoid to apply duplicated PAC on static variable
						//This is handled by checking assignments
						tree addr = build_fold_addr_expr(val);
						gassign *ass = gimple_build_assign(var, addr);
						gsi_insert_before(&it, ass, GSI_SAME_STMT);
					} else {
						gimple *pac = gimple_build_PAC(var,
												getContext(it, val, var));
						gsi_insert_before(&it, pac, GSI_SAME_STMT);
					}
					isMutable = true;
				}
			}
		});

		if (isMutable) {
			TREE_READONLY(v) = 0;
		}
	}
}

void xcfi_armv8_3_icv_gimple_pass::handleAssign(gimple_stmt_iterator &it)
{
	gimple *stmt = gsi_stmt(it);

	switch (gimple_num_ops(stmt)) {
	case 2:
	{
		tree lhs = gimple_assign_lhs(stmt);
		tree rhs = gimple_assign_rhs1(stmt);

		tree lhsType = TREE_TYPE(lhs);
		tree rhsType = TREE_TYPE(rhs);

		bool isLhsPointer = TREE_CODE(lhsType) == POINTER_TYPE;
		bool isRhsPointer = (TREE_CODE(rhsType) == POINTER_TYPE &&
								TREE_CODE(rhs) != INTEGER_CST);
		bool isLhsFuncPointer = getBuildtimeContext(lhs) != 0;
		bool isRhsFuncPointer = getBuildtimeContext(rhs) != 0;

		if (isLhsPointer && typedef_variant_p(lhsType)) {
			tree typeDef = TYPE_NAME(lhsType);
			for (tree attr_list = DECL_ATTRIBUTES(typeDef);
					attr_list != NULL_TREE;
					attr_list = TREE_CHAIN(attr_list)) {
				tree attr = TREE_PURPOSE(attr_list);
				if (TREE_CODE(attr) == IDENTIFIER_NODE &&
						get_tree_name(attr) == "user") {
					//user pointer will be handled like integer
					isLhsPointer = false;
					isLhsFuncPointer = false;
				}
			}
		}

		if (isRhsPointer && typedef_variant_p(rhsType)) {
			tree typeDef = TYPE_NAME(rhsType);
			for (tree attr_list = DECL_ATTRIBUTES(typeDef);
					attr_list != NULL_TREE;
					attr_list = TREE_CHAIN(attr_list)) {
				tree attr = TREE_PURPOSE(attr_list);
				if (TREE_CODE(attr) == IDENTIFIER_NODE &&
						get_tree_name(attr) == "user") {
					//user pointer will be handled like integer
					isRhsPointer = false;
					isRhsFuncPointer = false;
				}
			}
		}

		// Just ignore when NULL is assigned
		// for example : something = NULL
		if (TREE_CODE(rhs) == INTEGER_CST) {
			if (TREE_INT_CST_LOW(rhs) == 0) {
				break;
			}
		}

		// AUT when function pointer is casted for arthimetic operations
		// for example : integer = func_pointer
		if (!isLhsPointer && isRhsFuncPointer) {
			if (TREE_CODE(rhs) == ADDR_EXPR) {
				tree func = TREE_OPERAND(rhs, 0);
				if (TREE_CODE(func) == FUNCTION_DECL) {
					break;
				}
			}

			tree wrapped = wrapAUT(it, rhs, getContext(it, rhs, rhs), false, true);
			gimple_assign_set_rhs1(stmt, wrapped);
			gimple_set_modified(stmt, true);
		// for example : func_pointer = integer;
		} else if (isLhsFuncPointer && !isRhsPointer) {
			//TODO : Add comparation if RHS is equal to zero (unlikely),
			// Then, DO NOT PAC

			tree wrapped = wrapPAC(it, rhs, getContext(it, lhs, lhs));
			gimple_assign_set_rhs1(stmt, wrapped);
			gimple_set_modified(stmt, true);
		} else if (isLhsPointer && isRhsFuncPointer) {
			// for example : ptr = _function_;
			if (TREE_CODE(rhs) == ADDR_EXPR) {
				tree func = TREE_OPERAND(rhs, 0);
				if (TREE_CODE(func) == FUNCTION_DECL) {
					if (!PREF.isPACBlacked((get_tree_name(func)))) {
						tree wrapped = wrapPAC(it, rhs,
												getContext(it, rhs, lhs));
						gimple_assign_set_rhs1(stmt, wrapped);
						gimple_set_modified(stmt, true);
					}
				}
			// AUT and PAC for object ID based CFI
			// for example : something = object.func
			// for example : object.func = object.func
			// for example : object.func = something
			} else if (handled_component_p(rhs) || handled_component_p(lhs)) {
				long long beforectx = getBuildtimeContext(rhs, rhs);
				long long nextctx = getBuildtimeContext(rhs, lhs);

				tree lhsBind = getRuntimeContext(lhs);
				tree rhsBind = getRuntimeContext(rhs);

				if (beforectx != nextctx || lhsBind || rhsBind) {
					bool needRePAC = true;

					// Remove unnecessary PAC/AUT series for indirect call
					//
					// Actually this can make side-effects on the situation
					// that temporary values are used not only the next indirec
					// calls but also for other purporses.
					// In most cases, it wouldn't happen.
					//
					// But when it comes to comparation statements,
					// It must happen, because it's frequent to compare
					// the pointer before call its indirect calls.
					//
					// This is the reason of why it doesn't remove similar
					// PAC/AUT series on comparation statments.

					gimple_stmt_iterator nextit = it;
					gsi_next(&nextit);
					while (!gsi_end_p(nextit)) {
						gimple *nextstmt = gsi_stmt(nextit);
						if (is_gimple_call(nextstmt)) {
							if (gimple_call_fn(nextstmt) == lhs) {
								needRePAC = false;
								break;
							} else if (isBuiltinFuncCall(nextstmt)) {
								tree arg = gimple_call_arg(nextstmt, 1);
								if (arg == lhs) {
									gimple_call_set_arg(nextstmt, 1, rhs);
									gimple_set_modified(nextstmt, true);

									nextit = it;
									gsi_remove(&nextit, true);
									return;
								}
							}
						}
						gsi_next(&nextit);
					}

					if (needRePAC) {
						tree beforectxTree = getContext(it, rhs, rhs);
						tree nextctxTree = getContext(it, lhs, lhs);

						tree wrapped = wrapVerifiedRePAC(it, rhs,
												beforectxTree, nextctxTree);
						gimple_assign_set_rhs1(stmt, wrapped);
					} else {
						bool dump = true;
						if (getRetBindAttr(current_function_decl) != NULL_TREE)
							dump = false;

						tree wrapped = wrapAUT(it, rhs,
												getContext(it, rhs, rhs), dump);
						gimple_assign_set_rhs1(stmt, wrapped);

						gsi_next(&it);
						while (it.ptr != nextit.ptr) {
							handleStatement(it);
							gsi_next(&it);
						}
					}
					gimple_set_modified(stmt, true);
				}
			}
		}
		break;
	}
	case 3:
	{
		bool useNULL = false;
		for (int i = 1; i < 3; i++) {
			tree node = gimple_op(stmt, i);
			if (TREE_CODE(node) == INTEGER_CST) {
				if (TREE_INT_CST_LOW(node) == 0) {
					useNULL = true;
					break;
				}
			}
		}

		if (useNULL) {
			break;
		}

		// for example : value = (_function_ == PACed_ptr)
		for (int i = 1; i < 3; i++) {
			tree node = gimple_op(stmt, i);
			if (TREE_CODE(node) == SSA_NAME ||
					TREE_CODE(node) == PARM_DECL ||
					TREE_CODE(node) == VAR_DECL) {
				tree type = TREE_TYPE(node);
				if (TREE_CODE(type) == POINTER_TYPE) {
					type = TREE_TYPE(type);
					if (TREE_CODE(type) == FUNCTION_TYPE) {
						tree wrapped = wrapAUT(it, node,
											getContext(it, node, node), false);
						gimple_set_op(stmt, i, wrapped);
						gimple_set_modified(stmt, true);
					}
				}
			}
		}
		break;
	}
	}
}

void xcfi_armv8_3_icv_gimple_pass::handleCall(gimple_stmt_iterator &it)
{
	gimple *stmt = gsi_stmt(it);

	//Handle built-in function
	if (isBuiltinFuncCall(stmt)) {
		gimple_stmt_iterator iit = it;
		gsi_remove(&iit, true);

		tree ret = gimple_call_lhs(stmt);
		if (ret != NULL_TREE) {
			tree arg = gimple_call_arg(stmt, 1);
			tree ctx = getContext(it, arg, arg);

			arg = gimple_call_arg(stmt, 0);
			gassign *ass = gimple_build_assign(ret, BIT_XOR_EXPR, arg, ctx);

			gsi_next(&it);
			gsi_insert_before(&it, ass, GSI_NEW_STMT);
			return;
		}
	}

	// for example : ret = set_callback(_function_)
	int num_args = gimple_call_num_args(stmt);
	for (int i = 0; i < num_args; i++) {
		tree arg = gimple_call_arg(stmt, i);
		tree context = getContext(it, arg);

		if (context != 0) {
			if (TREE_CODE(arg) == ADDR_EXPR) {
				tree func = TREE_OPERAND(arg, 0);
				if (TREE_CODE(func) == FUNCTION_DECL) {
					if (!PREF.isPACBlacked((get_tree_name(func)))) {
						tree wrapper = getFuncDecl(stmt);
						bool dump = true;
						if (getRetBindAttr(wrapper) != NULL_TREE)
							dump = false;

						tree wrapped = wrapPAC(it, arg, context, dump, wrapper);  // We should use retbind here
						gimple_call_set_arg(stmt, i, wrapped);
						gimple_set_modified(stmt, true);
					}
				}
			} else if (TREE_CODE(arg) == INTEGER_CST) {
				if (TREE_INT_CST_LOW(arg) != 0) {
					tree wrapped = wrapPAC(it, arg, context, false);  // Ignore argument counting
					gimple_call_set_arg(stmt, i, wrapped);
					gimple_set_modified(stmt, true);
				}
			} else if (TREE_CODE(arg) == COMPONENT_REF) {
			}
		}
	}

	//for example : ptr (...)
	tree var = gimple_call_fn(stmt);
	if (var != NULL_TREE && (TREE_CODE(var) == SSA_NAME ||
						TREE_CODE(var) == PARM_DECL ||
						TREE_CODE(var) == VAR_DECL)) {
		bool dump = true;
		if (getRetBindAttr(current_function_decl) != NULL_TREE)
			dump = false;
		
		// [ONE MORE RULE] Global function pointers ==> we should use objbind

		tree wrapped = wrapAUT(it, var, getContext(it, var), dump);
		gimple_call_set_fn((gcall*)stmt, wrapped);
		gimple_set_modified(stmt, true);
	}
}

void xcfi_armv8_3_icv_gimple_pass::handleCond(gimple_stmt_iterator &it)
{
	gimple *stmt = gsi_stmt(it);

	bool useNULL = false;
	for (int i = 0; i < 2; i++) {
		tree node = gimple_op(stmt, i);
		if (TREE_CODE(node) == INTEGER_CST) {
			if (TREE_INT_CST_LOW(node) == 0) {
				useNULL = true;
				break;
			}
		}
	}

	if (useNULL) {
		return;
	}

	// for example : if ( _function_ == PACed_ptr )
	for (int i = 0; i < 2; i++) {
		tree node = gimple_op(stmt, i);
		if (TREE_CODE(node) == SSA_NAME ||
				TREE_CODE(node) == PARM_DECL ||
				TREE_CODE(node) == VAR_DECL) {
			tree type = TREE_TYPE(node);
			if (TREE_CODE(type) == POINTER_TYPE) {
				type = TREE_TYPE(type);
				if (TREE_CODE(type) == FUNCTION_TYPE) {
					tree wrapped = wrapAUT(it, node, getContext(it, node, node), false);
					gimple_set_op(stmt, i, wrapped);
					gimple_set_modified(stmt, true);
				}
			}
		}
	}
}

void xcfi_armv8_3_icv_gimple_pass::handleStatement(gimple_stmt_iterator &it)
{
	gimple *stmt = gsi_stmt(it);

	if (is_gimple_assign(stmt)) {
		handleAssign(it);
	} else if (is_gimple_call(stmt)) {
		handleCall(it);
	} else if (is_gimple_cond(stmt)) {
		handleCond(it);
	}
}

unsigned int xcfi_armv8_3_icv_gimple_pass::execute(function *f)
{
	if (PREF.isTargetBlacked(get_tree_name(f->decl))) {
		return 0;
	}

	wrapPACtoLocal(f);

	basic_block bb;
	FOR_ALL_BB_FN (bb, f)
		for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
				gsi_next(&it)) {
			handleStatement(it);
			bb = it.bb;
		}

	return 0;
}

class xcfi_armv8_3_icv_lto_gimple_pass : public gimple_opt_pass {
protected:
	void handleCall(gimple_stmt_iterator &it);

public:
	xcfi_armv8_3_icv_lto_gimple_pass(gcc::context * ctx);
	virtual xcfi_armv8_3_icv_lto_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
};

xcfi_armv8_3_icv_lto_gimple_pass::xcfi_armv8_3_icv_lto_gimple_pass(gcc::context * ctx)
 : gimple_opt_pass(xcfi_armv8_3_icv_gimple_pass_data, ctx)
{
}

xcfi_armv8_3_icv_lto_gimple_pass *xcfi_armv8_3_icv_lto_gimple_pass::clone()
{
	return this;
}

void xcfi_armv8_3_icv_lto_gimple_pass::handleCall(gimple_stmt_iterator &it)
{
	gimple *stmt = gsi_stmt(it);

	tree func = gimple_call_fn(stmt);

	if (func == NULL_TREE) {
		return;
	}

	func = TREE_OPERAND(func, 0);

	if (DECL_EXTERNAL(func)) {
		//add XPAC instruction when pointer goes outside
		int num_args = gimple_call_num_args(stmt);
		for (int i = 0; i < num_args; i++) {
			tree arg = gimple_call_arg(stmt, i);
			tree arg_type = TREE_TYPE(arg);

			if (TREE_CODE(arg_type) == POINTER_TYPE) {
				arg_type = TREE_TYPE(arg_type);
			}

			if (TREE_CODE(arg_type) == FUNCTION_TYPE) {
				tree wrapped = wrapXPAC(it, arg);
				gimple_call_set_arg(stmt, i, wrapped);
				gimple_set_modified(stmt, true);
			}
		}

		//Don't handle pointers come outside as return value
		//It is very complicated to be generalized.
	}
}

unsigned int xcfi_armv8_3_icv_lto_gimple_pass::execute(function *f)
{
	basic_block bb;

	if (PREF.isTargetBlacked(get_tree_name(f->decl))) {
		return 0;
	}

	FOR_ALL_BB_FN (bb, f)
		for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
				gsi_next(&it)) {

			gimple *stmt = gsi_stmt(it);

			if (is_gimple_call(stmt)) {
				handleCall(it);
			}

			bb = it.bb;
		}
	return 0;
}

} // namespace

void xcfi_register_armv8_3_icv_passes(void)
{
	struct register_pass_info pass;

	auto armv8_3_icv_pass = new xcfi_armv8_3_icv_gimple_pass(g);
    pass.pass = armv8_3_icv_pass;
	pass.reference_pass_name = "cfg";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(PREF.baseName.c_str(),
						PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);

	register_callback(PREF.baseName.c_str(),
						PLUGIN_ATTRIBUTES, register_attributes, NULL);

	register_callback(PREF.baseName.c_str(),
						PLUGIN_PASS_EXECUTION, pass_execution_cb, NULL);

	if (PREF.useLTO) {
		auto armv8_3_icv_lto_pass = new xcfi_armv8_3_icv_lto_gimple_pass(g);
    	pass.pass = armv8_3_icv_lto_pass;
		pass.reference_pass_name = "ehdisp";
		pass.ref_pass_instance_number = 1;
		pass.pos_op = PASS_POS_INSERT_BEFORE;

		register_callback(PREF.baseName.c_str(),
							PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
	}
}
