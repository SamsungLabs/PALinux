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
#ifndef __PASS_ARMV8_ICV_WRAP_H__
#define __PASS_ARMV8_ICV_WRAP_H__

#include "xcfi.h"

tree wrapPAC(gimple_stmt_iterator &it, tree target, tree context, bool dump = true, tree wrapper = NULL_TREE);
tree wrapAUT(gimple_stmt_iterator &it, tree target, tree context,
				bool dump = true, bool convertToInt = false);
tree wrapXPAC(gimple_stmt_iterator &it, tree target);

tree wrapVerifiedRePAC(gimple_stmt_iterator &it, tree target,
							tree beforectx, tree nextctx);

#endif //__PASS_ARMV8_ICV_WRAP_H__
