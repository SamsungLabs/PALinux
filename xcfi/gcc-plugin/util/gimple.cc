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

#include <sstream>
#include <iostream>
#include <unordered_map>

#include "gimple.h"

#define CONSTRUCTOR_IDENTIFIER "__base_ctor "

std::string get_tree_name(tree tree)
{
	if (tree == NULL) {
		return std::string();
	}

	if (TREE_CODE(tree) == IDENTIFIER_NODE) {
		return std::string(IDENTIFIER_POINTER(tree));
	}

	if (DECL_NAME(tree) == NULL) {
		return "D." + std::to_string(DECL_UID(tree));
	}

	return std::string(IDENTIFIER_POINTER(DECL_NAME(tree)));
}

std::string get_type_name(tree type)
{
	tree name = TYPE_NAME(TYPE_MAIN_VARIANT(type));
	if (name == NULL) {
		name = TYPE_NAME(type);
	}

	if (name == NULL) {
		return std::string();
	}

	return get_tree_name(name);
}

std::string get_mangled_tree_name(tree tree)
{
	return std::string(IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(tree)));
}

bool is_constructor(tree decl)
{
	return (get_tree_name(decl) == CONSTRUCTOR_IDENTIFIER);
}

bool is_rtti(tree decl)
{
	std::string name = get_tree_name(decl);
	return name.find("_ZTS") == 0;
}

bool is_vtable(tree decl)
{
	std::string name = get_tree_name(decl);
	return (name.find("_vt") == 0 ||
			name.find("_ZTV") == 0 ||
			name.find("_ZTT") == 0 ||
			name.find("_ZTI") == 0 ||
			name.find("_ZTC") == 0);
}

bool is_vtable_pointer(tree ptr)
{
	if (TREE_CODE(ptr) == COMPONENT_REF) {
		tree op1 = TREE_OPERAND(ptr, 1);

		if (TREE_CODE(op1) == FIELD_DECL) {
			if (get_tree_name(op1).find("_vptr.") == 0) {
				return true;
			}
		}
	}

	return false;
}

std::string get_class_name(tree method)
{
	std::string demangled(gimple_decl_printable_name(method, 0));
	return demangled.substr(0, demangled.find("::"));
}

