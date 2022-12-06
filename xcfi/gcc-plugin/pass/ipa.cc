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

static const pass_data xcfi_simple_ipa_pass_data = {
	SIMPLE_IPA_PASS,   /* type */
	"xcfi-simple-ipa", /* name */
	OPTGROUP_NONE,     /* opt_info flags */
	TV_NONE,           /* tv_id */
	0,                 /* properties_required */
	0,                 /* properties_provided */
	0,                 /* properties_destroyed */
	0,                 /* todo_flags_start */
	0                  /* todo_flags_finish */
};

class xcfi_simple_ipa_pass : public simple_ipa_opt_pass {
public:
	xcfi_simple_ipa_pass(gcc::context *ctx);

	virtual opt_pass *clone() override;
	virtual unsigned int execute(function *) override;
};

xcfi_simple_ipa_pass::xcfi_simple_ipa_pass(gcc::context * ctx) :
	simple_ipa_opt_pass(xcfi_simple_ipa_pass_data, ctx)
{
}

opt_pass *xcfi_simple_ipa_pass::clone()
{
	return this;
}

unsigned int xcfi_simple_ipa_pass::execute(function *)
{
	std::cout << "Simple IPA pass called" << std::endl;
	return 0;
}

const pass_data xcfi_ipa_pass_data = {
	IPA_PASS,       /* type */
	"xcfi-ipa",     /* name */
	OPTGROUP_NONE,  /* optinfo_flags */
	TV_NONE,        /* tv_id */
	0,              /* properties_required */
	0,              /* properties_provided */
	0,              /* properties_destroyed */
	0,              /* todo_flags_start */
	0               /* todo_flags_finish */
};

class xcfi_ipa_pass : public ipa_opt_pass_d {
public:
	xcfi_ipa_pass(gcc::context *ctxt);
	virtual bool gate(function *);
	virtual unsigned int execute(function *);
};

xcfi_ipa_pass::xcfi_ipa_pass(gcc::context *ctxt) :
	ipa_opt_pass_d(xcfi_ipa_pass_data, ctxt,
					NULL, /* generate_summary */
					NULL, /* write_summary */
					NULL, /* read_summary */
					NULL, /* write_optimization_summary */
					NULL, /* read_optimization_summary */
					NULL, /* stmt_fixup */
					0,    /* function_transform_todo_flags_start */
					NULL, /* function_transform */
					NULL) /* variable_transform */
{
}

/* opt_pass methods: */
bool xcfi_ipa_pass::gate(function *f)
{
	/* Do not re-run on ltrans stage.  */
	return !flag_ltrans;
}

unsigned int xcfi_ipa_pass::execute(function *f)
{
//	std::cout << "IPA pass called" << std::endl;
	return 0;
}

void xcfi_register_ipa_pass(struct plugin_name_args *plugin_info)
{
	struct register_pass_info pass;

    pass.pass = new xcfi_ipa_pass(g);
	pass.reference_pass_name = "inline";
	pass.ref_pass_instance_number = 1;
	pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
}
