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

#include "pacbench.h"
#include "measure.h"

#define ROUND_OPERATION	8192

#define STACK_BACKUP_PRE \
	{ \
		register long sp asm("sp"); \
		toy4 = sp; \
	}

#define STACK_BACKUP_POST \
	{ \
		register long sp asm("sp"); \
		sp = toy4; \
	}

#define MEASURE_NO_OPERAND_ASM(name) \
		MEASURE(name, ROUND_OPERATION, \
			__asm__ __volatile__ (#name"\n"), \
			STACK_BACKUP_PRE, STACK_BACKUP_POST);

#define MEASURE_ONE_OPERAND_ASM(name) \
		MEASURE(name, ROUND_OPERATION, \
			__asm__ __volatile__ (#name " %0\n" \
				: "=r"(toy1) : "0"(toy1)), \
			STACK_BACKUP_PRE, STACK_BACKUP_POST);

#define MEASURE_TWO_OPERAND_ASM(name) \
		MEASURE(name, ROUND_OPERATION, \
			__asm__ __volatile__ (#name " %0, %1\n" \
				: "=r"(toy1) : "r"(toy2), "0"(toy1)), \
			STACK_BACKUP_PRE, STACK_BACKUP_POST);

#define MEASURE_THREE_OPERAND_ASM(name) \
		MEASURE(name, ROUND_OPERATION, \
			__asm__ __volatile__ (#name " %0, %1, %2\n" \
				: "=r"(toy1) : "r"(toy2), "r"(toy3), "0"(toy1)), \
			STACK_BACKUP_PRE, STACK_BACKUP_POST);

void evaluate_instruction(struct feature *feature, struct seq_file *m)
{
//Requires GCC version 7
#if __GNUC__ >= 7
	MEASURE_NO_OPERAND_ASM(NOP);

	seq_printf(m, "----------------------------------------\n");
	seq_printf(m, "*** Sign ***\n");
	seq_printf(m, "----------------------------------------\n");

	MEASURE_NO_OPERAND_ASM(PACIASP);
	MEASURE_NO_OPERAND_ASM(PACIA1716);
	MEASURE_TWO_OPERAND_ASM(PACIA);
	MEASURE_ONE_OPERAND_ASM(PACIZA);
	MEASURE_TWO_OPERAND_ASM(PACDA);
	MEASURE_ONE_OPERAND_ASM(PACDZA);

	seq_printf(m, "----------------------------------------\n");

	MEASURE_NO_OPERAND_ASM(PACIBSP);
	MEASURE_NO_OPERAND_ASM(PACIB1716);
	MEASURE_TWO_OPERAND_ASM(PACIB);
	MEASURE_ONE_OPERAND_ASM(PACIZB);
	MEASURE_TWO_OPERAND_ASM(PACDB);
	MEASURE_ONE_OPERAND_ASM(PACDZB);

	seq_printf(m, "----------------------------------------\n");

	if (!feature->generic.arch && !feature->generic.imp_def) {
		MEASURE_THREE_OPERAND_ASM(PACGA);
		seq_printf(m, "----------------------------------------\n");
	}

	seq_printf(m, "*** Authentication ***\n");
	seq_printf(m, "----------------------------------------\n");

	MEASURE_NO_OPERAND_ASM(AUTIASP);
	MEASURE_NO_OPERAND_ASM(AUTIA1716);
	MEASURE_TWO_OPERAND_ASM(AUTIA);
	MEASURE_ONE_OPERAND_ASM(AUTIZA);
	MEASURE_TWO_OPERAND_ASM(AUTDA);
	MEASURE_ONE_OPERAND_ASM(AUTDZA);

	seq_printf(m, "----------------------------------------\n");

	MEASURE_NO_OPERAND_ASM(AUTIBSP);
	MEASURE_NO_OPERAND_ASM(AUTIB1716);
	MEASURE_TWO_OPERAND_ASM(AUTIB);
	MEASURE_ONE_OPERAND_ASM(AUTIZB);
	MEASURE_TWO_OPERAND_ASM(AUTDB);
	MEASURE_ONE_OPERAND_ASM(AUTDZB);

	seq_printf(m, "----------------------------------------\n");
#else
	seq_printf(m, "GCC version is too low for PAC oprations\n");
	seq_printf(m, "----------------------------------------\n");
#endif
}
