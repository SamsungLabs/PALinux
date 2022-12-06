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

#ifndef __PACBENCH_MEASURE_H__
#define __PACBENCH_MEASURE_H__

#include <linux/types.h>
#include <linux/time.h>

#define UNUSED(x) (void)(x)

#define MEASURE(name, round, target, pred, post) \
	{ \
		struct timespec oldts, newts; \
		bool warmup = true; \
		register long toy1 asm("x17"); \
		register long toy2 asm("x16"); \
		register long toy3 asm("x18"); \
		register long toy4 asm("x19"); \
		int i; \
		long result; \
 \
		UNUSED(toy1); \
		UNUSED(toy2); \
		UNUSED(toy3); \
		UNUSED(toy4); \
 \
		preempt_disable(); \
		while (1) { \
			pred; \
			getnstimeofday(&oldts); \
			for (i = 0; i < round; i++) { \
				target; \
			} \
			getnstimeofday(&newts); \
			post; \
			if (warmup) \
				warmup = !warmup; \
			else \
				break; \
		} \
		preempt_enable(); \
 \
		result = newts.tv_sec - oldts.tv_sec; \
		result *= 1000000; \
		result += newts.tv_nsec - oldts.tv_nsec; \
		result /= round / 1000; \
 \
		seq_printf(m, "%10s\t\t%8ld.%03ld ns\n", #name, result / 1000, result %1000); \
	}

#endif /* __PACBENCH_MEASURE_H__ */
