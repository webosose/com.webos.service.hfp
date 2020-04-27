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

#ifndef HFPHFLS2DATA_H_
#define HFPHFLS2DATA_H_

#include <map>
#include <string>

#include "bluetootherrors.h"
#include "hfphfdefines.h"

using LS2ParamList =  std::map<std::string, std::pair<HFGeneral::DataType, BluetoothErrorCode>>;

class HfpHFLS2Data
{
public:
	HfpHFLS2Data(const std::string &schema, const LS2ParamList &paramList) :
		mSchema(schema),
		mParamList(paramList)
	{}
	~HfpHFLS2Data()	{ mParamList.clear(); }

	void setSchema(const std::string &schema) noexcept { this->mSchema = schema; }
	void setParamList(const LS2ParamList &paramList) noexcept { this->mParamList = paramList; }

	std::string getSchema() const noexcept { return this->mSchema; }
	LS2ParamList getParamList() const noexcept { return this->mParamList; }

private:
	std::string mSchema;
	LS2ParamList mParamList;
};

#endif //HFPHFLS2DATA_H_
