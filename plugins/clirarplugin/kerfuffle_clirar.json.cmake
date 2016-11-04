{
    "KPlugin": {
        "Id": "kerfuffle_clirar", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "RAR archive plugin", 
        "Name[ca@valencia]": "Connector per arxius RAR", 
        "Name[ca]": "Connector per arxius RAR", 
        "Name[cs]": "Modul pro archiv RAR", 
        "Name[de]": "RAR-Archiv-Modul", 
        "Name[es]": "Complemento de archivo RAR", 
        "Name[et]": "RAR-arhiivi plugin", 
        "Name[fi]": "RAR-pakkaustuki", 
        "Name[fr]": "Module externe d'archive « RAR »", 
        "Name[gl]": "Complemento de arquivo RAR", 
        "Name[he]": "תוסף ארכיוני RAR", 
        "Name[it]": "Estensione per archivi RAR", 
        "Name[ja]": "RAR アーカイブ用プラグイン", 
        "Name[nb]": "Programtillegg for RAR-arkiv", 
        "Name[nl]": "RAR-archiefplug-in", 
        "Name[nn]": "RAR-arkivtillegg", 
        "Name[pl]": "Wtyczka archiwów RAR", 
        "Name[pt]": "'Plugin' de pacotes RAR", 
        "Name[pt_BR]": "Plugin de arquivos RAR", 
        "Name[ru]": "Поддержка архивов RAR", 
        "Name[sk]": "Modul RAR archívu", 
        "Name[sl]": "Vstavek za arhive RAR", 
        "Name[sr@ijekavian]": "Прикључак РАР архива", 
        "Name[sr@ijekavianlatin]": "Priključak RAR arhiva", 
        "Name[sr@latin]": "Priključak RAR arhiva", 
        "Name[sr]": "Прикључак РАР архива", 
        "Name[sv]": "Insticksprogram för RAR-arkiv", 
        "Name[uk]": "Додаток для архівів RAR", 
        "Name[x-test]": "xxRAR archive pluginxx", 
        "Name[zh_CN]": "RAR 归档插件", 
        "ServiceTypes": [
            "Kerfuffle/Plugin"
        ], 
        "Version": "@KDE_APPLICATIONS_VERSION@"
    }, 
    "X-KDE-Kerfuffle-ReadOnlyExecutables": [
        "unrar"
    ], 
    "X-KDE-Kerfuffle-ReadWrite": true, 
    "X-KDE-Kerfuffle-ReadWriteExecutables": [
        "rar"
    ], 
    "X-KDE-Priority": 120, 
    "application/vnd.rar": {
        "CompressionLevelDefault": 3, 
        "CompressionLevelMax": 5, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "RAR4", 
        "CompressionMethods": {
            "RAR4" : "4",
            "RAR5" : "5"
        },
        "HeaderEncryption": true, 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true, 
        "SupportsWriteComment": true
    }, 
    "application/x-rar": {
        "CompressionLevelDefault": 3, 
        "CompressionLevelMax": 5, 
        "CompressionLevelMin": 0, 
        "CompressionMethodDefault": "RAR4", 
        "CompressionMethods": {
            "RAR4" : "4",
            "RAR5" : "5"
        },
        "HeaderEncryption": true, 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true, 
        "SupportsWriteComment": true
    }
}
