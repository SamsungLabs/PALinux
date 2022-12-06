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

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <iostream>

#include "xcfi.h"
#include "preference.h"
#include "pass/vtv.h"
#include "util/gimple.h"

namespace {

const pass_data xcfi_icmap_gimple_pass_data = {
	GIMPLE_PASS,        /* type */
	"xcfi-icmap",       /* name */
	OPTGROUP_NONE,      /* optinfo_flags */
	TV_NONE,            /* tv_id */
	PROP_gimple_any,    /* properties_required */
	0,                  /* properties_provided */
	0,                  /* properties_destroyed */
	0,                  /* todo_flags_start */
	0                   /* todo_flags_finish */
};

typedef std::unordered_map<unsigned int, std::unordered_set<std::string>>
		uint_set_map;

class xcfi_icmap_gimple_pass : public gimple_opt_pass {
protected:
	uint_set_map icall_map;
	void update_icall_map(tree func);

public:
	xcfi_icmap_gimple_pass(gcc::context * ctx);
	virtual xcfi_icmap_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
	const uint_set_map &get_icall_map() const;
};

xcfi_icmap_gimple_pass::xcfi_icmap_gimple_pass(gcc::context * ctx)
 : gimple_opt_pass(xcfi_icmap_gimple_pass_data, ctx)
{
}

xcfi_icmap_gimple_pass *xcfi_icmap_gimple_pass::clone()
{
	return this;
}

unsigned int xcfi_icmap_gimple_pass::execute(function *f)
{
	basic_block bb;

	FOR_ALL_BB_FN (bb, f)
		for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
				gsi_next(&it)) {

			gimple *stmt = gsi_stmt(it);

			// for example : ptr = _function_;
			if (is_gimple_assign(stmt)) {
				tree rhs = gimple_assign_rhs1(stmt);
				if (TREE_CODE(rhs) == ADDR_EXPR) {
					tree func = TREE_OPERAND(rhs, 0);
					if (TREE_CODE(func) == FUNCTION_DECL) {
						update_icall_map(func);
					}
				}
			// for example : ret = set_callback(_function_)
			} else if (is_gimple_call(stmt)) {
				int num_args = gimple_call_num_args(stmt);
				for (int i = 0; i < num_args; i++) {
					tree arg = gimple_call_arg(stmt, i);
					if (TREE_CODE(arg) == ADDR_EXPR) {
						tree func = TREE_OPERAND(arg, 0);
						if (TREE_CODE(func) == FUNCTION_DECL) {
							update_icall_map(func);
						}
					}
				}
			}
	}

	return 0;
}
void xcfi_icmap_gimple_pass::update_icall_map(tree func)
{
	tree type = TREE_TYPE(func);
	unsigned int tid = TYPE_UID(type);
	std::string name = get_mangled_tree_name(func);
	icall_map.insert(std::make_pair(tid, std::unordered_set<std::string>()));
	icall_map.find(tid)->second.insert(name);
}

const uint_set_map &xcfi_icmap_gimple_pass::get_icall_map() const
{
	return this->icall_map;
}

////////////////////////////////////////////////////////////////

const pass_data xcfi_icchk_gimple_pass_data = {
	GIMPLE_PASS,        /* type */
	"xcfi-icchk",       /* name */
	OPTGROUP_NONE,      /* optinfo_flags */
	TV_NONE,            /* tv_id */
	PROP_gimple_any,    /* properties_required */
	0,                  /* properties_provided */
	0,                  /* properties_destroyed */
	0,                  /* todo_flags_start */
	0,                  /* todo_flags_finish */
};

typedef std::unordered_map<std::string, std::unordered_set<std::string>>
				string_set_map;

class xcfi_icchk_gimple_pass : public gimple_opt_pass {
protected:
	const uint_set_map &icall_map;

public:
	xcfi_icchk_gimple_pass(gcc::context * ctx, const uint_set_map &map);
	virtual xcfi_icchk_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
};

xcfi_icchk_gimple_pass::xcfi_icchk_gimple_pass(gcc::context * ctx, const uint_set_map &map)
 : gimple_opt_pass(xcfi_icchk_gimple_pass_data, ctx), icall_map(map)
{
}

xcfi_icchk_gimple_pass *xcfi_icchk_gimple_pass::clone()
{
	return this;
}

unsigned int xcfi_icchk_gimple_pass::execute(function *f)
{
	if (PREF.isTargetBlacked(get_tree_name(f->decl))) {
		return 0;
	}

	basic_block bb;

	FOR_ALL_BB_FN (bb, f) {
		for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
				gsi_next(&it)) {

			gimple *stmt = gsi_stmt(it);

			if (is_gimple_call(stmt)) {
				tree var = gimple_call_fn(stmt);
				if (var == NULL) {
					// This can occur when internal function is called
					continue;
				}

				if (TREE_CODE(var) == SSA_NAME) {
					tree orig = create_tmp_reg(ptr_type_node);
					gimple_call_set_fn((gcall*)stmt, orig);
					gassign *ass = gimple_build_assign(orig, var);
					gsi_insert_before(&it, ass, GSI_SAME_STMT);
					var = orig;
				}

				if (TREE_CODE(var) == VAR_DECL) {
					tree type = gimple_call_fntype(stmt);
					unsigned int tid = TYPE_UID(type);

					auto iit = icall_map.find(tid);
					if (iit == icall_map.end()) {
						std::cerr << "[WARN] no indirect call mapping information" << std::endl;
						continue;
					}

					auto begin = iit->second.begin(), end = iit->second.end();
					bb = add_verification_code([&begin, &end, &type]
												(gimple_stmt_iterator &pos) {
						if (begin == end)
							return (tree)NULL;

						std::string icall = *(begin++);
						tree fn_id = get_identifier(icall.c_str());
						tree fn_decl = build_decl(UNKNOWN_LOCATION, FUNCTION_DECL, fn_id, type);
						SET_DECL_ASSEMBLER_NAME(fn_decl, fn_id);
						return build_fold_addr_expr(fn_decl);;
					}, var, it);
				}
			}
		}
	}
	return 0;
}

} // namepsace

void xcfi_register_generic_icv_passes(void)
{
	struct register_pass_info pass;

	auto icmap_pass = new xcfi_icmap_gimple_pass(g);
    pass.pass = icmap_pass;
	pass.reference_pass_name = "cfg";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(PREF.baseName.c_str(),
						 PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);

	auto icchk_pass = new xcfi_icchk_gimple_pass(g, icmap_pass->get_icall_map());
	pass.pass = icchk_pass;
	pass.reference_pass_name = "ssa";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_BEFORE;

	register_callback(PREF.baseName.c_str(),
						 PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
}
