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

#include "hfpofonocallvolume.h"
#include "hfpofonomodem.h"
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include "logging.h"

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoCallVolume::HfpOfonoCallVolume(const std::string &objectPath, HfpOfonoModem *modem)
    : mModem(modem)
    , mObjectPath(objectPath)
    , mOfonoCallVolumeProxy(nullptr)
    , mMicrophoneVolume(0)
    , mSpeakerVolume(0)
{
	GError* error = nullptr;
	mOfonoCallVolumeProxy = ofono_call_volume_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", mObjectPath.c_str(), NULL, &error);
	if (error)
	{
		BT_ERROR("MSGID_FAILED_TO_CREATE_OFONO_HANDSFREE_PROXY", 0, "Failed to create dbus proxy for ofono handsfree on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	getCallVolumeProperties();

	g_signal_connect(G_OBJECT(mOfonoCallVolumeProxy), "property-changed", G_CALLBACK(handleCallVolumePropertyChanged), this);
}

HfpOfonoCallVolume::~HfpOfonoCallVolume()
{
	if (mOfonoCallVolumeProxy)
		g_object_unref(mOfonoCallVolumeProxy);
}

void HfpOfonoCallVolume::getCallVolumeProperties()
{
	GError *error = 0;
	GVariant *out;

	ofono_call_volume_call_get_properties_sync(mOfonoCallVolumeProxy, &out, NULL, &error);
	if (error)
	{
		BT_ERROR("MSGID_OBJECT_MANAGER_CREATION_FAILED", 0, "Failed to call: %s", error->message);
		g_error_free(error);
		return;
	}

	g_autoptr(GVariantIter) iter = NULL;
	g_variant_get(out, "a{sv}", &iter);
	gchar *name;
	GVariant *valueVar;
	std::string key;
	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(mOfonoCallVolumeProxy));

	while (g_variant_iter_loop (iter, "{sv}", &name, &valueVar))
	{
		key = name;

		if (key == "SpeakerVolume" && (g_variant_classify(valueVar) == G_VARIANT_CLASS_BYTE))
		{
			BT_DEBUG("SpeakerVolume property changed for device %s", objectPath);
			speakerVolumeChanged(g_variant_get_byte (valueVar));
			break;
		}
	}
}

void HfpOfonoCallVolume::handleCallVolumePropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData)
{
	HfpOfonoCallVolume *pThis = static_cast<HfpOfonoCallVolume*>(userData);
	if (!pThis)
		return;

	GVariant *va = g_variant_get_child_value(v, 0);
	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy));

	std::string key = name;
	if (key == "MicrophoneVolume" && (g_variant_classify(va) == G_VARIANT_CLASS_BYTE))
	{
		BT_DEBUG("MicrophoneVolume property changed for device %s", objectPath);
		//pThis->BatteryChargeLevelChanged(g_variant_get_byte (va));
	}
	else if (key == "SpeakerVolume" && (g_variant_classify(va) == G_VARIANT_CLASS_BYTE))
	{
		BT_DEBUG("SpeakerVolume property changed for device %s", objectPath);
		pThis->speakerVolumeChanged(g_variant_get_byte (va));
	}
}

void HfpOfonoCallVolume::microphoneVolumeChanged(int volume)
{
	BT_DEBUG("microphoneVolume changed to device %d", volume);
	mMicrophoneVolume = volume;

	if (mModem)
		mModem->updateMicrophoneVolume(volume);
}

void HfpOfonoCallVolume::speakerVolumeChanged(int volume)
{
	BT_DEBUG("speakerVolume changed to device %d", volume);
	mSpeakerVolume = volume;

	if (mModem)
		mModem->updateSpeakerVolume(volume);
}

bool HfpOfonoCallVolume::setMicrophoneVolume(int volume)
{
	return setVolume(volume, "MicrophoneVolume");
}

bool HfpOfonoCallVolume::setSpeakerVolume(int volume)
{
	return setVolume(volume, "SpeakerVolume");
}

bool HfpOfonoCallVolume::setVolume(int volume, const std::string &propertyName)
{
	GVariant *valueVar = 0;
	valueVar = g_variant_new_byte(volume);
	if (!valueVar)
		return false;

	GError *error = 0;

	ofono_call_volume_call_set_property_sync(mOfonoCallVolumeProxy, propertyName.c_str(), g_variant_new_variant(valueVar), NULL, &error);

	if (error)
	{
		BT_ERROR("MSGID_FAILED_TO_SET_VOLUME", 0, "Failed to setVolume %s: %s", error->message);
		g_error_free(error);
		return false;
	}

	return true;
}
