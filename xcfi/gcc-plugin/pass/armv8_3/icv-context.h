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
#ifndef __PASS_ARMV8_ICV_CONTEXT_H__
#define __PASS_ARMV8_ICV_CONTEXT_H__

#include "xcfi.h"

long long getBuildtimeContext(tree target, tree object = NULL_TREE, bool record = true);
tree getRetBindAttr(tree object);
tree getRuntimeContext(tree target);
tree getContext(gimple_stmt_iterator &it, tree target, tree object = NULL_TREE);

void registerBindAttribute(void *event_data, void *user_data);


#endif //__PASS_ARMV8_ICV_CONTEXT_H__
