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

#ifndef HFPHFDEVICESTATUS_H_
#define HFPHFDEVICESTATUS_H_

#include <queue>
#include <unordered_map>

#include "bluetootherrors.h"
#include "hfphfdefines.h"

class HfpHFRole;
class HfpDeviceInfo;
using HFDeviceList =  std::unordered_map<std::string, HfpDeviceInfo*>;

class HfpHFDeviceStatus
{
public:
	HfpHFDeviceStatus(HfpHFRole* roleObj);
	~HfpHFDeviceStatus();

	void updateStatus(const std::string &remoteAddr, std::string &resultCode);
	bool createDeviceInfo(const std::string &remoteAddr);
	bool removeDeviceInfo(const std::string &remoteAddr);
	HfpDeviceInfo* findDeviceInfo(const std::string &remoteAddr) const;
	bool isDeviceAvailable(const std::string &remoteAddr) const;
	BluetoothErrorCode checkAddress(const std::string &remoteAddr) const;
	bool isDeviceConnecting() const;
	HFDeviceList getDeviceInfoList() const { return mHfpDeviceInfo; }
	bool updateSCOStatus(const std::string &remoteAddr, bool status);
	void updateAudioVolume(const std::string &remoteAddr, int volume, bool isUpdated);
	void updateBVRAStatus(bool enabled) { mEnabledBVRA = enabled; }
	bool getBVRAStatus() const { return mEnabledBVRA; }

private:
	void updateCallStatus(const std::string &remoteAddr, const std::string &atCmd, std::string &arguments);
	void updateRingStatus(const std::string &remoteAddr, bool receive);
	void updateCLCC(const std::string &remoteAddr, const std::string &arguments);
	void clearCLCC(const std::string &remoteAddr);
	void eraseCallStatus(const std::string &remoteAddr);
	bool isCallActive(const std::string &remoteAddr);
	void setCINDType(const std::string &arguments);
	void storeCINDIndex(const std::string &type, int index);
	void setCINDValue(const std::string &remoteAddr, const std::string &arguments);
	void setBRSFValue(const std::string &remoteAddr, const std::string &arguments);
	bool setCIEVValue(const std::string &remoteAddr, const std::string &arguments);
	void flushDeviceInfo();

private:
	HFDeviceList mHfpDeviceInfo;
	HfpDeviceInfo* mTempDeviceInfo;
	HfpHFRole* mHFRole;
	std::string mActiveNumber;
	bool mDisconnectedHeldCall;
	bool mReceivedWaitingCall;
	bool mEnabledBVRA;
	int mHasCallStatus;
	std::queue<int> mReceivedATCmd;
};

#endif //HFPHFDEVICESTATUS_H_
