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

#include "hfpofonomodem.h"
#include "hfphfrole.h"
#include "hfpdeviceinfo.h"
#include "hfpofonovoicecall.h"
#include "utils.h"
#include "logging.h"
#include <glib.h>
#include <gio/gio.h>
#include <string>
#include <algorithm>
#include "hfpofonovoicecallmanager.h"

extern "C" {
#include "ofono-interface.h"
}

HfpOfonoModem::HfpOfonoModem(const std::string& objectPath, HfpHFRole *role) :
mHfpHFRole(role),
mObjectPath(objectPath),
mOfonoModemProxy(nullptr),
mVoiceCallManager(nullptr),
mEmergency(false),
mLockDown(false),
mOnline(false),
mPowered(false),
mAddress("")
{
	BT_DEBUG("ofonoModem instance created");
	GError *error = nullptr;
	mOfonoModemProxy = ofono_modem_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.ofono", objectPath.c_str(), NULL, &error);
	if (error)
	{
		BT_ERROR("FAILED_TO_CREATE_OFONO_MODEM_PROXY", 0, "Failed to create dbus proxy for ofono modem on path %s: %s",
			  objectPath.c_str(), error->message);
		g_error_free(error);
		return;
	}

	mVoiceCallManager = new HfpOfonoVoiceCallManager(objectPath, this);

	getModemProperties(mOfonoModemProxy);

	g_signal_connect(G_OBJECT(mOfonoModemProxy), "property-changed", G_CALLBACK(handleModemPropertyChanged), this);
}

HfpOfonoModem::~HfpOfonoModem()
{
	if(mVoiceCallManager)
		delete mVoiceCallManager;

	if (mOfonoModemProxy)
		g_object_unref(mOfonoModemProxy);

	mFeatures.clear();
	mInterfaces.clear();
}

void HfpOfonoModem::handleModemPropertyChanged(OfonoModem *proxy, char *name, GVariant *v, void *userData)
{
	GVariant *va = g_variant_get_child_value(v, 0);
	HfpOfonoModem *pThis = static_cast<HfpOfonoModem*>(userData);
	if (!pThis)
		return;

	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy));

	std::string key = name;

	if (key == "Interfaces")
	{
		BT_DEBUG("Interface property changed for device %s", objectPath);
		g_autoptr(GVariantIter) iter2;
		gchar *interface = nullptr;
		pThis->mInterfaces.clear();
		g_variant_get(va, "as", &iter2);
		while (g_variant_iter_loop (iter2, "s", &interface))
		{
			pThis->mInterfaces.push_back(interface);
		}
		if (!pThis->isModemConnected())
		{
			pThis->mAddress = "";
		}
	}

	if (key == "Serial")
	{
		const char *serial = nullptr;
		g_variant_get(va, "s", &serial);
		if (serial)
			pThis->mAddress = convertToLowerCase(serial);
	}
}

void HfpOfonoModem::getModemProperties(OfonoModem *modemProxy)
{
	GError *error = 0;
	GVariant *out;

	const char *objectPath = g_dbus_proxy_get_object_path(G_DBUS_PROXY(modemProxy));

	ofono_modem_call_get_properties_sync(modemProxy, &out, NULL, &error);
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

		if (key == "Interfaces")
		{
			BT_DEBUG("Interface property changed for device %s", objectPath);
			g_autoptr(GVariantIter) iter2;
			gchar *interface = nullptr;
			mInterfaces.clear();
			g_variant_get(valueVar, "as", &iter2);
			while (g_variant_iter_loop (iter2, "s", &interface))
			{
				mInterfaces.push_back(interface);
			}

			if (!isModemConnected())
			{
				mAddress = "";
			}
		}

		if (key == "Serial")
		{
			const char *serial = nullptr;
			g_variant_get(valueVar, "s", &serial);
			if (serial)
			{
				mAddress = convertToLower(serial);
			}
		}
	}
}

bool HfpOfonoModem::isModemConnected() const
{
	if (mInterfaces.empty())
		return false;

	for (auto it = mInterfaces.begin(); it != mInterfaces.end(); it++)
	{
		if (*it == "org.ofono.Handsfree")
			return true;
	}

	return false;
}

void HfpOfonoModem::callAdded(HfpOfonoVoiceCall *voiceCall)
{
	std::string adapterAddress = getAdapterAddress();

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, adapterAddress);
	if (!device)
	{
		BT_ERROR("DEVICE NOT FOUND", 0, "%s", voiceCall->getLineIdentification().c_str());
		return;
	}

	std::string phoneNumber = voiceCall->getLineIdentification();

	if (phoneNumber.empty())
		return;

	std::string voiceCallPath = voiceCall->getObjectPath();
	std::size_t found = voiceCallPath.find_last_of("voicecall");
	if (found != std::string::npos)
	{
		std::string index = voiceCallPath.substr(found + 1, voiceCallPath.length());
		device->setCallStatus(phoneNumber, CLCC::DeviceStatus::INDEX, index);
	}
}

void HfpOfonoModem::callRemoved(HfpOfonoVoiceCall *voiceCall)
{
	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE NOT FOUND", 0, "%s", voiceCall->getLineIdentification().c_str());
		return;
	}

	std::string phoneNumber = voiceCall->getLineIdentification();
	if (!phoneNumber.empty())
	{
		device->eraseCallStatus(phoneNumber);
		mHfpHFRole->notifySubscribersStatusChanged(true);
	}
}

void HfpOfonoModem::updateState(HfpOfonoVoiceCall *voiceCall)
{
	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE NOT FOUND", 0, "%s", voiceCall->getLineIdentification().c_str());
		return;
	}


	std::string phoneNumber = voiceCall->getLineIdentification();

	std::string callState = voiceCall->getCallState();


	if (!phoneNumber.empty() && !callState.empty())
	{
		device->setCallStatus(phoneNumber, CLCC::DeviceStatus::STATUS, callState);
		if (callState == "dialing")
			device->setCallStatus(phoneNumber, CLCC::DeviceStatus::DIRECTION, "outgoing");
		else if (callState == "incoming")
			device->setCallStatus(phoneNumber, CLCC::DeviceStatus::DIRECTION, "incoming");
	}

	mHfpHFRole->notifySubscribersStatusChanged(true);
}

std::string HfpOfonoModem::getAdapterAddress()
{
	auto adapterMap = mHfpHFRole->getAdapterMap();
	std::string hciName;

	std::size_t hciPos = mObjectPath.find("hci");
	if (hciPos != std::string::npos)
	{
		std::size_t indexPos = mObjectPath.find("/", hciPos);
		if (indexPos != std::string::npos)
		{
		    hciName = mObjectPath.substr(hciPos, indexPos - hciPos);
		}
	}

	 for (auto adapter = adapterMap.begin(); adapter != adapterMap.end(); adapter++)
	 {
		std::string adapterName = adapter->second;
		auto it = hciName.begin();
		bool matching = adapterName.size() >= hciName.size() &&
				std::all_of (std::next(adapterName.begin(),
				                       adapterName.size() - hciName.size()),
				                       adapterName.end(),
				                       [&it](const char & c)
				                       {
					                        return c == *(it++);
				                       }
				            );
		if (matching)
			return adapter->first;
	 }

	 //On failure return empty string
	 return std::string();
}