static std::unordered_map<int, std::string> treeCodeStringMap = {
	{IDENTIFIER_NODE, "IDENTIFIER_NODE"},
	{TREE_LIST, "TREE_LIST"},
	{TREE_VEC, "TREE_VEC"},
	{BLOCK, "BLOCK"},
	{OFFSET_TYPE, "OFFSET_TYPE"},
	{ENUMERAL_TYPE, "ENUMERAL_TYPE"},
	{BOOLEAN_TYPE, "BOOLEAN_TYPE"},
	{INTEGER_TYPE, "INTEGER_TYPE"},
	{REAL_TYPE, "REAL_TYPE"},
	{POINTER_TYPE, "POINTER_TYPE"},
	{REFERENCE_TYPE, "REFERENCE_TYPE"},
	{NULLPTR_TYPE, "NULLPTR_TYPE"},
	{FIXED_POINT_TYPE, "FIXED_POINT_TYPE"},
	{COMPLEX_TYPE, "COMPLEX_TYPE"},
	{VECTOR_TYPE, "VECTOR_TYPE"},
	{ARRAY_TYPE, "ARRAY_TYPE"},
	{RECORD_TYPE, "RECORD_TYPE"},
	{UNION_TYPE, "UNION_TYPE"},
	{QUAL_UNION_TYPE, "QUAL_UNION_TYPE"},
	{VOID_TYPE, "VOID_TYPE"},
	{POINTER_BOUNDS_TYPE, "POINTER_BOUNDS_TYPE"},
	{FUNCTION_TYPE, "FUNCTION_TYPE"},
	{METHOD_TYPE, "METHOD_TYPE"},
	{LANG_TYPE, "LANG_TYPE"},
	{VOID_CST, "VOID_CST"},
	{INTEGER_CST, "INTEGER_CST"},
	{REAL_CST, "REAL_CST"},
	{FIXED_CST, "FIXED_CST"},
	{VECTOR_CST, "VECTOR_CST"},
	{STRING_CST, "STRING_CST"},
	{FUNCTION_DECL, "FUNCTION_DECL"},
	{LABEL_DECL, "LABEL_DECL"},
	{FIELD_DECL, "FIELD_DECL"},
	{VAR_DECL, "VAR_DECL"},
	{CONST_DECL, "CONST_DECL"},
	{PARM_DECL, "PARM_DECL"},
	{TYPE_DECL, "TYPE_DECL"},
	{RESULT_DECL, "RESULT_DECL"},
	{DEBUG_EXPR_DECL, "DEBUG_EXPR_DECL"},
	{NAMESPACE_DECL, "NAMESPACE_DECL"},
	{IMPORTED_DECL, "IMPORTED_DECL"},
	{NAMELIST_DECL, "NAMELIST_DECL"},
	{TRANSLATION_UNIT_DECL, "TRANSLATION_UNIT_DECL"},
	{COMPONENT_REF, "COMPONENT_REF"},
	{BIT_FIELD_REF, "FIT_FIELD_REF"},
	{ARRAY_REF, "ARRAY_REF"},
	{ARRAY_RANGE_REF, "ARRAY_RANGE_REF"},
	{REALPART_EXPR, "REALPART_EXPR"},
	{VIEW_CONVERT_EXPR, "VIEW_CONVERT_EXPR"},
	{INDIRECT_REF, "INDIRECT_REF"},
	{OBJ_TYPE_REF, "OOBJ_TYPE_REF"},
	{CONSTRUCTOR, "CONSTRUCTOR"},
	{COMPOUND_EXPR, "COMPOUND_EXPR"},
	{MODIFY_EXPR, "MODIFY_EXPR"},
	{TARGET_EXPR, "TARGET_EXPR"},
	{COND_EXPR, "COND_EXPR"},
	{VEC_COND_EXPR, "VEC_COND_EXPR"},
	{VEC_PERM_EXPR, "VEC_PERM_EXPR"},
	{BIND_EXPR, "BIND_EXPR"},
	{CALL_EXPR, "CALL_EXPR"},
	{WITH_CLEANUP_EXPR, "WITH_CLEAN_EXPR"},
	{CLEANUP_POINT_EXPR, "CLEANUP_POINT_EXPR"},
	{PLACEHOLDER_EXPR, "PLACEHOLDER_EXPR"},
	{PLUS_EXPR, "PLUS_EXPR"},
	{MINUS_EXPR, "MINUS_EXPR"},
	{MULT_EXPR, "MULT_EXPR"},
	{POINTER_PLUS_EXPR, "POINTER_PLUS_EXPR"},
	{MULT_HIGHPART_EXPR, "MULT_HIGHPART_EXPR"},
	{TRUNC_DIV_EXPR, "TRUNC_DIV_EXPR"},
	{CEIL_DIV_EXPR, "CEIL_DIV_EXPR"},
	{FLOOR_DIV_EXPR, "FLOOR_DIV_EXPR"},
	{ROUND_DIV_EXPR, "ROUND_DIV_EXPR"},
	{TRUNC_MOD_EXPR, "TRUNC_MOD_EXPR"},
	{CEIL_MOD_EXPR, "CEIL_MOD_EXPR"},
	{FLOOR_MOD_EXPR, "FLOOR_MOD_EXPR"},
	{ROUND_MOD_EXPR, "ROUND_MOD_EXPR"},
	{RDIV_EXPR, "RDIV_EXPR"},
	{EXACT_DIV_EXPR, "EXACT_DIV_EXPR"},
	{FIX_TRUNC_EXPR, "FIX_TRUNC_EXPR"},
	{FLOAT_EXPR, "FLOAT_EXPR"},
	{NEGATE_EXPR, "NEGATE_EXPR"},
	{MIN_EXPR, "MIN_EXPR"},
	{MAX_EXPR, "MAX_EXPR"},
	{ABS_EXPR, "ABS_EXPR"},
	{LSHIFT_EXPR, "LSHIFT_EXPR"},
	{RSHIFT_EXPR, "RSHIFT_EXPR"},
	{LROTATE_EXPR, "LROTATE_EXPR"},
	{RROTATE_EXPR, "RROTATE_EXPR"},
	{BIT_IOR_EXPR, "BIT_IOR_EXPR"},
	{BIT_XOR_EXPR, "BIT_XOR_EXPR"},
	{BIT_AND_EXPR, "BIT_AND_EXPR"},
	{BIT_NOT_EXPR, "BIT_NOT_EXPR"},
	{TRUTH_ANDIF_EXPR, "TRUTH_ANDIF_EXPR"},
	{TRUTH_ORIF_EXPR, "TRUTH_ORIF_EXPR"},
	{TRUTH_AND_EXPR, "TRUTH_AND_EXPR"},
	{TRUTH_OR_EXPR, "TRUTH_OR_EXPR"},
	{TRUTH_XOR_EXPR, "TRUTH_XOR_EXPR"},
	{TRUTH_NOT_EXPR, "TRUTH_NOT_EXPR"},
	{LT_EXPR, "LT_EXPR"},
	{LE_EXPR, "LE_EXPR"},
	{GT_EXPR, "GT_EXPR"},
	{GE_EXPR, "GE_EXPR"},
	{EQ_EXPR, "EQ_EXPR"},
	{NE_EXPR, "NE_EXPR"},
	{UNORDERED_EXPR, "UNORDERED_EXPR"},
	{ORDERED_EXPR, "ORDERED_EXPR"},
	{UNLT_EXPR, "UNLT_EXPR"},
	{UNLE_EXPR, "UNLE_EXPR"},
	{UNGT_EXPR, "UNGT_EXPR"},
	{UNGE_EXPR, "UNGE_EXPR"},
	{UNEQ_EXPR, "UNEQ_EXPR"},
	{LTGT_EXPR, "LTGT_EXPR"},
	{RANGE_EXPR, "RANGE_EXPR"},
	{PAREN_EXPR, "PAREN_EXPR"},
	{CONVERT_EXPR, "CONVERT_EXPR"},
	{ADDR_SPACE_CONVERT_EXPR, "ADDR_SPACE_CONVERT_EXPR"},
	{FIXED_CONVERT_EXPR, "FIXED_CONVERT_EXPR"},
	{NOP_EXPR, "NOP_EXPR"},
	{NON_LVALUE_EXPR, "NON_LVALUE_EXPR"},
	{COMPOUND_LITERAL_EXPR, "COMPUND_LITERAL_EXPR"},
	{SAVE_EXPR, "SAVE_EXPR"},
	{ADDR_EXPR, "ADDR_EXPR"},
	{FDESC_EXPR, "FDESC_EXPR"},
	{COMPLEX_EXPR, "COMPLEX_EXPR"},
	{CONJ_EXPR, "CONJ_EXPR"},
	{VA_ARG_EXPR, "VA_ARG_EXPR"},
	{TRY_CATCH_EXPR, "TRY_CATCH_EXPR"},
	{TRY_FINALLY_EXPR, "TRY_FINALLY_EXPR"},
	{DECL_EXPR, "DECL_EXPR"},
	{LABEL_EXPR, "LABEL_EXPR"},
	{GOTO_EXPR, "GOTO_EXPR"},
	{RETURN_EXPR, "RETURN_EXPR"},
	{EXIT_EXPR, "EXIT_EXPR"},
	{LOOP_EXPR, "LOOP_EXPR"},
	{SWITCH_EXPR, "SWITCH_EXPR"},
	{CASE_LABEL_EXPR, "CASE_LABEL_EXPR"},
	{ASM_EXPR, "ASM_EXPR"},
	{SSA_NAME, "SSA_NAME"},
	{CATCH_EXPR, "CATCH_EXPR"},
	{EH_FILTER_EXPR, "EH_FILTER_EXPR"},
	{SCEV_KNOWN, "SCEV_KNOWN"},
	{SCEV_NOT_KNOWN, "SCEV_NOT_KNOWN"},
	{POLYNOMIAL_CHREC, "POLYNOMIAL_CHREC"},
	{STATEMENT_LIST, "STATEMENT_LIST"},
	{ASSERT_EXPR, "ASSERT_EXPR"},
	{TREE_BINFO, "TREE_BINFO"},
	{WITH_SIZE_EXPR, "WITH_SIZE_EXPR"},
	{REALIGN_LOAD_EXPR, "REALIGN_LOAD_EXPR"},
	{TARGET_MEM_REF, "TARGET_MEM_REF"},
	{MEM_REF, "MEM_REF"},
	{OACC_PARALLEL, "OACC_PARALLEL"},
	{OACC_KERNELS, "OACC_KERNELS"},
	{OACC_DATA, "OACC_DATA"},
	{OACC_HOST_DATA, "OACC_HOST_DATA"},
	{OMP_PARALLEL, "OMP_PARALLEL"},
	{OMP_TASK, "OMP_TASK"},
	{OMP_FOR, "OMP_FOR"},
	{OMP_SIMD, "OMP_SIMD"},
	{CILK_SIMD, "CILK_SIMD"},
	{OMP_DISTRIBUTE, "OMP_DISTRIBUTE"},
	{OACC_LOOP, "OACC_LOOP"},
	{OMP_TEAMS, "OMP_TEAMS"},
	{OMP_TARGET_DATA, "OMP_TARGET_DATA"},
	{OMP_TARGET, "OMP_TARGET"},
	{OMP_SECTIONS, "OMP_SECTIONS"},
	{OMP_SINGLE, "OMP_SINGLE"},
	{OMP_SECTION, "OMP_SECTION"},
	{OMP_MASTER, "OMP_MASTER"},
	{OMP_TASKGROUP, "OMP_TASKGROUP"},
	{OMP_ORDERED, "OMP_ORDERED"},
	{OMP_CRITICAL, "OMP_CRITICAL"},
	{OACC_CACHE, "OACC_CACHE"},
	{OACC_DECLARE, "OACC_DECLARE"},
	{OACC_ENTER_DATA, "OACC_ENTER_DATA"},
	{OACC_EXIT_DATA, "OACC_EXIT_DATA"},
	{OACC_UPDATE, "OACC_UPDATE"},
	{OMP_TARGET_UPDATE, "OMP_TARGET_UPDATE"},
	{OMP_ATOMIC, "OMP_ATOMIC"},
	{OMP_ATOMIC_READ, "OMP_ATOMIC_READ"},
	{OMP_ATOMIC_CAPTURE_OLD, "OMP_ATOMIC_CAPTURE_OLD"},
	{OMP_ATOMIC_CAPTURE_NEW, "OMP_ATOMIC_CAPTURE_NEW"},
	{OMP_CLAUSE, "OMP_CLAUSE"},
	{TRANSACTION_EXPR, "TRANSACTION_EXPR"},
	{REDUC_MIN_EXPR, "REDUC_MIN_EXPR"},
	{REDUC_MAX_EXPR, "REDUC_MAX_EXPR"},
	{REDUC_PLUS_EXPR, "REDUC_PLUS_EXPR"},
	{DOT_PROD_EXPR, "DOT_PROD_EXPR"},
	{WIDEN_SUM_EXPR, "WIDEN_SUM_EXPR"},
	{SAD_EXPR, "SAD_EXPR"},
	{WIDEN_MULT_EXPR, "WIDEN_MULT_EXPR"},
	{WIDEN_MULT_PLUS_EXPR, "WIDEN_MULT_PLUS_EXPR"},
	{WIDEN_MULT_MINUS_EXPR, "WIDEN_MULT_MINUS_EXPR"},
	{WIDEN_LSHIFT_EXPR, "WIDEN_LSHIFT_EXPR"},
	{FMA_EXPR, "FMA_EXPR"},
	{VEC_WIDEN_MULT_HI_EXPR, "VEC_WIDEN_MULTI_HI_EXPR"},
	{VEC_WIDEN_MULT_LO_EXPR, "VEC_WIDEN_MULT_LO_EXPR"},
	{VEC_WIDEN_MULT_EVEN_EXPR, "VEC_WIDEN_MULT_EVEN_EXPR"},
	{VEC_WIDEN_MULT_ODD_EXPR, "VEC_WIDEN_MULT_ODD_EXPR"},
	{VEC_UNPACK_HI_EXPR, "VEC_UNPACK_HI_EXPR"},
	{VEC_UNPACK_LO_EXPR, "VEC_UNPACK_LO_EXPR"},
	{VEC_UNPACK_FLOAT_HI_EXPR, "VEC_UNPACK_FLOAT_HI_EXPR"},
	{VEC_UNPACK_FLOAT_LO_EXPR, "VEC_UNPACK_FLOAT_LO_EXPR"},
	{VEC_PACK_TRUNC_EXPR, "VEC_PACK_TRUNC_EXPR"},
	{VEC_PACK_SAT_EXPR, "VEC_PACK_SAT_EXPR"},
	{VEC_PACK_FIX_TRUNC_EXPR, "VEC_PACK_FIX_TRUNC_EXPR"},
	{VEC_WIDEN_LSHIFT_HI_EXPR, "VEC_WIDEN_LSHIFT_HI_EXPR"},
	{VEC_WIDEN_LSHIFT_LO_EXPR, "VEC_WIDEN_LSHIFT_LO_EXPR"},
	{PREDICT_EXPR, "PREDICT_EXPR"},
	{OPTIMIZATION_NODE, "OPTIMIZATION_NODE"},
	{TARGET_OPTION_NODE, "TARGET_OPTION_NODE"},
	{ANNOTATE_EXPR, "ANNOTATE_EXPR"},
	{CILK_SPAWN_STMT, "CILK_SPAWN_STMT"},
	{CILK_SYNC_STMT, "CILK_SYNC_STMT"}
};


