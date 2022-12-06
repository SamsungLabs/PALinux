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
#include <iostream>

#include "xcfi.h"
#include "preference.h"
#include "pass/vtv.h"
#include "util/gimple.h"

namespace {

const pass_data xcfi_vtmap_gimple_pass_data = {
	GIMPLE_PASS,        /* type */
	"xcfi-vtmap",       /* name */
	OPTGROUP_NONE,      /* optinfo_flags */
	TV_NONE,            /* tv_id */
	PROP_gimple_any,    /* properties_required */
	0,                  /* properties_provided */
	0,                  /* properties_destroyed */
	0,                  /* todo_flags_start */
	0                   /* todo_flags_finish */
};

typedef std::unordered_map<std::string, std::unordered_set<std::string>>
				string_set_map;

class xcfi_vtmap_gimple_pass : public gimple_opt_pass {
protected:
	string_set_map vtable_map;

	void update_vtable_map(std::string current, std::string parent,
									std::string pointer);
	std::string get_vtable_pointer(gimple_stmt_iterator it);
	void handle_constructor(std::string class_name, basic_block bb);

public:
	xcfi_vtmap_gimple_pass(gcc::context * ctx);
	virtual xcfi_vtmap_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
	const string_set_map &get_vtable_map() const;
};

xcfi_vtmap_gimple_pass::xcfi_vtmap_gimple_pass(gcc::context * ctx)
 : gimple_opt_pass(xcfi_vtmap_gimple_pass_data, ctx)
{
}

xcfi_vtmap_gimple_pass *xcfi_vtmap_gimple_pass::clone()
{
	return this;
}

unsigned int xcfi_vtmap_gimple_pass::execute(function *f)
{
	basic_block bb;
	tree decl = f->decl;

	if (is_constructor(decl)) {
		std::string class_name(get_class_name(decl));
		FOR_ALL_BB_FN (bb, f)
			handle_constructor(get_class_name(decl), bb);
	}

	return 0;
}

const string_set_map &xcfi_vtmap_gimple_pass::get_vtable_map() const
{
	return this->vtable_map;
}

void xcfi_vtmap_gimple_pass::update_vtable_map(std::string current,
									std::string parent, std::string vtable)
{
	static std::unordered_map<std::string, std::string> dependency_map;

	dependency_map.insert(std::make_pair(current, parent));

	vtable_map.insert(std::make_pair(current,
										std::unordered_set<std::string>()));
	auto &vtable_set = vtable_map.find(current)->second;
	vtable_set.insert(vtable);

	for (auto dep_it = dependency_map.find(parent);
			parent != ""; dep_it = dependency_map.find(parent)) {

		vtable_map.insert(std::make_pair(parent, std::unordered_set<std::string>()));
		vtable_map.find(parent)->second.insert(vtable_set.begin(),
												vtable_set.end());

		parent = dep_it->second;
	}
}

std::string xcfi_vtmap_gimple_pass::get_vtable_pointer(gimple_stmt_iterator it)
{
	std::string vtable;
	tree rhs = gimple_assign_rhs1(gsi_stmt(it));

	gimple_stmt_iterator bit = it;
	gsi_prev(&bit);
	for (; !gsi_end_p(bit); gsi_prev(&bit)) {
		gimple *stmt = gsi_stmt(bit);
		if (is_gimple_assign(stmt)) {
			if (rhs == gimple_assign_lhs(stmt)) {
				rhs = gimple_assign_rhs1(stmt);

				if (TREE_CODE(rhs) != ADDR_EXPR) {
					std::cerr << "[WARN] inappropriate vtable" << std::endl;
					continue;
				}
				vtable = get_tree_name(TREE_OPERAND(rhs, 0));

				rhs = gimple_assign_rhs2(stmt);
				if (rhs) {
					if (TREE_CODE(rhs) != INTEGER_CST) {
						std::cerr << "[WARN] inappropriate vtable index" << std::endl;
						continue;
					}
					vtable += "+" + std::to_string(TREE_INT_CST_LOW(rhs));
				}
				continue;
			}
		}
	}

	return vtable;
}

void xcfi_vtmap_gimple_pass::handle_constructor(std::string class_name, basic_block bb)
{
	for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
			gsi_next(&it)) {

		gimple *stmt = gsi_stmt(it);

		if (is_gimple_assign(stmt)) {
			tree lhs = gimple_assign_lhs(stmt);
			if (TREE_CODE(lhs) == COMPONENT_REF) {
				tree op0 = TREE_OPERAND(lhs, 0);
				tree op1 = TREE_OPERAND(lhs, 1);

				std::string parent, current;

				// Does this assign pointer into vtable pointer?
				if (TREE_CODE(op1) == FIELD_DECL) {
					current = get_tree_name(op1);
					if (current.find("_vptr.") != 0)
						continue;
					current = current.substr(sizeof("_vptr.") - 1);
				} else
					continue;

				while (TREE_CODE(op0) == COMPONENT_REF) {
					op1 = TREE_OPERAND(op0, 1);
					op0 = TREE_OPERAND(op0, 0);

					parent = current;
					current = get_tree_name(op1);
				}

				// Does this handle value in "this"?
				if (TREE_CODE(op0) == MEM_REF) {
					op0 = TREE_OPERAND(op0, 0);
					if (get_tree_name(op0) != "this")
						continue;
				} else 
					continue;

				std::string vtable = get_vtable_pointer(it);

				if (vtable == "") {
					std::cerr << "[WARN] no vtable" << std::endl;
					continue;
				}

				update_vtable_map(current, parent, vtable);
			}
		}
	}
}

