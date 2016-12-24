{
    "KPlugin": {
        "Description": "Full support for the RAR archive format", 
        "Description[ca]": "Implementació completa del format d'arxiu RAR", 
        "Description[it]": "Supporto completo per il formato di archvi RAR", 
        "Description[nl]": "Volledige ondersteuning voor het RAR-archiefformaat", 
        "Description[pt]": "Suporte total para o formato de pacotes RAR", 
        "Description[sv]": "Fullt stöd för arkivformatet RAR", 
        "Description[uk]": "Повноцінна підтримка архівів у форматі RAR", 
        "Description[x-test]": "xxFull support for the RAR archive formatxx", 
        "Id": "kerfuffle_clirar", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "RAR plugin", 
        "Name[ca]": "Connector del RAR", 
        "Name[it]": "Estensione RAR", 
        "Name[nl]": "RAR-plug-in", 
        "Name[pt]": "'Plugin' do RAR", 
        "Name[sv]": "RAR-insticksprogram", 
        "Name[uk]": "Додаток RAR", 
        "Name[x-test]": "xxRAR pluginxx", 
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
            "RAR4": "4", 
            "RAR5": "5"
        }, 
        "EncryptionMethodDefault": "AES128", 
        "EncryptionMethods": [
            "AES128", 
            "AES256"
        ], 
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
            "RAR4": "4", 
            "RAR5": "5"
        }, 
        "EncryptionMethodDefault": "AES128", 
        "EncryptionMethods": [
            "AES128", 
            "AES256"
        ], 
        "HeaderEncryption": true, 
        "SupportsMultiVolume": true, 
        "SupportsTesting": true, 
        "SupportsWriteComment": true
    }
}
