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

#ifndef HFPAGSUBSCRIBE_H_
#define HFPAGSUBSCRIBE_H_

#include <string>
#include <unordered_map>
#include <algorithm>
#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>

typedef std::function<void(const pbnjson::JValue &)> SubscribeCbFunc;

class HfpAGRole;

class HfpAGSubscribe
{
public:
	HfpAGSubscribe(HfpAGRole *role, LSHandle *handle);
	~HfpAGSubscribe();

	class AGSubscribeInfo
	{
	public:
		AGSubscribeInfo(const int cbType, const std::string &uri, const std::string &payload,
		        const std::string &service, void *context, const bool autoCall, const bool subscribe) :
			cbType(cbType),
			uri(uri),
			payload(payload),
			service(service),
			context(context),
			autoCall(autoCall),
			subscribe(subscribe)
		{
		}

	public:
		int cbType;
		std::string uri;
		std::string payload;
		std::string service;
		void *context;
		bool autoCall;
		bool subscribe;
	};

	enum SubscribeCbType
	{
		CB_TEL_CALL_STATE = 0,
		CB_TEL_SIGNAL_STRENGTH,
		CB_TEL_NETWORK_STATUS,
		CB_TEL_NETWORK_ROAMING,
		CB_TEL_PHONE_NUMBER,
		CB_BT_DEVICE_STATUS,
		CB_BT_HFP_RECEIVEAT,
		CB_BT_HFP_STATUS,
		CB_LB_BATTERY_INFO,
		CB_AUDIO_CALL_VOLUME,
		MAX_CALLBACK_TYPE
	};

	void subscribeServices();
	void subscribeAll();
	void subscribeService(const std::string &service, const bool connected);
	bool subscribe(SubscribeCbType cbType);
	bool cancelSubscribe(SubscribeCbType cbType);
	bool setPayload(SubscribeCbType cbType, const std::string &payload);
	bool setCallbackFunc(SubscribeCbType cbType, SubscribeCbFunc cbFunc);
	HfpAGRole *getParent() { return mHfpAGRole; }

private:
	static bool subscribeCb(LSHandle *handle, LSMessage *reply, void *context);

private:
	SubscribeCbFunc mSubscribeCbs[MAX_CALLBACK_TYPE];
	LSMessageToken mLSCallToken[MAX_CALLBACK_TYPE];
	HfpAGRole *mHfpAGRole;
	LSHandle *mHandle;
	bool mServerStatusRegistered;

	std::unordered_map<int, HfpAGSubscribe::AGSubscribeInfo> mAGSubscribeInfo = {
		{CB_TEL_CALL_STATE, {CB_TEL_CALL_STATE, "luna://com.palm.telephony/callStateQuery", "{\"subscribe\":true}", "com.palm.telephony", nullptr, true, true}},
		{CB_TEL_SIGNAL_STRENGTH, {CB_TEL_SIGNAL_STRENGTH, "luna://com.palm.telephony/signalStrengthQuery", "{\"subscribe\":true}", "com.palm.telephony", nullptr, true, true}},
		{CB_TEL_NETWORK_STATUS, {CB_TEL_NETWORK_STATUS, "luna://com.palm.telephony/networkStatusQuery", "{\"subscribe\":true}", "com.palm.telephony", nullptr, true, true}},
		{CB_TEL_NETWORK_ROAMING, {CB_TEL_NETWORK_ROAMING, "luna://com.palm.telephony/isNetworkRoaming", "{\"subscribe\":true}", "com.palm.telephony", nullptr, true, true}},
		{CB_TEL_PHONE_NUMBER, {CB_TEL_PHONE_NUMBER, "luna://com.palm.telephony/phoneNumberQuery", "{}", "com.palm.telephony", nullptr, false, false}},
		{CB_BT_DEVICE_STATUS, {CB_BT_DEVICE_STATUS, "luna://com.webos.service.bluetooth2/device/getStatus", "{\"subscribe\":true}", "com.webos.service.bluetooth2", nullptr, true, true}},
		{CB_BT_HFP_RECEIVEAT, {CB_BT_HFP_RECEIVEAT, "luna://com.webos.service.bluetooth2/hfp/receiveAT", "{\"subscribe\":true}", "com.webos.service.bluetooth2", nullptr, true, true}},
		{CB_BT_HFP_STATUS, {CB_BT_HFP_STATUS, "luna://com.webos.service.bluetooth2/hfp/getStatus", "{\"subscribe\":true}", "com.webos.service.bluetooth2", nullptr, true, true}},
		{CB_LB_BATTERY_INFO, {CB_LB_BATTERY_INFO, "luna://com.palm.lunabus/signal/addmatch", "{\"category\":\"/com/palm/power\",\"method\":\"batteryStatus\"}", "com.palm.lunabus", nullptr, true, true}},
		{CB_AUDIO_CALL_VOLUME, {CB_AUDIO_CALL_VOLUME, "luna://com.palm.audio/phone/status", "{\"subscribe\":true}", "com.palm.audio", nullptr, true, true}}
	};
};

#endif

// HFPAGSUBSCRIBE_H_
