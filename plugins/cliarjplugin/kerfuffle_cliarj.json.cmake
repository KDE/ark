{
    "KPlugin": {
        "Description": "Full support for the ARJ archive formats",
        "Description[ca@valencia]": "Implementació completa del format d'arxiu ARJ",
        "Description[ca]": "Implementació completa del format d'arxiu ARJ",
        "Description[eu]": "ARJ artxibo formatuaren euskarri osoa",
        "Description[nl]": "Volledige ondersteuning voor de ARJ-archiefformaten",
        "Description[pt]": "Suporte total para o formato de pacotes ARJ",
        "Description[tr]": "ARJ arşiv biçimi için tam destek",
        "Description[uk]": "Повноцінна підтримка архівів у форматі ARJ",
        "Description[x-test]": "xxFull support for the ARJ archive formatsxx",
        "Id": "kerfuffle_cliarj",
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ],
        "Name": "ARJ plugin",
        "Name[ca@valencia]": "Connector d'ARJ",
        "Name[ca]": "Connector de l'ARJ",
        "Name[eu]": "ARJ plugina",
        "Name[nl]": "ARJ-plug-in",
        "Name[pt]": "'Plugin' do ARJ",
        "Name[ta]": "ARJ செருகுநிரல்",
        "Name[tr]": "ARJ eklentisi",
        "Name[uk]": "Додаток ARJ",
        "Name[x-test]": "xxARJ pluginxx",
        "Version": "@RELEASE_SERVICE_VERSION@"
    },
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "arj"
    ],
    "X-KDE-Kerfuffle-ReadWrite": true,
    "X-KDE-Kerfuffle-ReadWriteExecutables": [
        "arj"
    ],
    "X-KDE-Priority": 100,
    "application/arj": {
        "CompressionMethodDefault": "Good (default)",
        "CompressionMethods": {
            "FAST! less comp.": "3",
            "FASTEST! least comp.": "4",
            "Good (default)": "1",
            "Less mem. and comp.": "2",
            "Store (no compression)": "0"
        },
        "Encryption": true,
        "EncryptionMethodDefault": "Standard",
        "EncryptionMethods": [
            "Standard",
            "arjcrypt.so",
            "GOST 40-bit"
        ],
        "SupportsMultiVolume": true,
        "SupportsTesting": true,
        "SupportsWriteComment": true
    },
    "application/x-arj": {
        "CompressionMethodDefault": "Good (default)",
        "CompressionMethods": {
            "FAST! less comp.": "3",
            "FASTEST! least comp.": "4",
            "Good (default)": "1",
            "Less mem. and comp.": "2",
            "Store (no compression)": "0"
        },
        "Encryption": true,
        "EncryptionMethodDefault": "Standard",
        "EncryptionMethods": [
            "Standard",
            "arjcrypt.so",
            "GOST 40-bit"
        ],
        "SupportsMultiVolume": true,
        "SupportsTesting": true,
        "SupportsWriteComment": true
    }
}
