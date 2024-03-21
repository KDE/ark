###dummy
| CI Job        | Status                                                                                                                                                                                                  |
|---------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Gitlab Builds | [![Gitlab Builds Status](https://invent.kde.org/utilities/ark/badges/master/pipeline.svg)](https://invent.kde.org/utilities/ark/-/pipelines)                                                            |
| Flatpak Build | [![FlatPak Build Status](https://binary-factory.kde.org/view/Flatpak/job/Ark_x86_64_flatpak/badge/icon)](https://binary-factory.kde.org/view/Flatpak/job/Ark_x86_64_flatpak/)                           |

## What is it

Ark is a graphical file compression/decompression utility with support for multiple formats.
Ark can be used to browse, extract, create, and modify archives.

## Supported read-write formats

| Format                | Supported Mimetype                         | Notes                                                                                         |
|-----------------------|--------------------------------------------|-----------------------------------------------------------------------------------------------|
| 7-Zip                 | `application/x-7z-compressed`              |                                                                                               |
| Zip                   | `application/zip`                          |                                                                                               |
| JAR                   | `application/x-java-archive`               |                                                                                               |
| ARJ                   | `application/x-arj`, `application/arj`     | requires the `arj` binary                                                                     |
| RAR                   | `application/vnd.rar`                      | can only create RAR archives with the proprietary `rar` binary                                |
| TAR                   | `application/x-tar`                        |                                                                                               |
| GZip-compressed TAR   | `application/x-compressed-tar`             |                                                                                               |
| BZip2-compressed TAR  | `application/x-bzip2-compressed-tar`       | (`application/x-bzip-compressed-tar` with shared-mime-info < 2.3)                             |
| UNIX-compressed TAR   | `application/x-tarz`                       |                                                                                               |
| XZ-compressed TAR     | `application/x-xz-compressed-tar`          |                                                                                               |
| LZMA-compressed TAR   | `application/x-lzma-compressed-tar`        |                                                                                               |
| LZIP-compressed TAR   | `application/x-lzip-compressed-tar`        |                                                                                               |
| LRZIP-compressed TAR  | `application/x-lrzip-compressed-tar`       | requires the `lrzip` binary                                                                   |
| LZO-compressed TAR    | `application/x-tzo`                        | requires the `lzop` binary if libarchive >= 3.3 has been compiled without liblzo2 support     |
| LZ4-compressed TAR    | `application/x-lz4-compressed-tar`         |                                                                                               |
| ZSTD-compressed TAR   | `application/x-zstd-compressed-tar`        | requires the `zstd` binary if libarchive >= 3.3 has been compiled without libzstd support     |


## Supported read-only formats

| Format                    | Supported Mimetype                                                                                                             |
|---------------------------|--------------------------------------------------------------------------------------------------------------------------------|
| RAR                       | `application/vnd.rar`                                                                                                          |
| XAR                       | `application/x-xar`                                                                                                            |
| LHA                       | `application/x-lha`                                                                                                            |
| AppImage                  | `application/x-iso9660-appimage`                                                                                               |
| DEB package               | `application/vnd.debian.binary-package`, `application/x-deb`                                                                   |
| Raw CD image              | `application/vnd.efi.img`, `application/x-cd-image`                                                                            |
| CPIO variants             | `application/x-cpio`, `application/x-bcpio`, `application/x-cpio-compressed`, `application/x-sv4cpio`, `application/x-sv4crc`  |
| RPM (source) package      | `application/x-rpm`, `application/x-source-rpm`                                                                                |
| UNIX-compressed TAR       | `application/x-compress`                                                                                                       |
| GZip-compressed file      | `application/gzip`                                                                                                             |
| BZip2-compressed file     | `application/x-bzip2` (`application/x-bzip` with shared-mime-info < 2.3)                                                       |
| LZMA-compressed file      | `application/x-lzma`                                                                                                           |
| XZ-compressed file        | `application/x-xz`                                                                                                             |
| Zlib-compressed file      | `application/zlib`                                                                                                             |
| Zstd-compressed file      | `application/zstd`                                                                                                             |
| LZ4-compressed file       | `application/x-lz4`                                                                                                            |
| LZIP-compressed file      | `application/x-lzip`                                                                                                           |
| LRZIP-compressed file     | `application/x-lrzip`                                                                                                          |
| LZOP-compressed file      | `application/x-lzop`                                                                                                           |
| Windows theme pack        | `application/vnd.ms-cab-compressed`                                                                                            |
| AR                        | `application/x-archive`                                                                                                        |
| Stuffit                   | `application/x-stuffit`                                                                                                        |
| Self-extracting EXE       | `application/x-msdownload`                                                                                                     |

The notes about `lrzip`, `lzop` and `zstd` binaries apply also here.

## Plugins

The support for all the above formats in Ark is implemented in a number of plugins that can be enabled/disabled by the user.
More than one plugin can support the same format. An archive whose format is supported by more than one plugin, will be open using the enabled plugin with higher priority.

The plugins currently available in ark are the following:

* Libzip plugin: supports the Zip format by using the libzip library.
* 7-Zip plugin: supports the 7-Zip format by using the `7z` binary.
    * Supports both the upstream 7-Zip binary and the binary from a p7zip fork shipped by Archlinux et al.
* Unarchiver plugin: supports the RAR, LHA and Stuffit formats in read-only mode. Requires the `lsar` and `unar` binaries from the unarchiver project.
* RAR plugin: supports the RAR format by using the unrar binary. Requires the proprietary `rar` binary to enable read-write mode support to create RAR archives.
* Info-zip plugin (legacy): supports the Zip format by using the `zip` binary.
* ARJ plugin: supports the ARJ format by using the `arj` binary.
* Libarchive plugin: supports everything else by using the libarchive library and optionally the `lrzip`, `lzop` and `zstd` binaries.


## Contributing to ARK

Please refer to the [contributing document](CONTRIBUTING.md) for everything you need to know to get started contributing to ARK.
