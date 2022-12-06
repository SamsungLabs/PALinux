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
#ifndef __UTIL_AARCH64_H__
#define __UTIL_AARCH64_H__

#include "xcfi.h"

gimple *gimple_build_PAC(tree target, tree context = NULL_TREE);
gimple *gimple_build_AUT(tree target, tree context = NULL_TREE);
gimple *gimple_build_XPAC(tree target);

bool is_gimple_PAC(gasm *gas);
bool is_gimple_AUT(gasm *gas);
bool is_gimple_XPAC(gasm *gas);

#endif // __UTIL_AARCH64_H__
