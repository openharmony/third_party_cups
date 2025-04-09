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