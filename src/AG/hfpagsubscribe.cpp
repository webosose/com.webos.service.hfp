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

#include "hfpagsubscribe.h"
#include "hfpagrole.h"
#include "logging.h"
#include "bluetootherrors.h"
#include "ls2utils.h"

HfpAGSubscribe::HfpAGSubscribe(HfpAGRole *role, LSHandle *handle) :
        mHfpAGRole(role),
        mHandle(handle),
        mServerStatusRegistered(false)
{
	memset(mLSCallToken, LSMESSAGE_TOKEN_INVALID, sizeof(mLSCallToken));
	memset(mSubscribeCbs, 0x0, sizeof(mSubscribeCbs));
}

HfpAGSubscribe::~HfpAGSubscribe()
{
}

void HfpAGSubscribe::subscribeService(const std::string &service, const bool connected)
{
	for (int i = CB_TEL_CALL_STATE; i < MAX_CALLBACK_TYPE; i++)
	{
		auto info = mAGSubscribeInfo.find(i);
		if (info == mAGSubscribeInfo.end())
			continue;

		info->second.context = this;
		if (!info->second.autoCall)
			continue;

		if (info->second.service != service)
			continue;

		BT_DEBUG("subscribe:%d, connected:%d", i, connected);
		if (connected)
			subscribe((SubscribeCbType)i);
		else
			cancelSubscribe((SubscribeCbType)i);
	}
}

void HfpAGSubscribe::subscribeServices()
{
	if (mServerStatusRegistered)
	{
		BT_DEBUG("already registered");
		return;
	}

	auto callback = [](LSHandle *handle, const char *service, bool connected, void *context) -> bool {
		HfpAGSubscribe::AGSubscribeInfo *cbInfo = static_cast<HfpAGSubscribe::AGSubscribeInfo *>(context);
		if (nullptr == cbInfo)
			return true;

		HfpAGSubscribe *self = (HfpAGSubscribe *)cbInfo->context;
		if (nullptr == self)
			return true;

		self->subscribeService(service, connected);

		return true;
	};

	LSError lserror;
	std::unordered_map<std::string, int> serviceMap;
	for (int i = CB_TEL_CALL_STATE; i < MAX_CALLBACK_TYPE; i++)
	{
		auto subscribe = mAGSubscribeInfo.find(i);
		if (subscribe == mAGSubscribeInfo.end())
			continue;

		subscribe->second.context = this;
		if (!subscribe->second.autoCall)
			continue;

		auto iter = serviceMap.find(subscribe->second.service);
		if (iter != serviceMap.end())
			continue;

		serviceMap.insert(std::pair<std::string, int>(subscribe->second.service, i));

		LSErrorInit(&lserror);
		bool result = LSRegisterServerStatusEx(mHandle,
			subscribe->second.service.c_str(),
			callback,
			&(subscribe->second),
			nullptr,
			&lserror);
		if (LSErrorIsSet(&lserror))
		{
			BT_DEBUG("register server(%s) error:%s", subscribe->second.service.c_str(), lserror.message);
			LSErrorFree(&lserror);
		}
	}
	mServerStatusRegistered = true;
}

void HfpAGSubscribe::subscribeAll()
{
	for (int i = CB_TEL_CALL_STATE; i < MAX_CALLBACK_TYPE; i++)
	{
		auto subscribe = mAGSubscribeInfo.find(i);
		if (subscribe == mAGSubscribeInfo.end())
			continue;

		subscribe->second.context = this;
		if (!subscribe->second.autoCall)
			continue;

		if (LSMESSAGE_TOKEN_INVALID != mLSCallToken[i])
			continue;

		if (subscribe->second.subscribe)
			LSCall(mHandle, subscribe->second.uri.c_str(), subscribe->second.payload.c_str(), HfpAGSubscribe::subscribeCb,
			        &(subscribe->second), &mLSCallToken[i], nullptr);
		else
			LSCallOneReply(mHandle, subscribe->second.uri.c_str(), subscribe->second.payload.c_str(), HfpAGSubscribe::subscribeCb,
			        &(subscribe->second), &mLSCallToken[i], nullptr);
	}
}

