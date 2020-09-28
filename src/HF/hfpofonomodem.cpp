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
#include <cmath>
#include "hfpofonovoicecallmanager.h"
#include "hfpofonohandsfree.h"
#include "hfpofononetworkregistration.h"
#include "hfpofonocallvolume.h"

extern "C" {
#include "ofono-interface.h"
}

const char interfaceVoiceCallManager[] = "org.ofono.VoiceCallManager";
const char interfaceCallVolume[] = "org.ofono.CallVolume";
const char interfaceHandsfree[] = "org.ofono.Handsfree";
const char interfaceNetworkRegistration[] = "org.ofono.NetworkRegistration";

HfpOfonoModem::HfpOfonoModem(const std::string& objectPath, HfpHFRole *role) :
mHfpHFRole(role),
mObjectPath(objectPath),
mOfonoModemProxy(nullptr),
mVoiceCallManager(nullptr),
mHandsfree(nullptr),
mNetworkRegistration(nullptr),
mCallVolume(nullptr),
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

	mHandsfree = new HfpOfonoHandsfree(mObjectPath, this);
	mNetworkRegistration = new HfpOfonoNetworkRegistration(mObjectPath, this);
	mCallVolume = new HfpOfonoCallVolume(mObjectPath, this);

	g_signal_connect(G_OBJECT(mOfonoModemProxy), "property-changed", G_CALLBACK(handleModemPropertyChanged), this);
}

HfpOfonoModem::~HfpOfonoModem()
{
	if (mVoiceCallManager)
		delete mVoiceCallManager;

	if (mHandsfree)
		delete mHandsfree;

	if (mNetworkRegistration)
		delete mNetworkRegistration;

	if (mCallVolume)
		delete mCallVolume;

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

		pThis->interfacesChanged();

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

			interfacesChanged();
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

void HfpOfonoModem::callAdded(HfpOfonoVoiceCall *voiceCall)
{
	BT_DEBUG("callAdded ");
	std::string adapterAddress = getAdapterAddress();

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, adapterAddress);
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0, "%s", voiceCall->getLineIdentification().c_str());
		return;
	}

	std::string phoneNumber = voiceCall->getLineIdentification();

	BT_DEBUG("callAdded phoneNumber %s", phoneNumber.c_str());

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
	BT_DEBUG("callRemoved ");
	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0, "%s", voiceCall->getLineIdentification().c_str());
		return;
	}

	std::string phoneNumber = voiceCall->getLineIdentification();
	if (!phoneNumber.empty())
	{
		device->eraseCallStatus(phoneNumber);
		mHfpHFRole->notifySubscribersStatusChanged(true);
	}

	BT_DEBUG("callRemoved phoneNumber %s ", phoneNumber.c_str());
}

void HfpOfonoModem::updateState(HfpOfonoVoiceCall *voiceCall)
{
	BT_DEBUG("updateCallState");

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0, "%s", voiceCall->getLineIdentification().c_str());
		return;
	}


	std::string phoneNumber = voiceCall->getLineIdentification();

	std::string callState = voiceCall->getCallState();


	if (!phoneNumber.empty() && !callState.empty())
	{
		BT_DEBUG("updateCallState for phoneNumber: %s state: %s", phoneNumber.c_str(), callState.c_str());
		device->setCallStatus(phoneNumber, CLCC::DeviceStatus::STATUS, callState);
		if (callState == "dialing")
			device->setCallStatus(phoneNumber, CLCC::DeviceStatus::DIRECTION, "outgoing");
		else if (callState == "incoming")
			device->setCallStatus(phoneNumber, CLCC::DeviceStatus::DIRECTION, "incoming");

		mHfpHFRole->notifySubscribersStatusChanged(true);
	}
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

bool HfpOfonoModem::isInterfacePresent(const std::string interfaceName)
{
	if (mInterfaces.empty())
		return false;

	for (auto it = mInterfaces.begin(); it != mInterfaces.end(); it++)
	{
		if (*it == interfaceName)
			return true;
	}

	return false;
}

