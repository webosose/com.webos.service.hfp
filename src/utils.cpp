// Copyright (c) 2020-2021 LG Electronics, Inc.
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

#include <sstream>
#include <locale>

#include <glib.h>

#include "utils.h"

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}

std::string convertToLower(std::string input)
{
	std::string output;
	std::locale loc;
	for (std::string::size_type i=0; i<input.length(); ++i)
		output += std::tolower(input[i],loc);
	return output;
}

bool checkPathExists(const std::string &path)
{
	if (path.length() == 0)
		return false;

	gchar *fileBasePath = g_path_get_dirname(path.c_str());
	if (!fileBasePath)
		return false;

	bool result = g_file_test(fileBasePath, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR));

	g_free(fileBasePath);

	return result;
}

bool checkFileIsValid(const std::string &path)
{
	std::string testPath = path;

	if (testPath.length() == 0)
		return false;

	if (g_file_test(path.c_str(), (GFileTest) G_FILE_TEST_IS_SYMLINK))
		return false;

	return  g_file_test(testPath.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));
}

void dump(const uint8_t *data, size_t len, const char *ind)
{
#define DUMPW 16
        char buf[DUMPW + 1] = { '\0' };
        int cnt = (len + DUMPW - 1) / DUMPW;
        int i, j;

        if (!ind) ind = "";
        printf("%sdump %p %zu \n %s", (char *) ind, data, len, (char *) ind);

        if (!data) return;

        for (i = 0; i < cnt; ++i) {
                for (j = 0; j < DUMPW; ++j) {
                        sprintf(buf + j, "%c", (0x20 <= *data && *data <= 0x7e) ? (char)*data : '.');
                        printf("%02x ", *data++);
                        if (--len == 0)
                                break;
                }
                buf[j] = '\0';
                printf("%*s\n%s", (DUMPW - j) * 3, buf, ind);
        }
        printf("\n");
#undef DUMPW
}

uint32_t percentToIndRange(int percent)
{
	uint32_t range = 0;
	if (percent == 0)
		range = 0;
	else if (percent <= 20)
		range = 1;
	else if (percent <= 40)
		range = 2;
	else if (percent <= 60)
		range = 3;
	else if (percent <= 80)
		range = 4;
	else if (percent <= 100)
		range = 5;

	return range;
}

void removeSpace(std::string &sentence)
{
	int spaceIndex = sentence.find_last_of(" ");
	if (spaceIndex == std::string::npos)
		return;
	sentence = sentence.substr(spaceIndex + 1);
}

std::string convertAddressToLowerCase(const std::string &input)
{
	std::string output;
	std::locale loc;
	for (std::string::size_type i=0; i<input.length(); ++i)
		output += std::tolower(input[i],loc);
	return output;
}

std::string convertAddressToUpperCase(const std::string &input)
{
	std::string output;
	std::locale loc;
	for (std::string::size_type i=0; i<input.length(); ++i)
		output += std::toupper(input[i],loc);
	return output;
}

std::string convertToLowerCase(const std::string &input)
{
	std::string output;
	output = convertAddressToLowerCase(input);
	return output;
}

std::string convertToUpperCase(const std::string &input)
{
	std::string output;
	output = convertAddressToUpperCase(input);
	return output;
}
