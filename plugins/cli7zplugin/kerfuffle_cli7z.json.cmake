{
    "KPlugin": {
        "Description": "Full support for the zip and 7z archive formats", 
        "Id": "kerfuffle_cli7z", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "P7zip plugin", 
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ], 
        "Version": "@KDE_APPLICATIONS_VERSION@"
    }, 
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "7z"
    ], 
    "X-KDE-Kerfuffle-ReadWrite": true, 
    "X-KDE-Kerfuffle-ReadWriteExecutables": [
        "7z"
    ], 
    "X-KDE-Priority": 180, 
    "application/x-7z-compressed": {
        "CompressionLevelDefault": 5, 
        "CompressionLevelMax": 9, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "LZMA2", 
        "CompressionMethods": {
            "BZip2": "BZip2", 
            "Copy": "Copy", 
            "Deflate": "Deflate", 
            "LZMA": "LZMA", 
            "LZMA2": "LZMA2", 
            "PPMd": "PPMd"
        }, 
        "EncryptionMethodDefault": "AES256", 
        "EncryptionMethods": [
            "AES256"
        ], 
        "HeaderEncryption": true, 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true
    }, 
    "application/zip": {
        "CompressionLevelDefault": 5, 
        "CompressionLevelMax": 9, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "Deflate", 
        "CompressionMethods": {
            "BZip2": "BZip2", 
            "Copy": "Copy", 
            "Deflate": "Deflate", 
            "Deflate64": "Deflate64", 
            "LZMA": "LZMA", 
            "PPMd": "PPMd"
        }, 
        "Encryption": true, 
        "EncryptionMethodDefault": "AES256", 
        "EncryptionMethods": [
            "AES256", 
            "AES192", 
            "AES128", 
            "ZipCrypto"
        ], 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true
    }
}
