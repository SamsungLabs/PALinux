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
#include "icv-wrap.h"
#include "preference.h"

#include "util/gimple.h"
#include "util/aarch64.h"

namespace {

std::string get_func_type(tree type)
{
	if (type == NULL_TREE)
		return "Invalid";
	return get_tree_string(type);
}

void insertTrialToFault(gimple_stmt_iterator &it, tree target)
{
	vec<tree, va_gc> *outputs = NULL;
	vec<tree, va_gc> *inputs = NULL;

	tree tmpreg = create_tmp_reg(TREE_TYPE(target));

	tree output = build_tree_list(NULL_TREE, build_string(3, "=r"));
	output = chainon(NULL_TREE, build_tree_list(output, tmpreg));

	vec_safe_push(outputs, output);

	tree input = build_tree_list(NULL_TREE, build_string(2, "r"));
	input = chainon(NULL_TREE, build_tree_list(input, target));

	vec_safe_push(inputs, input);

	gasm *gas = gimple_build_asm_vec("LDR %0, [%1]",
										inputs, outputs, NULL, NULL);

	gimple_asm_set_volatile(gas, true);

	gsi_insert_before(&it, gas, GSI_SAME_STMT);
}

} // namespace

tree wrapPAC(gimple_stmt_iterator &it, tree target, tree context, bool dump, tree wrapper)
{
	gimple_stmt_iterator pac_it = it;

	//To consider loop invariant code motion
	if (TREE_CODE(target) == ADDR_EXPR) {
		tree func = TREE_OPERAND(target, 0);
		if (TREE_CODE(func) == FUNCTION_DECL) {
			pac_it = gsi_prev_loop(it);

			if (dump && PREF.dump != nullptr) {
				PREF.dump->addFunctionName(get_tree_name(func), get_tree_name(wrapper));
			}
		}
	}

	if (dump && PREF.dump != nullptr) {
		long long ctx = context != NULL_TREE ? TREE_INT_CST_LOW(context) : 0;
		PREF.dump->add(DumpPAC::PAC, get_func_type(TREE_TYPE(target)), ctx);
		PREF.dump->clearObjName();
	}

	tree reg = create_tmp_reg(TREE_TYPE(target));
	DECL_REGISTER(reg) = 1;
	gassign *ass = gimple_build_assign(reg, target);
	gsi_insert_before(&pac_it, ass, GSI_SAME_STMT);

	gimple *pac = gimple_build_PAC(reg, context);
	gimple_asm_set_volatile((gasm*)pac, true);
	gsi_insert_before(&pac_it, pac, GSI_SAME_STMT);

	return reg;
}

tree wrapAUT(gimple_stmt_iterator &it, tree target, tree context,
				bool dump, bool convertToInt)
{
	if (dump && PREF.dump != nullptr) {
		long long ctx = context != NULL_TREE ? TREE_INT_CST_LOW(context) : 0;
		PREF.dump->add(DumpPAC::AUT, get_func_type(TREE_TYPE(target)), ctx);
		PREF.dump->clearObjName();
	}

	tree reg = create_tmp_reg(TREE_TYPE(target));
	DECL_REGISTER(reg) = 1;
	gassign *ass = gimple_build_assign(reg, target);
	gsi_insert_before(&it, ass, GSI_SAME_STMT);

	if (convertToInt) {
		tree null = build_int_cst(ptr_type_node, 0);
		gcond *cond = gimple_build_cond(NE_EXPR, reg, null, NULL_TREE, NULL_TREE);
		gsi_insert_before(&it, cond, GSI_SAME_STMT);

		basic_block cond_bb, then_bb, fallthru_bb;

		edge e = split_block(it.bb, cond);
		e->flags = EDGE_FALSE_VALUE;
		e->probability = PROB_VERY_UNLIKELY;
		cond_bb = e->src;
		fallthru_bb = e->dest;

		then_bb = create_empty_bb(cond_bb);
		add_bb_to_loop(then_bb, cond_bb->loop_father);

		e = make_edge(cond_bb, then_bb, EDGE_TRUE_VALUE);
		e->probability = PROB_VERY_LIKELY;

		make_single_succ_edge(then_bb, fallthru_bb, EDGE_FALLTHRU);

		it = gsi_start_bb(then_bb);
		gimple *aut = gimple_build_AUT(reg, context);
		gimple_asm_set_volatile((gasm*)aut, true);
		gsi_insert_before(&it, aut, GSI_SAME_STMT);

		it = gsi_start_bb(fallthru_bb);
	} else {
		gimple *aut = gimple_build_AUT(reg, context);
		gimple_asm_set_volatile((gasm*)aut, true);
		gsi_insert_before(&it, aut, GSI_SAME_STMT);
	}

	return reg;
}

