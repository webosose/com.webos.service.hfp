// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include "asyncutils.h"

void glibAsyncMethodWrapper(GObject *sourceObject, GAsyncResult *result, gpointer user_data)
{
	GlibAsyncFunctionWrapper *wrapper = static_cast<GlibAsyncFunctionWrapper*>(user_data);
	wrapper->call(result);
	delete wrapper;
}

gboolean glibSourceMethodWrapper(gpointer user_data)
{
	GlibSourceFunctionWrapper *wrapper = static_cast<GlibSourceFunctionWrapper*>(user_data);
	gboolean result = wrapper->call();
	delete wrapper;
	return result;
}
