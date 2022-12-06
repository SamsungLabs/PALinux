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

#include <asm/cpu.h>

#include "pacbench.h"

#define CHECK_CPU_FEATURE(name, result) \
{ \
	result = cpus_have_const_cap(name); \
	seq_printf(m, "%20s\t%s\n", #name, (result)? "TRUE":"FALSE"); \
}

#ifndef ARM64_HAS_ADDRESS_AUTH_ARCH
#define ARM64_HAS_ADDRESS_AUTH_ARCH		38
#define ARM64_HAS_ADDRESS_AUTH_IMP_DEF	39
#define ARM64_HAS_GENERIC_AUTH_ARCH		40
#define ARM64_HAS_GENERIC_AUTH_IMP_DEF	41
#endif

void show_cpu_info(struct seq_file *m)
{
	u32 midr = read_cpuid_id();
	seq_printf(m, "*** CPU #%d - impl:0x%02x, part:0x%03x ***\n", get_cpu(),
						MIDR_IMPLEMENTOR(midr), MIDR_PARTNUM(midr));
}
void check_feature(struct feature *feature, struct seq_file *m)
{
	show_cpu_info(m);

	CHECK_CPU_FEATURE(ARM64_HAS_ADDRESS_AUTH_ARCH, feature->address.arch);
	CHECK_CPU_FEATURE(ARM64_HAS_ADDRESS_AUTH_IMP_DEF, feature->address.imp_def);
	CHECK_CPU_FEATURE(ARM64_HAS_GENERIC_AUTH_ARCH, feature->generic.arch);
	CHECK_CPU_FEATURE(ARM64_HAS_GENERIC_AUTH_IMP_DEF, feature->generic.imp_def);

	seq_printf(m, "----------------------------------------\n");
}
