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
#include "icv-context.h"

#include "preference.h"
#include "util/gimple.h"
#include "util/aarch64.h"

#include <iostream>

#define ATTRIBUTE_BIND "pacbind"
#define ATTRIBUTE_RET_BIND "retbind"

namespace {

const attribute_spec bind_attr = {
	ATTRIBUTE_BIND, /* name */
	2,		/* minimum num of arguments */
	-1,		/* maximum num of arguments */
	false,	/* decl_required */
	true,	/* type_required */
	false,	/* function_type_required */
	NULL,	/* handler */
	false	/* affects_type_identity */
};

const attribute_spec ret_bind_attr = {
	ATTRIBUTE_RET_BIND, /* name */
	-1,		/* minimum num of arguments */
	-1,		/* maximum num of arguments */
	false,	/* decl_required */
	false,	/* type_required */
	false,	/* function_type_required */
	NULL,	/* handler */
	false	/* affects_type_identity */
};

//Reconstruct component reference with insert temporary register
//because SSA can be created properly.
tree reconstructTree(gimple_stmt_iterator &it, tree target) {
	bool isAddr = false;
	if (TREE_CODE(target) == ADDR_EXPR) {
		tree op0 = TREE_OPERAND(target, 0);
		if (TREE_CODE(op0) == COMPONENT_REF) {
			target = op0;
			isAddr = true;
		}
	}

	switch (TREE_CODE(target)) {
	case COMPONENT_REF:
	{
		tree field = TREE_OPERAND(target, 1);
		target = TREE_OPERAND(target, 0);

		target = reconstructTree(it, target);
		target = build3(COMPONENT_REF, TREE_TYPE(field), target, field, NULL_TREE);
		break;
	}
	case MEM_REF:
	{
		target = TREE_OPERAND(target, 0);
		tree targetType = TREE_TYPE(target);

		tree tmpReg = create_tmp_reg(targetType);
		gassign *ass = gimple_build_assign(tmpReg, target);
		gsi_insert_before(&it, ass, GSI_SAME_STMT);

		target = build2(MEM_REF, TREE_TYPE(targetType), tmpReg,
							build_int_cst(targetType, 0));
		break;
	}
	default:
		break;
	}

	if (isAddr) {
		target = build_fold_addr_expr(target);
	}

	return target;
}

} // namespace

long long getFirstBuildtimeContext(tree target) {
	if (target == NULL_TREE) {
		return 0;
	}

	tree type = TREE_TYPE(target);

	if (TREE_CODE(type) == POINTER_TYPE) {
		type = TREE_TYPE(type);
	}

	if (TREE_CODE(type) != FUNCTION_TYPE) {
		return 0;
	}

	long long result = get_hash_function(type);
	return result;
}

long long getBuildtimeContext(tree target, tree object, bool record) {
	if (target == NULL_TREE) {
		return 0;
	}

	tree type = TREE_TYPE(target);

	if (TREE_CODE(type) == POINTER_TYPE) {
		type = TREE_TYPE(type);
	}

	if (TREE_CODE(type) != FUNCTION_TYPE) {
		return 0;
	}

	long long result = get_hash_function(type);
	std::string typeStr = get_type_string(type);
	bool recorded = false;

#if 0
	if (object != NULL_TREE && handled_component_p(object)) {
		if (result == getBuildtimeContext(object, NULL_TREE)) {
			tree objectType;
			do {
				object = TREE_OPERAND(object, 0);
				objectType = TREE_TYPE(object);
				if (TREE_CODE(objectType) == RECORD_TYPE ||
						TREE_CODE(objectType) == UNION_TYPE) {
					break;
				}
			} while (handled_component_p(object));

			std::string objName = get_type_name(objectType);

			long long type = result;
			if (objName.size() != 0 &&
					// don't know why real object type is differnt from C code
					// when the follow code is built.
					//  - drivers/clocksource/timer-probe.c
					// may be some bugs in GCC.
					objName != "of_device_id") {
				static std::hash<std::string> hasher;
				std::string concatName = typeStr + objName;
				
				result = hasher(concatName);

				if (PREF.dump != nullptr) {
					PREF.dump->setObjName(objName);
					recorded = true;
				}
			} 
		}
	}
#endif

	return result;
	// [test]
	//return 1;
	//if (result != 0)
	//	result = 0xfaba9db9c95abcb8; // full but same value for performance evaluation without issues
	//return result;
}

