﻿// Copyright (c) 2020 LG Electronics, Inc.
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

#include "bluetoothhfpservice.h"
#include "logging.h"
#include "utils.h"
#include "ls2utils.h"
#include "hfphfrole.h"
#include "hfpdeviceinfo.h"
#include "hfphfdevicestatus.h"
#include "hfphfsubscribe.h"
#include "hfpofonomanager.h"
#include "hfpofonomodem.h"
#include "hfpofonovoicecallmanager.h"

HfpHFRole::HfpHFRole(BluetoothHfpService *service) :
        HfpRole(service),
        mGetStatusSubscription(nullptr),
	mHFLS2Call(nullptr),
	mHFDevice(nullptr),
	mNameWatch(G_BUS_TYPE_SYSTEM, "org.ofono")
{
	LS_CREATE_CATEGORY_BEGIN(HfpHFRole, adapter)
		LS_CATEGORY_METHOD(answerCall)
		LS_CATEGORY_METHOD(terminateCall)
		LS_CATEGORY_METHOD(getStatus)
		LS_CATEGORY_METHOD(releaseHeldCalls)
		LS_CATEGORY_METHOD(releaseActiveCalls)
		LS_CATEGORY_METHOD(holdActiveCalls)
		LS_CATEGORY_METHOD(mergeCall)
		LS_CATEGORY_METHOD(setVolume)
		LS_CATEGORY_METHOD(call)
		LS_CATEGORY_METHOD(setVoiceRecognition)
	LS_CREATE_CATEGORY_END

	getService()->registerCategory("/hf", LS_CATEGORY_TABLE_NAME(adapter), nullptr, nullptr);
	getService()->setCategoryData("/hf", this);

	mLSHandle = getService()->get();
}

HfpHFRole::~HfpHFRole()
{
	if (mHFDevice != nullptr)
		delete mHFDevice;
	if (mGetStatusSubscription != nullptr)
		delete mGetStatusSubscription;
	if (mHFLS2Call != nullptr)
		delete mHFLS2Call;
	for (auto& iterContext : mContextList)
	{
		if (iterContext != nullptr)
			delete iterContext;
	}
	mContextList.clear();
}

void HfpHFRole::initialize()
{
	mHFDevice = new HfpHFDeviceStatus(this);
	mHFLS2Call = new HfpHFLS2Call();
	mHFSubscribe = new HfpHFSubscribe();

	mContextList.reserve(HFLS2::APIName::MAXVALUE);
	mContextList = {new LSContext(HFLS2::APIName::ADAPTERGETSTATUS, "", this, LSMESSAGE_TOKEN_INVALID),
                        new LSContext(HFLS2::APIName::RECEIVERESULT, "", this, LSMESSAGE_TOKEN_INVALID)};
	mScoContextList.reserve(HFLS2::APIName::MAXVALUE);

	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSRegisterServerStatusEx(this->mLSHandle, HFLS2::BTSERVICE.c_str(), mHFSubscribe->registerService, this, nullptr, &lserror))
	{
		BT_DEBUG("LS2 subscription is failed");
	}

	mNameWatch.watch([this](bool available) {
		if (available)
			createOfonoManager();
		else
			destroyOfonoManager();
	});
}

void HfpHFRole::createOfonoManager()
{
	mHfpOfonoManager = new HfpOfonoManager("/", this);
}

void HfpHFRole::destroyOfonoManager()
{
	if (mHfpOfonoManager)
		delete mHfpOfonoManager;
}

void HfpHFRole::unsubscribeService(HFLS2::APIName apiName)
{
	int index = findContextIndex(apiName);
	if (index == HFLS2::INVALIDINDEX || std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]) == LSMESSAGE_TOKEN_INVALID)
	{
		BT_DEBUG("LSCallCancel failed, token is invalid");
		return;
	}
	unsubscribeService(index);
}

void HfpHFRole::unsubscribeService(const std::string &remoteAddr)
{
	int index = findContextIndex(remoteAddr);
	if (index == HFLS2::INVALIDINDEX || std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]) == LSMESSAGE_TOKEN_INVALID)
	{
		BT_DEBUG("LSCallCancel failed, index is invalid");
		return;
	}
	unsubscribeService(index);
}

