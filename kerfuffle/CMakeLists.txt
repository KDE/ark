include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR}/..
                   )

ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE)

#kde4_add_kcfg_files(libark_common_SRCS ../settings.kcfgc)

########### next target ###############

set(kerfuffle_SRCS
	#arch.cpp 
    archive.cpp
    archiveinterface.cpp
    jobs.cpp
	threading.cpp
    archivebase.cpp
	extractiondialog.cpp
	adddialog.cpp
	queries.cpp
	addtoarchive.cpp
	batchextract.cpp
	cliinterface.cpp
   )

kde4_add_kcfg_files(kerfuffle_SRCS settings.kcfgc)

kde4_add_ui_files(kerfuffle_SRCS extractiondialog.ui adddialog.ui )

kde4_add_library(kerfuffle SHARED ${kerfuffle_SRCS})

target_link_libraries(kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS})

if (UNIX)
   target_link_libraries(kerfuffle ${KDE4_KPTY_LIBRARY})
endif (UNIX)

set_target_properties(kerfuffle PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kerfuffle ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES kerfufflePlugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES ark.kcfg                DESTINATION ${KCFG_INSTALL_DIR})
