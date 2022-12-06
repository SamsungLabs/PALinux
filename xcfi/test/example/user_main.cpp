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
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char name[];
extern int vulnerable();

int main(void)
{
	pid_t pid = fork();
	int ret;

	switch(pid) {
	case -1:
		std::cerr << "Fork error" << std::endl;
		break;
	case 0:
		ret = vulnerable();
		exit(ret);
		break;
	default:
		int status;
		pid = waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) == 0) {
				std::cout << "Detecting " << name << " : [Failed]"
							<< std::endl;
			} else {
				std::cout << "Detecting " << name << " : [Succeeded]"
							<< std::endl;
			}
		} else if (WIFSIGNALED(status)) {
			std::cout << "Detecting " << name << " : [Succeeded]" << std::endl;
		}
	}

	return 0;
}