void HfpHFRole::unsubscribeScoService(const std::string &remoteAddr, const std::string &adapterAddr)
{
	int index = findScoContextIndex(remoteAddr,adapterAddr);
	if (index == HFLS2::INVALIDINDEX || std::get<HFLS2::ScoContextData::SCOTOKEN>(*mScoContextList[index]) == LSMESSAGE_TOKEN_INVALID)
	{
		BT_DEBUG("LSCallCancel failed, index is invalid");
		return;
	}

	LSCallCancel(mLSHandle, std::get<HFLS2::ScoContextData::SCOTOKEN>(*mScoContextList[index]), nullptr);
	BT_DEBUG("Unsubscribe ScoService: %d", std::get<HFLS2::ScoContextData::SCOAPINAME>(*mScoContextList[index]));
	delete mScoContextList[index];
	mScoContextList.erase(mScoContextList.begin() + index);
}

void HfpHFRole::unsubscribeScoServicebyAdapterAddress(const std::string &adapterAddr)
{
	BT_DEBUG("");
	LSError lserror;
	LSErrorInit(&lserror);
	auto it = mScoContextList.begin();
	while (it != mScoContextList.end())
	{
		if (std::get<HFLS2::ScoContextData::ADAPTERADDRESS>(*(*it)).compare(adapterAddr) == 0)
		{
			LSCallCancel(mLSHandle, std::get<HFLS2::ScoContextData::SCOTOKEN>(*(*it)), &lserror);
			delete *it;
			it = mScoContextList.erase(it);
		}
		else
		{
			it++;
		}
	}
}


void HfpHFRole::unsubscribeService(int index)
{
	LSCallCancel(mLSHandle, std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]), nullptr);
	BT_DEBUG("Unsubscribe Service: %d", std::get<HFLS2::ContextData::APINAME>(*mContextList[index]));
}

void HfpHFRole::unsubscribeServiceAll()
{
	LSError lserror;
	LSErrorInit(&lserror);
	for (int i = 0; i < mContextList.size(); i++)
	{
		if (std::get<HFLS2::ContextData::TOKEN>(*mContextList[i]) != LSMESSAGE_TOKEN_INVALID)
		{
			LSCallCancel(mLSHandle, std::get<HFLS2::ContextData::TOKEN>(*mContextList[i]), &lserror);
		}
	}

	for (int i = 0; i < mScoContextList.size(); i++)
	{
		if (std::get<HFLS2::ScoContextData::SCOTOKEN>(*mScoContextList[i]) != LSMESSAGE_TOKEN_INVALID)
		{
			LSCallCancel(mLSHandle, std::get<HFLS2::ScoContextData::SCOTOKEN>(*mScoContextList[i]), &lserror);
		}
	}

	BT_DEBUG("Unregister callbacks");
}

void HfpHFRole::subscribeService()
{
	LSError lserror;
	LSErrorInit(&lserror);

	std::string lunaCmd = HFLS2::BTLSCALL + HFLS2::LUNAADAPTERGETSTATUS;
	unsubscribeService(HFLS2::APIName::ADAPTERGETSTATUS);
	int index = findContextIndex(HFLS2::APIName::ADAPTERGETSTATUS);
	LSCall(this->mLSHandle, lunaCmd.c_str(), HFLS2::LUNASUBSCRIBE.c_str(), mHFSubscribe->getAdapterStatusCallback,
                mContextList[index], &std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]), &lserror);

	/*lunaCmd = HFLS2::BTLSCALL + HFLS2::LUNAGETSTATUS;
	unsubscribeService(HFLS2::APIName::GETSTATUS);

	int index = findContextIndex(HFLS2::APIName::GETSTATUS);
	LSCall(this->mLSHandle, lunaCmd.c_str(), HFLS2::LUNASUBSCRIBE.c_str(), mHFSubscribe->getStatusCallback,
                mContextList[index], &std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]), &lserror);*/

	unsubscribeService(HFLS2::APIName::RECEIVERESULT);
	index = findContextIndex(HFLS2::APIName::RECEIVERESULT);
	lunaCmd = HFLS2::BTLSCALL + HFLS2::LUNARECEIVERESULT;
	LSCall(this->mLSHandle, lunaCmd.c_str(), HFLS2::LUNASUBSCRIBE.c_str(), mHFSubscribe->receiveResultCallback,
                mContextList[index], &std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]), &lserror);

	BT_DEBUG("LS2 subscription is success");
}

