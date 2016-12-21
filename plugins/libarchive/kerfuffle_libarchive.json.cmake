{
    "KPlugin": {
        "Description": "Full support for compressed TAR archives", 
        "Id": "kerfuffle_libarchive", 
        "MimeTypes": [
            "@SUPPORTED_READWRITE_MIMETYPES@"
        ], 
        "Name": "Libarchive plugin", 
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ], 
        "Version": "@KDE_APPLICATIONS_VERSION@"
    }, 
    "X-KDE-Kerfuffle-ReadWrite": true, 
    "X-KDE-Priority": 100, 
    "application/x-bzip-compressed-tar": {
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
    }
}