////////////////////////////////////////////////////////////////

const pass_data xcfi_vtchk_gimple_pass_data = {
	GIMPLE_PASS,        /* type */
	"xcfi-vtchk",       /* name */
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

class xcfi_vtchk_gimple_pass : public gimple_opt_pass {
protected:
	const string_set_map &vtable_map;
	static tree add_assign(gimple_stmt_iterator &it, tree &lhs, const std::string &expr);

public:
	xcfi_vtchk_gimple_pass(gcc::context * ctx, const string_set_map &map);
	virtual xcfi_vtchk_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
};

xcfi_vtchk_gimple_pass::xcfi_vtchk_gimple_pass(gcc::context * ctx, const string_set_map &map)
 : gimple_opt_pass(xcfi_vtchk_gimple_pass_data, ctx), vtable_map(map)
{
}

xcfi_vtchk_gimple_pass *xcfi_vtchk_gimple_pass::clone()
{
	return this;
}

tree xcfi_vtchk_gimple_pass::add_assign(gimple_stmt_iterator &it, tree &lhs, const std::string &expr)
{
	int i = std::stoi(expr.substr(expr.find("+")));
	std::string var = expr.substr(0, expr.find("+"));

	tree addr_id = get_identifier(var.c_str());
	tree addr_decl = build_decl(UNKNOWN_LOCATION, VAR_DECL, addr_id, ptr_type_node);
	SET_DECL_ASSEMBLER_NAME(addr_decl, addr_id);
	TREE_STATIC(addr_decl) = 1;
	tree addr = build_fold_addr_expr(addr_decl);
	tree index = build_int_cst(integer_type_node, i);
	gassign *as = gimple_build_assign(lhs, POINTER_PLUS_EXPR, addr, index);
	gsi_insert_after(&it, as, GSI_NEW_STMT);

	return lhs;
}

unsigned int xcfi_vtchk_gimple_pass::execute(function *f)
{
	if (PREF.isTargetBlacked(get_tree_name(f->decl))) {
		return 0;
	}

	basic_block bb;

	FOR_ALL_BB_FN (bb, f)
		for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
				gsi_next(&it)) {

			gimple *stmt = gsi_stmt(it);

			if (is_gimple_assign(stmt)) {
				tree lhs = gimple_assign_lhs(stmt);
				tree rhs = gimple_assign_rhs1(stmt);

				if (TREE_CODE(rhs) == COMPONENT_REF) {
					tree op0 = TREE_OPERAND(rhs, 0);
					tree op1 = TREE_OPERAND(rhs, 1);

					std::string current;

					// Does this assign pointer into vtable pointer?
					if (TREE_CODE(op1) == FIELD_DECL) {
						current = get_tree_name(op1);
						if (current.find("_vptr.") != 0)
							continue;
						current = current.substr(sizeof("_vptr.") - 1);
					} else
						continue;

					while (TREE_CODE(op0) == COMPONENT_REF) {
						op1 = TREE_OPERAND(op0, 1);
						op0 = TREE_OPERAND(op0, 0);

						current = get_tree_name(op1);
					}

					if (TREE_CODE(lhs) == SSA_NAME) {
							tree orig = create_tmp_reg(ptr_type_node);
							gimple_assign_set_lhs(stmt, orig);

							gassign *ass = gimple_build_assign(lhs, orig);
							gsi_insert_after (&it, ass, GSI_SAME_STMT);
							lhs = orig;
					}

					auto vit = vtable_map.find(current);
					if (vit == vtable_map.end()) {
						std::cerr << "[WARN] no vtable mapping information" << std::endl;
						continue;
					}

					tree tmpreg = create_tmp_reg(ptr_type_node);
					auto begin = vit->second.begin(), end = vit->second.end();
					bb = add_verification_code([&begin, &end, &tmpreg]
												(gimple_stmt_iterator &pos) {
							if (begin == end)
								return (tree)NULL;

							std::string vtable = *(begin++);
							return add_assign(pos, tmpreg, vtable);
					}, lhs, it);
				}
			}
		}
	return 0;
}

};

void xcfi_register_generic_vtv_passes(void)
{
	struct register_pass_info pass;

	auto vtmap_pass = new xcfi_vtmap_gimple_pass(g);
    pass.pass = vtmap_pass;
	pass.reference_pass_name = "cfg";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(PREF.baseName.c_str(),
						PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);

	auto vtchk_pass = new xcfi_vtchk_gimple_pass(g, vtmap_pass->get_vtable_map());
	pass.pass = vtchk_pass;
	pass.reference_pass_name = "ssa";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_BEFORE;

	register_callback(PREF.baseName.c_str(),
						PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
}
