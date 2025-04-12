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
#include "usb_monitor.h"
#include "usb_ipp_manager.h"

using namespace OHOS::CUPS;

static IppUsbManager& usbManager = IppUsbManager::GetInstance();

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
        fprintf(stderr, "DEBUG: USB_MONITOR ProcessMonitorPrinter fail\n");
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