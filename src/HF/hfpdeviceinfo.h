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

#ifndef HFPDEVICEINFO_H_
#define HFPDEVICEINFO_H_

#include "hfphfdefines.h"
#include "hfphfcallstatus.h"
#include <unordered_map>

using CallStatusList =  std::unordered_map<std::string, HfpHFCallStatus*>;

class HfpDeviceInfo
{
public:
	HfpDeviceInfo();
	~HfpDeviceInfo();

	bool setDeviceStatus(int index, int value);
	void setAudioStatus(int index, int value) {mAudioStatus[index] = value; }
	void setAGFeature(int index, bool value) { mAGFeature[index] = value; }
	void setBVRA(bool isEnabled) noexcept { mIsEnabledBVRA = isEnabled; }
	void setRING(bool received) noexcept { mIsReceivedRING = received; }
	void setCINDIndex(int index, int type) { mCINDIndex[index] = type; }
	void setCallStatus(const std::string &phoneNumber, int index, const std::string &value);
	void setNetworkOperatorName(std::string name) { mNetworkOperatorName = name; }
	void setNetworkRegistrationStatus(std::string status) { mNetworkRegistrationStatus = status; }
	void eraseCallStatus(const std::string &phoneNumber);
	void clearCLCC();

	int getDeviceStatus(int index) const { return mDeviceStatus[index]; }
	int getAudioStatus(int index) const {return mAudioStatus[index]; }
	bool getAGFeature(int index) const { return mAGFeature[index]; }
	bool getRING() const noexcept { return mIsReceivedRING; }
	bool getBVRA() const noexcept { return mIsEnabledBVRA; }
	std::string getCallStatus(const std::string &phoneNumber, int index);
	int getCINDIndex(int index) const { return mCINDIndex[index]; }
	CallStatusList getCallStatusList() const noexcept { return mCallStatus; }
	std::string getAdapterAddress() const {return mAdapterAddress;}
	std::string getNetworkOperatorName() const { return mNetworkOperatorName; }
	std::string getNetworkRegistrationStatus() const { return mNetworkRegistrationStatus; }

private:
	void initialize();
	HfpHFCallStatus* findCallStatusObj(const std::string &phoneNumber);

private:
	int mDeviceStatus[CIND::DeviceStatus::MAXSTATUS];
	int mAudioStatus[SCO::DeviceStatus::MAXSTATUS];
	bool mAGFeature[BRSF::DeviceStatus::MAXSTATUS];
	bool mIsEnabledBVRA;
	bool mIsReceivedRING;
	int mCINDIndex[CIND::DeviceStatus::MAXSTATUS];
	CallStatusList mCallStatus;
	std::string mAdapterAddress;
	std::string mNetworkOperatorName;
	std::string mNetworkRegistrationStatus;
};
#endif //__HFPDEVICEINFO_H_
