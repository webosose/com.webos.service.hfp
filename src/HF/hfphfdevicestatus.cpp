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

#include <bitset>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "logging.h"
#include "utils.h"
#include "hfphfdevicestatus.h"
#include "hfphfrole.h"
#include "hfpdeviceinfo.h"

HfpHFDeviceStatus::HfpHFDeviceStatus(HfpHFRole* roleObj) :
	mHFRole(roleObj),
	mDisconnectedHeldCall(false),
	mReceivedWaitingCall(false),
	mActiveNumber(""),
	mHasCallStatus(0),
	mEnabledBVRA(false),
	mTempDeviceInfo(nullptr)
{
}

HfpHFDeviceStatus::~HfpHFDeviceStatus()
{
	flushDeviceInfo();
	if (mTempDeviceInfo != nullptr)
		delete mTempDeviceInfo;
}

void HfpHFDeviceStatus::flushDeviceInfo()
{
	for (auto& i : mHfpDeviceInfo)
	{
		if (i.second != nullptr)
			delete i.second;
	}
	mHfpDeviceInfo.clear();
}

void HfpHFDeviceStatus::updateStatus(const std::string &remoteAddr, std::string &resultCode)
{
	transform(resultCode.begin(), resultCode.end(), resultCode.begin(), ::toupper);
	BT_DEBUG("resultCode = %s", resultCode.c_str());
	if (resultCode.find("CMEE") != std::string::npos)
	{
		mHFRole->sendResponseToClient(remoteAddr, false);
	}
	else if (resultCode.find(":") != std::string::npos)
	{
		std::string atCmd = resultCode.substr(resultCode.find_first_of("+") + 1);
		int iSub = atCmd.find_first_of(":");
		atCmd.erase(atCmd.begin() + iSub, atCmd.end());
		std::string arguments = resultCode.substr(resultCode.find_last_of(":") + 1);
		BT_DEBUG("address = %s, AT command = %s, arguments = %s", remoteAddr.c_str(), atCmd.c_str(), arguments.c_str());

		if (atCmd.compare("VGS") == 0)
		{
			updateAudioVolume(remoteAddr, std::stoi(arguments), false);
			mHFRole->setVolumeToAudio(remoteAddr);
		}
		else if (atCmd.compare("BVRA") == 0)
		{
			bool enabled = false;
			if (arguments.compare("1") == 0)
				enabled = true;
			updateBVRAStatus(enabled);
			mReceivedATCmd.push(receiveATCMD::ATCMD::BVRA);
		}
		else
			updateCallStatus(remoteAddr, atCmd, arguments);

		if (mReceivedATCmd.front() != receiveATCMD::ATCMD::CLCC)
		{
			if (mReceivedATCmd.front() == receiveATCMD::ATCMD::BRSF || mReceivedATCmd.front() == receiveATCMD::ATCMD::BVRA)
				mReceivedATCmd.pop();
			else
				mHFRole->notifySubscribersStatusChanged(true);
		}
	}
	else
	{
		if (resultCode.compare("RING") == 0)
			updateRingStatus(remoteAddr, true);
		else if (resultCode.find("OK") == 0)
		{
			int receivedATCmd = -1;
			if (!mReceivedATCmd.empty())
			{
				receivedATCmd = mReceivedATCmd.front();
				mReceivedATCmd.pop();
				BT_DEBUG("front = %d", receivedATCmd);
			}
			switch (receivedATCmd)
			{
			case receiveATCMD::ATCMD::CLCC:
				if (mDisconnectedHeldCall || (mReceivedWaitingCall && mHasCallStatus == 1))
					eraseCallStatus(remoteAddr);
				mHFRole->notifySubscribersStatusChanged(true);
				mHasCallStatus = 0;
				break;
			case receiveATCMD::ATCMD::VGS:
				mHFRole->notifySubscribersStatusChanged(true);
				mHFRole->setVolumeToAudio(remoteAddr);
				// fall-through
			default:
				mHFRole->sendResponseToClient(remoteAddr, true);
				break;
			}
		}
		else if (resultCode.find("ERROR") == 0)
			mHFRole->sendResponseToClient(remoteAddr, false);
		else
			BT_DEBUG("Unknown result code : %s", resultCode.c_str());
	}
}

