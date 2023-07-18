# cups

#### 简介 & 软件架构

- [参考官方文档](https://github.com/OpenPrinting/cups)

#### 使用说明

- [参考官方API文档](https://github.com/OpenPrinting/cups/blob/master/DEVELOPING.md)

#### patch包说明

以下patch包为openEuler:cups开源库本身携带
backport-Also-fix-cupsfilter.patch
backport-CVE-2022-26691.patch
backport-CVE-2023-32324.patch
backport-CVE-2023-34241.patch
backport-Remove-legacy-code-for-RIP_MAX_CACHE-environment-variable.patch
cups-banners.patch
cups-direct-usb.patch
cups-driverd-timeout.patch
cups-freebind.patch
cups-ipp-multifile.patch
cups-multilib.patch
cups-system-auth.patch
cups-uri-compat.patch
cups-usb-paperout.patch
cups-web-devices-timeout.patch
fix-httpAddrGetList-test-case-fail.patch

以下patch包为解决在OpenHarmony工程下编译存在的问题自行添加
cups_single_file.patch
pthread_cancel.patch

#### 参与贡献

[如何贡献](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/参与贡献.md)

[Commit message规范](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83)


#### 相关仓

[third_party_cups-filters](https://gitee.com/openharmony/third_party_cups-filters)

[print_print_fwk](https://gitee.com/openharmony/print_print_fwk)