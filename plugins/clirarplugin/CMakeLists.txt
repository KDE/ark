########### next target ###############

set(SUPPORTED_CLIRAR_MIMETYPES "application/x-rar;")
set(ENCRYPT_ENTRIES_MIMETYPES "application/x-rar")
set(ENCRYPT_HEADER_MIMETYPES "application/x-rar")

set(kerfuffle_clirar_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_clirar_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK
                                CATEGORY_NAME ark.clirar)

add_library(kerfuffle_clirar MODULE ${kerfuffle_clirar_SRCS})

target_link_libraries(kerfuffle_clirar kerfuffle)

configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/kerfuffle_clirar.desktop.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clirar.desktop
)

########### install files ###############

install(TARGETS kerfuffle_clirar  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kerfuffle_clirar.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

if(Qt5Test_FOUND)
    add_subdirectory(autotests)
endif()

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}${SUPPORTED_CLIRAR_MIMETYPES}" PARENT_SCOPE)