void HfpHFDeviceStatus::updateCallStatus(const std::string &remoteAddr, const std::string &atCmd, std::string &arguments)
{
	if (atCmd.compare("CIND") == 0)
	{
		if (arguments.find("CALL") != std::string::npos)
		{
			setCINDType(arguments);
		}
		else
		{
			removeSpace(arguments);
			setCINDValue(remoteAddr, arguments);
		}
	}
	else if (atCmd.compare("BRSF") == 0)
	{
		removeSpace(arguments);
		setBRSFValue(remoteAddr, arguments);
		mReceivedATCmd.push(receiveATCMD::ATCMD::BRSF);
	}
	else if (atCmd.compare("CIEV") == 0)
	{
		removeSpace(arguments);
		if (setCIEVValue(remoteAddr, arguments))
		{
			updateRingStatus(remoteAddr, false);
			if (!isCallActive(remoteAddr))
				clearCLCC(remoteAddr);
			mHFRole->sendCLCC(remoteAddr);
			mReceivedATCmd.push(receiveATCMD::ATCMD::CLCC);
		}
	}
	else if (atCmd.compare("CLCC") == 0)
	{
		updateCLCC(remoteAddr, arguments);
		mHasCallStatus++;
	}
	else if (atCmd.compare("CCWA") == 0)
		mReceivedWaitingCall = true;
}

void HfpHFDeviceStatus::updateRingStatus(const std::string &remoteAddr, bool receive)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return;
	}
	localDevice->setRING(receive);
}

void HfpHFDeviceStatus::updateAudioVolume(const std::string &remoteAddr, int volume, bool isUpdated)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return;
	}
	localDevice->setAudioStatus(SCO::DeviceStatus::VOLUME, volume);
	if (isUpdated)
		mReceivedATCmd.push(receiveATCMD::ATCMD::VGS);
}

void HfpHFDeviceStatus::clearCLCC(const std::string &remoteAddr)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return;
	}
	mActiveNumber = "";
	localDevice->clearCLCC();
}

void HfpHFDeviceStatus::updateCLCC(const std::string &remoteAddr, const std::string &arguments)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return;
	}

	auto convertStatus = [] (int status, int type)->std::string{

		if (type == CLCC::DeviceStatus::DIRECTION)
		{
			switch (status)
			{
			case HFGeneral::Status::STATUSFALSE:
				return "outgoing";
			case HFGeneral::Status::STATUSTRUE:
				return "incoming";
			default:
				return "inactive";
			}
		}
		else
		{
			switch (status)
			{
			case CLCC::CallStatus::ACTIVE:
				return "active";
			case CLCC::CallStatus::HELD:
				return "held";
			case CLCC::CallStatus::DIALING:
				return "dialing";
			case CLCC::CallStatus::ALERTING:
				return "alerting";
			case CLCC::CallStatus::INCOMING:
				return "incoming";
			case CLCC::CallStatus::WAITING:
				return "waiting";
			case CLCC::CallStatus::CALLHELDBYRESPONSE:
				return "callheldbyresponse";
			default:
				return "inactive";
			}
		}
	};

	std::vector<std::string> argArrays;
	boost::split(argArrays, arguments, boost::is_any_of(","));
	if (argArrays.size() < 7)
		return;

	std::string number = argArrays[CLCC::DeviceStatus::NUMBER].substr(1, argArrays[CLCC::DeviceStatus::NUMBER].size() - 2);
	for (int i = 0; i < CLCC::DeviceStatus::MAXSTATUS; i++)
	{
		if (i != CLCC::DeviceStatus::NUMBER)
		{
			if (i == CLCC::DeviceStatus::STATUS || i == CLCC::DeviceStatus::DIRECTION)
				localDevice->setCallStatus(number, i, convertStatus(std::stoi(argArrays[i]), i));
			else
				localDevice->setCallStatus(number, i, argArrays[i]);
		}
	}
	mActiveNumber = number;
}

