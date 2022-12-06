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

#include <iostream>

#include "xcfi.h"
#include "preference.h"
#include "pass/vtv.h"
#include "util/gimple.h"
#include "util/aarch64.h"

namespace {

const pass_data xcfi_armv8_3_vtv_gimple_pass_data = {
	GIMPLE_PASS,        /* type */
	"xcfi-armv8_3_vtv",        /* name */
	OPTGROUP_NONE,      /* optinfo_flags */
	TV_NONE,            /* tv_id */
	PROP_gimple_any,    /* properties_required */
	0,                  /* properties_provided */
	0,                  /* properties_destroyed */
	0,                  /* todo_flags_start */
	0                   /* todo_flags_finish */
};

class xcfi_armv8_3_vtv_gimple_pass : public gimple_opt_pass {
public:
	xcfi_armv8_3_vtv_gimple_pass(gcc::context * ctx);
	virtual xcfi_armv8_3_vtv_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
};

xcfi_armv8_3_vtv_gimple_pass::xcfi_armv8_3_vtv_gimple_pass(gcc::context * ctx)
 : gimple_opt_pass(xcfi_armv8_3_vtv_gimple_pass_data, ctx)
{
}

xcfi_armv8_3_vtv_gimple_pass *xcfi_armv8_3_vtv_gimple_pass::clone()
{
	return this;
}

unsigned int xcfi_armv8_3_vtv_gimple_pass::execute(function *f)
{
	if (PREF.isPACBlacked(get_tree_name(f->decl))) {
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

				if (is_vtable_pointer(lhs)) {
					tree op0 = TREE_OPERAND(lhs, 0);
					while (TREE_CODE(op0) == COMPONENT_REF) {
						op0 = TREE_OPERAND(op0, 0);
					}

					if (TREE_CODE(op0) == MEM_REF) {
							op0 = TREE_OPERAND(op0, 0);
							if (get_tree_name(op0) != "this") {
									continue;
							}
					} else {
							continue;
					}

					gimple *pac = gimple_build_PAC(rhs);
					gsi_insert_before(&it, pac, GSI_SAME_STMT);
				}

				if (is_vtable_pointer(rhs)) {
					if (TREE_CODE(lhs) == SSA_NAME) {
						tree orig = create_tmp_reg(ptr_type_node);
						gimple_assign_set_lhs(stmt, orig);

						gassign *ass = gimple_build_assign(lhs, orig);
						gsi_insert_after(&it, ass, GSI_SAME_STMT);
						lhs = orig;
					}

					gimple *aut = gimple_build_AUT(lhs);
					gsi_insert_after(&it, aut, GSI_SAME_STMT);
				}
			}
		}

	return 0;
}

class xcfi_armv8_3_vtv_lto_gimple_pass : public gimple_opt_pass {
public:
	xcfi_armv8_3_vtv_lto_gimple_pass(gcc::context * ctx);
	virtual xcfi_armv8_3_vtv_lto_gimple_pass *clone() override;
	virtual unsigned int execute(function *f) override;
};

xcfi_armv8_3_vtv_lto_gimple_pass::xcfi_armv8_3_vtv_lto_gimple_pass(gcc::context * ctx)
 : gimple_opt_pass(xcfi_armv8_3_vtv_gimple_pass_data, ctx)
{
}

xcfi_armv8_3_vtv_lto_gimple_pass *xcfi_armv8_3_vtv_lto_gimple_pass::clone()
{
	return this;
}

unsigned int xcfi_armv8_3_vtv_lto_gimple_pass::execute(function *f)
{
	basic_block bb;

	FOR_ALL_BB_FN (bb, f)
		for (gimple_stmt_iterator it = gsi_start_bb(bb); !gsi_end_p(it);
				gsi_next(&it)) {

			//TODO : remove AUT instrunction when external class vptr is derefer
		}

	return 0;
}

} // namespace

void xcfi_register_armv8_3_vtv_passes(void)
{
	struct register_pass_info pass;

	auto armv8_3_vtv_pass = new xcfi_armv8_3_vtv_gimple_pass(g);
    pass.pass = armv8_3_vtv_pass;
	pass.reference_pass_name = "cfg";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(PREF.baseName.c_str(),
						PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);

	if (PREF.useLTO) {
		auto armv8_3_vtv_lto_pass = new xcfi_armv8_3_vtv_lto_gimple_pass(g);
	    pass.pass = armv8_3_vtv_lto_pass;
		pass.reference_pass_name = "ehdisp";
		pass.ref_pass_instance_number = 1;
		pass.pos_op = PASS_POS_INSERT_BEFORE;

		register_callback(PREF.baseName.c_str(),
							 PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
	}
}
