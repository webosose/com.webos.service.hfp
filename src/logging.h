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


#ifndef LOGGING_H
#define LOGGING_H

#include <PmLogLib.h>

extern PmLogContext logContext;

#define BT_CRITICAL(msgid, kvcount, ...) \
	PmLogCritical(logContext, msgid, kvcount, ##__VA_ARGS__)

#define BT_ERROR(msgid, kvcount, ...) \
	PmLogError(logContext, msgid, kvcount,##__VA_ARGS__)

#define BT_WARNING(msgid, kvcount, ...) \
	PmLogWarning(logContext, msgid, kvcount, ##__VA_ARGS__)

#define BT_INFO(msgid, kvcount, ...) \
	PmLogInfo(logContext, msgid, kvcount, ##__VA_ARGS__)

#define BT_DEBUG(fmt, ...) \
	PmLogDebug(logContext, "%s:%s() " fmt, __FILE__, __FUNCTION__, ##__VA_ARGS__)

#endif // LOGGING_H
