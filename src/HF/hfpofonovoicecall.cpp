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

#include "hfpofonovoicecall.h"
#include "hfpofonomodem.h"
#include "logging.h"
#include <glib.h>
#include <gio/gio.h>

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoVoiceCall::HfpOfonoVoiceCall(const std::string& objectPath, HfpOfonoModem *modem):
mHfpModem(modem),
mObjectPath(objectPath),
mOfonoVoiceCallProxy(nullptr),
mMultiparty(false),
mEmergency(false)
{
	BT_DEBUG("ofono voiceCall instance created %s", objectPath.c_str());
	GError *error = nullptr;
	mOfonoVoiceCallProxy = ofono_voice_call_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", objectPath.c_str(), NULL, &error);
	if (error)
	{
		BT_ERROR("Failed_to_create_ofono_voice_call_proxy", 0, "Failed to create dbus proxy for ofono voice call on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	getProperties();

	g_signal_connect(G_OBJECT(mOfonoVoiceCallProxy), "property-changed", G_CALLBACK(handleVoiceCallPropertyChanged), this);
}

HfpOfonoVoiceCall::~HfpOfonoVoiceCall()
{
	if (mOfonoVoiceCallProxy)
		g_object_unref(mOfonoVoiceCallProxy);
}

void HfpOfonoVoiceCall::getProperties()
{
	GVariant *properties = NULL;
    GError *error = NULL;

	(void) ofono_voice_call_call_get_properties_sync(mOfonoVoiceCallProxy, &properties, NULL, &error);
	if (error)
	{
		BT_ERROR("Failed_to_get_ofono_voice_call_property", 0, "Failed get properties from voicecall %s: %s",
				mObjectPath.c_str(), error->message);
		return;
	}

	g_autoptr(GVariantIter) iter = NULL;
	g_variant_get(properties, "a{sv}", &iter);
	gchar *name;
	GVariant *valueVar;
	std::string key;

	while (g_variant_iter_loop (iter, "{sv}", &name, &valueVar))
	{
		key = name;
		updateProperties(key, valueVar);
	}
}

void HfpOfonoVoiceCall::updateProperties(const std::string &key, GVariant *valueVar)
{
	const char *valueStr = nullptr;

	if (key == "LineIdentification")
	{
		g_variant_get(valueVar, "s", &valueStr);
		mLineIdentification = valueStr;
	}
	else if (key == "IncomingLine")
	{
		g_variant_get(valueVar, "s", &valueStr);
		mIncomingLine =  valueStr;
	}
	else if (key == "Name")
	{
		g_variant_get(valueVar, "s", &valueStr);
		mName =  valueStr;
	}
	else if (key == "State")
	{
		g_variant_get(valueVar, "s", &valueStr);
		mState =  valueStr;
	}
	else if (key == "StartTime")
	{
		g_variant_get(valueVar, "s", &valueStr);
		mStartTime =  valueStr;
	}
	else if (key == "Information")
	{
		g_variant_get(valueVar, "s", &valueStr);
		mInformation =  valueStr;
	}
}

void HfpOfonoVoiceCall::handleVoiceCallPropertyChanged(OfonoVoiceCall *object, const gchar *name, GVariant *value, void *userData)
{
	HfpOfonoVoiceCall *pThis = static_cast<HfpOfonoVoiceCall*>(userData);
	if (!pThis)
		return;

	GVariant *va = g_variant_get_child_value(value, 0);
	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(object));

	BT_DEBUG("voice call property changed for %s", objectPath);

	std::string key = name;

	pThis->updateProperties(key, va);

	//If state changes then only update all properties to up
	if (key == "State")
		pThis->mHfpModem->updateState(pThis);
}

bool HfpOfonoVoiceCall::answer()
{
	GError *error = nullptr;
	(void) ofono_voice_call_call_answer_sync (mOfonoVoiceCallProxy, NULL, &error);
	if (error)
	{
		BT_ERROR("ANSWER_CALL_FAILED", 0, "reason %s", error->message);
		return false;
	}

	return true;
}

bool HfpOfonoVoiceCall::hangup()
{
	GError *error = nullptr;
	(void) ofono_voice_call_call_hangup_sync (mOfonoVoiceCallProxy, NULL, &error);
	if (error)
	{
		BT_ERROR("HANGUP_CALL_FAILED", 0, "reason %s", error->message);
		return false;
	}

	return true;
}
