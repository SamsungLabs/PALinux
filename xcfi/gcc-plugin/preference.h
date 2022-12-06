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

#ifndef __XCFI_PREFERENCE_H__
#define __XCFI_PREFERENCE_H__

#include <unordered_set>
#include <memory>
#include <string>
#include <map>

#include "xcfi.h"
#include "pass/armv8_3/dump.h"
#include <plugin.h>

typedef struct _ObjbindGuide {
	std::string structName;
	int fieldIdx;
	int diversityScore;
	bool isAddrBind;
} ObjbindGuide;

typedef struct _RetbindGuide {
	std::string funcName;
	int depth;
	int callers;
} RetbindGuide;

class Preference {
public:
	std::string baseName;
	std::string outputFileName;

	std::unordered_set<std::string> blackTargets;
	std::unordered_set<std::string> blackPACs;

	std::map<std::string, ObjbindGuide> objbindGuide;  // key: struct name
	std::map<std::string, RetbindGuide> retbindGuide;  // key: function name

	bool useLTO = true;
	bool usePAC = false;

	bool enableVTV = true;
	bool enableICV = true;

	bool isTargetBlacked(std::string name) const
	{
		return blackTargets.find(name) != blackTargets.end();
	}

	bool isPACBlacked(std::string name) const
	{
		return blackPACs.find(name) != blackPACs.end();
	}

	static Preference &getInstance()
	{
		return mInstance;
	}

	ObjbindGuide *getObjbindGuide(std::string structName)
	{
		auto it = objbindGuide.find(structName);
		if (it == objbindGuide.end())
			return NULL;
		return &it->second;
	}

	RetbindGuide *getRetbindGuide(std::string functionName)
	{
		auto it = retbindGuide.find(functionName);
		if (it == retbindGuide.end())
			return NULL;
		return &it->second;
	}

	void enableDump(const std::string &name)
	{
		dump.reset(new DumpPAC(name));
	}

	static void setArgument(struct plugin_gcc_version *version,
							struct plugin_name_args *args);

	std::string PACKey = "A";
	std::unique_ptr<DumpPAC> dump;

protected:
	static Preference mInstance;
};

#define PREF Preference::getInstance()

#endif /*!__XCFI_PREFERENCE_H__*/