void HfpHFRole::subscribeGetDeviceStatus(const std::string &adapterAddr, bool available)
{
	BT_DEBUG("");
	unsubscribeService(adapterAddr);

	if(available)
	{
		BT_DEBUG("Subscribing");
		std::string lunaCmd = HFLS2::BTLSCALL + HFLS2::LUNAGETSTATUS;
		std::string payload = "{\"subscribe\":true, \"adapterAddress\":\"" + adapterAddr + "\"}";
		mContextList.push_back(new LSContext(HFLS2::APIName::GETSTATUS, adapterAddr, this, LSMESSAGE_TOKEN_INVALID));
		int index = findContextIndex(adapterAddr);
		LSCall(this->mLSHandle, lunaCmd.c_str(), payload.c_str(), mHFSubscribe->getStatusCallback,
	                   mContextList[index], &std::get<HFLS2::ContextData::TOKEN>(*mContextList[index]), nullptr);
	}
}

void HfpHFRole::subscribeGetSCOStatus(const std::string &remoteAddr, const std::string& adapterAddress, bool connected)
{
	BT_DEBUG("");
	unsubscribeScoService(remoteAddr,adapterAddress);

	if (connected)
	{
		BT_DEBUG("Subscribing");
		pbnjson::JValue params = pbnjson::Object();
		params.put("adapterAddress",adapterAddress.c_str());
        params.put("address",remoteAddr.c_str());
		params.put("subscribe",true);
		std::string payload;
		LSUtils::generatePayload(params, payload);
		std::string lunaCmd = HFLS2::BTLSCALL + HFLS2::LUNASCOSTATUS;
		//std::string payload = "{\"subscribe\":true, \"address\":\"" + remoteAddr + ""\" ,\"address\":\"+ }";
		mScoContextList.push_back(new LSScoContext(HFLS2::APIName::SCOSTATUS, remoteAddr, adapterAddress, this, LSMESSAGE_TOKEN_INVALID));
		int index = findScoContextIndex(remoteAddr ,adapterAddress);
		LSCall(this->mLSHandle, lunaCmd.c_str(), payload.c_str(), mHFSubscribe->getSCOStatusCallback,
                       mScoContextList[index], &std::get<HFLS2::ScoContextData::SCOTOKEN>(*mScoContextList[index]), nullptr);
	}
}

void HfpHFRole::sendResponseToClient(const std::string &remoteAddr, bool returnValue)
{
	auto responseMessageIter = mResponseMessage.find(remoteAddr);
	if (responseMessageIter != mResponseMessage.end())
	{
		LS::Message response = responseMessageIter->second;
		pbnjson::JValue responseObj = pbnjson::Object();
		responseObj.put("returnValue", returnValue);
		LSUtils::postToClient(response, responseObj);
		mResponseMessage.erase(remoteAddr);
	}
}

