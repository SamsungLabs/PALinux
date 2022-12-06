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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>

#include "preference.h"

#define REQUIRED_VERSION_FOR_PAC	"7.0"

Preference Preference::mInstance;

void Preference::setArgument(struct plugin_gcc_version *version,
								struct plugin_name_args *args)
{
	mInstance.baseName = args->base_name;

	const char *env = getenv("COLLECT_GCC_OPTIONS");
	if (env != NULL) {
		std::stringstream envs(env);
		while (!envs.eof()) {
			std::string word;
			envs >> word;
			if (word.find("'-march=armv") == 0) {
				float ver = std::stof(word.substr(sizeof("'-march=armv") - 1));
				if (ver >= 8.3) {
					mInstance.usePAC = true;
					if (std::stof(version->basever) < std::stof(REQUIRED_VERSION_FOR_PAC)) {
						std::cerr << "warning: HW support requires gcc version >= "
									<< REQUIRED_VERSION_FOR_PAC << std::endl;
						std::cerr << "Alternatives will be used (not recommended)" << std::endl;
					}
				}
			} else if (word == "'-o'") {
				envs >> word;
				mInstance.outputFileName = word.substr(1, word.size() - 2);
			}
		}
	}

	for (int i = 0; i < args->argc; i++) {
		std::string key = args->argv[i].key;
		std::string value;
		if (args->argv[i].value != NULL) {
			value = args->argv[i].value;
		}

		if (key == "black-target") {
			std::stringstream ss(value);
			std::string token;

			while(std::getline(ss, token, ',')) {
				mInstance.blackTargets.insert(token);
			}
		} else if (key == "black-pac") {
			std::stringstream ss(value);
			std::string token;

			while(std::getline(ss, token, ',')) {
				mInstance.blackPACs.insert(token);
			}
		} else if (key == "no-lto") {
			mInstance.useLTO = false;
		} else if (key == "no-pac") {
			mInstance.usePAC = false;
		} else if (key == "no-vtv") {
			mInstance.enableVTV = false;
		} else if (key == "no-icv") {
			mInstance.enableICV = false;
		} else if (key == "pac-key") {
			std::transform(value.begin(), value.end(), value.begin(), toupper);
			if (value == "A" || value == "B") {
				mInstance.PACKey = value;
			} else {
				mInstance.PACKey = "";
				if (value != "NONE") {
					std::cerr << "There's no key " << value << std::endl;
					std::cerr << "Alternatives will be used (not recommended)" << std::endl;
				}
			}
		} else if (key == "dump-pac") {
			mInstance.enableDump(value);
		} else if (key == "objbind-guide") {
			std::string line;
			std::fstream fs;

			fs.open(value, std::fstream::in);
			while(std::getline(fs, line)) {
				ObjbindGuide guide;
				std::stringstream ss(line);
				std::string token;

				std::getline(ss, token, ',');
				guide.structName = token.substr(7);  // skip "struct."

				std::getline(ss, token, ',');
				guide.fieldIdx = std::stoi(token);
				if ((guide.fieldIdx == -3) || (guide.fieldIdx == -2)) { // magic value
					guide.isAddrBind = true;
				} else {
					guide.isAddrBind = false;
				}

				std::getline(ss, token, ',');
				guide.diversityScore = std::stoi(token);
				
				if (guide.fieldIdx != -1)
					mInstance.objbindGuide.insert(std::make_pair(guide.structName, guide));
			}
			fs.close();
		} else if (key == "retbind-guide") {
			std::cerr << "[JINBUM] in retbind-guide" << std::endl; // [TODO] why is this not called?

			std::string line;
			std::fstream fs;

			fs.open(value, std::fstream::in);
			while(std::getline(fs, line)) {
				RetbindGuide guide;
				std::stringstream ss(line);
				std::string token;

				std::getline(ss, token, ',');
				guide.funcName = token;

				std::getline(ss, token, ',');
				guide.depth = std::stoi(token);

				std::getline(ss, token, ',');
				guide.callers = std::stoi(token);

				mInstance.retbindGuide.insert(std::make_pair(guide.funcName, guide));
			}
			fs.close();

			// For test
			for(auto it = mInstance.retbindGuide.begin(); it != mInstance.retbindGuide.end(); ++it) {
				std::cout << it->second.funcName << "," << it->second.depth << "," << it->second.callers << std::endl;
			}
		}
	}

	if (!(mInstance.usePAC && mInstance.enableICV) &&
			mInstance.blackPACs.size() != 0) {
		std::cerr << "warning: black-pac option will be ignored" << std::endl;
	}
}
