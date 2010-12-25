
include_directories( ${LIBARCHIVE_INCLUDE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )


########### next target ###############
set(LIBARCHIVE_RPM_SUPPORT_MIN_VERSION "2.8.0")
if(LIBARCHIVE_VERSION VERSION_LESS ${LIBARCHIVE_RPM_SUPPORT_MIN_VERSION})
  set(LIBARCHIVE_RPM_SUPPORT "")
else(LIBARCHIVE_VERSION VERSION_LESS ${LIBARCHIVE_RPM_SUPPORT_MIN_VERSION})
  set(LIBARCHIVE_RPM_SUPPORT "application/x-rpm;")
endif(LIBARCHIVE_VERSION VERSION_LESS ${LIBARCHIVE_RPM_SUPPORT_MIN_VERSION})

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_libarchive_readonly.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop
)

set(kerfuffle_libarchive_SRCS libarchivehandler.cpp)

kde4_add_plugin(kerfuffle_libarchive ${kerfuffle_libarchive_SRCS})

target_link_libraries(kerfuffle_libarchive  ${KDE4_KIO_LIBS} ${KDE4_KDECORE_LIBS} ${LIBARCHIVE_LIBRARY} kerfuffle )

install(TARGETS kerfuffle_libarchive  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_libarchive.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_libarchive_readonly.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
