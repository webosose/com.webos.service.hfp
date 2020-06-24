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

#ifndef OFONO_MODEM_H
#define OFONO_MODEM_H

#include <string>
#include <vector>

extern "C" {
#include "ofono-interface.h"
}

class HfpOfonoVoiceCallManager;
class HfpHFRole;
class HfpOfonoVoiceCall;
class HfpOfonoHandsfree;

class HfpOfonoModem
{
public:
	HfpOfonoModem(const std::string& objectPath, HfpHFRole *role);
	~HfpOfonoModem();

	HfpOfonoModem(const HfpOfonoModem&) = delete;
	HfpOfonoModem& operator = (const HfpOfonoModem&) = delete;
	void getModemProperties(OfonoModem *modemProxy);
	bool isModemConnected() const;
	HfpOfonoVoiceCallManager* getVoiceCallManager() const { return mVoiceCallManager; }
	std::string getAddress() const { return mAddress; }
	void updateState(HfpOfonoVoiceCall *call);
	void callAdded(HfpOfonoVoiceCall *voiceCall);
	void callRemoved(HfpOfonoVoiceCall *voiceCall);
	std::string getAdapterAddress();

	void interfaceAdded();
	void interfaceRemoved();
	void updateBatteryChargeLevel(unsigned char batteryChargeLevel);
	void notifyProperties();

	static void handleModemPropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData);

private:
	HfpHFRole *mHfpHFRole;
	std::string mObjectPath;
	OfonoModem *mOfonoModemProxy;
	HfpOfonoVoiceCallManager *mVoiceCallManager;
	HfpOfonoHandsfree* mHandsfree;
	bool mEmergency;
	bool mLockDown;
	bool mOnline;
	bool mPowered;
	std::vector <std::string> mFeatures;
	std::vector <std::string> mInterfaces;
	std::string mName;
	std::string mAddress;
	std::string type;
};

#endif
