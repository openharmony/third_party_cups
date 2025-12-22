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
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <memory>
#include "uri_processor.h"
#include "smb_log.h"
#include "smb_spool.h"
#include "backend.h"

int32_t main(int32_t argc, char* argv[]) {
    OHHILOG("DEBUG: SmbBackend enter backend\n");

    if (argc == 1) {
        return CUPS_BACKEND_OK;
    }

    if (argc != 6) {
        OHHILOG("DEBUG: argc = [%d]\n", argc);
        return CUPS_BACKEND_FAILED;
    }
    const char* deviceURI = argv[0];

    ParsedURI uri = URIProcessor::ParseURI(deviceURI);

    if (uri.server.empty() || uri.share.empty()) {
        OHHILOG("DEBUG: Invalid device URI format\n");
        return CUPS_BACKEND_FAILED;
    }

    const char *username = getenv("AUTH_USERNAME");
    if (username) {
        uri.username = std::string(username);
    }
    const char *password = getenv("AUTH_PASSWORD");
    SMBSpool spooler;
    if (!spooler.ConnectToShare(uri, password)) {
        OHHILOG("DEBUG: Failed to connect to SMB\n");
        const char *authInfoRequired = "username,password";
	    fprintf(stderr, "ATTR: auth-info-required=%s\n", authInfoRequired);
        return CUPS_BACKEND_AUTH_REQUIRED;
    }

    FILE* inputFile = stdin;

    bool success = spooler.PrintFile(inputFile);

    if (!success) {
        OHHILOG("DEBUG: Print failed\n");
        return CUPS_BACKEND_FAILED;
    }

    return CUPS_BACKEND_OK;
}