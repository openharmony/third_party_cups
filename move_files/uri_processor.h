/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef URI_PROCESSOR_H
#define URI_PROCESSOR_H
#include <string>

constexpr uint32_t MAX_AUTH_LENGTH_SIZE = 64;

struct ParsedURI {
    std::string username;
    std::string server;
    std::string share;
};

class URIProcessor {
public:
    static ParsedURI ParseURI(const std::string& uri);

private:
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static void DecodeSpacesInShareName(std::string &input);
};
#endif  // URI_PROCESSOR_H