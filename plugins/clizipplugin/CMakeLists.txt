########### next target ###############

set(SUPPORTED_CLIZIP_MIMETYPES "application/x-java-archive;application/zip;")

set(kerfuffle_clizip_SRCS cliplugin.cpp)

add_library(kerfuffle_clizip MODULE ${kerfuffle_clizip_SRCS})

target_link_libraries(kerfuffle_clizip kerfuffle )

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_clizip.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clizip.desktop
)

########### install files ###############

install(TARGETS kerfuffle_clizip  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clizip.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIZIP_MIMETYPES}" PARENT_SCOPE)
