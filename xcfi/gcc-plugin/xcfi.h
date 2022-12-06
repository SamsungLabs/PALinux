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

#ifndef __XCFI_H__
#define __XCFI_H__

#include <gcc-plugin.h>

#include <context.h>
#include <coretypes.h>
#include <diagnostic.h>
#include <input.h>
#include <line-map.h>
#include <basic-block.h> 
#include <bb-reorder.h>

#include <tree.h> 
#include <tree-pass.h>
#include <tree-ssa-alias.h>
#include <tree-ssa-operands.h>

#include <gimple-builder.h>
#include <gimple-expr.h>
#include <gimple-fold.h>
#include <gimple-low.h>
#include <gimple-match.h>
#include <gimple.h>
#include <gimple-iterator.h>
#include <gimple-pretty-print.h>
#include <gimple-ssa.h>
#include <gimple-walk.h>
#include <gimplify-me.h>
#include <gimplify.h>
    
#include <tree-phinodes.h>

#include <cfg.h>
#include <cfganal.h>
#include <cfgbuild.h>
#include <cfgcleanup.h>
#include <cfghooks.h>
#include <cfgloop.h>
#include <cfgrtl.h>
#include <cgraph.h>

#include <ssa-iterators.h>

#include <varasm.h>
#include <attribs.h>

#include <stringpool.h>

#endif /*!__XCFI_H_H__*/