tree wrapXPAC(gimple_stmt_iterator &it, tree target)
{
	tree reg = create_tmp_reg(TREE_TYPE(target));
	DECL_REGISTER(reg) = 1;
	gassign *ass = gimple_build_assign(reg, target);
	gsi_insert_before(&it, ass, GSI_SAME_STMT);

	gimple *xpac = gimple_build_XPAC(reg);
	gsi_insert_before(&it, xpac, GSI_SAME_STMT);

	return reg;
}

tree wrapVerifiedRePAC(gimple_stmt_iterator &it,
						tree target, tree beforectx, tree nextctx)
{
	if (PREF.dump != nullptr) {
		long long ctx = beforectx != NULL_TREE ? TREE_INT_CST_LOW(beforectx) : 0;
		PREF.dump->add(DumpPAC::RePAC, get_func_type(TREE_TYPE(target)), ctx);
		PREF.dump->clearObjName();
	}

	tree reg = create_tmp_reg(TREE_TYPE(target));
	DECL_REGISTER(reg) = 1;
	gassign *ass = gimple_build_assign(reg, target);
	gsi_insert_before(&it, ass, GSI_SAME_STMT);

	tree null = build_int_cst(ptr_type_node, 0);
	gcond *cond = gimple_build_cond(NE_EXPR, reg, null, NULL_TREE, NULL_TREE);
	gsi_insert_before(&it, cond, GSI_SAME_STMT);

	basic_block cond_bb, then_bb, else_bb, fallthru_bb;

	edge e = split_block(it.bb, cond);
	e->flags = EDGE_FALSE_VALUE;
	e->probability = PROB_VERY_UNLIKELY;
	cond_bb = e->src;
	fallthru_bb = e->dest;

	then_bb = create_empty_bb(cond_bb);
	add_bb_to_loop(then_bb, cond_bb->loop_father);

	e = make_edge(cond_bb, then_bb, EDGE_TRUE_VALUE);
	e->probability = PROB_VERY_LIKELY;

	make_single_succ_edge(then_bb, fallthru_bb, EDGE_FALLTHRU);

	it = gsi_start_bb(then_bb);

	tree xpac = wrapXPAC(it, reg);

	gimple *gaut = gimple_build_AUT(reg, beforectx);
	gsi_insert_before(&it, gaut, GSI_SAME_STMT);

	if (PREF.PACKey.size() != 0) {
		cond = gimple_build_cond(EQ_EXPR, reg, xpac, NULL_TREE, NULL_TREE);
	} else {
		//Because XPAC semantic was changed on alternative situation
		cond = gimple_build_cond(NE_EXPR, reg, xpac, NULL_TREE, NULL_TREE);
	}
	gsi_insert_before(&it, cond, GSI_SAME_STMT);

	e = split_block(it.bb, cond);
	e->flags = EDGE_TRUE_VALUE;
	e->probability = PROB_VERY_LIKELY;
	cond_bb = e->src;
	then_bb = e->dest;

	else_bb = create_empty_bb(cond_bb);
	add_bb_to_loop(else_bb, cond_bb->loop_father);

	e = make_edge(cond_bb, else_bb, EDGE_FALSE_VALUE);
	e->probability = PROB_VERY_UNLIKELY;

	make_single_succ_edge(else_bb, fallthru_bb, EDGE_FALLTHRU);

	it = gsi_start_bb(then_bb);

	gimple *gpac = gimple_build_PAC(reg, nextctx);
	gimple_asm_set_volatile((gasm*)gpac, true);
	gsi_insert_before(&it, gpac, GSI_SAME_STMT);

	it = gsi_start_bb(else_bb);

	insertTrialToFault(it, reg);

	it = gsi_start_bb(fallthru_bb);

	return reg;
}

tree wrapVerifiedRePAC(gimple_stmt_iterator &it,
						tree target, long long beforectx, long long nextctx)
{
	tree beforectxTree = build_int_cst(long_long_integer_type_node, beforectx);
	tree nextctxTree = build_int_cst(long_long_integer_type_node, nextctx);
	return wrapVerifiedRePAC(it, target, beforectxTree, nextctxTree);
}
