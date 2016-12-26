{
    "KPlugin": {
        "Description": "Legacy support for the zip archive format", 
        "Description[ca]": "Implementació pel format d'arxiu «zip» antic", 
        "Description[es]": "Uso heredado para el formato de archivo comprimido zip", 
        "Description[it]": "Supporto originale per il formato di archivi zip", 
        "Description[nl]": "Verouderde ondersteuning voor het zip-archiefformaat", 
        "Description[pl]": "Obsługa przestarzałego formatu archiwów zip", 
        "Description[pt]": "Suporte antigo para o formato de pacotes ZIP", 
        "Description[sv]": "Stöd för föråldrat zip-arkivformat", 
        "Description[uk]": "Підтримка архівів у застарілій версії формату zip", 
        "Description[x-test]": "xxLegacy support for the zip archive formatxx", 
        "Id": "kerfuffle_clizip", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "Info-zip plugin", 
        "Name[ca]": "Connector de l'Info-zip", 
        "Name[es]": "Complemento Info-zip", 
        "Name[it]": "Estensione Info-zip", 
        "Name[nl]": "Info-zip-plug-in", 
        "Name[pl]": "Wtyczka info-zip", 
        "Name[pt]": "'Plugin' do Info-zip", 
        "Name[sv]": "Info-zip-insticksprogram", 
        "Name[uk]": "Додаток info-zip", 
        "Name[x-test]": "xxInfo-zip pluginxx", 
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
