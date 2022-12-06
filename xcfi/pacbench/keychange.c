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

#ifndef SYS_APIAKEYLO_EL1
#define SYS_APIAKEYLO_EL1		sys_reg(3, 0, 2, 1, 0)
#define SYS_APIAKEYHI_EL1		sys_reg(3, 0, 2, 1, 1)
#define SYS_APIBKEYLO_EL1		sys_reg(3, 0, 2, 1, 2)
#define SYS_APIBKEYHI_EL1		sys_reg(3, 0, 2, 1, 3)
#define SYS_APDAKEYLO_EL1		sys_reg(3, 0, 2, 2, 0)
#define SYS_APDAKEYHI_EL1		sys_reg(3, 0, 2, 2, 1)
#define SYS_APDBKEYLO_EL1		sys_reg(3, 0, 2, 2, 2)
#define SYS_APDBKEYHI_EL1		sys_reg(3, 0, 2, 2, 3)
#define SYS_APGAKEYLO_EL1		sys_reg(3, 0, 2, 3, 0)
#define SYS_APGAKEYHI_EL1		sys_reg(3, 0, 2, 3, 1)
#endif

struct __ptrauth_key {
	unsigned long lo, hi;
};

#define __ptrauth_key_save(k, v) \
	do { \
		struct __ptrauth_key __pki_v = (v); \
		write_sysreg_s(__pki_v.lo, SYS_ ## k ## KEYLO_EL1); \
		write_sysreg_s(__pki_v.hi, SYS_ ## k ## KEYHI_EL1); \
	} while (0)

#define __ptrauth_key_load(k, v) \
	do { \
		v.lo = read_sysreg_s(SYS_ ## k ## KEYLO_EL1); \
		v.hi = read_sysreg_s(SYS_ ## k ## KEYHI_EL1); \
	} while (0)

#define MEASURE_KEY_CHANGE(name, backup) \
	MEASURE(name, ROUND_OPERATION, \
			{ \
				__ptrauth_key_save(name, backup); \
			}, \
			{}, \
			{ \
				__ptrauth_key_load(name, backup); \
			}) \

#define BACKUP_AND_SHOW_KEY(name, backup) \
	__ptrauth_key_load(name, backup); \
	seq_printf(m, "%s\t%016lx%016lx\n", #name, backup.hi, backup.lo);

void evaluate_keychange(struct feature *feature, struct seq_file *m)
{
	struct __ptrauth_key apia;
	struct __ptrauth_key apda;
	struct __ptrauth_key apib;
	struct __ptrauth_key apdb;
	struct __ptrauth_key apga;

	seq_printf(m, "*** Current keys value ***\n");
	seq_printf(m, "----------------------------------------\n");

	BACKUP_AND_SHOW_KEY(APIA, apia);
	BACKUP_AND_SHOW_KEY(APIB, apib);
	BACKUP_AND_SHOW_KEY(APDA, apda);
	BACKUP_AND_SHOW_KEY(APDB, apdb);
	BACKUP_AND_SHOW_KEY(APGA, apga);

	seq_printf(m, "----------------------------------------\n");
	seq_printf(m, "*** Key changes ***\n");
	seq_printf(m, "----------------------------------------\n");

	MEASURE_KEY_CHANGE(APIA, apia);
	MEASURE_KEY_CHANGE(APIB, apib);
	MEASURE_KEY_CHANGE(APDA, apda);
	MEASURE_KEY_CHANGE(APDB, apdb);
	MEASURE_KEY_CHANGE(APGA, apga);

	seq_printf(m, "----------------------------------------\n");
}
