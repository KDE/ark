{
    "KPlugin": {
        "Description": "Open and extract RAR and LHA archives",
        "MimeTypes": [
            "application/vnd.rar",
            "application/x-lha",
            "application/x-stuffit"
        ],
        "Name": "The Unarchiver plugin",
        "Version": "@RELEASE_SERVICE_VERSION@"
    },
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "lsar",
        "unar"
    ],
    "X-KDE-Kerfuffle-ReadWrite": false,
    "X-KDE-Priority": 100,
    "application/vnd.rar": {
        "HeaderEncryption": true
    }
}
