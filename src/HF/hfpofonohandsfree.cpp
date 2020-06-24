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

#include "hfpofonohandsfree.h"
#include "hfpofonomodem.h"
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include "logging.h"

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoHandsfree::HfpOfonoHandsfree(const std::string &objectPath, HfpOfonoModem *modem)
    : mModem(modem)
    , mObjectPath(objectPath)
    , mOfonoHandsfreeProxy(nullptr)
    , mBatteryChargeLevel(0)
{
	GError* error = nullptr;
	mOfonoHandsfreeProxy = ofono_handsfree_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", mObjectPath.c_str(), NULL, &error);
	if (error)
	{
		BT_ERROR("MSGID_FAILED_TO_CREATE_OFONO_HANDSFREE_PROXY", 0, "Failed to create dbus proxy for ofono handsfree on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	getHandsfreeProperties(mOfonoHandsfreeProxy);

	g_signal_connect(G_OBJECT(mOfonoHandsfreeProxy), "property-changed", G_CALLBACK(handleHandsfreePropertyChanged), this);
}

HfpOfonoHandsfree::~HfpOfonoHandsfree()
{
	if (mOfonoHandsfreeProxy)
		g_object_unref(mOfonoHandsfreeProxy);
}

void HfpOfonoHandsfree::getHandsfreeProperties(OfonoHandsfree *handsfreeProxy)
{
	GError *error = 0;
	GVariant *out;

	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(handsfreeProxy));

	ofono_handsfree_call_get_properties_sync(handsfreeProxy, &out, NULL, &error);
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

	while (g_variant_iter_loop (iter, "{sv}", &name, &valueVar))
	{
		key = name;

		if (key == "BatteryChargeLevel" && (g_variant_classify(valueVar) == G_VARIANT_CLASS_BYTE))
		{
			BT_DEBUG("BatteryChargeLevel property changed for device %s", objectPath);
			BatteryChargeLevelChanged(g_variant_get_byte (valueVar));
			break;
		}
	}
}

void HfpOfonoHandsfree::handleHandsfreePropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData)
{
	HfpOfonoHandsfree *pThis = static_cast<HfpOfonoHandsfree*>(userData);
	if (!pThis)
		return;

	GVariant *va = g_variant_get_child_value(v, 0);
	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy));

	std::string key = name;
	if (key == "BatteryChargeLevel" && (g_variant_classify(va) == G_VARIANT_CLASS_BYTE))
	{
		BT_DEBUG("BatteryChargeLevel property changed for device %s", objectPath);
		pThis->BatteryChargeLevelChanged(g_variant_get_byte (va));
	}
}

void HfpOfonoHandsfree::BatteryChargeLevelChanged(unsigned char batteryChargeLevel)
{
	BT_DEBUG("BatteryChargeLevel changed to device %d", batteryChargeLevel);
	mBatteryChargeLevel = batteryChargeLevel;

	if (mModem)
		mModem->updateBatteryChargeLevel(batteryChargeLevel);
}
