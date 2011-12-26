macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "qjson" "A library for processing and serializing JSON files" "http://qjson.sourceforge.net" FALSE "" "Required for compiling Ark's unit tests")

########### next target ###############

set(kerfuffle_SRCS
    archive.cpp
    archiveinterface.cpp
    jobs.cpp
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

if (QJSON_FOUND)
  # This is a hack to make QJSON work with both 0.7.1 (the latest stable)
  # and the current master (b440550), which uses a different casing for
  # the CMake variables.
  # It should be removed when QJSON master becomes sane and reverts the
  # casing again.
  if (QJSON_LIBRARIES AND QJSON_INCLUDE_DIR)
    set(KERFUFFLE_QJSON_LIBRARIES "${QJSON_LIBRARIES}")
    set(KERFUFFLE_QJSON_INCLUDE_DIR "${QJSON_INCLUDE_DIR}")
  else (QJSON_LIBRARIES AND QJSON_INCLUDE_DIR)
    set(KERFUFFLE_QJSON_LIBRARIES "${qjson_LIBRARIES}")
    set(KERFUFFLE_QJSON_INCLUDE_DIR "${qjson_INCLUDE_DIR}")
  endif (QJSON_LIBRARIES AND QJSON_INCLUDE_DIR)

  add_subdirectory(tests)
endif (QJSON_FOUND)
