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

#include "hfpofonovoicecallmanager.h"
#include "hfpofonovoicecall.h"
#include "hfpofonomodem.h"
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include "logging.h"

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoVoiceCallManager::HfpOfonoVoiceCallManager(const std::string &objectPath, HfpOfonoModem *modem) :
mModem(modem),
mObjectPath(objectPath),
mOfonoVoiceCallManagerProxy(nullptr)
{
	GError* error = nullptr;
	mOfonoVoiceCallManagerProxy = ofono_voice_call_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", mObjectPath.c_str(), NULL, &error);
	if (error)
	{
		BT_ERROR("MSGID_FAILED_TO_CREATE_OFONO_VOICE_MANAGER_PROXY", 0, "Failed to create dbus proxy for ofono voicemanager on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	g_signal_connect(G_OBJECT(mOfonoVoiceCallManagerProxy), "call-added", G_CALLBACK(handleCallAdded), this);
	g_signal_connect(G_OBJECT(mOfonoVoiceCallManagerProxy), "call-removed", G_CALLBACK(handleCallRemoved), this);
}

HfpOfonoVoiceCallManager::~HfpOfonoVoiceCallManager()
{
	if (mOfonoVoiceCallManagerProxy)
		g_object_unref(mOfonoVoiceCallManagerProxy);
}

std::string HfpOfonoVoiceCallManager::dial(const std::string &phoneNumber)
{
	gchar *outPath = nullptr;
	GError *error = nullptr;
	std::string callId = "";
	ofono_voice_call_manager_call_dial_sync(mOfonoVoiceCallManagerProxy, phoneNumber.c_str(), "default", &outPath, NULL, &error);
	if (error)
	{
		BT_DEBUG("Not able to make call error  %s", error->message);
		g_error_free(error);
		return std::string("");
	}

	std::string voicePath = outPath;
	std::size_t found = voicePath.find_last_of("/");
	if (found != std::string::npos)
	{
		callId = voicePath.substr(found + 1, voicePath.length());
		return callId;
	}

	return callId;
}

HfpOfonoVoiceCall* HfpOfonoVoiceCallManager::getVoiceCall(const std::string &state)
{
	BT_DEBUG("getVoiceCall with state %s", state.c_str());

	for (auto it = mCallMap.begin(); it != mCallMap.end(); it++)
	{
		if (it->second->getCallState() == state)
			return it->second.get();
	}

	return nullptr;
}

HfpOfonoVoiceCall* HfpOfonoVoiceCallManager::getVoiceCall(int idx)
{
	std::string index = std::to_string(idx);

	if (index.length() == 1)
	{
		index = "voicecall0" + index;
	}
	else
	{
		index = "voicecall" + index;
	}

	BT_DEBUG("getVoiceCall with index %s", index.c_str());

	for (auto it = mCallMap.begin(); it != mCallMap.end(); it++)
	{
		std::size_t found = it->first.find("voicecall");
		if (found != std::string::npos)
		{
			if (it->first.substr(found) == index)
				return it->second.get();
		}
	}
	return nullptr;
}

void HfpOfonoVoiceCallManager::handleCallAdded(OfonoVoiceCallManager *object, const gchar *path, GVariant *properties, void *userData)
{
	HfpOfonoVoiceCallManager *pThis = static_cast<HfpOfonoVoiceCallManager*>(userData);
	if (!pThis)
		return;

	BT_DEBUG("callAdded %s", path);
	std::unique_ptr<HfpOfonoVoiceCall> call (new HfpOfonoVoiceCall(path, pThis->mModem));

	if (pThis->mModem)
	{
		pThis->mModem->callAdded(call.get());
		pThis->mModem->updateState(call.get());

		pThis->mCallMap[path] = std::move(call);
	}
}

void HfpOfonoVoiceCallManager::handleCallRemoved(OfonoVoiceCallManager *object, const gchar *path, void *userData)
{
	BT_DEBUG("callRemoved %s", path);
	HfpOfonoVoiceCallManager *pThis = static_cast<HfpOfonoVoiceCallManager*>(userData);
	if (!pThis)
		return;

	auto it = pThis->mCallMap.find(path);

	if (it != pThis->mCallMap.end())
		it->second->getModem()->callRemoved(it->second.get());

	pThis->mCallMap.erase(path);
}
