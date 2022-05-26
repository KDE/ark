{
    "KPlugin": {
        "Description": "Full support for the zip and 7z archive formats",
        "Description[ar]": "دعم كامل لتنسيقات الأرشيف zip و 7z",
        "Description[ast]": "Sofitu completu pa los formatos d'archivos zip y 7z",
        "Description[az]": "Zip və 7z formatlı arxivlərin tam dəstəklənməsi",
        "Description[bg]": "Пълна поддръжка на архивни формати zip и 7z",
        "Description[ca@valencia]": "Implementació completa dels formats d'arxiu «zip» i «7z»",
        "Description[ca]": "Implementació completa dels formats d'arxiu «zip» i «7z»",
        "Description[cs]": "Plná podpora archivačních formátů zip a 7z",
        "Description[da]": "Fuld understøttelse af zip- og 7z-arkivformater",
        "Description[de]": "Vollständige Unterstützung für Zip- und 7z-Archivformate",
        "Description[el]": "Πλήρης υποστήριξη για την αρχειοθήκη μορφής zip και 7z",
        "Description[en_GB]": "Full support for the zip and 7z archive formats",
        "Description[es]": "Uso total de los formatos de archivos comprimidos zip y 7z",
        "Description[et]": "Zip ja 7z arhiivivormingu täielik toetus",
        "Description[eu]": "zip eta 7z artxibo fitxategientzako euskarri osoa",
        "Description[fi]": "Täysi Zip- ja 7z-tiedostomuotojen tuki",
        "Description[fr]": "Prise en charge complète des formats d'archive zip et 7z",
        "Description[gl]": "Compatibilidade total cos formatos de arquivo zip and 7z.",
        "Description[hu]": "Teljeskörű támogatás a zip és 7z archívumformátumhoz",
        "Description[ia]": "Supporto complete per le formato de archivo zip e 7z",
        "Description[id]": "Dukungan penuh untuk arsip berformat zip dan 7z",
        "Description[it]": "Supporto completo per i formati di archivi zip e 7z",
        "Description[ja]": "zip と 7z アーカイブ形式に完全に対応",
        "Description[ko]": "zip 및 7z 압축 형식 지원",
        "Description[nl]": "Volledige ondersteuning voor de zip- en 7z-archiefformaten",
        "Description[nn]": "Full støtte for arkivformata ZIP og 7z",
        "Description[pl]": "Pełna obsługa dla archiwów zip oraz 7z",
        "Description[pt]": "Suporte total para os formatos de pacotes ZIP e 7z",
        "Description[pt_BR]": "Suporte total para os formatos de arquivo ZIP e 7z",
        "Description[ro]": "Suport complet pentru formatele de arhivă zip și 7zip",
        "Description[ru]": "Полная поддержка архивов ZIP и 7z",
        "Description[sk]": "Plná podpora pre archívne formáty zip a 7z",
        "Description[sl]": "Polna podpora za arhive vrste zip in 7z",
        "Description[sr@ijekavian]": "Пуна подршка за архивске формате ЗИП и 7зип",
        "Description[sr@ijekavianlatin]": "Puna podrška za arhivske formate ZIP i 7zip",
        "Description[sr@latin]": "Puna podrška za arhivske formate ZIP i 7zip",
        "Description[sr]": "Пуна подршка за архивске формате ЗИП и 7зип",
        "Description[sv]": "Fullt stöd för zip- och 7z-arkivformaten",
        "Description[tr]": "Zip ve 7z arşiv biçimleri için tam destek",
        "Description[uk]": "Повноцінна підтримка архівів у форматах zip і 7z",
        "Description[x-test]": "xxFull support for the zip and 7z archive formatsxx",
        "Description[zh_CN]": "完整支持 ZIP 和 7z 压缩包格式",
        "Description[zh_TW]": "對 zip 與 7z 壓縮檔格式的完整支援",
        "Id": "kerfuffle_cli7z",
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ],
        "Name": "7z plugin",
        "Name[ca]": "Connector del 7z",
        "Name[es]": "Complemento 7z",
        "Name[nl]": "7z-plug-in",
        "Name[sl]": "Vstavek 7z",
        "Name[tr]": "7z eklentisi",
        "Name[uk]": "Додаток 7z",
        "Name[x-test]": "xx7z pluginxx",
        "Version": "@RELEASE_SERVICE_VERSION@"
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
