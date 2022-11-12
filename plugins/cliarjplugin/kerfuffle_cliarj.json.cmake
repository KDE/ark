{
    "KPlugin": {
        "Description": "Full support for the ARJ archive formats",
        "Description[ar]": "دعم كامل لتنسيقات الأرشيف ARJ",
        "Description[az]": "ARJ formatlı arxivlərin tam dəstəklənməsi",
        "Description[bg]": "Пълна поддръжка на архивен формат ARJ",
        "Description[ca@valencia]": "Implementació completa del format d'arxiu ARJ",
        "Description[ca]": "Implementació completa del format d'arxiu ARJ",
        "Description[es]": "Uso total de los formatos de archivo comprimido ARJ",
        "Description[eu]": "ARJ artxibo formatuaren euskarri osoa",
        "Description[fr]": "Prise en charge complète des formats d'archive « ARJ »",
        "Description[ia]": "Supporto complete per le formato de archivo ARJ",
        "Description[ie]": "Complet supporte por archives del formate ARJ",
        "Description[it]": "Supporto completo per il formato di archivi ARJ",
        "Description[ka]": "ARJ არქვივების სრული მხარდაჭერა",
        "Description[nl]": "Volledige ondersteuning voor de ARJ-archiefformaten",
        "Description[pl]": "Pełna obsługa dla archiwów ARJ",
        "Description[pt]": "Suporte total para o formato de pacotes ARJ",
        "Description[pt_BR]": "Suporte total para o formato de arquivos ARJ",
        "Description[sl]": "Polna podpora za arhive vrste ARJ",
        "Description[tr]": "ARJ arşiv biçimi için tam destek",
        "Description[uk]": "Повноцінна підтримка архівів у форматі ARJ",
        "Description[x-test]": "xxFull support for the ARJ archive formatsxx",
        "Id": "kerfuffle_cliarj",
        "MimeTypes": [
            "@SUPPORTED_MIMETYPES@"
        ],
        "Name": "ARJ plugin",
        "Name[ar]": "ملحق ARJ",
        "Name[az]": "ARJ qoşması",
        "Name[bg]": "Приставка ARJ",
        "Name[ca@valencia]": "Connector d'ARJ",
        "Name[ca]": "Connector de l'ARJ",
        "Name[es]": "Complemento ARJ",
        "Name[eu]": "ARJ plugina",
        "Name[fr]": "Module externe « ARJ »",
        "Name[ia]": "Plug-in ARJ",
        "Name[ie]": "Plugin ARJ",
        "Name[it]": "Estensione ARJ",
        "Name[ka]": "ARJ -ის მხარდაჭერა",
        "Name[nl]": "ARJ-plug-in",
        "Name[pl]": "Wtyczka ARJ",
        "Name[pt]": "'Plugin' do ARJ",
        "Name[pt_BR]": "Plugin ARJ",
        "Name[sl]": "Vstavek ARJ",
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
