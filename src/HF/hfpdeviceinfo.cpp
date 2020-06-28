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

#include <memory.h>
#include "hfpdeviceinfo.h"
#include "logging.h"

HfpDeviceInfo::HfpDeviceInfo():
        mIsEnabledBVRA(false),
        mIsReceivedRING(false)
{
	initialize();
}

void HfpDeviceInfo::initialize()
{
	memset(mDeviceStatus, HFGeneral::Status::STATUSNONE, sizeof(mDeviceStatus));
	memset(mAudioStatus, HFGeneral::Status::STATUSNONE, sizeof(mAudioStatus));
	memset(mCINDIndex, 0, sizeof(mCINDIndex));
	memset(mAGFeature, false, sizeof(mAGFeature));
}

HfpDeviceInfo::~HfpDeviceInfo()
{
}

bool HfpDeviceInfo::setDeviceStatus(int index, int value)
{
	//int type = mCINDIndex[index];
	mDeviceStatus[index] = value;

/*
	if ((type == CIND::DeviceStatus::CALL && value == CIND::Call::INACTIVE) || type == CIND::DeviceStatus::CALLSETUP ||
                type == CIND::DeviceStatus::CALLHELD)
		return true;
*/
	return false;
}

void HfpDeviceInfo::setCallStatus(const std::string &phoneNumber, int index, const std::string &value)
{
	HfpHFCallStatus* localCallStatus = findCallStatusObj(phoneNumber);
	if (localCallStatus == nullptr)
	{
		localCallStatus = new HfpHFCallStatus;
		mCallStatus.insert(std::make_pair(phoneNumber, localCallStatus));
	}
	localCallStatus->setCallStatus(index, value);
}

std::string HfpDeviceInfo::getCallStatus(const std::string &phoneNumber, int index)
{
	HfpHFCallStatus* localCallStatus = findCallStatusObj(phoneNumber);
	if (localCallStatus == nullptr)
		return "";
	return localCallStatus->getCallStatus(index);
}

HfpHFCallStatus* HfpDeviceInfo::findCallStatusObj(const std::string &phoneNumber)
{
	auto localCallStatus = mCallStatus.find(phoneNumber);

	if (localCallStatus != mCallStatus.end())
		return localCallStatus->second;

	return nullptr;
}

void HfpDeviceInfo::clearCLCC()
{
	for (auto& i : mCallStatus)
	{
		if (i.second != nullptr)
			delete i.second;
	}
	mCallStatus.clear();
}

void HfpDeviceInfo::eraseCallStatus(const std::string &phoneNumber)
{
	auto eraseCallStatus = mCallStatus.find(phoneNumber);
	if (eraseCallStatus != mCallStatus.end())
	{
		delete eraseCallStatus->second;
		mCallStatus.erase(phoneNumber);
	}
}
