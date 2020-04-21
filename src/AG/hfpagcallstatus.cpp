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

#include "hfpagcallstatus.h"
#include "hfpagrole.h"
#include "defines.h"
#include "logging.h"

HfpAGCallStatus::HfpAGCallStatus()
{
	memset(mCINDState, 0x0, sizeof(mCINDState));
	clearCallInfo();
}

HfpAGCallStatus::~HfpAGCallStatus()
{
}

void HfpAGCallStatus::setCallInfo(const int callIndex, const CallInfoPos pos, const int value)
{
	if (callIndex >= MAX_CALL || pos >= MAX_CALLINFO_POS)
		return;

	mCallInfo[callIndex][pos] = value;
}

void HfpAGCallStatus::setCallStatus(const int callIndex, const std::string &status)
{
	if (status == "active")
		mCallInfo[callIndex][STATUS] = ACTIVE;
	else if ((status == "incoming") || (status == "waiting"))
		mCallInfo[callIndex][STATUS] = INCOMING;
	else if (status == "dialing")
		mCallInfo[callIndex][STATUS] = DIALING;
	else if (status == "hold")
		mCallInfo[callIndex][STATUS] = HOLD;
	else
		mCallInfo[callIndex][STATUS] = DISCONNECTED;
}

void HfpAGCallStatus::setCallNumber(const int callIndex, const std::string &number)
{
	if (callIndex >= MAX_CALL)
		return;

	mCallNumber[callIndex] = number;
}

void HfpAGCallStatus::setCINDState(const CINDStatePos pos, const uint32_t value)
{
	if (pos >= MAX_CINDSTATE_POS)
		return;

	mCINDState[pos] = value;
}

std::string HfpAGCallStatus::getCIEVResult(const CINDStatePos pos) const
{
	if (pos >= MAX_CINDSTATE_POS)
		return "";

	std::string result("+CIEV:");
	switch (pos)
	{
	case CINDStatePos::CALL:
		result += std::to_string(BTA_AG_IND_CALL);
		break;
	case CINDStatePos::CALLSETUP:
		result += std::to_string(BTA_AG_IND_CALLSETUP);
		break;
	case CINDStatePos::CALLHOLD:
		result += std::to_string(BTA_AG_IND_CALLHELD);
		break;
	default:
		return "";
	}
	result += ",";
	result += std::to_string(mCINDState[pos]);

	return result;
}

std::string HfpAGCallStatus::getCINDResult() const
{
	std::string result("+CIND:");
	result += std::to_string(mCINDState[CINDStatePos::CALL]);
	result += "," + std::to_string(mCINDState[CINDStatePos::CALLSETUP]);
	result += "," + std::to_string(mCINDState[CINDStatePos::REGISTRATION]);
	result += "," + std::to_string(mCINDState[CINDStatePos::STRENGTH]);
	result += "," + std::to_string(mCINDState[CINDStatePos::ROAMING]);
	result += "," + std::to_string(mCINDState[CINDStatePos::LEVEL]);
	result += "," + std::to_string(mCINDState[CINDStatePos::CALLHOLD]);
	return result;
}

void HfpAGCallStatus::clearCallInfo()
{
	memset(mCallInfo, 0x0, sizeof(mCallInfo));
	for (int i = 0; i < MAX_CALL; i++)
		mCallNumber[i] = "";
}

void HfpAGCallStatus::printCINDState(const std::string &tag) const
{
	std::string printLog = "[" + tag + "] mCINDState:";
	for (int i = 0; i < CINDStatePos::MAX_CINDSTATE_POS; i++)
	{
		if (i != 0)
			printLog += ",";
		printLog += std::to_string(i) + "-";
		printLog += std::to_string(mCINDState[i]);
	}
	BT_DEBUG("%s", printLog.c_str());
}

void HfpAGCallStatus::printCallInfo(const std::string &tag) const
{
	std::string printLog;
	for (int i = 0; i < MAX_CALL; i++)
	{
		printLog = "[" + tag + "] mCallInfo[" + std::to_string(i) + "]:";
		for (int j = 0; j < CallInfoPos::MAX_CALLINFO_POS; j++)
		{
			if (i != 0)
				printLog += ",";
			printLog += std::to_string(j) + "-";
			printLog += std::to_string(mCallInfo[i][j]);
		}
		printLog += ",callNumber-" + mCallNumber[i];
		BT_DEBUG("%s", printLog.c_str());
	}
}
