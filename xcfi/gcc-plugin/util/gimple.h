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
#ifndef __UTIL_GIMPLE_H__
#define __UTIL_GIMPLE_H__

#include <functional>
#include <string>

#include "xcfi.h"

std::string get_tree_name(tree tree);
std::string get_mangled_tree_name(tree tree);
std::string get_type_string(tree fntype);

std::string get_type_name(tree type);

bool is_constructor(tree decl);
bool is_rtti(tree decl);
bool is_vtable(tree decl);
bool is_vtable_pointer(tree ptr);

std::string get_class_name(tree method);

std::string get_tree_code_string(tree ref);
std::string get_tree_string(tree node);

std::string get_gimple_string(gimple *stmt);

static inline bool is_gimple_asm(gimple *gs) {
	return gimple_code(gs) == GIMPLE_ASM;
}

static inline bool is_gimple_cond(gimple *gs) {
	return gimple_code(gs) == GIMPLE_COND;
}

basic_block add_verification_code
					(std::function<tree(gimple_stmt_iterator&)> getCandidate,
						tree target, gimple_stmt_iterator pos);

tree add_global_variable(tree type, std::string name, tree initial);

long long get_hash_function(tree fntype);

gcall *gimple_build_call_string(std::string name, tree ret, unsigned int nargs, ...);

void iterate_constructor(tree variable, tree constructor,
						std::function<void(tree,tree)> func);

tree build_component_ref(tree object, std::string fieldName);
tree build_component_ref_with_idx(tree object, int fieldIdx);

gimple_stmt_iterator gsi_prev_loop(gimple_stmt_iterator it);

#endif // __UTIL_GIMPLE_H__
