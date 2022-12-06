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

#ifndef __XCFI_ICV_H__
#define __XCFI_ICV_H__

/* System-dependent passes */
void xcfi_register_generic_icv_passes(void);
void xcfi_register_armv8_3_icv_passes(void);

void xcfi_register_icv_passes(void);

#endif /*!__XCFI_ICV_H__*/
