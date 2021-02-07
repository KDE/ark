########### install files ###############
#
kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en SUBDIR ark)
kdoctools_create_manpage(man-ark.1.docbook 1 INSTALL_DESTINATION ${KDE_INSTALL_MANDIR})

