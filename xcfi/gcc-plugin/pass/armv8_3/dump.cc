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

#include <iostream>
#include <fstream>
#include <map>
#include "dump.h"

DumpPAC::DumpPAC(const std::string &name)
	: path(name)
{
	objName = "";
	typeHash = 0;
	objHash = 0;
}

DumpPAC::~DumpPAC()
{
	create();
}

void DumpPAC::create()
{
	std::ofstream file;
	file.open(path, std::ios::app);
	file << "[ Result -> Total number of context : " << data.size() << " ]" << std::endl;

	for (auto &d : data) {
		std::string ctx;
		char ctxStr[256] = {0,};
		char typeStr[256] = {0,};
		char objStr[256] = {0,};

		if (d.first == 0)
			ctx = "Runtime dependent contexts";
		else {
			//ctx = std::to_string((long long)d.first);
			sprintf(ctxStr, "%llx", (long long)d.first);
			ctx = ctxStr;
		}

		if (d.first == 0)
			file << "Context/Runtime dependent contexts/" << std::endl;
		else
			file << "Context/" << ctx << "/" << types[d.first] << std::endl;

		for (auto &c : d.second) {
			sprintf(typeStr, "%llx", c.second.typeHash);
			sprintf(objStr, "%llx", c.second.objHash);

			//file << "Result/" << ctx << "/" << typeToStr(c.first) << "/" << c.second << std::endl;
			file << "Result/" << ctx << "/" << typeStr << "/" << objStr << "/" << typeToStr(c.first) << "/" << c.second.objName << "/" << c.second.count << std::endl;

			if (c.first == Type::PAC) {
				if (c.second.funcName != "") {
					file << "PacFunc/" << c.second.funcName << std::endl;
				}
				if (c.second.wrapperFuncName != "") {
					file << "PacWrapperFunc/" << c.second.wrapperFuncName << std::endl;
				}
			}
		}
		file << std::endl;
	}
	file.close();
}

const std::string DumpPAC::typeToStr(Type type)
{
	switch (type) {
	case Type::PAC:
		return "PAC";
	case Type::AUT:
		return "AUT";
	case Type::RePAC:
		return "Repac";
	case Type::FromInt:
		return "FromInt";
	case Type::FunctionType:
		return "FunctionType";
	case Type::ObjectType:
		return "ObjectType";
	case Type::PACBind:
		return "PACBind";
	default:
		return "Invalid";
	}
}

void DumpPAC::add(Type type, const std::string &name, long long context)
{
	if (types.find(context) == types.end() || types.at(context) == "")
		types[context].assign(name);

	auto &d = data[context];
	d[type].count++;
	d[type].objHash = objHash;
	d[type].typeHash = typeHash;
	d[type].objName = objName;
	d[type].funcName = funcName;
	d[type].wrapperFuncName = wrapperFuncName;
}

void DumpPAC::setObjName(const std::string &name)
{
	objName = name;
}

void DumpPAC::setTypeHash(long long type)
{
	typeHash = type;
}

void DumpPAC::setObjHash(long long type)
{
	objHash = type;
}

void DumpPAC::clearObjName(void)
{
	objName = "";
	funcName = "";
	wrapperFuncName = "";
	typeHash = 0;
	objHash = 0;
}

void DumpPAC::addFunctionName(const std::string &name, const std::string &wrapper)
{
	funcName = name;
	wrapperFuncName = wrapper;
}

bool DumpPAC::objNameEmpty(void)
{
	if (objName == "")
		return true;
	else
		return false;
}
