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

#include "config.h"
#include "logging.h"
#include "bluetoothhfpservice.h"

PmLogContext logContext;

static const char* const logContextName = "webos-hfp-service";

static gboolean option_version = FALSE;

static GOptionEntry options[] = {
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
		"Show version information and exit" },
	{ NULL },
};

int main(int argc, char **argv)
{
	try
	{
		GMainLoop *mainLoop;
		GOptionContext *context;
		GError *err = NULL;

		context = g_option_context_new(NULL);
		g_option_context_add_main_entries(context, options, NULL);

		if (g_option_context_parse(context, &argc, &argv, &err) == FALSE) {
			if (err != NULL) {
				g_printerr("%s\n", err->message);
				g_error_free(err);
			} else
				g_printerr("An unknown error occurred\n");
			exit(1);
		}

		g_option_context_free(context);

		if (option_version == TRUE) {
			printf("%s\n", VERSION);
			exit(0);
		}

		PmLogErr error = PmLogGetContext(logContextName, &logContext);
		if (error != kPmLogErr_None)
		{
			fprintf(stderr, "Failed to setup up log context %s\n", logContextName);
			abort();
		}

		mainLoop = g_main_loop_new(NULL, FALSE);

		BluetoothHfpService service;
		service.attachToLoop(mainLoop);

		g_main_loop_run(mainLoop);

		g_main_loop_unref(mainLoop);

	}
	catch (const LS::Error& error) // 	CID 163476 (#1 of 1): Uncaught exception (UNCAUGHT_EXCEPT)
	{
		BT_ERROR("BT_EXCEPTION", 0, "Failed to LS::Error: %s", error.what());
	}
	return 0;
}

