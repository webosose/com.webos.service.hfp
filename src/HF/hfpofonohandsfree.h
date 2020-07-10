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

#ifndef OFONO_HANDSFREE_H
#define OFONO_HANDSFREE_H

#include <string>

extern "C" {
#include "ofono-interface.h"
}

class HfpOfonoModem;

class HfpOfonoHandsfree
{
public:
	HfpOfonoHandsfree(const std::string &objectPath, HfpOfonoModem *modem);
	~HfpOfonoHandsfree();
	HfpOfonoHandsfree(const HfpOfonoHandsfree&) = delete;
	HfpOfonoHandsfree& operator = (const HfpOfonoHandsfree&) = delete;

	void getHandsfreeProperties();
	int getBatteryChargeLevel() const { return mBatteryChargeLevel; }

	static void handleHandsfreePropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData);

private:
	HfpOfonoModem* mModem;
	std::string mObjectPath;
	OfonoHandsfree* mOfonoHandsfreeProxy;

	int mBatteryChargeLevel;
	void BatteryChargeLevelChanged(int batteryChargeLevel);
};

#endif
