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

#ifndef DEFINES_H_
#define DEFINES_H_

#ifndef FALSE
#define FALSE                   0
#endif

#ifndef TRUE
#define TRUE                    (!(FALSE))
#endif

/* indicator constants HFP 1.1 and later */
#define BTA_AG_IND_CALL             1   /* position of call indicator */
#define BTA_AG_IND_CALLSETUP        2   /* position of callsetup indicator */
#define BTA_AG_IND_SERVICE          3   /* position of service indicator */

/* indicator constants HFP 1.5 and later */
#define BTA_AG_IND_SIGNAL           4   /* position of signal strength indicator */
#define BTA_AG_IND_ROAM             5   /* position of roaming indicator */
#define BTA_AG_IND_BATTCHG          6   /* position of battery charge indicator */
#define BTA_AG_IND_CALLHELD         7   /* position of callheld indicator */
#define BTA_AG_IND_BEARER           8   /* position of bearer indicator */

#define BLUETOOTH_HFP_GAIN_STEP     6
#define HFP_DEFUAL_VOLUME          10

#define TYPE_NATIONAL             129
#define TYPE_INTERNATIONAL        145

#endif

// DEFINES_H_
