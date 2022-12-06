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

#include <typeinfo>
#include <iostream>

#include "b.h"

void BA::whoami() {
	std::cout << typeid(this).name() << " - B" << std::endl;
}

void BA::iamBA() {
	std::cout << typeid(this).name() << std::endl;
}

void BB::whoami() {
	std::cout << typeid(this).name() << " - B" << std::endl;
}

void BB::iamBB() {
	std::cout << typeid(this).name() << std::endl;
}

void BC::whoami() {
	std::cout << typeid(this).name() << " - B" << std::endl;
}

void BC::iamBC() {
	std::cout << typeid(this).name() << std::endl;
}

