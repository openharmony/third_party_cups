# CUPS
## Introduction
OpenPrinting CUPS is the most current version of CUPS, a standards-based, open source printing system for Linux® and other Unix®-like operating systems. CUPS supports printing to:
- AirPrint™ and IPP Everywhere™ printers,
- Network and local (USB) printers with Printer Applications, and
- Network and local (USB) printers with (legacy) PPD-based printer drivers.

You can also learn more about the CUPS project through [the official website](https://github.com/OpenPrinting/cups)

## Background Brief
In the process of OpenHarmony's southward ecological development, it is necessary to be compatible with printers in the stock market. The use of CUPS printing system can directly connect with most printers in the market, which also reduces the difficulty for printer manufacturers to adapt to OpenHarmony.

## How to use
### 1、Header file import
```c
#include <cups/cups-private.h>
```
### 2、Add Compilation Dependency
Add in the bundle. json file
```json
"deps": {
  "third_party": [
    "cups"
  ]
}
```
Add dependencies where needed in BUILD.gn

```json
deps += [ "//third_party/cups:cups" ]
```
### 3、Example of interface usage
```c
// Example of using CUPS interface to query printer capabilities
ipp_t *request; /* IPP Request */
ipp_t *response; /* IPP response */
http_t *http = NULL;
char scheme[HTTP_MAX_URI]; // Protocol 
char username[HTTP_MAX_URI];
char host[HTTP_MAX_URI];
int port;
// Declare which printer capabilities need to be queried, here are all
static const char * const pattrs[] = {
    "all"
};

// Connect to printer
http = httpConnect2(host, port, NULL, AF_UNSPEC, HTTP_ENCRYPTION_IF_REQUESTED, 1, TIME_OUT, NULL);
if (http == nullptr) {
    return;
}
request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printerUri.c_str());
ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser());
ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes",
(int)(sizeof(pattrs) / sizeof(pattrs[0])), NULL, pattrs);
response = cupsDoRequest(http, request, "/");
// parse response
if (cupsLastError() > IPP_STATUS_OK_CONFLICTING) {
    ippDelete(response);
    return;
}
// close http
httpClose(http);
```

#### Repositories Involved
[third_party_cups-filters](https://gitee.com/openharmony/third_party_cups-filters)

[print_print_fwk](https://gitee.com/openharmony/print_print_fwk)

#### Contribution
[How to involve](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/参与贡献.md)

[Commit message spec](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83)

