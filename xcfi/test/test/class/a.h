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

#ifndef __A_H__
#define __A_H__

class AA {
private:
	int privateAA;
protected:
	int protectedAA;
public:
	virtual void whoami();
	virtual void iamAA();
};

class AB : public AA {
private:
	int privateAB;
protected:
	int protectedAB;
public:
	virtual void whoami();
	virtual void iamAB();
};

class AC : public AB {
private:
	int privateAC;
protected:
	int protectedAC;
public:
	virtual void whoami();
	virtual void iamAC();
};

#endif
