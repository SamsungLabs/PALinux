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

#include <vector>
#include <iostream>
#include <functional>

#include "a.h"
#include "../error.h"

unsigned long __builtin_pac_get_context(unsigned long, ...);

void cb_a() {
	std::cout << __func__ << std::endl;
}

void cb_b(int a) {
	std::cout << __func__ << "(" << a << ")" << std::endl;
}

void cb_c(int a, int b) {
	std::cout << __func__ << "(" << a << "," << b << ")" <<  std::endl;
}

int cb_d(int a) {
	std::cout << __func__ << "(" << a << ")" << std::endl;
	return a + 10;
}

void cb_1_a() {
	std::cout << __func__ << std::endl;
}

void cb_1_b(int a) {
	std::cout << __func__ << "(" << a << ")" << std::endl;
}

void cb_1_c(int a, int b) {
	std::cout << __func__ << "(" << a << "," << b << ")" <<  std::endl;
}

int cb_1_d(int a) {
	std::cout << __func__ << "(" << a << ")" << std::endl;
	return a + 10;
}

void cb_2_a() {
	std::cout << __func__ << std::endl;
}

void cb_3_a() {
	std::cout << __func__ << std::endl;
}

typedef void (*v_v_type)(void);
typedef void (*v_i_type)(int);
typedef void (*v_i_i_type)(int, int);
typedef int (*i_i_type)(int);

struct st_a {
	v_v_type handler;
};

v_v_type global_cb = NULL;
void set_cb(v_v_type func) {
	global_cb = func;
}
void trigger_cb() {
	if (global_cb == NULL) {
		std::cout << "callback is not set" << std::endl;
		return;
	}
	global_cb();
}

void parameter_call(v_i_type func) {
	func(15);
}

auto global = cb_a;

struct st_a globals[] =  {
	{
		.handler = cb_2_a
	},
	{
		.handler = cb_3_a
	},
};

void test_callback_world()
{
	void* cbs[] = {(void*)cb_a, (void*)cb_b, (void*)cb_c, (void*)cb_d,
					(void*)cb_1_a, (void*)cb_1_b, (void*)cb_1_c, (void*)cb_1_d};

	cbs[0] = (void*)cb_a;

	std::cout << "v_v_type : " << std::endl;
	for (auto &cb : cbs) {
		SEGV_SAFE {
			std::cout << "TEST : ";
			auto ptr = (v_v_type)cb;
			ptr();
		}
	};
	std::cout << std::endl;

	std::cout << "v_i_type : " << std::endl;
	for (auto &cb : cbs) {
		SEGV_SAFE {
			std::cout << "TEST : ";
			auto ptr = (v_i_type)cb;
			ptr(10);
		}
	};
	std::cout << std::endl;

	std::cout << "v_i_i_type : " << std::endl;
	for (auto &cb : cbs) {
		SEGV_SAFE {
			std::cout << "TEST : ";
			auto ptr = (v_i_i_type)cb;
			ptr(20, 30);
		}
	};
	std::cout << std::endl;

	std::cout << "i_i_type : " << std::endl;
	for (auto &cb : cbs) {
		SEGV_SAFE {
			std::cout << "TEST : ";
			auto ptr = (i_i_type)cb;
			int ret = ptr(40);
			std::cout << "result = " << ret << std::endl;
		}
	};
	std::cout << std::endl;

	struct st_a sta = {(v_v_type)cb_a};

	std::cout << "struct : " << std::endl;
	SEGV_SAFE {
		std::cout << "TEST : ";
		sta.handler();
	}
	std::cout << std::endl;

	const struct st_a sts[] = {{(v_v_type)cb_a}, {(v_v_type)cb_b},
							{(v_v_type)cb_c}, {(v_v_type)cb_d},
							{(v_v_type)cb_1_a}, {(v_v_type)cb_1_b},
							{(v_v_type)cb_1_c}, {(v_v_type)cb_1_d}};

	std::cout << "struct : " << std::endl;
	for (auto &st : sts) {
		SEGV_SAFE {
			std::cout << "TEST : ";
			st.handler();
		}
	};
	std::cout << std::endl;

	std::cout << "other file struct : " << std::endl;
	struct A a;
	getA(&a);
	SEGV_SAFE {
		std::cout << "TEST : ";
		a.a();
	}
	SEGV_SAFE {
		std::cout << "TEST : ";
		a.b(10);
	}
	SEGV_SAFE {
		std::cout << "TEST : ";
		a.c(10, 20);
	}
	SEGV_SAFE {
		std::cout << "TEST : ";
//		int ret = a.d();
		int ret;
		ret = a.d();
		std::cout << "result = " << ret << std::endl;
	}
	SEGV_SAFE {
		std::cout << "TEST : ";
//		int ret = a.e(30);
		int ret;
		ret = a.e(30);
		std::cout << "result = " << ret << std::endl;
	}
	SEGV_SAFE {
		std::cout << "TEST : ";
//		int ret = a.f(10, 20);
		int ret;
		ret = a.f(10, 20);
		std::cout << "result = " << ret << std::endl;
	}

	std::cout << std::endl;

	std::function<void(void)> cb = cb_a, cb1 = cb_2_a, cb2 = (v_v_type)cb_1_b;
	std::cout << "std::function : " << std::endl;
	SEGV_SAFE {
		cb();
	}
	SEGV_SAFE {
		cb1();
	}
	SEGV_SAFE {
		cb2();
	}
	std::cout << std::endl;

	SEGV_SAFE {
		trigger_cb();
	}
	set_cb(cb_3_a);
	SEGV_SAFE {
		trigger_cb();
	}
	set_cb((v_v_type)cb_1_d);
	SEGV_SAFE {
		trigger_cb();
	}

	SEGV_SAFE {
		parameter_call(cb_b);
	}

	v_v_type cb_compare = cb_2_a;
	if (cb_compare == cb_a) {
		std::cout << "if compare ERROR" << std::endl;
	}
	if (cb_compare != cb_2_a) {
		std::cout << "if compare ERROR" << std::endl;
	}
	bool result_bool = (cb_compare != cb_a);
	if (!result_bool) {
		std::cout << "compare assign ERROR" << std::endl;
	}
	result_bool = (cb_compare == cb_2_a);
	if (!result_bool) {
		std::cout << "compare assign ERROR" << std::endl;
	}
	std::cout << ((cb_compare == cb_2_a)? "No Error" : "Error") << std::endl;

	std::cout << "global struct : " << std::endl;
	for (auto &st : globals) {
		SEGV_SAFE {
			st.handler();
		}
	};
	std::cout << std::endl;

	std::cout << "global : " << std::endl;
	SEGV_SAFE {
		global();
	}
	std::cout << std::endl;

#ifndef NO_PLUGIN
	std::cout << "ctx1 : " << __builtin_pac_get_context(0, globals[0].handler);
	globals[0].handler();

	auto ctx2func = globals[0].handler;
	std::cout << "ctx2 : " << __builtin_pac_get_context(0, ctx2func);
	ctx2func();

	std::cout << std::endl;

	struct __attribute__((pacbind("id", "func", "func1"))) {
		v_v_type func;
		v_v_type func1;
		int id;
	} binds;

	binds.id = 3;
	binds.func = cb_2_a;
	binds.func1 = cb_3_a;

	binds.func();
#endif
}