std::string get_tree_code_string(tree ref)
{
	auto it = treeCodeStringMap.find(TREE_CODE(ref));
	if (it != treeCodeStringMap.end()) {
		return it->second;
	}
	return "UNKNOWN<" + std::to_string(TREE_CODE(ref)) + ">";
}

std::string get_tree_string(tree node)
{
	pretty_printer pp;
	dump_generic_node(&pp, node, 0, 0, false);
	return output_buffer_formatted_text(pp_buffer(&pp));
}

std::string get_gimple_string(gimple *stmt)
{
	pretty_printer pp;
	pp_gimple_stmt_1(&pp, stmt, 0, 0);
	return output_buffer_formatted_text(pp_buffer(&pp));
}

basic_block add_verification_code
					(std::function<tree(gimple_stmt_iterator&)> getCandidate,
						tree target, gimple_stmt_iterator pos)
{
	basic_block cond_bb, then_bb = pos.bb, fallthru_bb = NULL;
	loops_state_set(LOOPS_NEED_FIXUP);

	if (TREE_CODE(target) != VAR_DECL) {
		std::cerr << "[WARN] targets are stronly recomended to be VAR_DECL" << std::endl;
	}

	for (tree cand = getCandidate(pos); cand != NULL; cand = getCandidate(pos)) {
		gcond *cond = gimple_build_cond(NE_EXPR, target, cand, NULL_TREE, NULL_TREE);
		gsi_insert_before(&pos, cond, GSI_NEW_STMT);

		edge e = split_block(then_bb, cond);
		cond_bb = e->src;

		if (fallthru_bb == NULL) {
			fallthru_bb = e->dest;

			then_bb = create_empty_bb(cond_bb);
			add_bb_to_loop(then_bb, cond_bb->loop_father);

			make_edge(cond_bb, then_bb, EDGE_TRUE_VALUE);

			make_single_succ_edge(then_bb, fallthru_bb, EDGE_FALLTHRU);
		} else {
			then_bb = e->dest;

			make_edge(cond_bb, fallthru_bb, EDGE_FALSE_VALUE);
		}
		pos = gsi_start_bb(then_bb);

		e = find_edge(cond_bb, then_bb);
		e->flags = EDGE_TRUE_VALUE;
		e->probability = PROB_VERY_UNLIKELY;

		e = find_edge(cond_bb, fallthru_bb);
		e->flags = EDGE_FALSE_VALUE;
		e->probability = PROB_VERY_LIKELY;
	}

	tree null_ptr = build_int_cst(ptr_type_node, 0);
	gassign *ass = gimple_build_assign(target, NOP_EXPR, null_ptr);
	gsi_insert_after (&pos, ass, GSI_NEW_STMT);

	return fallthru_bb;
}

