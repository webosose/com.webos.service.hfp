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

#ifndef HFPAGCALLSTATUS_H_
#define HFPAGCALLSTATUS_H_

#define MAX_CALL       3

#include <cstdint>
#include <string>

class HfpAGRole;

class HfpAGCallStatus
{
public:
	HfpAGCallStatus();
	~HfpAGCallStatus();

	enum CallInfoPos
	{
		INDEX = 0,
		DIRECTION,
		STATUS,
		MODE,
		MULTIPARTY,
		TYPE,
		MAX_CALLINFO_POS
	};

	enum CINDStatePos
	{
		LEVEL = 0,
		STRENGTH,
		REGISTRATION,
		CALL,
		CALLSETUP,
		CALLHOLD,
		ROAMING,
		MAX_CINDSTATE_POS
	};

	enum CallStatus
	{
		DISCONNECTED = -1,
		ACTIVE,
		HOLD,
		DIALING,
		INCOMING = 4
	};

	void setCallInfo(const int callIndex, const CallInfoPos pos, const int value);
	void setCallStatus(const int callIndex, const std::string &status);
	void setCallNumber(const int callIndex, const std::string &number);
	void setCINDState(const CINDStatePos pos, const uint32_t value);
	uint32_t getCINDState(const CINDStatePos pos) const { return mCINDState[pos]; }
	int getCallInfo(const int callIndex, const CallInfoPos pos) const { return mCallInfo[callIndex][pos]; }
	std::string getCallNumber(const int callIndex) const { return mCallNumber[callIndex]; }
	std::string getCIEVResult(const CINDStatePos pos) const;
	std::string getCINDResult() const;
	void clearCallInfo();
	void printCINDState(const std::string &tag) const;
	void printCallInfo(const std::string &tag) const;

private:
	std::string mCallNumber[MAX_CALL];
	int mCallInfo[MAX_CALL][MAX_CALLINFO_POS];
	uint32_t mCINDState[MAX_CINDSTATE_POS];
};

#endif

// HFPAGCALLSTATUS_H_
