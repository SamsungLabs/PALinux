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
#define BUF_SIZE (sizeof(void*) * 2)

char name[] = "fortified indirect branch";

int isPasswordCorrect(char *password)
{
	if (password[0] == 'i' &&
			password[1] == 'm' &&
			password[2] == 'r' &&
			password[3] == 'o' &&
			password[4] == 'o' &&
			password[5] == 't') {
		return 1;
	}
	return 0;
}

int toggleState(void)
{
	static int state = 0;
	state = !state;
	return state;
}

void DoBOF(char *buf) {
	void **fake_id = (void**)buf;
	fake_id[0] = (void*)0xDEADEAD;
	fake_id[1] = (void*)0xDEADEAD;
	fake_id[2] = (void*)toggleState;
}

int vulnerable(void) {
	struct {
		char passwd[BUF_SIZE];
		int (*login)(char *);
		int (*toggle)(void);
	} wrapper;

	wrapper.login = isPasswordCorrect;
	wrapper.toggle = toggleState;

	DoBOF(wrapper.passwd);

	if (wrapper.login(wrapper.passwd)) {
		//Doing something
		return -1;
	}
	return 0;
}
