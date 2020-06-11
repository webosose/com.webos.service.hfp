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

#include <string>
#include <map>

#include "bluetootherrors.h"

static std::map<BluetoothErrorCode, std::string> bluetoothErrorTextTable =
{
	{BT_ERR_NO_ERROR, "No error"},
	{BT_ERR_BAD_JSON, "Invalid JSON input"},
	{BT_ERR_SCHEMA_VALIDATION_FAIL, "The JSON input does not match the expected schema"},
	{BT_ERR_SUBSCRIBED_PARAM_MISSING, "Required 'subscribed' parameter not supplied"},
	{BT_ERR_RETURN_VALUE_PARAM_MISSING, "Required 'returnValue' parameter not supplied"},
	{BT_ERR_ADAPTER_ADDR_PARAM_MISSING, "Required 'adapterAddress' parameter not supplied"},
	{BT_ERR_DEVICES_PARAM_MISSING, "Required 'devices' parameter not supplied"},
	{BT_ERR_LINES_PARAM_MISSING, "Required 'lines' parameter not supplied"},
	{BT_ERR_ADDRESS_INVALID, "'address' parameter is invalid"},
	{BT_ERR_ADDR_PARAM_MISSING, "Required 'address' parameter not supplied"},
	{BT_ERR_VOLUME_PARAM_ERROR, "The range of 'volume' parameter is from 0 to 15."},
	{BT_ERR_CALL_PARAM_ERROR, "'number' and 'memoryDialing' parameters are not supplied together."},
	{BT_ERR_VOLUME_PARAM_MISSING, "Required 'volume' paramter not supplied"},
	{BT_ERR_ENABLED_PARAM_MISSING, "Required 'enabled' parameter not supplied"},
	{BT_ERR_DEVICE_NOT_CONNECTED, "Device with supplied address is not connected"},
	{BT_ERR_ANSWER_NO_INCOMING_CALL, "No voice call to answer"},
	{BT_ERR_ANSWER_CALL_FAILED, "Answer call failed"},
	{BT_ERR_INDEX_PARAM_MISSING, "VoiceCall Index is missing"},
	{BT_ERR_TERMINATE_CALL_FAILED, "Terminate call failed"},
	{BT_ERR_NO_VOICE_CALL_FOUND, "No voice call found"},
	{BT_ERR_ADAPTER_IS_NOT_AVAILABLE, "Adapter is not available"},
	{BT_ERR_HOLD_ACTIVE_CALLS_FAILED, "Hold Active Call failed"},
	{BT_ERR_NO_ACTIVE_VOICE_CALL, "No active voice call"},
	{BT_ERR_NO_WAITING_VOICE_CALL, "No waiting voice call"},
	{BT_ERR_NO_HELD_VOICE_CALL, "No voice call on hold"},
	{BT_ERR_MERGE_VOICE_CALL_FAILED, "Merge voice call failed"},
	{BT_ERR_RELEASE_ACTIVE_CALLS_FAILED, "Release Active voice call failed"}
};

const std::string retrieveErrorText(BluetoothErrorCode errorCode)
{
	return bluetoothErrorTextTable[errorCode];
}