tree add_global_variable(tree type, std::string name, tree initial)
{
	tree new_name = get_identifier(name.c_str());

	tree new_decl = build_decl(UNKNOWN_LOCATION, VAR_DECL,
								new_name, type);
	SET_DECL_ASSEMBLER_NAME(new_decl, new_name);

	TREE_STATIC(new_decl) = 1;
	TREE_USED(new_decl) = DECL_READ_P(new_decl) = 1;

	DECL_INITIAL(new_decl) = initial;
	DECL_PRESERVE_P(new_decl) = 1;

	varpool_node::add(new_decl);
	varpool_node *node = varpool_node::get_create (new_decl);
	node->analyze();

	return new_decl;
}

long long get_hash_tree(tree type)
{
	std::hash<std::string> hasher;
	std::ostringstream data;

	data.setf(std::ios::hex, std::ios::basefield);

	data << get_tree_code_string(type);

	switch (TREE_CODE(type)) {
	case VOID_TYPE:
		break;

	case OFFSET_TYPE:
		data << " " << get_hash_tree(TREE_TYPE(type)); 
		data << " " << get_hash_tree(TYPE_OFFSET_BASETYPE(type)); 
		break;

	case FUNCTION_TYPE:
		data << " " << get_hash_function(type); 
		break;

	case RECORD_TYPE:
	case UNION_TYPE:
		data << " " << get_type_name(type); 
		break;

	case POINTER_TYPE:
	case REFERENCE_TYPE:
		data << " " << get_hash_tree(TREE_TYPE(type)); 
		break;

	case VECTOR_TYPE:
		data << " " << get_hash_tree(TREE_TYPE(type)); 
		data << " " << TYPE_PRECISION(TREE_TYPE(type));
		break;

	case ARRAY_TYPE:
		data << " " << get_hash_tree(TREE_TYPE(type)); 
		break;

	case REAL_TYPE:
		data << " " << TYPE_PRECISION(TYPE_MAIN_VARIANT(type));
		break;

	case ENUMERAL_TYPE:
		data << " " << get_type_name(type);
	case BOOLEAN_TYPE:
		data << " " << TYPE_PRECISION(type);
		break;

	case INTEGER_TYPE: {
		data << " " << TYPE_PRECISION(type);
		break;
	}
	default:
		break;
	}

	return hasher(data.str());
}