tree getRuntimeContext(tree object)
{
	if (object == NULL_TREE) {
		return NULL_TREE;
	}

	tree field = NULL_TREE, objectType;
	while (handled_component_p(object)) {
		field = TREE_OPERAND(object, 1);
		object = TREE_OPERAND(object, 0);
		objectType = TREE_TYPE(object);
		if (TREE_CODE(objectType) == RECORD_TYPE ||
				TREE_CODE(objectType) == UNION_TYPE) {
			break;
		}
	}

	if (field == NULL_TREE || TREE_CODE(field) != FIELD_DECL) {
		return NULL_TREE;
	}

	std::string fieldName = get_tree_name(field);

	// 1. apply runtime context with manual annotation
	for (tree attr_list = TYPE_ATTRIBUTES(objectType); attr_list != NULL_TREE;
			attr_list = TREE_CHAIN(attr_list)) {
		tree attr = TREE_PURPOSE(attr_list);

		if (TREE_CODE(attr) == IDENTIFIER_NODE &&
				get_tree_name(attr) == ATTRIBUTE_BIND) {
			tree args = TREE_VALUE(attr_list);

			tree arg = TREE_VALUE(args);
			if (TREE_CODE(arg) != STRING_CST) {
				error("'pacbind' attribute needs string typed-arguments on %qF",
						object);
			}
			std::string hintName = TREE_STRING_POINTER(arg);
			bool isHintAddr = false;
			if (hintName.find("&") == 0) {
				isHintAddr = true;
				hintName = hintName.substr(1);
			}

			for (args = TREE_CHAIN(args); args != NULL_TREE;
					args = TREE_CHAIN(args)) {

				arg = TREE_VALUE(args);
				if (TREE_CODE(arg) != STRING_CST) {
					error("'pacbind' attribute needs string typed-arguments on %qF",
							object);
				}
				std::string targetName = TREE_STRING_POINTER(arg);

				if (fieldName == targetName || targetName == "*") {
					tree result = build_component_ref(object, hintName);
					if (result == NULL_TREE) {
						error("field '%s' doesn't exist in %qF",
								hintName.c_str(), object);
					}

					if (isHintAddr) {
						result = build_fold_addr_expr(result);
					}

					return result;
				}
			}
		}
	}

	// When no manual annotation is given, it tries to reflect annotation guide deriven from context analyzer
	// 2. apply runtime context with guides coming from context analyzer
	std::string objectTypeName = get_type_name(objectType);
	ObjbindGuide *guide = PREF.getObjbindGuide(objectTypeName);
	if (guide && objectTypeName == guide->structName) {
		// [ISSUE] applying field-objbind to struct.irqaction causes an compiler error.. workaround--
		bool isAddrBind = guide->isAddrBind;
		if (objectTypeName == "irqaction" ||
			objectTypeName == "cpu_operations" ||
			objectTypeName == "generic_pm_domain") {
			isAddrBind = true;
		}

		if (isAddrBind) {  // address binding
			// field idx-0
			std::cerr << "Address objbind from context analyzer: " << guide->structName << std::endl;
			tree result = build_component_ref_with_idx(object, 0);
			result = build_fold_addr_expr(result);
			return result;
		} else {  // field binding
			std::cerr << "Field objbind from context analyzer: " << guide->structName << std::endl;
			return build_component_ref_with_idx(object, guide->fieldIdx);
		}
	}

	return NULL_TREE;
}

tree getRetBindAttr(tree object)
{
	tree objectType, attr;

	if (object == NULL_TREE) {
		return NULL_TREE;
	}

	objectType = TREE_TYPE(object);

	if (TREE_CODE(objectType) != FUNCTION_TYPE) {
		return NULL_TREE;
	}

	attr = lookup_attribute(ATTRIBUTE_RET_BIND, DECL_ATTRIBUTES(object));
	if (attr == NULL_TREE) {
		return NULL_TREE;
	}

	return attr;
}

void registerBindAttribute(void *event_data, void *user_data)
{
	register_attribute(&bind_attr);
	register_attribute(&ret_bind_attr);
}

tree getContext(gimple_stmt_iterator &it, tree target, tree object)
{
	long long typeCtx = getFirstBuildtimeContext(target);

	if (PREF.dump != nullptr) {
		PREF.dump->setObjName(std::string(""));
		PREF.dump->setTypeHash(typeCtx);
	}

	long long ctx = getBuildtimeContext(target, object);

	if (ctx == 0) {
		return NULL_TREE;
	}

	if (PREF.dump != nullptr) {
		if (typeCtx != ctx)
			PREF.dump->setObjHash(ctx);
		else
			PREF.dump->setObjHash(0);
	}

	tree buildtimeCtxTree = build_int_cst(long_long_integer_type_node, ctx);
	tree runtimeCtxTree = getRuntimeContext(object);

	gassign *ass;

	if (runtimeCtxTree) {
		runtimeCtxTree = reconstructTree(it, runtimeCtxTree);

		tree tmpReg1 = create_tmp_reg(TREE_TYPE(runtimeCtxTree));
		ass = gimple_build_assign(tmpReg1, runtimeCtxTree);
		gsi_insert_before(&it, ass, GSI_SAME_STMT);

		tree tmpReg2 = create_tmp_reg(long_long_integer_type_node);
		ass = gimple_build_assign(tmpReg2, BIT_XOR_EXPR,
											tmpReg1, buildtimeCtxTree);
		gsi_insert_before(&it, ass, GSI_SAME_STMT);
		return tmpReg2;
	}

	return buildtimeCtxTree;
}

