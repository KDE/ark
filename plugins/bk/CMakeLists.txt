
include_directories(
                     ${CMAKE_CURRENT_SOURCE_DIR}/../..
                     ${CMAKE_CURRENT_BINARY_DIR}/../..
                   )

########### next target ###############

set(kerfuffle_bk_SRCS
    bkplugin.cpp
    bkAdd.c
    bkCache.c
    bkDelete.c
    bkError.c
    bkExtract.c
    bkGet.c
    bkLink.c
    bkMangle.c
    bkMisc.c
    bkPath.c
    bkRead7x.c
    bkRead.c
    bkSet.c
    bkSort.c
    bkTime.c
    bkWrite7x.c
    bkWrite.c
    )

kde4_add_plugin(kerfuffle_bk ${kerfuffle_bk_SRCS})

target_link_libraries(kerfuffle_bk  ${KDE4_KDECORE_LIBS} kerfuffle )

install(TARGETS kerfuffle_bk  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kerfuffle_bk.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

