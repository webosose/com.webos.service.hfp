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

#ifndef OFONO_NETWORK_REGISTRATION_H
#define OFONO_NETWORK_REGISTRATION_H

#include <string>
#include <unordered_map>
#include <memory>

extern "C" {
#include "ofono-interface.h"
}

class HfpOfonoModem;

class HfpOfonoNetworkRegistration
{
public:
	HfpOfonoNetworkRegistration(const std::string& objectPath, HfpOfonoModem* modem);
	~HfpOfonoNetworkRegistration();
	HfpOfonoNetworkRegistration(const HfpOfonoNetworkRegistration&) = delete;
	HfpOfonoNetworkRegistration& operator = (const HfpOfonoNetworkRegistration&) = delete;

	void getNetworkRegistrationProperties();
	int getNetworkSignalStrength() const { return mNetworkSignalStrength; }

	static void handleNetworkRegistrationPropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData);

private:
	HfpOfonoModem* mModem;
	std::string mObjectPath;
	OfonoNetworkRegistration* mOfonoNetworkRegistrationProxy;
	int mNetworkSignalStrength;
	std::string mNetworkOperatorName;
	std::string mNetworkRegistrationStatus;

	void networkSignalStrengthChanged(int networkSignalStrength);
	void networkOperatorNameChanged(const std::string &name);
	void networkRegistrationStatusChanged(const std::string &status);
};

#endif
