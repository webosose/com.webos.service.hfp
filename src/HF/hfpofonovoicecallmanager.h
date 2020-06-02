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

#ifndef OFONO_VOICE_CALL_MANAGER_H
#define OFONO_VOICE_CALL_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>

extern "C" {
#include "ofono-interface.h"
}

class HfpOfonoVoiceCall;
class HfpOfonoModem;

class HfpOfonoVoiceCallManager
{
public:
	HfpOfonoVoiceCallManager(const std::string &objectPath, HfpOfonoModem *modem);
	~HfpOfonoVoiceCallManager();
	HfpOfonoVoiceCallManager(const HfpOfonoVoiceCallManager&) = delete;
	HfpOfonoVoiceCallManager& operator = (const HfpOfonoVoiceCallManager&) = delete;

	std::string dial(const std::string &phoneNumber);
	HfpOfonoVoiceCall* getVoiceCall(const std::string &state);
	HfpOfonoVoiceCall* getVoiceCall(int index);
	bool holdAndAnswer();

	static void handleCallAdded(OfonoVoiceCallManager *object, const gchar *path, GVariant *properties, void *userData);
	static void handleCallRemoved(OfonoVoiceCallManager *object, const gchar *path, void *userData);

private:
	HfpOfonoModem* mModem;
	std::string mObjectPath;
	OfonoVoiceCallManager* mOfonoVoiceCallManagerProxy;
	std::unordered_map <std::string, std::unique_ptr <HfpOfonoVoiceCall>> mCallMap;
};

#endif
