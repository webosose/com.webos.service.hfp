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

#ifndef OFONO_VOICE_CALL_H
#define OFONO_VOICE_CALL_H

#include <string>
#include <vector>

extern "C" {
#include "ofono-interface.h"
}

class HfpOfonoModem;

class HfpOfonoVoiceCall
{
public:
	HfpOfonoVoiceCall(const std::string& objectPath, HfpOfonoModem *modem);
	~HfpOfonoVoiceCall();

	HfpOfonoVoiceCall(const HfpOfonoVoiceCall&) = delete;
	HfpOfonoVoiceCall& operator = (const HfpOfonoVoiceCall&) = delete;

	void getProperties();
	void updateProperties(const std::string &key, GVariant *valueVar);
	std::string getObjectPath() const { return mObjectPath; }
	HfpOfonoModem *getModem() const { return mHfpModem; }
	std::string getCallState() const { return mState; }
	std::string getLineIdentification() const { return mLineIdentification; }
	static void handleVoiceCallPropertyChanged(OfonoVoiceCall *object, const gchar *name, GVariant *value, void *userData);

private:
	HfpOfonoModem *mHfpModem;
	std::string mObjectPath;
	OfonoVoiceCall *mOfonoVoiceCallProxy;
	std::string mLineIdentification;
	std::string mIncomingLine;
	std::string mName;
	std::string mState;
	std::string mStartTime;
	std::string mInformation;
	bool mMultiparty;
	bool mEmergency;
};

#endif
