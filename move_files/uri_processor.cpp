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
#include <regex>
#include <stdexcept>
#include "smb_log.h"
#include "uri_processor.h"

namespace {
    constexpr int32_t INDEX_0 = 0;
    constexpr int32_t INDEX_1 = 1;
    constexpr int32_t INDEX_2 = 2;
    constexpr int32_t INDEX_3 = 3;
    constexpr int32_t INDEX_6 = 6;
    static const char* URL_SPACE_ENCODING  = "%20";
}

/*
    The format of the input URI:
    smb://server/share
*/
ParsedURI URIProcessor::ParseURI(const std::string& uri)
{
    ParsedURI result;
    
    if (uri.substr(0, INDEX_6) != "smb://") {
        OHHILOG("DEBUG: SmbBackend uri format is error\n");
        return result;
    }
    
    std::string path = uri.substr(INDEX_6);
    
    std::vector<std::string> parts = Split(path, '/');
    
    if (parts.size() >= INDEX_2) {
        result.server = parts[INDEX_0];
        result.share = parts[INDEX_1];
        DecodeSpacesInShareName(result.share);
    }
    return result;
}

void URIProcessor::DecodeSpacesInShareName(std::string &input)
{
    size_t pos = 0;
    
    while ((pos = input.find(URL_SPACE_ENCODING, pos)) != std::string::npos) {
        input.replace(pos, INDEX_3, " ");
        pos += 1;
    }
}

std::vector<std::string> URIProcessor::Split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;
    
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        std::string token = str.substr(start, end - start);
        tokens.push_back(token);
        start = end + 1;
    }
    
    std::string lastToken = str.substr(start);
    if (start <= str.length()) {
        tokens.push_back(lastToken);
    }
    
    return tokens;
}
