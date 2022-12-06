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
#include <cstring>
#include <string>

#define BUF_SIZE sizeof(void*)

char name[] = "fortified virtual table";

class A {
public:
	virtual std::string getName(void)
	{
		return "A";
	}
};

class B : public A {
public:
	std::string getName(void)
	{
		return "B";
	}
};

std::string getFakeNameA(void)
{
	return "A";
}

std::string getFakeNameB(void)
{
	return "B";
}

void DoBOF(char *buf)
{
	static void *fake_class[] = { //injected data
		NULL,
		fake_class + 3,
		fake_class + 5,
		fake_class + 4,
		(void*)getFakeNameA,
		fake_class + 6,
		(void*)getFakeNameB,
	};

	memcpy(buf, fake_class, sizeof(fake_class));
}

int vulnerable(void)
{
	struct {
		char buf[BUF_SIZE];
		class A *pa = new A;
		class A *pb = new B;
	} wrapper;

	DoBOF(wrapper.buf);

	if (wrapper.pa->getName() != "A" || wrapper.pb->getName() != "B") {
		return -1;
	}

	return 0;
}
