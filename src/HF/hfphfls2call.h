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

#ifndef HFPHFLS2CALL_H_
#define HFPHFLS2CALL_H_

#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>
#include <unordered_map>

#include "hfphfls2data.h"

using LS2Result =  std::unordered_map<std::string, std::string>;

class HfpHFLS2Call
{
public:
	HfpHFLS2Call();
	~HfpHFLS2Call();

	bool parseLSMessage(LS::Message &request, const HfpHFLS2Data &ls2Data, LS2Result &result);
	bool parseSubscriptionData(LS::Message &request, pbnjson::JValue &requestObj);
	std::string getParam(const LS2Result &result, const std::string &key);

private:
	bool parseParam(LS::Message &request, const std::string &schema, pbnjson::JValue &requestObj,
                        const LS2ParamList &paramList);
};

#endif //HFPHFLS2CALL_H_