/**
Accept a incoming call of AG.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If AG answers the incoming call successfully, returnValue is the true. Otherwise it is the false.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::answerCall(LSMessage &message)
{
	std::string remoteAddr = "";
	const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(address, string))REQUIRED_1(address));
	LS2ParamList localParam =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, localParam), remoteAddr, localResult, false))
		handleSendAT(remoteAddr, "basic", "A");
	return true;
}

/**
Terminate an active call of AG.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If AG terminates an active call successfully, returnValue is the true. Otherwise it is the false.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::terminateCall(LSMessage &message)
{
	std::string remoteAddr = "";
	std::string index = "";
	const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(address, string))REQUIRED_1(address));
	LS2ParamList localParam =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, localParam), remoteAddr, localResult, false))
		handleSendAT(remoteAddr, "action", "CHUP");
	return true;
}

/**
Release held calls or waiting calls.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If the method succeeds, returnValue will contain true. If the method fails, returnValue will contain false.
                              The method may fail because of one of the error conditions described
                              in the Error Codes Reference table of this method. See the Error Code Reference table for more information.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::releaseHeldCalls(LSMessage &message)
{
	std::string remoteAddr = "";
	std::string param = "";
	const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(address, string))REQUIRED_1(address));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false))
		handleSendAT(remoteAddr, "set", "CHLD", "0");
	return true;
}

/**
Release active calls (if any exist) and accepts the other (held or waiting) call.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If the method succeeds, returnValue will contain true. If the method fails, returnValue will contain false.
                              The method may fail because of one of the error conditions described
                              in the Error Codes Reference table of this method. See the Error Code Reference table for more information.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::releaseActiveCalls(LSMessage &message)
{
	std::string remoteAddr = "";
	std::string param = "";
	const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(address, string))REQUIRED_1(address));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false))
		handleSendAT(remoteAddr, "set", "CHLD", "1");
	return true;
}

/**
Place active calls (if any exist) on hold and accepts the other (held or waiting) call.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)
index | No | Number | The index of call to be active

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If the method succeeds, returnValue will contain true. If the method fails, returnValue will contain false.
                              The method may fail because of one of the error conditions described
                              in the Error Codes Reference table of this method. See the Error Code Reference table for more information.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::holdActiveCalls(LSMessage &message)
{
	std::string remoteAddr = "";
	std::string param = "";
	const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(address, string),PROP(index, integer))REQUIRED_1(address));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)},
                        {"index", std::make_pair(HFGeneral::DataType::INTEGER, BT_ERR_SCHEMA_VALIDATION_FAIL)}};
	LS2Result localResult;
	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false))
	{
		std::string index = mHFLS2Call->getParam(localResult, "index");
		if (!index.empty())
			handleSendAT(remoteAddr, "set", "CHLD", "2<" + index + ">");
		else
			handleSendAT(remoteAddr, "set", "CHLD", "2");
	}
	return true;
}

/**
Adds a held call to the conversation.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If the method succeeds, returnValue will contain true. If the method fails, returnValue will contain false.
                              The method may fail because of one of the error conditions described
                              in the Error Codes Reference table of this method. See the Error Code Reference table for more information.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::mergeCall(LSMessage &message)
{
	std::string remoteAddr = "";
	std::string param = "";
	const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(address, string))REQUIRED_1(address));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false))
		handleSendAT(remoteAddr, "set", "CHLD", "3");
	return true;
}

/**
Command issued by the HF to report its current speaker gain level setting to the AG.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)
volume | Yes | Number | SCO volume to be set. The range of the value is from 0 to 15

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | Value is true if AG sends the OK; false otherwise.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::setVolume(LSMessage &message)
{
	std::string remoteAddr = "";
	const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(address, string),PROP(volume, integer))REQUIRED_2(address, volume));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)},
                        {"volume", std::make_pair(HFGeneral::DataType::INTEGER, BT_ERR_VOLUME_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false))
	{
		std::string volume = mHFLS2Call->getParam(localResult, "volume");
		int iVolume = std::stoi(volume);
		if (iVolume < 0 || iVolume > 15)
		{
			LS::Message request(&message);
			LSUtils::respondWithError(request, BT_ERR_VOLUME_PARAM_ERROR);
			mResponseMessage.erase(remoteAddr);
			return true;
		}
		handleSendAT(remoteAddr, "set", "VGS", volume);
		mHFDevice->updateAudioVolume(remoteAddr, iVolume, false);
	}
	return true;
}

/**
Place a call to a phone number or a memory dialing.
If the optional fields of number and memoryDialing are set together, error will be  returned.
If there is no input both of number and memoryDialing, AG may place a call to latest outgoing number (depends on AG).

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)
number | No | String | phone number
memoryDialing | No | Number | number for memory dialing

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | Value is true if AG sends the OK; false otherwise.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::call(LSMessage &message)
{
	LS::Message request(&message);
	std::string remoteAddr = "";
	const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(address, string),PROP(adapterAddress, string),PROP(number, string),PROP(memoryDialing,integer))
                                                REQUIRED_2(address, adapterAddress));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)},
                        {"adapterAddress", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADAPTER_ADDR_PARAM_MISSING)},
                        {"number", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_SCHEMA_VALIDATION_FAIL)},
                        {"memoryDialing", std::make_pair(HFGeneral::DataType::INTEGER, BT_ERR_SCHEMA_VALIDATION_FAIL)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false, true))
	{
		std::string number = mHFLS2Call->getParam(localResult, "number");
		std::string memoryDialing = mHFLS2Call->getParam(localResult, "memoryDialing");
		std::string adapterAddress = mHFLS2Call->getParam(localResult, "adapterAddress");

		if(!number.empty())
		{
			auto modem = mHfpOfonoManager->getModem(adapterAddress, remoteAddr);
			if (!modem)
			{
				LSUtils::respondWithError(request, BT_ERR_DEVICE_NOT_CONNECTED);
				return true;
			}

			auto voiceManager = modem->getVoiceManager();
			if (!voiceManager)
			{
				LSUtils::respondWithError(request, BT_ERR_DEVICE_NOT_CONNECTED);
				return true;
			}

			std::string voice = voiceManager->dial(number);
			if (!voice.empty())
			{
				pbnjson::JValue responseObj = pbnjson::Object();

				responseObj.put("returnValue", true);
				responseObj.put("address", remoteAddr);

				LSUtils::postToClient(request, responseObj);
				return true;
			}
			else
			{
				LSUtils::respondWithError(request, BT_ERR_DEVICE_NOT_CONNECTED);
			}
		}
	}
	return true;
}

/**
Enable/Disable the voice recognition in the AG.
If voice recognition is enabled, a voice recognition application will be activated in AG.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
address | Yes | String | Address of the remote device (AG)
enabled | Yes | Boolean | True if voice recognition is enabled. Otherwise, false.

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | Value is true if AG sends the OK; false otherwise.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.

@par Returns(Subscription)

As for a successful call
 */
