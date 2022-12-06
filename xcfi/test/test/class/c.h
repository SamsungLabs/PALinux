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

#ifndef __C_H__
#define __C_H__

class CA {
private:
	int privateCA;
protected:
	int protectedCA;
public:
	virtual void whoami();
	virtual void iamCA();
};

class CB : public CA {
private:
	int privateCB;
protected:
	int protectedCB;
public:
	virtual void whoami();
	virtual void iamCB();
};

class CC : public CA {
private:
	int privateCC;
protected:
	int protectedCC;
public:
	virtual void whoami();
	virtual void iamCC();
};

#endif
