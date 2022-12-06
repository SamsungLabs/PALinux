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

#ifndef __PACBENCH_PACBENCH_H__
#define __PACBENCH_PACBENCH_H__

#include <linux/seq_file.h>
#include <linux/types.h>

struct auth {
	bool arch, imp_def;
};

struct feature {
	struct auth address, generic;
};

void check_feature(struct feature *f, struct seq_file *m);
void evaluate_keychange(struct feature *f, struct seq_file *m);
void evaluate_instruction(struct feature *f, struct seq_file *m);

#endif /*!__PACBENCH_PACBENCH_H__*/
