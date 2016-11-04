{
    "KPlugin": {
        "Id": "kerfuffle_cli7z", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "7zip archive plugin", 
        "Name[ca@valencia]": "Connector per arxius 7zip", 
        "Name[ca]": "Connector per arxius 7zip", 
        "Name[cs]": "Modul pro archiv 7zip", 
        "Name[de]": "7zip-Archiv-Modul", 
        "Name[es]": "Complemento de archivo 7zip", 
        "Name[et]": "7zip arhiivi plugin", 
        "Name[fi]": "7zip-pakkaustuki", 
        "Name[fr]": "Module externe d'archive « 7zip »", 
        "Name[gl]": "Complemento de arquivo de 7zip", 
        "Name[he]": "תוסף ארכיוני 7zip", 
        "Name[it]": "Estensione per archivi 7zip", 
        "Name[nb]": "Programtillegg for 7zip-arkiv", 
        "Name[nl]": "7zip-archiefplug-in", 
        "Name[nn]": "7zip-arkivtillegg", 
        "Name[pl]": "Wtyczka archiwów 7zip", 
        "Name[pt]": "'Plugin' de pacotes 7zip", 
        "Name[pt_BR]": "Plugin de arquivos 7zip", 
        "Name[ru]": "Поддержка архивов 7zip", 
        "Name[sk]": "Modul 7zip archívu", 
        "Name[sl]": "Vstavek za arhive 7zip", 
        "Name[sr@ijekavian]": "Прикључак 7зип архива", 
        "Name[sr@ijekavianlatin]": "Priključak 7zip arhiva", 
        "Name[sr@latin]": "Priključak 7zip arhiva", 
        "Name[sr]": "Прикључак 7зип архива", 
        "Name[sv]": "Insticksprogram för 7zip-arkiv", 
        "Name[uk]": "Додаток для архівів 7zip", 
        "Name[x-test]": "xx7zip archive pluginxx", 
        "Name[zh_CN]": "7zip 归档插件", 
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
            "BZip2" : "BZip2",
            "Copy" : "Copy",
            "Deflate" : "Deflate",
            "LZMA" : "LZMA",
            "LZMA2" : "LZMA2",
            "PPMd" : "PPMd"
        },
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
            "BZip2" : "BZip2",
            "Copy" : "Copy",
            "Deflate" : "Deflate",
            "Deflate64" : "Deflate64",
            "LZMA" : "LZMA",
            "PPMd": "PPMd"
        },
        "Encryption": true, 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true
    }
}
