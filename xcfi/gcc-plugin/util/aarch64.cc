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

#include <string>
#include <sstream>

#include "aarch64.h"
#include "../preference.h"

namespace {

enum PACCode {
	PAC,
	PACZ,
	AUT,
	AUTZ,
	XPAC
};

std::string get_assemble(enum PACCode number)
{
	static std::string code[5];

	if (code[number].size() == 0) {
		std::string key = PREF.PACKey;
		std::ostringstream codeStream;

		if (key.size() != 0) {
			switch (number) {
			case PAC:
				codeStream << "PACI" << key << " %0, %1" << std::endl;
				break;
			case PACZ:
				codeStream << "PACIZ" << key << " %0" << std::endl;
				break;
			case AUT:
				codeStream << "AUTI" << key << " %0, %1" << std::endl;
				break;
			case AUTZ:
				codeStream << "AUTIZ" << key << " %0" << std::endl;
				break;
			case XPAC:
				codeStream << "XPACI %0" << std::endl;
				break;
			}
		} else {
			codeStream << "EOR %0, %0, #0xffff0000" << std::endl;
			codeStream << "EOR %0, %0, #0x00001ff0" << std::endl;
			codeStream << "EOR %0, %0, #0x03000000" << std::endl;
			codeStream << "EOR %0, %0, #0x0000001c" << std::endl;
			codeStream << "EOR %0, %0, #0x0003c000" << std::endl;
			codeStream << "EOR %0, %0, #0x3f000000" << std::endl;
			switch (number) {
			case PAC:
			case AUT:
				codeStream << "EOR %0, %0, %1" << std::endl;
				break;
			case PACZ:
			case AUTZ:
			case XPAC:
				codeStream << "EOR %0, %0, #0x1" << std::endl;
				break;
			}
		}
		code[number] = codeStream.str();
	}
	return code[number];
}

inline gimple *gimple_build_assemble(enum PACCode number, tree target)
{
	vec<tree, va_gc> *outputs = NULL;
	vec<tree, va_gc> *inputs = NULL;
	vec<tree, va_gc> *clobbers = NULL;

	tree output = build_tree_list(NULL_TREE, build_string(3, "=r"));
	output = chainon(NULL_TREE, build_tree_list(output, target));

	vec_safe_push(outputs, output);

	tree input = build_tree_list(NULL_TREE, build_string(2, "0"));
	input = chainon(NULL_TREE, build_tree_list(input, target));

	vec_safe_push(inputs, input);

	tree clobber = build_tree_list(NULL_TREE, build_string(7, "memory"));

	vec_safe_push(clobbers, clobber);

	gasm *gas = gimple_build_asm_vec(
		get_assemble(number).c_str(),
		inputs, outputs, clobbers, NULL);

	return gas;
}

inline gimple *gimple_build_assemble(enum PACCode number, tree target,
								tree context)
{
	vec<tree, va_gc> *outputs = NULL;
	vec<tree, va_gc> *inputs = NULL;
	vec<tree, va_gc> *clobbers = NULL;

	tree output = build_tree_list(NULL_TREE, build_string(3, "=r"));
	output = chainon(NULL_TREE, build_tree_list(output, target));

	vec_safe_push(outputs, output);

	tree input = build_tree_list(NULL_TREE, build_string(2, "r"));
	input = chainon(NULL_TREE, build_tree_list(input, context));

	vec_safe_push(inputs, input);

	input = build_tree_list(NULL_TREE, build_string(2, "0"));
	input = chainon(NULL_TREE, build_tree_list(input, target));

	vec_safe_push(inputs, input);

	tree clobber = build_tree_list(NULL_TREE, build_string(7, "memory"));

	vec_safe_push(clobbers, clobber);

	gasm *gas = gimple_build_asm_vec(
		get_assemble(number).c_str(),
		inputs, outputs, clobbers, NULL);

	return gas;
}

}; // namespace

gimple *gimple_build_PAC(tree target, tree context)
{
	if (context == NULL_TREE) {
		return gimple_build_assemble(PACZ, target);
	}

	return gimple_build_assemble(PAC, target, context);
}

gimple *gimple_build_AUT(tree target, tree context)
{
	if (context == NULL_TREE) {
		return gimple_build_assemble(AUTZ, target);
	}

	return gimple_build_assemble(AUT, target, context);
}

gimple *gimple_build_XPAC(tree target)
{
	return gimple_build_assemble(XPAC, target);
}

bool is_gimple_PAC(gasm *gas)
{
	return (std::string(gas->string) == get_assemble(PAC)) ||
			(std::string(gas->string) == get_assemble(PACZ));
}

bool is_gimple_AUT(gasm *gas)
{
	return (std::string(gas->string) == get_assemble(AUT)) ||
			(std::string(gas->string) == get_assemble(AUTZ));
}

bool is_gimple_XPAC(gasm *gas)
{
	return (std::string(gas->string) == get_assemble(XPAC));
}
