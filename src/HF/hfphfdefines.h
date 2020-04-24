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

#ifndef HFPHFDEFINES_H_
#define HFPHFDEFINES_H_

#include <string>
#include "defines.h"

namespace HFLS2
{
	const std::string BTSERVICE = "com.webos.service.bluetooth2";
	const std::string BTLSCALL = "luna://com.webos.service.bluetooth2/";
	const std::string AUDIODLSCALL = "luna://com.webos.service.audio/";
	const std::string LUNAGETSTATUS = "device/getStatus";
	const std::string LUNAADAPTERGETSTATUS = "adapter/getStatus";
	const std::string LUNASCOSTATUS = "hfp/getStatus";
	const std::string LUNARECEIVERESULT = "hfp/receiveResult";
	const std::string LUNASUBSCRIBE = "{\"subscribe\":true}";
	const std::string LUNASENDAT = "hfp/sendAT";
	const std::string LUNASETVOLUME="phone/setVolume";
	const int INVALIDINDEX = -1;

	enum APIName
	{
		GETSTATUS,
		RECEIVERESULT,
		SCOSTATUS,
		ADAPTERGETSTATUS,
		MAXVALUE
	};

	enum ContextData
	{
		APINAME,
		ADDRESS,
		OBJECT,
		TOKEN
	};

	enum ScoContextData
	{
		SCOAPINAME,
		REMOTEADDRESS,
		ADAPTERADDRESS,
		SCOOBJECT,
		SCOTOKEN
	};
}

namespace HFGeneral
{
	enum Status
	{
		STATUSFALSE = 0,
		STATUSTRUE = 1,
		STATUSNONE = -1
	};

	enum DataType
	{
		NONE,
		INTEGER,
		STRING,
		BOOLEAN
	};
}

namespace CIND
{
	enum DeviceStatus
	{
		SERVICE,
		CALL,
		CALLSETUP,
		CALLHELD,
		SIGNAL,
		ROAMING,
		BATTCHG,
		MAXSTATUS
	};

	enum Call
	{
		INACTIVE = 0,
		ACTIVE
	};

	enum CallSetup
	{
		NOCALLSETUP = 0,
		INCOMINGCALL,
		OUTGOINGCALL,
		ALERTING
	};

	enum CallHeld
	{
		NOHELD = 0,
		SWAPCALL,
		ONHOLD
	};
}

namespace BRSF
{
	enum DeviceStatus
	{
		THREEWAYCALL,
		NREC,
		VOICERECOGNITION,
		INBANDRING,
		VOICETAG,
		REJECTCALL,
		CALLSTATUS,
		CALLCONTROL,
		ERRORRESULT,
		CODECNEGOTIATION,
#if HFP_V_1_7 == TRUE
		HFPINDICATOR,
		ESCOSETTING,
#endif
		MAXSTATUS
	};

}

namespace CLCC
{
	enum CallStatus
	{
		ACTIVE,
		HELD,
		DIALING,
		ALERTING,
		INCOMING,
		WAITING,
		CALLHELDBYRESPONSE
	};

	enum DeviceStatus
	{
		INDEX,
		DIRECTION,
		STATUS,
		MODE,
		MULTIPARTY,
		NUMBER,
		TYPE,
		MAXSTATUS
	};
}

namespace SCO
{
	enum DeviceStatus
	{
		CONNECTED,
		VOLUME,
		MAXSTATUS
	};
	const int DEFAULT_VOLUME = 10;
	const int HFP_GAIN_STEP = 6;
}

namespace receiveATCMD
{
	enum ATCMD
	{
		CLCC = 1,
		VGS,
		BRSF,
		BVRA,
		MAXATCMD
	};
}

#endif // HFPHFDEFINES_H_
