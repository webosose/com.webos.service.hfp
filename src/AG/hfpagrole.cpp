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

#include "hfpagrole.h"
#include "bluetoothhfpservice.h"
#include "bluetootherrors.h"
#include "ls2utils.h"
#include "utils.h"
#include "logging.h"
#include "defines.h"
#include "hfpagsubscribe.h"
#include "hfpagcallstatus.h"

using namespace std::placeholders;

HfpAGRole::HfpAGRole(BluetoothHfpService *service) :
        HfpRole(service),
        mNetworkName(""),
        mHfpConnected(false),
        mHfpSco(false),
        mIndicateToken(LSMESSAGE_TOKEN_INVALID)
{
	mSubscribe = new HfpAGSubscribe(this, getService()->get());
	mCallStatus = new HfpAGCallStatus();
}

HfpAGRole::~HfpAGRole()
{
	if (mSubscribe)
	{
		delete mSubscribe;
		mSubscribe = nullptr;
	}
	if (mCallStatus)
	{
		delete mCallStatus;
		mCallStatus = nullptr;
	}
}

void HfpAGRole::initialize()
{
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_TEL_CALL_STATE, std::bind(&HfpAGRole::callStateCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_BT_DEVICE_STATUS, std::bind(&HfpAGRole::deviceStatusCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_LB_BATTERY_INFO, std::bind(&HfpAGRole::batteryInfoCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_AUDIO_CALL_VOLUME, std::bind(&HfpAGRole::callVolumeCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_TEL_SIGNAL_STRENGTH, std::bind(&HfpAGRole::signalStrengthCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_TEL_NETWORK_STATUS, std::bind(&HfpAGRole::networkStatusCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_TEL_NETWORK_ROAMING, std::bind(&HfpAGRole::networkRoamingCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_BT_HFP_RECEIVEAT, std::bind(&HfpAGRole::receiveAtCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_BT_HFP_STATUS, std::bind(&HfpAGRole::hfpStatusCb, this, _1));
	mSubscribe->setCallbackFunc(HfpAGSubscribe::CB_TEL_PHONE_NUMBER, std::bind(&HfpAGRole::phoneNumberCb, this, _1));

	mSubscribe->subscribeServices();
}

void HfpAGRole::deviceStatusCb(const pbnjson::JValue &replyObj)
{
	if (!replyObj.hasKey("devices"))
		return;

	auto devicesObjArray = replyObj["devices"];
	for (int i = 0; i < devicesObjArray.arraySize(); i++)
	{
		auto deviceObj = devicesObjArray[i];
		if (!deviceObj.hasKey("address") || !deviceObj.hasKey("connectedProfiles"))
			continue;

		auto address = deviceObj["address"].asString();
		auto connectedProfiles = deviceObj["connectedProfiles"];
		bool bFound = false;
		for (int n = 0; n < connectedProfiles.arraySize(); n++)
		{
			std::string profile = connectedProfiles[n].asString();
			if (profile != "hfp")
				continue;

			bFound = true;
			addConnectedDevice(address);
			BT_DEBUG("Add device:%s", address.c_str());
			break;
		}

		if (!bFound)
			removeConnectedDevice(address);
	}
}

void HfpAGRole::batteryInfoCb(const pbnjson::JValue &replyObj)
{
	if (!replyObj.hasKey("percent"))
		return;

	int percent = replyObj["percent"].asNumber<int32_t>();
	int batteryLevel = percentToIndRange(percent);
	if (mCallStatus->getCINDState(HfpAGCallStatus::LEVEL) == batteryLevel)
		return;

	mCallStatus->setCINDState(HfpAGCallStatus::LEVEL, batteryLevel);
	BT_DEBUG("CIND LEVEL:%d", mCallStatus->getCINDState(HfpAGCallStatus::LEVEL));

	std::string resultCode("+CIEV:");
	resultCode += std::to_string(BTA_AG_IND_BATTCHG);
	resultCode += ",";
	resultCode += std::to_string(batteryLevel);
	sendResult(resultCode);
}

void HfpAGRole::callVolumeCb(const pbnjson::JValue &replyObj)
{
	if (!replyObj.hasKey("volume"))
		return;

	int volume = replyObj["volume"].asNumber<int32_t>();
	if (volume <= 10)
		volume = 0;
	else
		volume = (volume-HFP_DEFUAL_VOLUME)/BLUETOOTH_HFP_GAIN_STEP;
	std::string resultCode("+VGS:");
	resultCode += std::to_string(volume);
	sendResult(resultCode);
}

void HfpAGRole::signalStrengthCb(const pbnjson::JValue &replyObj)
{
	int bars = 0;
	int maxBars = 0;
	if (replyObj.hasKey("eventSignal"))
	{
		auto eventSignalObj = replyObj["eventSignal"];
		if (eventSignalObj.hasKey("bars"))
			bars = eventSignalObj["bars"].asNumber<int32_t>();
		if (eventSignalObj.hasKey("maxBars"))
			maxBars = eventSignalObj["maxBars"].asNumber<int32_t>();
	}
	else
	{
		if (replyObj.hasKey("bars"))
			bars = replyObj["bars"].asNumber<int32_t>();
		if (replyObj.hasKey("maxBars"))
			maxBars = replyObj["maxBars"].asNumber<int32_t>();
	}

	int percent = 0;
	if ((bars > 0) && (maxBars > 0))
		percent = (bars*100)/maxBars;

	int strength = percentToIndRange(percent);
	if (mCallStatus->getCINDState(HfpAGCallStatus::STRENGTH) == strength)
		return;

	mCallStatus->setCINDState(HfpAGCallStatus::STRENGTH, strength);
	BT_DEBUG("CIND STRENGTH:%d", mCallStatus->getCINDState(HfpAGCallStatus::STRENGTH));

	std::string resultCode("+CIEV:");
	resultCode += std::to_string(BTA_AG_IND_SIGNAL);
	resultCode += ",";
	resultCode += std::to_string(strength);
	sendResult(resultCode);
}

void HfpAGRole::networkStatusCb(const pbnjson::JValue &replyObj)
{
	if (replyObj.hasKey("extended"))
	{
		auto extendedObj = replyObj["extended"];
		if (extendedObj.hasKey("state"))
		{
			int registration = 0;
			std::string state = extendedObj["state"].asString();
			if (state.compare("service") == 0)
				registration = 1;
			else
				registration = 0;
			if (mCallStatus->getCINDState(HfpAGCallStatus::REGISTRATION) != registration)
			{
				mCallStatus->setCINDState(HfpAGCallStatus::REGISTRATION, registration);
				BT_DEBUG("(1)CIND REGISTRATION:%d", mCallStatus->getCINDState(HfpAGCallStatus::REGISTRATION));

				std::string resultCode("+CIEV:");
				resultCode += std::to_string(BTA_AG_IND_SERVICE);
				resultCode += "," + std::to_string(registration);
				sendResult(resultCode);
			}
		}
		if (extendedObj.hasKey("networkName"))
		{
			std::string networkName = extendedObj["networkName"].asString();
			if (networkName != "")
				mNetworkName = networkName;
		}
	}

	if (replyObj.hasKey("eventnetwork"))
	{
		int registration = 0;
		std::string state = replyObj["state"].asString();
		if (state.compare("service") == 0)
			registration = 1;
		else
			registration = 0;
		if (mCallStatus->getCINDState(HfpAGCallStatus::REGISTRATION) != registration)
		{
			mCallStatus->setCINDState(HfpAGCallStatus::REGISTRATION, registration);
			BT_DEBUG("(2)CIND REGISTRATION:%d", mCallStatus->getCINDState(HfpAGCallStatus::REGISTRATION));

			std::string resultCode("+CIEV:");
			resultCode += std::to_string(BTA_AG_IND_SERVICE);
			resultCode += "," + std::to_string(registration);
			sendResult(resultCode);
		}
	}
}

void HfpAGRole::networkRoamingCb(const pbnjson::JValue &replyObj)
{
	if (!replyObj.hasKey("RoamingStatus"))
		return;

	auto roamingStatusObj = replyObj["RoamingStatus"];
	if (!roamingStatusObj.hasKey("isRoaming"))
		return;

	bool isRoaming = roamingStatusObj["isRoaming"].asBool();
	int roaming = isRoaming? 1 : 0;
	if (mCallStatus->getCINDState(HfpAGCallStatus::ROAMING) == roaming)
		return;

	mCallStatus->setCINDState(HfpAGCallStatus::ROAMING, roaming);
	BT_DEBUG("CIND ROAMING:%d", mCallStatus->getCINDState(HfpAGCallStatus::ROAMING));

	std::string resultCode("+CIEV:");
	resultCode += std::to_string(BTA_AG_IND_ROAM);
	resultCode += "," + std::to_string(roaming);
	sendResult(resultCode);
}

void HfpAGRole::hfpStatusCb(const pbnjson::JValue &replyObj)
{
	if (replyObj.hasKey("connected"))
	{
		bool connected = replyObj["connected"].asBool();
		if (mHfpConnected != connected) {
			if (connected == false)
				LSCallOneReply(getService()->get(), "luna://com.palm.audio/state/setNREC", "{\"NRECOn\":true}", nullptr, nullptr, nullptr, nullptr);
			mHfpConnected = connected;
		}
	}
	if (replyObj.hasKey("sco"))
	{
		bool sco = replyObj["sco"].asBool();
		if (mHfpSco != sco) {
			BT_DEBUG("sco:%d", sco);
			if (sco == true)
				LSCallOneReply(getService()->get(), "luna://com.palm.audio/phone/enableScenario", "{\"scenario\":\"phone_bluetooth_sco\"}", nullptr, nullptr, nullptr, nullptr);
			else
				LSCallOneReply(getService()->get(), "luna://com.palm.audio/phone/disableScenario", "{\"scenario\":\"phone_bluetooth_sco\"}", nullptr, nullptr, nullptr, nullptr);
			mHfpSco = sco;
		}
	}
}

void HfpAGRole::phoneNumberCb(const pbnjson::JValue &replyObj)
{
	if (replyObj.hasKey("extended"))
	{
		char buf[128];
		auto extendedObj = replyObj["extended"];
		int offset = snprintf(buf, sizeof(buf), "+CNUM:");

		if (extendedObj.hasKey("number"))
		{
			std::string numStr = extendedObj["number"].asString();
			const char *number = numStr.c_str();

			snprintf(buf + offset, sizeof(buf) - offset, ",\"%s\",%d,,4", number, *number == '+' ? TYPE_INTERNATIONAL : TYPE_NATIONAL);
		}
		sendResult(buf);
	}
}

void HfpAGRole::receiveAtCb(const pbnjson::JValue &replyObj)
{
	gchar param[128];

	std::string command = replyObj["command"].asString();
	std::string type = replyObj["type"].asString();
	std::string arguments = replyObj["arguments"].asString();
	BT_DEBUG("[receive AT] command:%s, type:%s, arguments:%s", command.c_str(), type.c_str(), arguments.c_str());
	if (type == "set")
	{
		if (command.find("+VTS=") == 0)
		{
			snprintf(param, sizeof(param), "{\"toneSequence\":\"%s\"}", replyObj["arguments"].asString().c_str());
			LSCallOneReply(getService()->get(), "luna://com.palm.telephony/sendDtmf", param, nullptr, nullptr, nullptr, nullptr);
		}
		else if (command.find("+NREC=") == 0)
		{
			if (replyObj["arguments"].asNumber<int32_t>() == 0)
			{
				snprintf(param, sizeof(param), "{\"NRECOn\":false}");
				LSCallOneReply(getService()->get(), "luna://com.palm.audio/state/setNREC", param, nullptr, nullptr, nullptr, nullptr);
			}
		}
		else if (command.find("+VGS=") == 0)
		{
			snprintf(param, sizeof(param), "{\"scenario\":\"phone_bluetooth_sco\",\"volume\":%d}", replyObj["arguments"].asNumber<int32_t>());
			LSCallOneReply(getService()->get(), "luna://com.palm.audio/phone/setVolume", param, nullptr, nullptr, nullptr, nullptr);
		}
	}
	else if (type == "action")
	{
		if (command == "+CNUM")
		{
			mSubscribe->subscribe(HfpAGSubscribe::CB_TEL_PHONE_NUMBER);
		}
	}
	else if (type == "read")
	{
		if (command == "+CIND")
		{
			sendResult(mCallStatus->getCINDResult());
			BT_DEBUG("sendResult:%s", mCallStatus->getCINDResult().c_str());
		}
	}
}

void HfpAGRole::sendResult(const std::string &resultCode)
{
	gchar param[512];
	std::string address;
	for (int i = 0; i < mConnectedDevices.size(); i++)
	{
		address = mConnectedDevices[i];
		snprintf(param, 512, "{\"address\":\"%s\",\"resultCode\":\"%s\"}",
			address.c_str(), resultCode.c_str());
		LSCallOneReply(getService()->get(), "luna://com.webos.service.bluetooth2/hfp/sendResult",
			param, nullptr, nullptr, nullptr, nullptr);
	}
}

void HfpAGRole::indicateCall(const std::string &number)
{
	if (mConnectedDevices.size() <= 0)
		return;

	if (mIndicateToken != LSMESSAGE_TOKEN_INVALID) {
		LSCallCancel(getService()->get(), mIndicateToken, nullptr);
		mIndicateToken = LSMESSAGE_TOKEN_INVALID;
	}

	auto indicateCallCb = [](LSHandle *handle, LSMessage *reply, void *context) -> bool {
		return true;
	};

	gchar param[512];
	std::string address = mConnectedDevices[0];
	snprintf(param, 512, "{\"address\":\"%s\",\"number\":\"%s\", \"subscribe\":true}",
		address.c_str(), number.c_str());
	LSCall(getService()->get(), "luna://com.webos.service.bluetooth2/hfp/indicateCall",
		param, indicateCallCb, nullptr, &mIndicateToken, nullptr);
	BT_DEBUG("indicateCall(address:%s, number:%s, token:%lu)", address.c_str(), number.c_str(), mIndicateToken);
}

void HfpAGRole::requestSCOchannel(bool isOpen, const std::string &address)
{
	if (mConnectedDevices.size() < 1)
		return;

	char param[64];
	snprintf(param, sizeof(param), "{\"address\":\"%s\"}", address.c_str());

	if (isOpen)
		LSCallOneReply(getService()->get(), "luna://com.webos.service.bluetooth2/hfp/openSCO", param, nullptr, nullptr, nullptr, nullptr);
	else
		LSCallOneReply(getService()->get(), "luna://com.webos.service.bluetooth2/hfp/closeSCO", param, nullptr, nullptr, nullptr, nullptr);
}

void HfpAGRole::setCallStatus(const pbnjson::JValue &inputObj, HfpAGCallStatus &callStatus)
{
	for (int i = 0; i < inputObj.arraySize(); i++)
	{
		auto lineObj = inputObj[i];
		if (lineObj.hasKey("state"))
			callStatus.setCallStatus(i, lineObj["state"].asString());

		if (!lineObj.hasKey("calls"))
			continue;

		auto callsObjArray = lineObj["calls"];
		auto callObj = callsObjArray[0];
		if (callObj.hasKey("id"))
			callStatus.setCallInfo(i, HfpAGCallStatus::INDEX, callObj["id"].asNumber<int32_t>());
		if (callObj.hasKey("origin"))
		{
			std::string origin = callObj["origin"].asString();
			if (origin.compare("outgoing") == 0)
				callStatus.setCallInfo(i, HfpAGCallStatus::DIRECTION, 0);
			else if (origin.compare("incoming") == 0)
				callStatus.setCallInfo(i, HfpAGCallStatus::DIRECTION, 1);
		}

		if (callObj.hasKey("imsconfinfo"))
		{
			bool imsconfinfo = callObj["imsconfinfo"].asBool();
			if (imsconfinfo)
				callStatus.setCallInfo(i, HfpAGCallStatus::MULTIPARTY, 1);
			else
				callStatus.setCallInfo(i, HfpAGCallStatus::MULTIPARTY, 0);
		}
		if (callObj.hasKey("address"))
		{
			std::string address = callObj["address"].asString();
			callStatus.setCallNumber(i, address);
			if(address[0] == '+')
				callStatus.setCallInfo(i, HfpAGCallStatus::TYPE, TYPE_INTERNATIONAL);
			else
				callStatus.setCallInfo(i, HfpAGCallStatus::TYPE, TYPE_NATIONAL);
		}
	}
}

void HfpAGRole::processSingleCall(const HfpAGCallStatus &callStatus)
{
	if (nullptr == mCallStatus)
		return;

	callStatus.printCallInfo("SNC");
	mCallStatus->printCallInfo("SCC");
	mCallStatus->printCINDState("SB");

	for (int i = 0; i < MAX_CALL; i++)
	{
		if (0 == callStatus.getCallInfo(i, HfpAGCallStatus::INDEX))
			continue;

		int status = callStatus.getCallInfo(i, HfpAGCallStatus::STATUS);
		if (HfpAGCallStatus::INCOMING == status)
		{
			indicateCall(callStatus.getCallNumber(i));
			mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 1);
		}
		else if (HfpAGCallStatus::ACTIVE == status)
		{
			if ((mCallStatus->getCINDState(HfpAGCallStatus::CALLSETUP) > 0) && (0 == mCallStatus->getCINDState(HfpAGCallStatus::CALL)))
			{
				mCallStatus->setCINDState(HfpAGCallStatus::CALL, 1);
				mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
				sendResult("OK");
				sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALL));
				sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLSETUP));
			}
			else if (2 == mCallStatus->getCINDState(HfpAGCallStatus::CALLHOLD))
			{
				mCallStatus->setCINDState(HfpAGCallStatus::CALL, 1);
				mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
				mCallStatus->setCINDState(HfpAGCallStatus::CALLHOLD, 0);
				sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLHOLD));
			}
			else
			{
				mCallStatus->setCINDState(HfpAGCallStatus::CALL, 1);
				mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
			}

			if (mIndicateToken != LSMESSAGE_TOKEN_INVALID) {
				LSCallCancel(getService()->get(), mIndicateToken, nullptr);
				mIndicateToken = LSMESSAGE_TOKEN_INVALID;
			}
			requestSCOchannel(true, mConnectedDevices[0]);
		}
		else if (HfpAGCallStatus::DIALING == status)
		{
			mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 2);
			sendResult("OK");
			sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLSETUP));
		}
		else if (HfpAGCallStatus::DISCONNECTED == status)
		{
			mCallStatus->setCINDState(HfpAGCallStatus::CALL, 0);
			mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
			sendResult("+CHUP");

			if (mIndicateToken != LSMESSAGE_TOKEN_INVALID) {
				LSCallCancel(getService()->get(), mIndicateToken, nullptr);
				mIndicateToken = LSMESSAGE_TOKEN_INVALID;
			}
			requestSCOchannel(false, mConnectedDevices[0]);
		}
	}
	mCallStatus->printCINDState("SA");
}

