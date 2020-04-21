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

#ifndef BLUETOOTH_ERRORS_H_
#define BLUETOOTH_ERRORS_H_

#include <pbnjson.hpp>

enum BluetoothErrorCode
{
	BT_ERR_NO_ERROR,
	BT_ERR_BAD_JSON,
	BT_ERR_SCHEMA_VALIDATION_FAIL,
	BT_ERR_SUBSCRIBED_PARAM_MISSING,
	BT_ERR_RETURN_VALUE_PARAM_MISSING,
	BT_ERR_ADAPTER_ADDR_PARAM_MISSING,
	BT_ERR_DEVICES_PARAM_MISSING,
	BT_ERR_LINES_PARAM_MISSING,
	BT_ERR_ADDRESS_INVALID,
	BT_ERR_ADDR_PARAM_MISSING,
	BT_ERR_VOLUME_PARAM_ERROR,
	BT_ERR_CALL_PARAM_ERROR,
	BT_ERR_VOLUME_PARAM_MISSING,
	BT_ERR_ENABLED_PARAM_MISSING,
	BT_ERR_DEVICE_NOT_CONNECTED
};

const std::string retrieveErrorText(BluetoothErrorCode errorCode);

#endif //BLUETOOTH_ERRORS_H_
