## 三方开源软件cups
### cups简介
cups（Common Unix Printing System）是一种开源打印系统，现在由OpenPrinting组织维护。cups主要功能包括打印队列管理、打印驱动程序管理、网络打印支持等。cups支持多种打印协议，包括IPP（Internet Printing Protocol）、LPD（Line Printer Daemon Protocol）、AppSocket等。

cups支持以下类型的打印机：１、AirPrint和IPP Everywhere认证的打印机；２、带打印机应用程序的网络和USB打印机；３、基于PPD（PostScript Printer Definition）打印驱动程序的网络和本地（USB）打印机。 

您也可以通过[cups官网主页](https://github.com/OpenPrinting/cups)了解更多关于cups项目的信息。

### 引入背景简述
Openharmony南向生态发展过程中，需要对存量市场的打印机进行兼容。使用cups打印系统能直接对接市场上大部分的打印机，也减少了打印机驱动适配OpenHarmony系统的难度。

### 目录结构
```
- LICENSE                           版权文件
- OAT.xml                           OAT.xml过滤配置文件
- README.OpenSource                 项目README.OpenSource文件
- README.md                         英文说明
- README_zh.md                      中文说明
- backport-CVE-2022-26691.patch     CVE修复补丁
- backport-CVE-2023-32324.patch     CVE修复补丁
- backport-CVE-2023-34241.patch     CVE修复补丁
- cups-2.4.0-source.tar.gz          cups2.4.0源码压缩tar包
- backport-xxx.patch                上游更新补丁列表
- cups-xxx.patch                    上游更新补丁列表
- cups.spec                         上游更新记录说明
- cups.yaml                         上游yaml文件
- cups_single_file.patch            适配OH编译补丁文件
- pthread_cancel.patch              适配OH编译补丁文件
- install.sh                        适配OH编译sh脚本文件
- generate_mime_convs.py            适配OH编译python脚本文件
```

### 相关仓
[third_party_cups-filters](https://gitee.com/openharmony/third_party_cups-filters)

[print_print_fwk](https://gitee.com/openharmony/print_print_fwk)

### 参与贡献
[如何贡献](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/参与贡献.md)

[Commit message规范](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83)

