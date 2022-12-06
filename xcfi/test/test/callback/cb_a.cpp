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

#include <iostream>

#include "a.h"

static void cb_a() {
	std::cout << __func__ << std::endl;
}

static void cb_b(int a) {
	std::cout << __func__ << "(" << a << ")" << std::endl;
}

static void cb_c(int a, int b) {
	std::cout << __func__ << "(" << a << "," << b << ")" <<  std::endl;
}


static int cb_d() {
	std::cout << __func__ << std::endl;
	return 0;
}

static int cb_e(int a) {
	std::cout << __func__ << "(" << a << ")" << std::endl;
	return 1;
}

static int cb_f(int a, int b) {
	std::cout << __func__ << "(" << a << "," << b << ")" <<  std::endl;
	return 2;
}

void getA(struct A *out)
{
	out->a = cb_a;
	out->b = cb_b;
	out->c = cb_c;
	out->d = cb_d;
	out->e = cb_e;
	out->f = cb_f;
}