bool HfpHFRole::setVoiceRecognition(LSMessage &message)
{
	std::string remoteAddr = "";
	const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(address, string),PROP(enabled, boolean))REQUIRED_2(address, enabled));
	LS2ParamList paramList =
                        {{"address", std::make_pair(HFGeneral::DataType::STRING, BT_ERR_ADDR_PARAM_MISSING)},
                        {"enabled", std::make_pair(HFGeneral::DataType::BOOLEAN, BT_ERR_ENABLED_PARAM_MISSING)}};
	LS2Result localResult;

	if (parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, false))
	{
		std::string enabled = mHFLS2Call->getParam(localResult, "enabled");
		bool bEnabled = false;
		if (enabled.compare("1") == 0)
			bEnabled = true;
		if (mHFDevice->getBVRAStatus() != bEnabled)
		{
			handleSendAT(remoteAddr, "set", "BVRA", enabled);
			mHFDevice->updateBVRAStatus(bEnabled);
		}
		sendResponseToClient(remoteAddr, true);
	}
	return true;
}

bool HfpHFRole::sendCLCC(const std::string &remoteAddr)
{
	return handleSendAT(remoteAddr, "action", "CLCC");
}

void HfpHFRole::sendNREC(const std::string &remoteAddr)
{
	handleSendAT(remoteAddr, "set", "NREC", "0");
}

bool HfpHFRole::handleSendAT(const std::string &remoteAddr, const std::string &type, const std::string &command)
{
	return handleSendAT(remoteAddr, type, command, "");
}

bool HfpHFRole::handleSendAT(const std::string &remoteAddr, const std::string &type, const std::string &command, const std::string &arguments)
{
	std::string lscall = HFLS2::BTLSCALL + HFLS2::LUNASENDAT;
	std::string payload = "";
	if (!arguments.empty())
		payload = "{\"address\":\"" + remoteAddr + "\", \"type\":\"" + type + "\", \"command\":\"" + command +
                                        "\", \"arguments\":\"" + arguments + "\"}";
	else
		payload = "{\"address\":\"" + remoteAddr + "\", \"type\":\"" + type + "\", \"command\":\"" + command + "\"}";
	LSCallOneReply(mLSHandle, lscall.c_str(), payload.c_str(), nullptr, nullptr, nullptr, nullptr);

	return true;
}

bool HfpHFRole::parseLSMessage(LSMessage &message, const HfpHFLS2Data &ls2Data, std::string &remoteAddr, LS2Result &result,
                                bool isSubscribeFunc, bool isMultiAdapterSupport)
{
	LS::Message request(&message);
	if (!mHFLS2Call->parseLSMessage(request, ls2Data, result))
		return false;

	if (isSubscribeFunc)
	{
		handleSubscribeFunc(request);
		return true;
	}
	else
	{
		remoteAddr = mHFLS2Call->getParam(result, "address");
		if (isMultiAdapterSupport)
		{
			std::string adapterAddress = mHFLS2Call->getParam(result, "adapterAddress");
			return handleOneReplyFunc(request, remoteAddr, adapterAddress);
		}
		return handleOneReplyFunc(request, remoteAddr);
	}
}

void HfpHFRole::handleSubscribeFunc(LS::Message &request)
{
	bool subscribed = false;
	if (request.isSubscription())
	{
		if (mGetStatusSubscription == nullptr)
		{
			mGetStatusSubscription = new LS::SubscriptionPoint;
			mGetStatusSubscription->setServiceHandle(getService());
		}

		BT_DEBUG("Register subscription");
		mGetStatusSubscription->subscribe(request);
		subscribed = true;
	}
	notifySubscribersStatusChanged(subscribed, request);
}

bool HfpHFRole::handleOneReplyFunc(LS::Message &request, const std::string &remoteAddr)
{
	BluetoothErrorCode errorCode = mHFDevice->checkAddress(remoteAddr);
	if (errorCode != BT_ERR_NO_ERROR)
	{
		LSUtils::respondWithError(request, errorCode);
		return false;
	}
	mResponseMessage.insert(std::make_pair(remoteAddr, request));
	return true;
}

