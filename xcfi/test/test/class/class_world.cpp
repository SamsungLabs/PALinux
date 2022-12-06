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

#include "a.h"
#include "b.h"
#include "c.h"
#include "../error.h"

template <typename A>
std::vector<A*> createTestCase() {
	return {(A*)new AA(), (A*)new AB(), (A*)new AC(),
			(A*)new BA(), (A*)new BB(), (A*)new BC(),
			(A*)new CA(), (A*)new CB(), (A*)new CC()};
}

template<typename A>
void testTestCase(std::vector<A*> &ptrs)
{
	std::cout << "Now : ";
	A a;
	a.whoami();

	for(A* ptr : ptrs) {
		SEGV_SAFE {
			ptr->whoami();
		}
	}

	std::cout << std::endl;
}

void test_class_world()
{
	auto aa = createTestCase<AA>();
	auto ab = createTestCase<AB>();
	auto ac = createTestCase<AC>();

	auto ba = createTestCase<BA>();
	auto bb = createTestCase<BB>();
	auto bc = createTestCase<BC>();

	auto ca = createTestCase<CA>();
	auto cb = createTestCase<CB>();
	auto cc = createTestCase<CC>();

	testTestCase(aa);
	testTestCase(ab);
	testTestCase(ac);

	testTestCase(ba);
	testTestCase(bb);
	testTestCase(bc);

	testTestCase(ca);
	testTestCase(cb);
	testTestCase(cc);
}