void HfpAGRole::processMultiCall(int count, const HfpAGCallStatus &callStatus)
{
	if (nullptr == mCallStatus)
		return;

	callStatus.printCallInfo("MNC");
	mCallStatus->printCallInfo("MCC");
	mCallStatus->printCINDState("MB");

	uint32_t disconnected = 0;
	uint32_t active = 0;
	uint32_t hold = 0;
	uint32_t incoming = 0;
	std::string number = "";
	for (int i = 0; i < count; i++)
	{
		int callIndex = callStatus.getCallInfo(i, HfpAGCallStatus::INDEX);
		if (0 == callIndex)
			continue;

		int status = callStatus.getCallInfo(i, HfpAGCallStatus::STATUS);
		if (HfpAGCallStatus::DISCONNECTED == status)
			disconnected = callIndex;
		else if (HfpAGCallStatus::ACTIVE == status)
			active = callIndex;
		else if (HfpAGCallStatus::HOLD == status)
			hold = callIndex;
		else if (HfpAGCallStatus::INCOMING == status)
		{
			incoming = 1;
			number = callStatus.getCallNumber(i);
		}
	}

	uint32_t oldDisconnected = 0;
	uint32_t oldActive = 0;
	uint32_t oldHold = 0;
	uint32_t oldIncoming = 0;
	for (int i = 0; i < MAX_CALL; i++)
	{
		int callIndex = mCallStatus->getCallInfo(i, HfpAGCallStatus::INDEX);
		if (0 == callIndex)
			continue;

		int status = mCallStatus->getCallInfo(i, HfpAGCallStatus::STATUS);
		if (HfpAGCallStatus::DISCONNECTED == status)
			oldDisconnected = callIndex;
		else if (HfpAGCallStatus::ACTIVE == status)
			oldActive = callIndex;
		else if (HfpAGCallStatus::HOLD == status)
			oldHold = callIndex;
		else if (HfpAGCallStatus::INCOMING == status)
			oldIncoming = callIndex;
	}

	if ((active > 0) && (incoming > 0))
	{
		mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 1);
		sendResult("+CCWA:" + number);
	}
	else if ((active > 0) && (disconnected > 0))
	{
		if (disconnected == oldIncoming)
			mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
		if (!hold)
			mCallStatus->setCINDState(HfpAGCallStatus::CALLHOLD, 0);
	}
	else if ((active > 0) && (hold > 0))
	{
		mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
		mCallStatus->setCINDState(HfpAGCallStatus::CALL, 1);
		mCallStatus->setCINDState(HfpAGCallStatus::CALLHOLD, 1);

		if (active == oldIncoming)
			sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLSETUP));
		sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLHOLD));
	}
	else if ((hold > 0) && (disconnected > 0) && (disconnected == oldActive))
	{
		mCallStatus->setCINDState(HfpAGCallStatus::CALL, 0);
		mCallStatus->setCINDState(HfpAGCallStatus::CALLHOLD, 2);
		sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLHOLD));
	}
	else if ((disconnected > 0) && (incoming > 0))
	{
		mCallStatus->setCINDState(HfpAGCallStatus::CALL, 0);
		sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALL));

		mCallStatus->setCINDState(HfpAGCallStatus::CALL, 1);
		mCallStatus->setCINDState(HfpAGCallStatus::CALLSETUP, 0);
		sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALL));
		sendResult(mCallStatus->getCIEVResult(HfpAGCallStatus::CALLSETUP));
	}
	mCallStatus->printCINDState("MA");
}

