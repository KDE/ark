include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR}/..
                   )

macro_optional_find_package(qjson)
macro_log_feature(qjson_FOUND "qjson" "A library for processing and serializing JSON files" "http://qjson.sourceforge.net" FALSE "" "Required for compiling Ark's unit tests")

########### next target ###############

set(kerfuffle_SRCS
    archive.cpp
    archiveinterface.cpp
    jobs.cpp
    archivebase.cpp
	extractiondialog.cpp
	adddialog.cpp
	queries.cpp
	addtoarchive.cpp
	cliinterface.cpp
   )

kde4_add_kcfg_files(kerfuffle_SRCS settings.kcfgc)

kde4_add_ui_files(kerfuffle_SRCS extractiondialog.ui adddialog.ui )

kde4_add_library(kerfuffle SHARED ${kerfuffle_SRCS})

target_link_libraries(kerfuffle ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS})
if (NOT WIN32)
  target_link_libraries(kerfuffle ${KDE4_KPTY_LIBS})
endif (NOT WIN32)

set_target_properties(kerfuffle PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kerfuffle ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES kerfufflePlugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES ark.kcfg                DESTINATION ${KCFG_INSTALL_DIR})

if (qjson_FOUND)
  add_subdirectory(tests)
endif (qjson_FOUND)
