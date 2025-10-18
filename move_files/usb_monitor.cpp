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
#include <cstring>
#include <memory>
#include <arpa/inet.h>
#include <vector>
#include <thread>
#include <utility>
#include <unordered_set>
#include "usb_monitor.h"
#include "usb_ipp_manager.h"

using namespace OHOS::CUPS;

static IppUsbManager& usbManager = IppUsbManager::GetInstance();
static std::pair<std::string, std::string> GetComparedStateReasons(const char* oldReasons, const char* newReasons);

bool IsSupportIppOverUsb(const char* uri)
{
    fprintf(stderr, "DEBUG: USB_MONITOR enter IsSupportIppOverUsb\n");
    if (uri == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR uri is a nullptr\n");
        return false;
    }
    return usbManager.IsSupportIppOverUsb(std::string(uri));
}

bool StartMonitorIppPrinter(MonitorPrinterCallback callback, const char* uri)
{
    fprintf(stderr, "DEBUG: USB_MONITOR enter MonitorPrinter\n");
    if (callback == nullptr || uri == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR callback or uri is nullptr\n");
        return false;
    }
    std::string uriStr(uri);
    if (!usbManager.ConnectUsbPinter(uriStr)) {
        fprintf(stderr, "DEBUG: USB_MONITOR ConnectUsbPinter fail\n");
        return false;
    }
    if (!usbManager.ProcessMonitorPrinter(uriStr, callback)) {
        fprintf(stderr, "DEBUG: USB_MONITOR ProcessMonitorPrinter exit\n");
        usbManager.DisConnectUsbPinter(uriStr);
        return false;
    }
    usbManager.DisConnectUsbPinter(uriStr);
    fprintf(stderr, "DEBUG: USB_MONITOR end MonitorPrinter\n");
    return true;
}

void SetTerminalSingal()
{
    fprintf(stderr, "DEBUG: USB_MONITOR enter SetTerminalSingal\n");
    usbManager.SetTerminalSingal();
}

std::pair<std::string, std::string> GetComparedStateReasons(const char* oldReasons, const char* newReasons)
{
    std::vector<std::string> oldReasonsVec;
    std::vector<std::string> newReasonsVec;
    std::stringstream ssOldReasons(oldReasons);
    std::stringstream ssNewReasons(newReasons);
    std::string token;
    while (std::getline(ssOldReasons, token, ',')) {
        oldReasonsVec.push_back(token);
    }
    while (std::getline(ssNewReasons, token, ',')) {
        newReasonsVec.push_back(token);
    }
    std::unordered_set<std::string> setOldReasons(oldReasonsVec.begin(), oldReasonsVec.end());
    std::unordered_set<std::string> setNewReasons(newReasonsVec.begin(), newReasonsVec.end());
    std::vector<std::string> addedVec;
    std::vector<std::string> deletedVec;
    for (const auto& item : setNewReasons) {
        if (setOldReasons.find(item) == setOldReasons.end()) {
            addedVec.push_back(item);
        }
    }
    for (const auto& item : setOldReasons) {
        if (setNewReasons.find(item) == setNewReasons.end()) {
            deletedVec.push_back(item);
        }
    }
    std::string addedStr;
    std::string deletedStr;
    for (size_t i = 0; i < addedVec.size(); ++i) {
        if (i != 0) {
            addedStr += ",";
        }
        addedStr += addedVec[i];
    }
    for (size_t i = 0; i < deletedVec.size(); ++i) {
        if (i != 0) {
            deletedStr += ",";
        }
        deletedStr += deletedVec[i];
    }
    return std::make_pair(addedStr, deletedStr);
}

void ComparePrinterStateReasons(const char* oldReasons, const char* newReasons,
    char** addedReasons, char** deletedReasons)
{
    if (oldReasons == nullptr || newReasons == nullptr ||
        addedReasons == nullptr || deletedReasons == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR ComparePrinterStateReasons nullptr\n");
        return;       
    }
    if (strcmp(oldReasons, newReasons) == 0) {
        return;
    }
    auto comparedStr = GetComparedStateReasons(oldReasons, newReasons);
    std::string addedStr = comparedStr.first;
    std::string deletedStr = comparedStr.second;
    auto getComparedStr = [](const std::string& str) -> char* {
        int32_t len = static_cast<int32_t>(str.size()) + 1;
        if (len > PRINTER_STATE_REASONS_SIZE) {
            fprintf(stderr, "DEBUG: USB_MONITOR The length exceeds the maximum value.\n");
            return nullptr;
        }
        char* cStr = new (std::nothrow) char[len]{};
        if (cStr == nullptr) {
            fprintf(stderr, "DEBUG: USB_MONITOR cStr new fail\n");
            return nullptr;
        }
        if (strcpy_s(cStr, len, str.c_str()) != 0) {
            fprintf(stderr, "DEBUG: USB_MONITOR ComparePrinterStateReasons strcpy_s fail\n");
            delete[] cStr;
            return nullptr;
        }
        return cStr;
    };

    *addedReasons = getComparedStr(addedStr);
    *deletedReasons = getComparedStr(deletedStr);
}

void FreeCompareStringsResult(char** addedReasons, char** deletedReasons)
{
    if (addedReasons != nullptr && *addedReasons != nullptr) {
        delete[] *addedReasons;
        *addedReasons = nullptr;
    }
    if (deletedReasons != nullptr && *deletedReasons != nullptr) {
        delete[] *deletedReasons;
        *deletedReasons = nullptr;
    }
}

bool IsUsbPrinterStateNormalIdle(const char* uri)
{
    if (uri == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR uri is nullptr\n");
        return true;
    }
    return usbManager.IsUsbPrinterStateNormalIdle(uri);
}