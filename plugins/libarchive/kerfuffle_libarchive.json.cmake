{
    "KPlugin": {
        "Description": "Full support for compressed TAR archives",
        "MimeTypes": [
            "application/x-tar",
            "application/x-compressed-tar",
            "application/x-bzip-compressed-tar",
            "application/x-bzip2-compressed-tar",
            "application/x-tarz",
            "application/x-xz-compressed-tar",
            "application/x-lzma-compressed-tar",
            "application/x-lzip-compressed-tar",
            "application/x-tzo",
            "application/x-lrzip-compressed-tar",
            "application/x-lz4-compressed-tar",
            "application/x-zstd-compressed-tar"
        ],
        "Name": "Libarchive plugin",
        "Version": "@RELEASE_SERVICE_VERSION@"
    },
    "X-KDE-Kerfuffle-ReadWrite": true,
    "X-KDE-Priority": 100,
    "application/x-bzip-compressed-tar": {
        "CompressionLevelDefault": 9,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 1
    },
    "application/x-bzip2-compressed-tar": {
        "CompressionLevelDefault": 9,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 1
    },
    "application/x-compressed-tar": {
        "CompressionLevelDefault": 6,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 1
    },
    "application/x-lrzip-compressed-tar": {
        "CompressionLevelDefault": 1,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 1
    },
    "application/x-lz4-compressed-tar": {
        "CompressionLevelDefault": 1,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 1
    },
    "application/x-lzip-compressed-tar": {
        "CompressionLevelDefault": 6,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 0
    },
    "application/x-lzma-compressed-tar": {
        "CompressionLevelDefault": 6,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 0
    },
    "application/x-tzo": {
        "CompressionLevelDefault": 5,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 1
    },
    "application/x-xz-compressed-tar": {
        "CompressionLevelDefault": 6,
        "CompressionLevelMax": 9,
        "CompressionLevelMin": 0
    },
    "application/x-zstd-compressed-tar": {
        "CompressionLevelDefault": 3,
        "CompressionLevelMax": 22,
        "CompressionLevelMin": 1
    }
}
