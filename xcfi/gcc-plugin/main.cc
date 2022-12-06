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
#include <sstream>

#include <gcc-plugin.h>
#include <config.h>

#include "preference.h"
#include "pass/vtv.h"
#include "pass/icv.h"

#define REQUIRED_VERSION	"6.2"

// We must assert that this plugin is GPL compatible
int plugin_is_GPL_compatible = 1;

static struct plugin_info my_gcc_plugin_info = {
	"1.0",
	"This is a very simple plugin"
};

static void xcfi_finish(void *gcc_data, void *user_data)
{
}

static void xcfi_register_plugin_callback(struct plugin_name_args *args)
{
	register_callback(args->base_name, PLUGIN_INFO, NULL, &my_gcc_plugin_info);
	register_callback(args->base_name, PLUGIN_FINISH, xcfi_finish, NULL);
}

static int xcfi_version_check(struct plugin_gcc_version *version)
{
	if (std::stof(version->basever) >= std::stof(REQUIRED_VERSION)) {
		return 1;
	}
	return 0;
}

int plugin_init(struct plugin_name_args *args, struct plugin_gcc_version *version)
{
	if (!xcfi_version_check(version)) {
		std::cerr << "warning: This GCC plugin requires version >= "
					<< REQUIRED_VERSION
					<< "(This GCC plugin is built on version "
					<< version->basever << ")" << std::endl;
		return 1;
	}

	xcfi_register_plugin_callback(args);

	Preference::setArgument(version, args);

	if (Preference::getInstance().enableVTV) {
		xcfi_register_vtv_passes();
	}

	if (Preference::getInstance().enableICV) {
		xcfi_register_icv_passes();
	}

	return 0;
}

