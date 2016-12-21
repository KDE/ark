{
    "KPlugin": {
        "Description": "Legacy support for the zip archive format", 
        "Id": "kerfuffle_clizip", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "Info-zip plugin", 
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ], 
        "Version": "@KDE_APPLICATIONS_VERSION@"
    }, 
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "zipinfo", 
        "unzip"
    ], 
    "X-KDE-Kerfuffle-ReadWrite": true, 
    "X-KDE-Kerfuffle-ReadWriteExecutables": [
        "zip"
    ], 
    "X-KDE-Priority": 160, 
    "application/x-java-archive": {
        "CompressionLevelDefault": 6, 
        "CompressionLevelMax": 9, 
        "CompressionLevelMin": 0, 
        "Encryption": true, 
        "SupportsTesting": true
    }, 
    "application/zip": {
        "CompressionLevelDefault": 6, 
        "CompressionLevelMax": 9, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "Deflate", 
        "CompressionMethods": {
            "BZip2": "bzip2", 
            "Deflate": "deflate", 
            "Store": "store"
        }, 
        "Encryption": true, 
        "EncryptionMethodDefault": "ZipCrypto", 
        "EncryptionMethods": [
            "ZipCrypto"
        ], 
        "SupportsTesting": true
    }
}
