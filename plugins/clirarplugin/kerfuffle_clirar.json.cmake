{
    "KPlugin": {
        "Description": "Full support for the RAR archive format", 
        "Description[ca@valencia]": "Implementació completa del format d'arxiu RAR", 
        "Description[ca]": "Implementació completa del format d'arxiu RAR", 
        "Description[cs]": "Plná podpora archivačního formátu RAR", 
        "Description[el]": "Πλήρης υποστήριξη για την αρχειοθήκη μορφής RAR", 
        "Description[es]": "Uso total del formato de archivo comprimido RAR", 
        "Description[it]": "Supporto completo per il formato di archvi RAR", 
        "Description[nl]": "Volledige ondersteuning voor het RAR-archiefformaat", 
        "Description[pl]": "Pełna obsługa dla formatów archiwów RAR", 
        "Description[pt]": "Suporte total para o formato de pacotes RAR", 
        "Description[sk]": "Plná podpora pre archívny formát RAR", 
        "Description[sl]": "Polna podpora za arhive vrste RAR", 
        "Description[sr@ijekavian]": "Пуна подршка за архивски формат РАР", 
        "Description[sr@ijekavianlatin]": "Puna podrška za arhivski format RAR", 
        "Description[sr@latin]": "Puna podrška za arhivski format RAR", 
        "Description[sr]": "Пуна подршка за архивски формат РАР", 
        "Description[sv]": "Fullt stöd för arkivformatet RAR", 
        "Description[uk]": "Повноцінна підтримка архівів у форматі RAR", 
        "Description[x-test]": "xxFull support for the RAR archive formatxx", 
        "Id": "kerfuffle_clirar", 
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ], 
        "Name": "RAR plugin", 
        "Name[ca@valencia]": "Connector del RAR", 
        "Name[ca]": "Connector del RAR", 
        "Name[cs]": "Modul pro RAR", 
        "Name[el]": "Πρόσθετο RAR ", 
        "Name[es]": "Complemento RAR", 
        "Name[it]": "Estensione RAR", 
        "Name[nl]": "RAR-plug-in", 
        "Name[pl]": "Wtyczka RAR", 
        "Name[pt]": "'Plugin' do RAR", 
        "Name[sk]": "Plugin RAR", 
        "Name[sl]": "Vstavek RAR", 
        "Name[sr@ijekavian]": "Прикључак за РАР", 
        "Name[sr@ijekavianlatin]": "Priključak za RAR", 
        "Name[sr@latin]": "Priključak za RAR", 
        "Name[sr]": "Прикључак за РАР", 
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