long long get_hash_function(tree fntype)
{
	std::hash<std::string> hasher;
	std::ostringstream data;

	data.setf(std::ios::hex, std::ios::basefield);

	if (TREE_CODE(fntype) != FUNCTION_TYPE) {
		return 0;
	}

	data << get_hash_tree(TREE_TYPE(fntype));

	function_args_iterator args_iter;
	tree arg;

	FOREACH_FUNCTION_ARGS(fntype, arg, args_iter) {
		data << " " << get_hash_tree(arg);
	}
	return hasher(data.str());
}

std::string get_type_string(tree fntype)
{
	std::hash<std::string> hasher;
	std::ostringstream data;

	data.setf(std::ios::hex, std::ios::basefield);

	if (TREE_CODE(fntype) != FUNCTION_TYPE) {
		return 0;
	}

	data << get_hash_tree(TREE_TYPE(fntype));

	function_args_iterator args_iter;
	tree arg;

	FOREACH_FUNCTION_ARGS(fntype, arg, args_iter) {
		data << " " << get_hash_tree(arg);
	}

	return data.str();
}

gcall *gimple_build_call_string(std::string name, tree ret, unsigned int nargs, ...)
{
	tree ret_type = void_type_node, arg_types = NULL_TREE;

	va_list ap;
	va_start (ap, nargs);
	for (unsigned int i = 0; i < nargs; i++) {
		tree type = TREE_TYPE(va_arg(ap, tree));
		arg_types = tree_cons(NULL_TREE, type, arg_types);
	}
	va_end(ap);

	if (ret != NULL_TREE) {
		ret_type = TREE_TYPE(ret);
	}

	if (arg_types == NULL_TREE) {
		arg_types = void_list_node;
	} else {
		tree last = arg_types;
		arg_types = nreverse(arg_types);
		TREE_CHAIN(last) = void_list_node;
	}

	tree fn_type = build_function_type(ret_type, arg_types);

	tree fn_id = get_identifier(name.c_str());
	tree fn_decl = build_decl(UNKNOWN_LOCATION, FUNCTION_DECL, fn_id, fn_type);
	SET_DECL_ASSEMBLER_NAME(fn_decl, fn_id);

	va_start (ap, nargs);
	gcall *call = gimple_build_call_valist(fn_decl, nargs, ap);
	va_end(ap);

	if (ret != NULL_TREE) {
		gimple_call_set_lhs (call, ret);
	}

	return call;
}
void iterate_constructor(tree variable, tree constructor,
						std::function<void(tree,tree)> func)
{
	unsigned long long ix;
	tree index, value;

	if (TREE_CODE(constructor) == CONSTRUCTOR) {
		FOR_EACH_CONSTRUCTOR_ELT(CONSTRUCTOR_ELTS(constructor), ix,
				index, value) {

				tree next;
				if (TREE_CODE(TREE_TYPE(constructor)) == ARRAY_TYPE) {
					next = build4(ARRAY_REF, TREE_TYPE(value),
								variable, index, NULL_TREE, NULL_TREE);
				} else {
					next = build3(COMPONENT_REF, TREE_TYPE(value),
								variable, index, NULL_TREE);
				}
				iterate_constructor(next, value, func);
			}
	} else {
		func(variable, constructor);
	}
}

