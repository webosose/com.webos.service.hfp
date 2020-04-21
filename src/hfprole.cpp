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

#include "hfprole.h"

HfpRole::HfpRole(BluetoothHfpService *service)
{
	mHfpService = service;
}

HfpRole::~HfpRole()
{
}

bool HfpRole::isDeviceConnected(const std::string &address)
{
	return (std::find(mConnectedDevices.begin(), mConnectedDevices.end(), address) != mConnectedDevices.end());
}

void HfpRole::addConnectedDevice(const std::string &address)
{
	if (isDeviceConnected(address))
		return;

	mConnectedDevices.push_back(address);
}

void HfpRole::removeConnectedDevice(const std::string &address)
{
	auto iter = std::find(mConnectedDevices.begin(), mConnectedDevices.end(), address);
	if (mConnectedDevices.end() == iter)
		return;

	mConnectedDevices.erase(iter);
}
