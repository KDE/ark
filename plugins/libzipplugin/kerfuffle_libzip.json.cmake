{
    "KPlugin": {
        "Description": "LibZip Plugin for Kerfuffle",
        "Id": "kerfuffle_libzip",
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ],
        "Name": "kerfuffle_libzip",
        "Name[x-test]": "xxkerfuffle_libzipxx",
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ],
        "Version": "@KDE_APPLICATIONS_VERSION@"
    },
    "X-KDE-Kerfuffle-ReadWrite": true,
    "X-KDE-Priority": 200,
    "application/zip": {
        "SupportsWriteComment": true,
        "SupportsTesting": true,
        "Encryption": true,
        "EncryptionMethodDefault": "AES256",
        "EncryptionMethods": [
            "AES256",
            "AES192",
            "AES128"
        ]
    }
}
