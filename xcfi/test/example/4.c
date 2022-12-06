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
#define BUF_SIZE (sizeof(void*))

#include <iostream>

char name[] = "fortified return address";

void *ra_backup, **raa_backup;

int fake_return() {
	//Doing something
	*raa_backup = ra_backup;

	register void **sp asm("sp");
	sp = raa_backup - 1 + 4;

	std::cout << "";

	return -1;
}

void DoBOF(char *buf) {
	void **addr = (void**)buf;

	raa_backup = addr - 2;
	ra_backup = *(addr - 2);

	*(addr - 2) = (void*)fake_return;
}
int vulnerable(void);

int go_action(void) {
	char buffer[BUF_SIZE];

	DoBOF(buffer);

	//Do something wrong

	return -1;
}

int vulnerable(void) {
	int result;
	result = go_action();

	return result;
}
