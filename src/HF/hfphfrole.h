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

#ifndef HFPHFROLE_H_
#define HFPHFROLE_H_

#include <unordered_map>
#include <tuple>

#include "hfprole.h"
#include "hfphfls2call.h"
#include "hfphfdevicestatus.h"
#include "dbusutils.h"
#include "ls2utils.h"

class HfpDeviceInfo;
class HfpHFDeviceStatus;
class HfpHFLS2Data;
class HfpHFSubscribe;
class HfpHFRole;
class HfpOfonoManager;

using LSContext = std::tuple<HFLS2::APIName, std::string, HfpHFRole*, LSMessageToken>;
using LSScoContext = std::tuple<HFLS2::APIName, std::string, std::string , HfpHFRole*, LSMessageToken>;


class HfpHFRole : public HfpRole
{
public:
	HfpHFRole(BluetoothHfpService *service);
	~HfpHFRole();

	void initialize();
	bool getStatus(LSMessage &message);
	bool answerCall(LSMessage &message);
	bool terminateCall(LSMessage &message);
	bool releaseHeldCalls(LSMessage &message);
	bool releaseActiveCalls(LSMessage &message);
	bool holdActiveCalls(LSMessage &message);
	bool mergeCall(LSMessage &message);
	bool setVolume(LSMessage &message);
	bool call(LSMessage &message);
	bool setVoiceRecognition(LSMessage &message);

	bool sendCLCC(const std::string &remoteAddr);
	void sendNREC(const std::string &remoteAddr);
	void sendResponseToClient(const std::string &remoteAddr, bool returnValue);
	void notifySubscribersStatusChanged(bool subscribed);
	void setVolumeToAudio(const std::string &remoteAddr);
	void setVolumeToAudio(const std::string &remoteAddr, const std::string &adapterAddress);
	void handleAdapterGetStatus(LSMessage* reply);
	void handleGetStatus(LSMessage* reply, const std::string &adapterAddr);
	void handleReceiveResult(LSMessage* reply);
	void handleGetSCOStatus(LSMessage* reply, const std::string &remoteAddr);
	void subscribeService();
	void unsubscribeServiceAll();
	void unsubscribeScoServicebyAdapterAddress(const std::string &adapterAddr);
	HfpHFDeviceStatus* getHfDevice() const { return mHFDevice;}
	std::unordered_map<std::string ,std::string> & getAdapterMap() { return mAdapterMap; }

private:
	void buildGetStatusResp(const std::string &remoteAddr, const HfpDeviceInfo &localDevice, const std::string &adapterAddr, pbnjson::JValue &AGObj);
	void notifySubscribersStatusChanged(bool subscribed, LS::Message &request);

	void unsubscribeService(HFLS2::APIName apiName);
	void unsubscribeService(const std::string &remoteAddr);
	void unsubscribeService(int index);
	void unsubscribeScoService(const std::string &remoteAddr, const std::string &adapterAddr);
	void subscribeGetSCOStatus(const std::string &remoteAddr, const std::string& adapterAddress, bool connected);
	void subscribeGetDeviceStatus(const std::string &adapterAddr, bool available);
	int findContextIndex(HFLS2::APIName apiName);
	int findContextIndex(const std::string &remoteAddr);
	int findScoContextIndex(const std::string &remoteAddr, const std::string &adapterAddr);
	bool handleSendAT(const std::string &remoteAddr, const std::string &type, const std::string &command, const std::string &arguments);
	bool handleSendAT(const std::string &remoteAddr, const std::string &type, const std::string &command);
	bool parseLSMessage(LSMessage &message, const HfpHFLS2Data &ls2Data, std::string &remoteAddr, LS2Result &result, bool isSubscribeFunc, bool isMultiAdapterSupport = false);
	void handleSubscribeFunc(LS::Message &request);
	bool handleOneReplyFunc(LS::Message &request, const std::string &remoteAddr);
	bool handleOneReplyFunc(LS::Message &request, const std::string &remoteAddr, std::string &adapterAddress);
	void createOfonoManager();
	void destroyOfonoManager();
	std::string getDefaultAdapterAddress() const;
#ifdef MULTI_SESSION_SUPPORT
	std::string getAdapterAddress(LSUtils::DisplaySetId idx) const;
#endif

private:
	LS::SubscriptionPoint* mGetStatusSubscription;
	LSHandle* mLSHandle;
	std::unordered_map<std::string, LS::Message> mResponseMessage;
	HfpHFDeviceStatus* mHFDevice;
	HfpHFSubscribe* mHFSubscribe;
	HfpHFLS2Call* mHFLS2Call;
	std::vector<LSContext*> mContextList;
	std::vector<LSScoContext*> mScoContextList;
	HfpOfonoManager *mHfpOfonoManager;
	DBusUtils::NameWatch mNameWatch;
	std::unordered_map<std::string ,std::string> mAdapterMap; //address to name map
};

#endif
// HFPHFROLE_H_
