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

#include "ls2utils.h"
#include "hfphfls2call.h"

HfpHFLS2Call::HfpHFLS2Call()
{}

HfpHFLS2Call::~HfpHFLS2Call()
{}

bool HfpHFLS2Call::parseLSMessage(LS::Message &request, const HfpHFLS2Data &ls2Data, LS2Result &result)
{
	pbnjson::JValue requestObj;
	if (!parseParam(request, ls2Data.getSchema(), requestObj, ls2Data.getParamList()))
		return false;

	auto convertParam = [&requestObj, &result] (std::string key, int dataType){
		std::string convertedParam = "";
		switch (dataType)
		{
		case HFGeneral::DataType::INTEGER:
			convertedParam = std::to_string(requestObj[key].asNumber<int>());
			break;
		case HFGeneral::DataType::STRING:
			convertedParam = requestObj[key].asString();
			break;
		case HFGeneral::DataType::BOOLEAN:
			convertedParam = std::to_string(requestObj[key].asBool());
			break;
		default:
			break;
		}
		std::pair<std::string, std::string> insertParam(key, convertedParam);
		result.insert(insertParam);
	};
	auto localParam = ls2Data.getParamList();
	for (auto iterParam : localParam)
	{
		if (requestObj.hasKey(iterParam.first))
			convertParam(iterParam.first, iterParam.second.first);
	}
	return true;
}

bool HfpHFLS2Call::parseParam(LS::Message &request, const std::string &schema, pbnjson::JValue &requestObj,
                                const LS2ParamList &paramList)
{
	int parseError = 0;
	if (LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
		return true;

	BluetoothErrorCode errorCode = BT_ERR_BAD_JSON;
	if (JSON_PARSE_SCHEMA_ERROR == parseError)
	{
		int updatedErrorCode = false;
		for (auto iterList : paramList)
		{
			if (!requestObj.hasKey(iterList.first))
			{
				errorCode = iterList.second.second;
				updatedErrorCode = true;
				break;
			}
		}
		if (!updatedErrorCode)
			errorCode = BT_ERR_SCHEMA_VALIDATION_FAIL;
	}
	LSUtils::respondWithError(request, errorCode);
	return false;
}

bool HfpHFLS2Call::parseSubscriptionData(LS::Message &request, pbnjson::JValue &requestObj)
{
	int parseError = 0;

	if (!LSUtils::parsePayload(request.getPayload(), requestObj, "", &parseError))
	{
		if (JSON_PARSE_SCHEMA_ERROR != parseError)
			LSUtils::respondWithError(request, BT_ERR_BAD_JSON);
		return false;
	}
	return true;
}

std::string HfpHFLS2Call::getParam(const LS2Result &result, const std::string &key)
{
	auto iterResult = result.find(key);
	if (iterResult != result.end())
		return iterResult->second;
	return "";
}
