function(kerfuffle_add_plugin plugin)
    kcoreaddons_add_plugin(${plugin}
        SOURCES ${ARGN}
        INSTALL_NAMESPACE kerfuffle)
    target_link_libraries(${plugin} kerfuffle)
endfunction()

# Utility function that parses the 'MimeTypes' JSON array defined inside the plugin JSON metadata.
# Inputs:
# - PLUGIN_JSON_FILEPATH: the absolute path of the plugin JSON metadata file
# Outputs:
# - PLUGIN_SUPPORTED_MIMETYPES is a list with all the mimetypes parsed from the JSON file.
function(kerfuffle_parse_json_mimetypes PLUGIN_JSON_FILEPATH)
    file(READ ${PLUGIN_JSON_FILEPATH} PLUGIN_JSON_METADATA)
    string(JSON KPLUGIN_JSON_OBJECT GET ${PLUGIN_JSON_METADATA} KPlugin)
    string(JSON MIMETYPES_JSON_ARRAY GET ${KPLUGIN_JSON_OBJECT} MimeTypes)
    string(JSON MIMETYPES_COUNT LENGTH ${MIMETYPES_JSON_ARRAY})
    math(EXPR MIMETYPES_RANGE "${MIMETYPES_COUNT} - 1")
    foreach(INDEX RANGE ${MIMETYPES_RANGE})
        string(JSON CURRENT_MIMETYPE GET ${MIMETYPES_JSON_ARRAY} ${INDEX})
        list(APPEND PARSED_MIMETYPES ${CURRENT_MIMETYPE})
    endforeach()
    # NOTE: need to manually add trailing semicolon because cmake doesn't do it.
    set(PLUGIN_SUPPORTED_MIMETYPES "${PARSED_MIMETYPES};" PARENT_SCOPE)
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