void HfpOfonoModem::interfacesChanged()
{
	//Get org.ofono.VoiceCallManager properties
	if (isInterfacePresent(interfaceVoiceCallManager) && mVoiceCallManager)
	{
		mVoiceCallManager->addExistingVoiceCalls();
	}

	//Get org.ofono.CallVolume properties
	if (isInterfacePresent(interfaceCallVolume))
	{
		//ToDo: Volume functionality will be added
	}

	//Get org.ofono.Handsfree properties
	if (isInterfacePresent(interfaceHandsfree))
	{
		if (mHandsfree)
			mHandsfree->getHandsfreeProperties();
	}
	else
	{
		mAddress = "";
	}

	//Get org.ofono.NetworkRegistration properties
	if (isInterfacePresent(interfaceNetworkRegistration) && mNetworkRegistration)
	{
		mNetworkRegistration->getNetworkRegistrationProperties();
	}
}

void HfpOfonoModem::updateBatteryChargeLevel(int batteryChargeLevel)
{
	BT_DEBUG("batteryChargeLevel");

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0, "Setting BatteryChargeLevel failed");
		return;
	}

	if (device->getDeviceStatus(CIND::DeviceStatus::BATTCHG) == batteryChargeLevel)
		return;

	BT_DEBUG("setDeviceStatus for BatteryChargeLevel: %d ", batteryChargeLevel);
	device->setDeviceStatus(CIND::DeviceStatus::BATTCHG, batteryChargeLevel);

	mHfpHFRole->notifySubscribersStatusChanged(true);
}

void HfpOfonoModem::updateNetworkSignalStrength(int networkSignalStrength)
{
	BT_DEBUG("networkSignalStrength");

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0, "Setting networkSignalStrength failed");
		return;
	}

	if (device->getDeviceStatus(CIND::DeviceStatus::SIGNAL) == networkSignalStrength)
		return;

	BT_DEBUG("setDeviceStatus for networkSignalStrength: %d ", networkSignalStrength);
	device->setDeviceStatus(CIND::DeviceStatus::SIGNAL, networkSignalStrength);

	mHfpHFRole->notifySubscribersStatusChanged(true);
}

void HfpOfonoModem::updateNetworkOperatorName(const std::string &name)
{
	BT_DEBUG("NetworkOperatorName");

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0,"Setting NetworkOperatorName failed");
		return;
	}

	if (name.compare(device->getNetworkOperatorName()) == 0)
		return;

	BT_DEBUG("setDeviceStatus for NetworkOperatorName: %s ", name.c_str());
	device->setNetworkOperatorName(name);

	mHfpHFRole->notifySubscribersStatusChanged(true);
}

void HfpOfonoModem::updateNetworkRegistrationStatus(const std::string &status)
{
	BT_DEBUG("NetworkRegistrationStatus");

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0,"Setting NetworkRegistrationStatus failed");
		return;
	}

	if (status.compare(device->getNetworkRegistrationStatus()) == 0)
		return;

	BT_DEBUG("setDeviceStatus for NetworkRegistrationStatus: %s ", status.c_str());
	device->setNetworkRegistrationStatus(status);

	mHfpHFRole->notifySubscribersStatusChanged(true);
}

void HfpOfonoModem::notifyProperties()
{
	if (mHandsfree)
		updateBatteryChargeLevel(mHandsfree->getBatteryChargeLevel());

	if (mNetworkRegistration)
		updateNetworkSignalStrength(mNetworkRegistration->getNetworkSignalStrength());
}

void HfpOfonoModem::updateSpeakerVolume(int volume)
{
	BT_DEBUG("updateSpeakerVolume");

	HfpDeviceInfo *device = mHfpHFRole->getHfDevice()->findDeviceInfo(mAddress, getAdapterAddress());
	if (!device)
	{
		BT_ERROR("DEVICE_NOT_FOUND", 0, "Setting updateSpeakerVolume failed");
		return;
	}

	int volumeLevel = std::lround((volume/100.0)*15.0);

	if (device->getAudioStatus(SCO::DeviceStatus::VOLUME) == volumeLevel)
		return;

	BT_DEBUG("setDeviceStatus for speakerVolume: %d ", volumeLevel);

	device->setAudioStatus(SCO::DeviceStatus::VOLUME, volumeLevel);

	mHfpHFRole->setVolumeToAudio(mAddress, getAdapterAddress());

	mHfpHFRole->notifySubscribersStatusChanged(true);
}

void HfpOfonoModem::updateMicrophoneVolume(int volume)
{
	//TODO notify hfp service
}

bool HfpOfonoModem::setSpeakerVolume(int volume)
{
	return mCallVolume->setSpeakerVolume(volume);
}

bool HfpOfonoModem::setMicrophoneVolume(int volume)
{
	return mCallVolume->setMicrophoneVolume(volume);
}
