[Desktop Entry]
Type=Service
ServiceTypes=KonqPopupMenu/Plugin
MimeType=@SUPPORTED_ARK_MIMETYPES@
Actions=arkAutoExtractHere;arkExtractTo;arkExtractHere;
X-KDE-Priority=TopLevel
X-KDE-StartupNotify=false
X-KDE-Submenu=Extract
X-KDE-Submenu[ar]=استخرِج
X-KDE-Submenu[ast]=Estrayer
X-KDE-Submenu[bg]=Извличане
X-KDE-Submenu[bs]=Raspakivanje
X-KDE-Submenu[ca]=Extreu
X-KDE-Submenu[ca@valencia]=Extrau
X-KDE-Submenu[cs]=Rozbalit
X-KDE-Submenu[da]=Pak ud
X-KDE-Submenu[de]=Entpacken
X-KDE-Submenu[el]=Εξαγωγή
X-KDE-Submenu[en_GB]=Extract
X-KDE-Submenu[es]=Extraer
X-KDE-Submenu[et]=Lahtipakkimine
X-KDE-Submenu[eu]=Atera
X-KDE-Submenu[fi]=Pura
X-KDE-Submenu[fr]=Extraire
X-KDE-Submenu[ga]=Bain amach
X-KDE-Submenu[gl]=Extraer
X-KDE-Submenu[hr]=Otpakiraj
X-KDE-Submenu[hu]=Kibontás
X-KDE-Submenu[ia]=Extrahe
X-KDE-Submenu[it]=Estrai
X-KDE-Submenu[ja]=展開
X-KDE-Submenu[kk]=Тарқату
X-KDE-Submenu[km]=ស្រង់ចេញ​
X-KDE-Submenu[ko]=압축 풀기
X-KDE-Submenu[lt]=Išpakuoti
X-KDE-Submenu[mr]=पुर्ववत करा
X-KDE-Submenu[nb]=Pakk ut
X-KDE-Submenu[nds]=Utpacken
X-KDE-Submenu[nl]=Uitpakken
X-KDE-Submenu[pa]=ਇੱਥੇ ਖਿਲਾਰੋ
X-KDE-Submenu[pl]=Wypakuj
X-KDE-Submenu[pt]=Extrair
X-KDE-Submenu[pt_BR]=Extrair
X-KDE-Submenu[ro]=Extrage
X-KDE-Submenu[ru]=Распаковать
X-KDE-Submenu[sk]=Rozbaliť
X-KDE-Submenu[sl]=Razširi
X-KDE-Submenu[sr]=Распакуј
X-KDE-Submenu[sr@ijekavian]=Распакуј
X-KDE-Submenu[sr@ijekavianlatin]=Raspakuj
X-KDE-Submenu[sr@latin]=Raspakuj
X-KDE-Submenu[sv]=Packa upp
X-KDE-Submenu[tr]=Çıkart
X-KDE-Submenu[ug]=ئايرىش
X-KDE-Submenu[uk]=Видобути
X-KDE-Submenu[x-test]=xxExtractxx
X-KDE-Submenu[zh_CN]=解压缩
X-KDE-Submenu[zh_TW]=解開

[Desktop Action arkExtractHere]
Name=Extract archive here
Icon=ark
Exec=ark --batch --autodestination %F

[Desktop Action arkExtractTo]
Name=Extract archive to...
Icon=ark
Exec=ark --batch --autodestination --dialog %F

[Desktop Action arkAutoExtractHere]
Name=Extract archive here, autodetect subfolder
Icon=ark
Exec=ark --batch --autodestination --autosubfolder %F