void HfpAGRole::callStateCb(const pbnjson::JValue &replyObj)
{
	if (nullptr == mCallStatus)
		return;

	if (!replyObj.hasKey("lines"))
	{
		mCallStatus->clearCallInfo();
		return;
	}

	auto linesObjArray = replyObj["lines"];
	if (0 == linesObjArray.arraySize())
	{
		mCallStatus->clearCallInfo();
		return;
	}

	HfpAGCallStatus newCallStatus;
	setCallStatus(linesObjArray, newCallStatus);
	newCallStatus.printCallInfo("NC");
	mCallStatus->printCallInfo("CC");

	if (1 == linesObjArray.arraySize())
		processSingleCall(newCallStatus);
	else
		processMultiCall(linesObjArray.arraySize(), newCallStatus);

	for (int i = 0; i < MAX_CALL; i++)
	{
		if (0 == newCallStatus.getCallInfo(i, HfpAGCallStatus::INDEX))
		{
			mCallStatus->setCallInfo(i, HfpAGCallStatus::INDEX, -1);
			continue;
		}
		mCallStatus->setCallInfo(i, HfpAGCallStatus::DIRECTION, newCallStatus.getCallInfo(i, HfpAGCallStatus::DIRECTION));
		mCallStatus->setCallInfo(i, HfpAGCallStatus::INDEX, newCallStatus.getCallInfo(i, HfpAGCallStatus::INDEX));
		mCallStatus->setCallInfo(i, HfpAGCallStatus::MODE, newCallStatus.getCallInfo(i, HfpAGCallStatus::MODE));
		mCallStatus->setCallInfo(i, HfpAGCallStatus::MULTIPARTY, newCallStatus.getCallInfo(i, HfpAGCallStatus::MULTIPARTY));
		mCallStatus->setCallInfo(i, HfpAGCallStatus::TYPE, newCallStatus.getCallInfo(i, HfpAGCallStatus::TYPE));
		mCallStatus->setCallInfo(i, HfpAGCallStatus::STATUS, newCallStatus.getCallInfo(i, HfpAGCallStatus::STATUS));
		mCallStatus->setCallNumber(i, newCallStatus.getCallNumber(i));
	}
}
