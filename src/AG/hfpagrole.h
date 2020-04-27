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

#ifndef HFPAGROLE_H_
#define HFPAGROLE_H_

#include <glib.h>
#include <luna-service2/lunaservice.hpp>

#include "hfprole.h"

class HfpAGSubscribe;
class HfpAGCallStatus;

namespace pbnjson
{
	class JValue;
}

class HfpAGRole : public HfpRole
{
public:
	HfpAGRole(BluetoothHfpService *service);
	~HfpAGRole();

	void initialize();

private:
	void sendResult(const std::string &resultCode);
	void indicateCall(const std::string &number);
	void requestSCOchannel(bool isOpen, const std::string &address);
	void setCallStatus(const pbnjson::JValue &inputObj, HfpAGCallStatus &callStatus);
	void processSingleCall(const HfpAGCallStatus &callStatus);
	void processMultiCall(int count, const HfpAGCallStatus &callStatus);

	void callStateCb(const pbnjson::JValue &replyObj);
	void deviceStatusCb(const pbnjson::JValue &replyObj);
	void batteryInfoCb(const pbnjson::JValue &replyObj);
	void callVolumeCb(const pbnjson::JValue &replyObj);
	void signalStrengthCb(const pbnjson::JValue &replyObj);
	void networkStatusCb(const pbnjson::JValue &replyObj);
	void networkRoamingCb(const pbnjson::JValue &replyObj);
	void receiveAtCb(const pbnjson::JValue &replyObj);
	void hfpStatusCb(const pbnjson::JValue &replyObj);
	void phoneNumberCb(const pbnjson::JValue &replyObj);

private:
	LSMessageToken mIndicateToken;
	std::string mNetworkName;
	bool mHfpConnected;
	bool mHfpSco;
	HfpAGSubscribe *mSubscribe;
	HfpAGCallStatus *mCallStatus;
};

#endif

// HFPAGROLE_H_
