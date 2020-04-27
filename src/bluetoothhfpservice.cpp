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

#include <pbnjson.hpp>

#include "bluetoothhfpservice.h"
#include "AG/hfpagrole.h"
#include "HF/hfphfrole.h"
#include "utils.h"
#include "logging.h"
#include "config.h"

BluetoothHfpService::BluetoothHfpService() :
	LS::Handle("com.webos.service.hfp")
{
	initialize();
}

BluetoothHfpService::~BluetoothHfpService()
{
}

void BluetoothHfpService::initialize()
{
	bool enableHF = false;
	bool enableAG = false;
	std::vector<std::string> enabledRole = split(std::string(WEBOS_HFP_ENABLED_ROLE), ' ');
	for (auto enable : enabledRole)
	{
		if (enable == "HF")
			enableHF = true;
		else if (enable == "AG")
			enableAG = true;
	}
	BT_DEBUG("initialize enableHF:%d, enableAG:%d", enableHF, enableAG);

	if (enableHF)
	{
		mHFRole = new HfpHFRole(this);
		mHFRole->initialize();
	}
	if (enableAG)
	{
		mAGRole = new HfpAGRole(this);
		mAGRole->initialize();
	}
}
