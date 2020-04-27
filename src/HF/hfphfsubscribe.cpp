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

#include "logging.h"
#include "hfphfsubscribe.h"
#include "hfphfrole.h"

bool HfpHFSubscribe::getStatusCallback(LSHandle* handle, LSMessage* reply, void* context)
{
	if (context == nullptr)
		return true;

	LSContext* lsContext = static_cast<LSContext*>(context);
	if (lsContext == nullptr)
		return true;

	HfpHFRole* selfHfpHFRole = std::get<HFLS2::ContextData::OBJECT>(*lsContext);
	std::string adapterAddr = std::get<HFLS2::ContextData::ADDRESS>(*lsContext);
	selfHfpHFRole->handleGetStatus(reply,adapterAddr);
	return true;
}

bool HfpHFSubscribe::receiveResultCallback(LSHandle* handle, LSMessage* reply, void* context)
{
	if (context == nullptr)
		return true;

	LSContext* lsContext = static_cast<LSContext*>(context);
	if (lsContext == nullptr)
		return true;

	HfpHFRole* selfHfpHFRole = std::get<HFLS2::ContextData::OBJECT>(*lsContext);
	selfHfpHFRole->handleReceiveResult(reply);
	return true;
}

bool HfpHFSubscribe::getSCOStatusCallback(LSHandle* handle, LSMessage* reply, void* context)
{
	if (context == nullptr)
		return true;

	LSScoContext* lsScoContext = static_cast<LSScoContext*>(context);
	if (lsScoContext == nullptr)
		return true;

	HfpHFRole* selfHfpHFRole = std::get<HFLS2::ScoContextData::SCOOBJECT>(*lsScoContext);
	std::string remoteAddr = std::get<HFLS2::ScoContextData::REMOTEADDRESS>(*lsScoContext);
	selfHfpHFRole->handleGetSCOStatus(reply, remoteAddr);
	return true;
}

bool HfpHFSubscribe::registerService(LSHandle* sh, const char* serviceName, bool connected, void* ctx)
{
	HfpHFRole* selfHfpHFRole = static_cast<HfpHFRole*>(ctx);
	if (selfHfpHFRole == nullptr)
		return true;

	if (connected)
		selfHfpHFRole->subscribeService();
	else
		selfHfpHFRole->unsubscribeServiceAll();

	return true;
}

bool HfpHFSubscribe::getAdapterStatusCallback(LSHandle* handle, LSMessage* reply, void* context)
{
	if (context == nullptr)
		return true;

	LSContext* lsContext = static_cast<LSContext*>(context);
	if (lsContext == nullptr)
		return true;

	HfpHFRole* selfHfpHFRole = std::get<HFLS2::ContextData::OBJECT>(*lsContext);
	selfHfpHFRole->handleAdapterGetStatus(reply);
	return true;
}
