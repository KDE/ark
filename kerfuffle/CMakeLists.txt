ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE)

#kde4_add_kcfg_files(libark_common_SRCS ../settings.kcfgc)

########### next target ###############

set(kerfuffle_SRCS
    arch.cpp 
    archiveinterface.cpp
    jobs.cpp
    archivebase.cpp
   )

kde4_add_library(kerfuffle SHARED ${kerfuffle_SRCS})

target_link_libraries(kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_THREADWEAVER_LIBRARIES} ${LIBARCHIVE_LIBRARY} ${KDE4_KPARTS_LIBS})

set_target_properties(kerfuffle PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kerfuffle DESTINATION ${LIB_INSTALL_DIR})

install(FILES kerfufflePlugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
