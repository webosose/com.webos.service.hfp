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

#ifndef HFPROLE_H_
#define HFPROLE_H_

#include <string>
#include <vector>
#include <algorithm>

class BluetoothHfpService;

class HfpRole
{
public:
	HfpRole(BluetoothHfpService *service);
	~HfpRole();

protected:
	BluetoothHfpService *getService() const { return mHfpService; };

	bool isDeviceConnected(const std::string &address);
	void addConnectedDevice(const std::string &address);
	void removeConnectedDevice(const std::string &address);

protected:
	std::vector<std::string> mConnectedDevices;

private:
	BluetoothHfpService *mHfpService;
};

#endif

// HFPROLE_H_
