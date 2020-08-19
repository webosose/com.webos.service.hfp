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

#include "hfpofononetworkregistration.h"
#include "hfpofonomodem.h"
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include "logging.h"

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoNetworkRegistration::HfpOfonoNetworkRegistration(const std::string &objectPath, HfpOfonoModem *modem)
	: mModem(modem)
	, mObjectPath(objectPath)
	, mOfonoNetworkRegistrationProxy(nullptr)
	, mNetworkSignalStrength(-1)
{
	GError* error = nullptr;
	mOfonoNetworkRegistrationProxy = ofono_network_registration_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", mObjectPath.c_str(), NULL, &error);
	if (error)
	{
		BT_ERROR("MSGID_FAILED_TO_CREATE_OFONO_NETWORK_REGISTRATION_PROXY", 0, "Failed to create dbus proxy for ofono network registration on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	getNetworkRegistrationProperties();

	g_signal_connect(G_OBJECT(mOfonoNetworkRegistrationProxy), "property-changed", G_CALLBACK(handleNetworkRegistrationPropertyChanged), this);
}

HfpOfonoNetworkRegistration::~HfpOfonoNetworkRegistration()
{
	if (mOfonoNetworkRegistrationProxy)
		g_object_unref(mOfonoNetworkRegistrationProxy);
}

void HfpOfonoNetworkRegistration::getNetworkRegistrationProperties()
{
	GError *error = 0;
	GVariant *out;

	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(mOfonoNetworkRegistrationProxy));

	ofono_network_registration_call_get_properties_sync(mOfonoNetworkRegistrationProxy, &out, NULL, &error);
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

		BT_DEBUG("%s property changed for device %s", key.c_str(), objectPath);
		if (key == "Strength" && (g_variant_classify(valueVar) == G_VARIANT_CLASS_BYTE))
		{
			networkSignalStrengthChanged(g_variant_get_byte (valueVar));
			break;
		}
		else if (key == "Name" && (g_variant_classify(valueVar) == G_VARIANT_CLASS_STRING))
		{
			gsize len;
			const gchar* str = g_variant_get_string (valueVar, &len);
			if (len > 0)
				networkOperatorNameChanged(std::string(str, len));
		}
		else if (key == "Status" && (g_variant_classify(valueVar) == G_VARIANT_CLASS_STRING))
		{
			gsize len;
			const gchar* str = g_variant_get_string (valueVar, &len);
			if (len > 0)
				networkRegistrationStatusChanged(std::string(str, len));
		}
	}
}

void HfpOfonoNetworkRegistration::handleNetworkRegistrationPropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData)
{
	HfpOfonoNetworkRegistration *pThis = static_cast<HfpOfonoNetworkRegistration*>(userData);
	if (!pThis)
		return;

	GVariant *va = g_variant_get_child_value(v, 0);
	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy));

	std::string key = name;
	BT_DEBUG("%s property changed for device %s", key.c_str(), objectPath);

	if (key == "Strength" && (g_variant_classify(va) == G_VARIANT_CLASS_BYTE))
	{
		pThis->networkSignalStrengthChanged(g_variant_get_byte (va));
	}
	else if (key == "Name" && (g_variant_classify(va) == G_VARIANT_CLASS_STRING))
	{
		gsize len;
		const gchar* str = g_variant_get_string (va, &len);
		if (len > 0)
			pThis->networkOperatorNameChanged(std::string(str, len));
	}
	else if (key == "Status" && (g_variant_classify(va) == G_VARIANT_CLASS_STRING))
	{
		gsize len;
		const gchar* str = g_variant_get_string (va, &len);
		if (len > 0)
			pThis->networkRegistrationStatusChanged(std::string(str, len));
	}
}

void HfpOfonoNetworkRegistration::networkSignalStrengthChanged(int networkSignalStrength)
{
	BT_DEBUG("NetworkSignalStrength changed to device %d", networkSignalStrength);
	mNetworkSignalStrength = networkSignalStrength/20;
	mModem->updateNetworkSignalStrength(mNetworkSignalStrength);
}

void HfpOfonoNetworkRegistration::networkOperatorNameChanged(const std::string &name)
{
	mNetworkOperatorName = name;
	BT_DEBUG("NetworkOperatorName changed to device %s", mNetworkOperatorName.c_str());
	mModem->updateNetworkOperatorName(mNetworkOperatorName);
}

void HfpOfonoNetworkRegistration::networkRegistrationStatusChanged(const std::string &status)
{
	mNetworkRegistrationStatus = status;
	BT_DEBUG("NetworkRegistrationStatus changed to device %s", mNetworkRegistrationStatus.c_str());
	mModem->updateNetworkRegistrationStatus(mNetworkRegistrationStatus);
}