bool HfpHFDeviceStatus::isCallActive(const std::string &remoteAddr)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice->getDeviceStatus(CIND::DeviceStatus::CALL) == CIND::Call::INACTIVE)
		return false;

	return true;
}

HfpDeviceInfo* HfpHFDeviceStatus::findDeviceInfo(const std::string &remoteAddr) const
{
	auto localDevice = mHfpDeviceInfo.find(remoteAddr);

	if (localDevice != mHfpDeviceInfo.end())
		return localDevice->second;

	return nullptr;
}

bool HfpHFDeviceStatus::createDeviceInfo(const std::string &remoteAddr)
{
	if (isDeviceAvailable(remoteAddr))
	{
		BT_DEBUG("Local device already has %s's DeviceInfo", remoteAddr.c_str());
		return false;
	}

	HfpDeviceInfo* deviceInfo;
	if (mTempDeviceInfo == nullptr)
	{
		deviceInfo = new HfpDeviceInfo;
	}
	else
	{
		deviceInfo = mTempDeviceInfo;
		mTempDeviceInfo = nullptr;
	}

	mHfpDeviceInfo.insert(std::make_pair(remoteAddr, deviceInfo));
	deviceInfo->setAudioStatus(SCO::DeviceStatus::VOLUME, 9);
	BT_DEBUG("Create the %s's DeviceInfo", remoteAddr.c_str());

	if (isCallActive(remoteAddr))
	{
		mHFRole->sendCLCC(remoteAddr);
		mReceivedATCmd.push(receiveATCMD::ATCMD::CLCC);
	}
	return true;
}

bool HfpHFDeviceStatus::removeDeviceInfo(const std::string &remoteAddr)
{
	if (!isDeviceAvailable(remoteAddr))
	{
		BT_DEBUG("Local device doesn't have %s's DeviceInfo", remoteAddr.c_str());
		return false;
	}
	BT_DEBUG("Remove the %s's Info", remoteAddr.c_str());

	mHfpDeviceInfo.erase(remoteAddr);

	return true;
}

void HfpHFDeviceStatus::setBRSFValue(const std::string &remoteAddr, const std::string &arguments)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the device info : %s", remoteAddr.c_str());
		return;
	}

	int iValue = std::stoi(arguments);
#if HFP_V_1_7 == TRUE
	std::bitset<12> bValue(iValue);
#else
	std::bitset<10> bValue(iValue);
#endif
	for (int i = 0; i < bValue.size(); i ++)
	{
		localDevice->setAGFeature(i, bValue.test(i));
	}

	if (bValue.test(BRSF::DeviceStatus::NREC))
	{
		mHFRole->sendNREC(remoteAddr);
		BT_DEBUG("%s is supporting the NREC", remoteAddr.c_str());
	}
}

void HfpHFDeviceStatus::setCINDType(const std::string &arguments)
{
	int firstIndex = 0;
	int secondIndex = 0;
	for(int i = 0; i < CIND::DeviceStatus::MAXSTATUS; i++)
	{
		firstIndex = arguments.find_first_of("\"", secondIndex);
		secondIndex = arguments.find_first_of("\"", firstIndex + 2);

		std::string sIndex = arguments.substr(firstIndex + 1, secondIndex - firstIndex - 1);
		storeCINDIndex(sIndex, i);
		secondIndex += 2;
	}
}

