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
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include "logging.h"

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoVoiceCallManager::HfpOfonoVoiceCallManager(const std::string &objectPath) :
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