bool HfpHFRole::handleOneReplyFunc(LS::Message &request, const std::string &remoteAddr, std::string &adapterAddress)
{
	BluetoothErrorCode errorCode = mHFDevice->checkAddress(remoteAddr, adapterAddress);
	if (errorCode != BT_ERR_NO_ERROR)
	{
		LSUtils::respondWithError(request, errorCode);
		return false;
	}
	mResponseMessage.insert(std::make_pair(remoteAddr, request));
	return true;
}

void HfpHFRole::handleAdapterGetStatus(LSMessage* reply)
{
	BT_DEBUG("");
	LS::Message replyMsg(reply);
	pbnjson::JValue replyObj;
	bool isAdapterRemoved = false;
	if (!mHFLS2Call->parseSubscriptionData(replyMsg, replyObj))
		return;
	if (!replyObj.hasKey("adapters"))
		return;

	auto adaptersObjArray = replyObj["adapters"];
	BT_DEBUG("Size %d exizsting adapter %d",adaptersObjArray.arraySize(),mAdapterMap.size());
	if(adaptersObjArray.arraySize() < mAdapterMap.size())
	{
		BT_DEBUG("Removing Adapter");
		auto itr = mAdapterMap.begin();
		while(itr != mAdapterMap.end())
		{
			bool found = false;
			for(int i = 0; i<adaptersObjArray.arraySize();i++)
			{
				auto adapterObj = adaptersObjArray[i];
				if (!adapterObj.hasKey("adapterAddress") || !adapterObj.hasKey("name"))
					continue;

				auto adapterAaddress = adapterObj["adapterAddress"].asString();
				if(itr->first ==  adapterAaddress)
				{
					found == true;
					break;
				}
			}

			if(!found)
			{
				unsubscribeService(itr->first);
				itr = mAdapterMap.erase(itr);
			}
			else
			{
				itr++;
			}
		}
	}
	else
	{
		for (int i = 0; i < adaptersObjArray.arraySize(); i++)
		{
			BT_DEBUG("New Adapter");
			auto adapterObj = adaptersObjArray[i];
			if (!adapterObj.hasKey("adapterAddress") || !adapterObj.hasKey("name"))
				continue;

			if(!adapterObj["adapterAddress"].asString().empty() || !adapterObj["name"].asString().empty())
				continue;

			auto adapterAaddress = adapterObj["adapterAddress"].asString();
			auto adapterName = adapterObj["name"].asString();
			auto itr = mAdapterMap.find(adapterAaddress.c_str());
			if(itr == mAdapterMap.end() && !adapterName.empty() && !adapterAaddress.empty())
			{
				BT_DEBUG("Adding Adapter %s",adapterName.c_str());
				mAdapterMap.insert(std::make_pair(adapterAaddress,adapterName));
				subscribeGetDeviceStatus(adapterAaddress,true);
			}
		}
	}
	BT_DEBUG(" Exit");
}


void HfpHFRole::handleGetStatus(LSMessage* reply, const std::string &adapterAddr)
{
	BT_DEBUG("");
	LS::Message replyMsg(reply);
	pbnjson::JValue replyObj;

	if (!mHFLS2Call->parseSubscriptionData(replyMsg, replyObj))
		return;
	if (!replyObj.hasKey("devices"))
		return;

	auto devicesObjArray = replyObj["devices"];
	if (devicesObjArray.arraySize() == 0)
	{
		unsubscribeScoServicebyAdapterAddress(adapterAddr);
		mHFDevice->removeAllDevicebyAdapterAddress(adapterAddr);
		notifySubscribersStatusChanged(true);
	}

	for (int i = 0; i < devicesObjArray.arraySize(); i++)
	{
		auto deviceObj = devicesObjArray[i];
		if (!deviceObj.hasKey("address") || !deviceObj.hasKey("connectedRoles"))
			continue;
		// Sometimes its coming empty , hence not considering
		//auto adapterAddress = replyObj["adapterAddress"].asString();
		auto address = deviceObj["address"].asString();
		auto connectedRoles = deviceObj["connectedRoles"];
		bool bFound = false;
		for (int n = 0; n < connectedRoles.arraySize(); n++)
		{
			std::string profile = connectedRoles[n].asString();
			if (profile != "HFP_AG")
				continue;

			bFound = true;
			mHFDevice->createDeviceInfo(address,adapterAddr);

			BT_DEBUG("Add device:%s  for adapter %s", address.c_str(),adapterAddr.c_str());
			subscribeGetSCOStatus(address,adapterAddr, true);
			break;
		}

		if (!bFound)
		{
			if (mHFDevice->removeDeviceInfo(address, adapterAddr))
			{
				subscribeGetSCOStatus(address,adapterAddr, false);
				notifySubscribersStatusChanged(true);
			}
		}
	}
}

