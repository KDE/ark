{
    "KPlugin": {
        "Description": "Full support for the RAR archive format",
        "MimeTypes": [
            "application/vnd.rar"
        ],
        "Name": "RAR plugin",
        "Version": "@RELEASE_SERVICE_VERSION@"
    },
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "unrar"
    ],
    "X-KDE-Kerfuffle-ReadWrite": true,
    "X-KDE-Kerfuffle-ReadWriteExecutables": [
        "rar"
    ],
    "X-KDE-Priority": 120,
    "application/vnd.rar": {
        "CompressionLevelDefault": 3,
        "CompressionLevelMax": 5,
        "CompressionLevelMin": 0,
        "CompressionMethodDefault": "RAR4",
        "CompressionMethods": {
            "RAR4": "4",
            "RAR5": "5"
        },
        "EncryptionMethodDefault": "AES128",
        "EncryptionMethods": [
            "AES128",
            "AES256"
        ],
        "HeaderEncryption": true,
        "SupportsMultiVolume": true,
        "SupportsTesting": true,
        "SupportsWriteComment": true
    }
}
