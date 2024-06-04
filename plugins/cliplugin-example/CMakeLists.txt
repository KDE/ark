set(kerfuffle_cli_SRCS cliplugin.cpp)

ecm_qt_declare_logging_category(kerfuffle_cli_SRCS
                                HEADER ark_debug.h
                                IDENTIFIER ARK_LOG
                                CATEGORY_NAME ark.cliexample)

kerfuffle_add_plugin(kerfuffle_cli ${kerfuffle_cli_SRCS})