void HfpHFRole::handleReceiveResult(LSMessage* reply)
{
	LS::Message replyMsg(reply);
	pbnjson::JValue replyObj;

	if (!mHFLS2Call->parseSubscriptionData(replyMsg, replyObj))
		return;

	std::string address = replyObj["address"].asString();
	std::string resultCode = replyObj["resultCode"].asString();
	if (!resultCode.empty())
		mHFDevice->updateStatus(address, resultCode);
}

void HfpHFRole::handleGetSCOStatus(LSMessage* reply, const std::string &remoteAddr)
{
	BT_DEBUG("");
	if (remoteAddr.empty())
		return;

	LS::Message replyMsg(reply);
	pbnjson::JValue replyObj;

	if (!mHFLS2Call->parseSubscriptionData(replyMsg, replyObj))
		return;

	auto scoStatus = replyObj["sco"].asBool();
	auto adapterAddress = replyObj["adapterAddress"].asString();
	BT_DEBUG("addr: %s, sco: %d adapter %s", remoteAddr.c_str(), scoStatus ,adapterAddress.c_str());
	if ((!adapterAddress.empty())&&(mHFDevice->updateSCOStatus(remoteAddr, adapterAddress, scoStatus)))
		notifySubscribersStatusChanged(true);
}

/**
Nofity the device's status to subscribers.

@par Parameters

Name | Required | Type | Description
-----|--------|------|----------
subscribed | No | Boolean | To be informed of changes to the state, set subscribe to true.
                            Otherwise, set subscribe to false. The default valus of subscribe is false.

@par Returns(Call)

Name | Required | Type | Description
-----|--------|------|----------
returnValue | Yes | Boolean | If the method succeeds, returnValue will contain true.
                              If the method fails, returnValue will contain false.
                              The method may fail because of one of the error conditions described
                              in the Error Codes Reference table of this method. See the Error Code Reference table for more information.
subscribed | Yes | String | If the subscription was set for this method, subscribed will contain true.
                           If the subscription was not set for this method, subscribed will contain false.
errorText | No | String | errorText contains the error text if the method fails. The method will return errorText only if it fails.
                          See the Error Codes Reference of this method for more details.
errorCode | No | Number | errorCode contains the error code if the method fails. The method will return errorCode only if it fails.
                          See the Error Codes Reference of this method for more details.
address | No | String | The address (bdaddr) of the remote device.
callStatus | No | String | Call status contains one of active, held, dialing, incoming, waiting, call held by response and hold, alerting.
index | No | Number | The numbering (starting with 1) of the call given by the sequence of setting up or receiving the calls
                     (active, held or waiting) as seen by the served subscriber. Calls hold their number until they are released.
                      New calls take the lowest available number. Refer to the HFP 1.6 spec. document.
direction | No | String | "outgoing" or "incoming"
number | No | String | call number
signal | No | Number | Signal Strength indicator of the AG, this value ranges from 0 to 5.
battery | No | Number | Battery charge indicator of the AG, this value ranges from 0 to 5.
volume | No | String | SCO volume gain of the AG, this value ranges 0 to 15.
ring | No | Boolean | True during ringing.
sco | No | Boolean | If there is a SCO connection between the local and the remote device, sco will contain true.
                     Otherwise, sco will contain false.

@par Returns(Subscription)

As for a successful call
 **/
bool HfpHFRole::getStatus(LSMessage &message)
{
	const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(subscribe, boolean)));
	std::string remoteAddr = "";
	LS2ParamList paramList;
	LS2Result localResult;

	parseLSMessage(message, HfpHFLS2Data(schema, paramList), remoteAddr, localResult, true);

	return true;
}

void HfpHFRole::notifySubscribersStatusChanged(bool subscribed)
{
	if (mGetStatusSubscription != nullptr)
	{
		LS::Message request;
		notifySubscribersStatusChanged(subscribed, request);
	}
}

