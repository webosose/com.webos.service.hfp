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

#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>
#include <vector>

std::vector<std::string> split(const std::string &s, char delim);
std::string convertToLower(std::string input);

bool checkPathExists(const std::string &path);
bool checkFileIsValid(const std::string &path);
void dump(const uint8_t *data, size_t len, const char *ind);
uint32_t percentToIndRange(int percent);
void removeSpace(std::string &sentence);

std::string convertAddressToLowerCase(const std::string &input);
std::string convertAddressToUpperCase(const std::string &input);
std::string convertToLowerCase(const std::string &input);
std::string convertToUpperCase(const std::string &input);
#endif // UTILS_H
