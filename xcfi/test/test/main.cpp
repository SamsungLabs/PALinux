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

#include <signal.h>
#include <setjmp.h>

#include "class_world.h"
#include "callback_world.h"

jmp_buf jbuf;

void segv_signal_handler(int signo)
{
	siglongjmp(jbuf, 1);
}

int main()
{
	if (signal(SIGSEGV, segv_signal_handler) == SIG_ERR) {
       std::cerr << "Failed to catch SIGSEGV" << std::endl;
    }
	if (signal(SIGILL, segv_signal_handler) == SIG_ERR) {
       std::cerr << "Failed to catch SIGILL" << std::endl;
    }
	if (signal(SIGBUS, segv_signal_handler) == SIG_ERR) {
       std::cerr << "Failed to catch SIGBUS" << std::endl;
    }
	if (signal(SIGTRAP, segv_signal_handler) == SIG_ERR) {
       std::cerr << "Failed to catch SIGTRAP" << std::endl;
    }

	test_class_world();
	test_callback_world();

	return 0;
}