void HfpHFRole::notifySubscribersStatusChanged(bool subscribed, LS::Message &request)
{
	if (subscribed && mHFDevice->isDeviceConnecting())
		return;

	pbnjson::JValue devicesObj = pbnjson::Array();
	pbnjson::JValue remoteObj = pbnjson::Object();
	HFDeviceList localList = mHFDevice->getDeviceInfoList();
	for (auto adapterList : localList)
	{
		for(auto localDevice : adapterList.second)
			buildGetStatusResp(localDevice.first, *localDevice.second,adapterList.first, devicesObj);
	}
	pbnjson::JValue responseObj = pbnjson::Object();
	responseObj.put("audioGateways", devicesObj);
	responseObj.put("returnValue", true);
	responseObj.put("subscribed", subscribed);

	if (subscribed)
		LSUtils::postToSubscriptionPoint(mGetStatusSubscription, responseObj);
	else
		LSUtils::postToClient(request, responseObj);
}

void HfpHFRole::setVolumeToAudio(const std::string &remoteAddr)
{
	auto localDevice = mHFDevice->findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
		return;

	int convertedVolume = (SCO::HFP_GAIN_STEP * localDevice->getAudioStatus(SCO::DeviceStatus::VOLUME)) + SCO::DEFAULT_VOLUME;
	std::string lscall = HFLS2::AUDIODLSCALL + HFLS2::LUNASETVOLUME;
	std::string payload = "{\"scenario\":\"phone_bluetooth_sco\", \"volume\":" + std::to_string(convertedVolume) + "}";
	LSCallOneReply(mLSHandle, lscall.c_str(), payload.c_str(), nullptr, nullptr, nullptr, nullptr);
}

void HfpHFRole::buildGetStatusResp(const std::string &remoteAddr, const HfpDeviceInfo &localDevice, const std::string &adapterAddr, pbnjson::JValue &AGObj)
{

	auto printDeviceStatus = [&](pbnjson::JValue &resObj, const std::string &addr, const HfpDeviceInfo &device){
		resObj.put("address", addr);
		resObj.put("adapterAddress", adapterAddr);
		resObj.put("signal", device.getDeviceStatus(CIND::DeviceStatus::SIGNAL));
		resObj.put("battey", device.getDeviceStatus(CIND::DeviceStatus::BATTCHG));
		bool scoStatus = false;
		if (device.getAudioStatus(SCO::DeviceStatus::CONNECTED) ==  HFGeneral::Status::STATUSTRUE)
			scoStatus = true;
		resObj.put("sco", scoStatus);
		resObj.put("volume", device.getAudioStatus(SCO::DeviceStatus::VOLUME));
		resObj.put("ring",device.getRING());
	};

	for (auto iterCallStatus : localDevice.getCallStatusList())
	{
		pbnjson::JValue responseObj = pbnjson::Object();
		responseObj.put("number", iterCallStatus.first);
		responseObj.put("callStatus", iterCallStatus.second->getCallStatus(CLCC::DeviceStatus::STATUS));
		responseObj.put("direction", iterCallStatus.second->getCallStatus(CLCC::DeviceStatus::DIRECTION));
		responseObj.put("index", std::stoi(iterCallStatus.second->getCallStatus(CLCC::DeviceStatus::INDEX)));
		printDeviceStatus(responseObj, remoteAddr, localDevice);
		AGObj.append(responseObj);
	}
	if (localDevice.getCallStatusList().empty())
	{
		pbnjson::JValue responseObj = pbnjson::Object();
		printDeviceStatus(responseObj, remoteAddr, localDevice);
		AGObj.append(responseObj);
	}
}

int HfpHFRole::findContextIndex(HFLS2::APIName apiName)
{
	for (int i = 0; i < mContextList.size(); i++)
	{
		if (std::get<HFLS2::ContextData::APINAME>(*mContextList[i]) == apiName)
			return i;
	}
	return HFLS2::INVALIDINDEX;
}

int HfpHFRole::findContextIndex(const std::string &remoteAddr)
{
	for (int i = 0; i < mContextList.size(); i++)
	{
		if (std::get<HFLS2::ContextData::ADDRESS>(*mContextList[i]).compare(remoteAddr) == 0)
			return i;
	}
	return HFLS2::INVALIDINDEX;
}

int HfpHFRole::findScoContextIndex(const std::string &remoteAddr, const std::string &adapterAddr)
{
	for (int i = 0; i < mScoContextList.size(); i++)
	{
		if ((std::get<HFLS2::ScoContextData::REMOTEADDRESS>(*mScoContextList[i]).compare(remoteAddr) == 0)&&
			(std::get<HFLS2::ScoContextData::ADAPTERADDRESS>(*mScoContextList[i]).compare(adapterAddr) == 0))
			return i;
	}
	return HFLS2::INVALIDINDEX;
}