bool HfpAGSubscribe::subscribe(SubscribeCbType cbType)
{
	auto subscribe = mAGSubscribeInfo.find(cbType);
	if (subscribe == mAGSubscribeInfo.end())
		return false;

	bool result = false;
	BT_DEBUG("subscribe 1.mLSCallToken[%d]:%lu", cbType, mLSCallToken[cbType]);
	if (LSMESSAGE_TOKEN_INVALID != mLSCallToken[cbType])
		return false;

	if (subscribe->second.subscribe)
		result = LSCall(mHandle, subscribe->second.uri.c_str(), subscribe->second.payload.c_str(), HfpAGSubscribe::subscribeCb,
			&(subscribe->second), &mLSCallToken[cbType], nullptr);
	else
		result = LSCallOneReply(mHandle, subscribe->second.uri.c_str(), subscribe->second.payload.c_str(), HfpAGSubscribe::subscribeCb,
			&(subscribe->second), &mLSCallToken[cbType], nullptr);
	BT_DEBUG("subscribe 2.mLSCallToken[%d]:%lu", cbType, mLSCallToken[cbType]);

	return result;
}

bool HfpAGSubscribe::cancelSubscribe(SubscribeCbType cbType)
{
	if (LSMESSAGE_TOKEN_INVALID == mLSCallToken[cbType])
		return false;

	bool result = false;
	BT_DEBUG("cancelSubscribe mLSCallToken[%d]:%lu", cbType, mLSCallToken[cbType]);
	LSError lserror;
	LSErrorInit(&lserror);
	result = LSCallCancel(mHandle, mLSCallToken[cbType], &lserror);
	if (LSErrorIsSet(&lserror))
	{
		BT_DEBUG("LSCallCancel error:%s", lserror.message);
		LSErrorFree(&lserror);
	}
	mLSCallToken[cbType] = LSMESSAGE_TOKEN_INVALID;

	return result;
}

bool HfpAGSubscribe::setPayload(SubscribeCbType cbType, const std::string &payload)
{
	auto subscribe = mAGSubscribeInfo.find(cbType);
	if (subscribe == mAGSubscribeInfo.end())
		return false;

	subscribe->second.payload = payload;
	return true;
}

bool HfpAGSubscribe::setCallbackFunc(SubscribeCbType cbType, SubscribeCbFunc cbFunc)
{
	mSubscribeCbs[cbType] = cbFunc;
	return true;
}

bool HfpAGSubscribe::subscribeCb(LSHandle *handle, LSMessage *reply, void *context)
{
	HfpAGSubscribe::AGSubscribeInfo *cbInfo = static_cast<HfpAGSubscribe::AGSubscribeInfo *>(context);
	if (nullptr == cbInfo)
		return true;

	HfpAGSubscribe *self = (HfpAGSubscribe *)cbInfo->context;
	if (nullptr == self)
		return true;

	LS::Message replyMsg(reply);
	pbnjson::JValue replyObj;
	int parseError = 0;

	if (!LSUtils::parsePayload(replyMsg.getPayload(), replyObj, "", &parseError))
	{
		if (JSON_PARSE_SCHEMA_ERROR != parseError)
			BT_DEBUG("subscribe callback error:%s", retrieveErrorText(BT_ERR_BAD_JSON).c_str());
		else
			BT_DEBUG("subscribe callback error:%s", retrieveErrorText(BT_ERR_SCHEMA_VALIDATION_FAIL).c_str());

		return true;
	}

	if (cbInfo->cbType >= MAX_CALLBACK_TYPE)
		return true;

	BT_DEBUG("subscribe callback:%d", cbInfo->cbType);
	if (self->mSubscribeCbs[cbInfo->cbType])
		self->mSubscribeCbs[cbInfo->cbType](replyObj);

	return true;
}
