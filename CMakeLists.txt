cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "21")
set (RELEASE_SERVICE_VERSION_MINOR "04")
set (RELEASE_SERVICE_VERSION_MICRO "2")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(ark VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION 5.12.0)
set(KF5_MIN_VERSION 5.77.0)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(GenerateExportHeader)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)

add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

ecm_setup_version(${RELEASE_SERVICE_VERSION}
                  VARIABLE_PREFIX ARK
                  VERSION_HEADER "ark_version.h")

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX KERFUFFLE)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
    Concurrent
    Core
    Gui
    Widgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Archive
                                                        Config
                                                        Crash
                                                        DBusAddons
                                                        DocTools
                                                        I18n
                                                        ItemModels
                                                        KIO
                                                        Service
                                                        Parts
                                                        Pty
                                                        WidgetsAddons)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG QUIET)
set_package_properties(Qt5Test PROPERTIES
    PURPOSE "Required for tests"
    TYPE OPTIONAL)

if(NOT Qt5Test_FOUND)
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
endif()

find_package(LibArchive 3.3.3 REQUIRED)
set_package_properties(LibArchive PROPERTIES
                       URL "https://www.libarchive.org/"
                       DESCRIPTION "A library for dealing with a wide variety of archive file formats"
                       PURPOSE "Required for among others tar, tar.gz, tar.bz2 formats in Ark.")

find_package(LibZip 1.3.0)
set_package_properties(LibZip PROPERTIES
                       URL "https://nih.at/libzip/"
                       DESCRIPTION "A library for handling zip archives"
                       PURPOSE "Optional for zip archives.")

option(WITH_TEST_COVERAGE "Build with test coverage support" OFF)
if (WITH_TEST_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif (WITH_TEST_COVERAGE)

set(SUPPORTED_ARK_MIMETYPES "")

add_definitions(-DTRANSLATION_DOMAIN="ark")
add_definitions(-DQT_NO_FOREACH)
add_definitions(-DQT_NO_KEYWORDS)

add_subdirectory(plugins)
add_subdirectory(kerfuffle)
add_subdirectory(part)
add_subdirectory(app)
add_subdirectory(doc)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

ki18n_install(po)
kdoctools_install(po)

ecm_qt_install_logging_categories(EXPORT ARK FILE ark.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
