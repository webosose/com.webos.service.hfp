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

#include <pbnjson.hpp>
#include <luna-service2/lunaservice.hpp>
#include "logging.h"
#include "ls2utils.h"

#ifdef MULTI_SESSION_SUPPORT

#include <map>
#include <string>

std::map<std::string, std::string> sessionInfoMap;

LSUtils::DisplaySetId LSUtils::getDisplaySetIdIndex(LSMessage &message, LS::Handle *handle)
{
	if (LSMessageGetSessionId(&message) != NULL)
	{
		std::string sessionId = LSMessageGetSessionId(&message);
		BT_INFO("INFO_SESSION", 0, "session id is %s", sessionId.c_str());

		if (sessionId == "host")
			return HOST;

		pbnjson::JValue payload = pbnjson::Object();
		payload.put("sessionId", sessionId.c_str());
		pbnjson::JGenerator serializer(nullptr);
		std::string payloadstr;
		serializer.toString(payload, pbnjson::JSchema::AllSchema(), payloadstr);

		auto it = sessionInfoMap.find(sessionId);

		std::string deviceSetId;
		if( it == sessionInfoMap.end())
		{
			auto reply = handle->callOneReply("luna://com.webos.service.account/getSession", payloadstr.c_str()).get();
			pbnjson::JValue replyObj = pbnjson::Object();
			LSUtils::parsePayload(reply.getPayload(), replyObj);

			bool returnValue = replyObj["returnValue"].asBool();
			if (!returnValue) {
				return HOST;
			}

			auto sessionInfo = replyObj["session"];
			auto deviceInfo = sessionInfo["deviceSetInfo"];
			auto deviceSetId = deviceInfo["deviceSetId"].asString();

			sessionInfoMap[sessionId] = deviceSetId;

		}
		else
		{
			deviceSetId = it->second;
		}

		BT_INFO("INFO_SESSION", 0, "deviceSetId %s", deviceSetId.c_str());

		return getDisplaySetIdIndex(deviceSetId);
	}

	BT_INFO("INFO_SESSION", 0, "session is null");

	//failure
	return HOST;
}

LSUtils::DisplaySetId LSUtils::getDisplaySetIdIndex(const std::string &deviceSetId)
{
	if ("RSE-L" == deviceSetId)
	{
		return RSE_L;
	} else if ("RSE-R" == deviceSetId)
	{
		return RSE_R;
	}
	else if ("AVN" == deviceSetId)
	{
		return AVN;
	}

	return HOST;
}
#endif
