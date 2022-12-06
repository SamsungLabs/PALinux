/*
 * Copyright (C) 2019 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Author: SeolHeui Kim <s414.kim@samsung.com>
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

#ifndef __XCFI_DUMP_H__
#define __XCFI_DUMP_H__

#include <map>
#include <string>

typedef struct _CountData {
	int count;
	long long typeHash;
	long long objHash;
	std::string objName;
	std::string funcName;
	std::string wrapperFuncName;
} CountData;

class DumpPAC {
public:
	enum Type {
		PAC = 1,
		AUT,
		RePAC,
		FromInt,		//for 'ptr = address' case
		FunctionType,	//ctx = function type
		ObjectType,		//ctx = ^ object type
		PACBind,		//ctx = ^ object type ^ runtime values
	};

	DumpPAC(const std::string &name);
	~DumpPAC();

	void add(Type type, const std::string &name, long long context);
	void addFunctionName(const std::string &name, const std::string &wrapper);
	void setObjName(const std::string &name);
	void setTypeHash(long long type);
	void setObjHash(long long type);
	bool objNameEmpty(void);
	void clearObjName(void);

private:
	//using Count = std::map<Type, int>;
	//using Data = std::map<long long, Count>;
	using Count = std::map<Type, CountData>;
	using Data = std::map<long long, Count>;
	using TypeMap = std::map<unsigned long long, std::string>;

	const std::string typeToStr(Type type);
	void create();
	Data data;
	TypeMap types;
	std::string path;
	std::string objName;
	std::string funcName;
	std::string wrapperFuncName;
	long long typeHash;
	long long objHash;
};

#endif /*!__XCFI_DUMP_H__*/