void HfpHFDeviceStatus::storeCINDIndex(const std::string &type, int index)
{
	if (mTempDeviceInfo == nullptr)
		mTempDeviceInfo = new HfpDeviceInfo;

	if (type.compare("CALL") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::CALL);
	else if (type.compare("CALLSETUP") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::CALLSETUP);
	else if (type.compare("SERVICE") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::SERVICE);
	else if (type.compare("SIGNAL") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::SIGNAL);
	else if (type.compare("ROAM") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::ROAMING);
	else if (type.compare("BATTCHG") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::BATTCHG);
	else if (type.compare("CALLHELD") == 0)
		mTempDeviceInfo->setCINDIndex(index, CIND::DeviceStatus::CALLHELD);
}

void HfpHFDeviceStatus::setCINDValue(const std::string &remoteAddr, const std::string &arguments)
{
	if (mTempDeviceInfo != nullptr)
	{
		int firstIndex = 0;
		for (int i; i < CIND::DeviceStatus::MAXSTATUS; i++)
		{
			std::string sValue = arguments.substr(firstIndex, 1);
			int iValue = std::stoi(sValue);
			mTempDeviceInfo->setDeviceStatus(i, iValue);
			firstIndex += 2;
		}
	}
	else
		BT_DEBUG("mTempDeviceInfo is NULL");
}

bool HfpHFDeviceStatus::setCIEVValue(const std::string &remoteAddr, const std::string &arguments)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return false;
	}

	std::string sIndex = arguments.substr(0, 1);
	int iIndex = std::stoi(sIndex) - 1;

	std::string sValue = arguments.substr(2, 3);
	int iValue = std::stoi(sValue);
	if (localDevice->setDeviceStatus(iIndex, iValue))
	{
		if (localDevice->getCINDIndex(iIndex) == (CIND::DeviceStatus::CALLHELD) &&
                        iValue == CIND::CallHeld::NOHELD)
			mDisconnectedHeldCall = true;
		if (localDevice->getCINDIndex(iIndex) == (CIND::DeviceStatus::CALLHELD))
			mReceivedWaitingCall = false;
		return true;
	}

	return false;
}

bool HfpHFDeviceStatus::isDeviceAvailable(const std::string &remoteAddr) const
{
	auto deviceInfo = mHfpDeviceInfo.find(remoteAddr);

	if (deviceInfo == mHfpDeviceInfo.end())
		return false;

	return true;
}

BluetoothErrorCode HfpHFDeviceStatus::checkAddress(const std::string &remoteAddr) const
{
	if (remoteAddr.size() != 17)
		return BT_ERR_ADDRESS_INVALID;
	if (isDeviceAvailable(remoteAddr))
		return BT_ERR_NO_ERROR;
	else
		return BT_ERR_DEVICE_NOT_CONNECTED;
}

bool HfpHFDeviceStatus::isDeviceConnecting() const
{
	if (mHfpDeviceInfo.empty() && mTempDeviceInfo != nullptr)
	{
		BT_DEBUG("Device is connecting...");
		return true;
	}
	return false;
}

void HfpHFDeviceStatus::eraseCallStatus(const std::string &remoteAddr)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return;
	}
	BT_DEBUG("activeNumber = %s", mActiveNumber.c_str());
	for (auto& iterCallStatus : localDevice->getCallStatusList())
	{
		if (iterCallStatus.first.compare(mActiveNumber) != 0)
			localDevice->eraseCallStatus(iterCallStatus.first);
	}
	mActiveNumber = "";
	mDisconnectedHeldCall = false;
}

bool HfpHFDeviceStatus::updateSCOStatus(const std::string &remoteAddr, bool status)
{
	HfpDeviceInfo* localDevice = findDeviceInfo(remoteAddr);
	if (localDevice == nullptr)
	{
		BT_DEBUG("Can't find the deviceinfo : %s", remoteAddr.c_str());
		return false;
	}
	int SCOStatus = status ? HFGeneral::Status::STATUSTRUE : HFGeneral::Status::STATUSFALSE;
	if (SCOStatus == localDevice->getAudioStatus(SCO::DeviceStatus::CONNECTED))
		return false;

	localDevice->setAudioStatus(SCO::DeviceStatus::CONNECTED, SCOStatus);
	return true;
}
