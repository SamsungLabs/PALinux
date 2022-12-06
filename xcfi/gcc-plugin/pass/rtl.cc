/*
 * Copyright (C) 2019 Samsung Electronics Co., Ltd All Rights Reserved
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

static const pass_data xcfi_rtl_pass_data = {
	RTL_PASS,      /* type */
	"xcfi-rtl",    /* name */
	OPTGROUP_NONE, /* optinfo_flags */
	TV_NONE,       /* tv_id */
	0,             /* properties_required */
	0,             /* properties_provided */
	0,             /* propertiesd_destroyed */
	0,             /* todo_flags_start */
	0,             /* todo_flags_finish */
};

class xcfi_rtl_pass : public rtl_opt_pass {
public:
	xcfi_rtl_pass(gcc::context *ctx);

	virtual opt_pass *clone() override;
	virtual unsigned int execute(function *) override;
};


xcfi_rtl_pass::xcfi_rtl_pass(gcc::context *ctx) :
	rtl_opt_pass(xcfi_rtl_pass_data, ctx)
{
}

opt_pass *xcfi_rtl_pass::clone()
{
	return this;
}

unsigned int xcfi_rtl_pass::execute(function *)
{
//	std::cout << "RTL pass called" << std::endl;
	return 0;
}

void xcfi_register_rtl_pass(struct plugin_name_args *plugin_info)
{
	struct register_pass_info pass;

    pass.pass = new xcfi_rtl_pass(g);
	pass.reference_pass_name = "jump";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
}
