{
    "KPlugin": {
        "Description": "Full support for the zip archive format", 
        "Id": "kerfuffle_libzip", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "Libzip plugin", 
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ], 
        "Version": "@KDE_APPLICATIONS_VERSION@"
    }, 
    "X-KDE-Kerfuffle-ReadWrite": true, 
    "X-KDE-Priority": 200, 
    "application/zip": {
        "Encryption": true, 
        "EncryptionMethodDefault": "AES256", 
        "EncryptionMethods": [
            "AES256", 
            "AES192", 
            "AES128"
        ], 
        "SupportsTesting": true, 
        "SupportsWriteComment": true
    }
}
