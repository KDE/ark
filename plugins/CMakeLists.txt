function(kerfuffle_add_plugin plugin)
    kcoreaddons_add_plugin(${plugin}
        SOURCES ${ARGN}
        INSTALL_NAMESPACE kerfuffle)
    target_link_libraries(${plugin} kerfuffle)
endfunction()

add_subdirectory( libarchive )
add_subdirectory( clirarplugin )
add_subdirectory( cli7zplugin )
add_subdirectory( clizipplugin )
add_subdirectory(cliunarchiverplugin)
if(LibZip_FOUND)
  add_subdirectory(libzipplugin)
endif(LibZip_FOUND)
add_subdirectory( cliarjplugin )

set(SUPPORTED_ARK_MIMETYPES "${SUPPORTED_ARK_MIMETYPES}" PARENT_SCOPE)
set(INSTALLED_KERFUFFLE_PLUGINS "${INSTALLED_KERFUFFLE_PLUGINS}" PARENT_SCOPE)
