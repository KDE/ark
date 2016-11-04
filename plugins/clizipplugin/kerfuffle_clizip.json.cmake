{
    "KPlugin": {
        "Id": "kerfuffle_clizip", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "ZIP archive plugin", 
        "Name[ca@valencia]": "Connector per arxius ZIP", 
        "Name[ca]": "Connector per arxius ZIP", 
        "Name[cs]": "Modul pro archiv ZIP", 
        "Name[de]": "ZIP-Archiv-Modul", 
        "Name[es]": "Complemento de archivo ZIP", 
        "Name[et]": "ZIP-arhiivi plugin", 
        "Name[fi]": "ZIP-pakkaustuki", 
        "Name[fr]": "Module externe d'archive « zip »", 
        "Name[gl]": "Complemento de arquivo ZIP", 
        "Name[he]": "תוסף ארכיוני ZIP", 
        "Name[it]": "Estensione per archivi ZIP", 
        "Name[nb]": "Programtillegg for ZIP-arkiv", 
        "Name[nl]": "ZIP-archiefplug-in", 
        "Name[nn]": "ZIP-arkivtillegg", 
        "Name[pl]": "Wtyczka archiwów ZIP", 
        "Name[pt]": "'Plugin' de pacotes ZIP", 
        "Name[pt_BR]": "Plugin de arquivos ZIP", 
        "Name[ru]": "Поддержка архивов ZIP", 
        "Name[sk]": "Modul ZIP archívu", 
        "Name[sl]": "Vstavek za arhive ZIP", 
        "Name[sr@ijekavian]": "Прикључак ЗИП архива", 
        "Name[sr@ijekavianlatin]": "Priključak ZIP arhiva", 
        "Name[sr@latin]": "Priključak ZIP arhiva", 
        "Name[sr]": "Прикључак ЗИП архива", 
        "Name[sv]": "Insticksprogram för ZIP-arkiv", 
        "Name[uk]": "Додаток для архівів ZIP", 
        "Name[x-test]": "xxZIP archive pluginxx", 
        "Name[zh_CN]": "ZIP 归档插件", 
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
            "BZip2" : "bzip2",
            "Deflate" : "deflate",
            "Store" : "store"
        },
        "Encryption": true, 
        "SupportsTesting": true
    }
}