tree build_component_ref(tree object, std::string fieldName)
{
	tree result = NULL_TREE;

	for (tree field = TYPE_FIELDS(TREE_TYPE(object)); field != NULL_TREE;
			field = DECL_CHAIN (field)) {
		if (get_tree_name(field) == fieldName) {
			result = build3(COMPONENT_REF, TREE_TYPE(field),
							object, field, NULL_TREE);
			break;
		}
	}

	return result;
}

tree build_component_ref_with_idx(tree object, int fieldIdx)
{
	tree result = NULL_TREE;

	int idx = 0;
	for (tree field = TYPE_FIELDS(TREE_TYPE(object)); field != NULL_TREE;
			field = DECL_CHAIN (field)) {
		if (idx == fieldIdx) {
			result = build3(COMPONENT_REF, TREE_TYPE(field),
							object, field, NULL_TREE);
			break;
		}
		idx++;
	}

	return result;
}

gimple_stmt_iterator gsi_prev_loop(gimple_stmt_iterator it) {
	struct loop *cur_loop = it.bb->loop_father;
	if (loop_outer(cur_loop)) {
		edge e;
		edge_iterator ei;
		basic_block header = cur_loop->header;
		FOR_EACH_EDGE (e, ei, header->preds) {
			basic_block bb = e->src;
			if (loop_outer(cur_loop) == bb->loop_father) {
				if (EDGE_COUNT(bb->preds) != 0) {
					return gsi_last_bb(bb);
				}
			}
		}
	}
	return it;
}
