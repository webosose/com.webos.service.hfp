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

#include "hfpofonomanager.h"
#include "hfpofonomodem.h"
#include "logging.h"
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include <memory>
#include <unordered_map>

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoManager::HfpOfonoManager(const std::string& objectPath) :
mObjectPath(objectPath),
mOfonoManagerProxy(nullptr)
{
	BT_DEBUG("ofonoManager instance created");
	GError *error = nullptr;
	mOfonoManagerProxy = ofono_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", "/", NULL, &error);
	if (error)
	{
		BT_ERROR("MSGID_FAILED_TO_CREATE_OFONO_MANAGER_PROXY", 0, "Failed to create dbus proxy for ofono manager on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	getModemsFromOfonoManager();

	g_signal_connect(G_OBJECT(mOfonoManagerProxy), "modem-added", G_CALLBACK(handleModemAdded), this);
	g_signal_connect(G_OBJECT(mOfonoManagerProxy), "modem-removed", G_CALLBACK(handleModemRemoved), this);
}

HfpOfonoManager::~HfpOfonoManager()
{
	if (mOfonoManagerProxy)
		g_object_unref(mOfonoManagerProxy);

	mModemsMap.clear();
}

void HfpOfonoManager::handleModemAdded(OfonoManager *object, const gchar *path, GVariant *properties, void *userData)
{
	HfpOfonoManager *pThis = static_cast<HfpOfonoManager*>(userData);
	if (!pThis)
		return;

	std::unique_ptr<HfpOfonoModem> modem (new HfpOfonoModem(path));
	pThis->mModemsMap[path] = std::move(modem);
}

void HfpOfonoManager::handleModemRemoved(OfonoManager *object, const gchar *path, void *userData)
{
	HfpOfonoManager *pThis = static_cast<HfpOfonoManager*>(userData);
	if (!pThis)
		return;

	pThis->mModemsMap.erase(path);
	BT_DEBUG("OfonoModemManager  handleObjectRemoved in ofono %s", path);
}

void HfpOfonoManager::getModemsFromOfonoManager()
{
	GVariant *modems = nullptr;
	GError *error = nullptr;
	ofono_manager_call_get_modems_sync(mOfonoManagerProxy, &modems, NULL, &error);
	if (error)
	{
		BT_DEBUG("Not able to get modems");
		g_error_free(error);
	}

	g_autoptr(GVariantIter) iter1 = NULL;
	g_variant_get (modems, "a(oa{sv})", &iter1);

	const gchar *objectPath;
	g_autoptr(GVariantIter) iter2 = NULL;

	while (g_variant_iter_loop (iter1, "(&oa{sv})", &objectPath, &iter2))
	{
		const gchar *key;
		g_autoptr(GVariant) value = NULL;

		std::unique_ptr<HfpOfonoModem> modem (new HfpOfonoModem(objectPath));
		mModemsMap.insert(std::make_pair(objectPath, std::move(modem)));
	}
}

HfpOfonoModem* HfpOfonoManager::getModemFromMap(const std::string &address) const
{
	for (auto it = mModemsMap.begin(); it != mModemsMap.end(); it++)
	{
		if ((it->second).get()->getAddress() == address)
			return (it->second).get();
	}

	return nullptr;
}
