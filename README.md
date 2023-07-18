# cups

#### Introduction & Software Architecture
- [Refer to the official documentation](ttps://github.com/OpenPrinting/cups)

#### Usage Guidelines

- [Refer to the official API documentation](https://github.com/OpenPrinting/cups/blob/master/DEVELOPING.md)

#### Generates  Header File

The following patches is carried by the openEuler:libnl3 open source library:

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

The following patches are added to solve the compilation problem under the OpenHarmony project:

cups_single_file.patch
pthread_cancel.patch

#### Contribution

[How to involve](https://gitee.com/openharmony/docs/blob/HEAD/zh-cn/contribute/参与贡献.md)

[Commit message spec](https://gitee.com/openharmony/device_qemu/wikis/Commit%20message%E8%A7%84%E8%8C%83)

#### Repositories Involved

[third_party_cups-filters](https://gitee.com/openharmony/third_party_cups-filters)

[print_print_fwk](https://gitee.com/openharmony/print_print_fwk)