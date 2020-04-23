// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OFONO_MANAGER_H
#define OFONO_MANAGER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <gio/gio.h>

extern "C" {
#include "ofono-interface.h"
}

class HfpOfonoModem;
class HfpHFRole;

class HfpOfonoManager
{
public:
	HfpOfonoManager(const std::string& objectPath, HfpHFRole* hfRole);
	~HfpOfonoManager();

	HfpOfonoManager(const HfpOfonoManager&) = delete;
	HfpOfonoManager& operator = (const HfpOfonoManager&) = delete;

	void getModemsFromOfonoManager();
	HfpOfonoModem * getModem(const std::string &adapterAddress, const std::string &address) const;
	static void handleModemAdded(OfonoManager *object, const gchar *path, GVariant *properties, void *userData);
	static void handleModemRemoved(OfonoManager *object, const gchar *path, void *userData);

private:
	HfpHFRole *mHfpHFRole;
	std::string mObjectPath;
	OfonoManager *mOfonoManagerProxy;
	std::unordered_map <std::string, std::unique_ptr <HfpOfonoModem>> mModemsMap;
};

#endif
