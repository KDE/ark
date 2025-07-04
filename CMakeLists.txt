cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(ark VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION 6.6.0)
set(KF_MIN_VERSION 6.3.0)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(GenerateExportHeader)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(ECMDeprecationSettings)
include(KDEClangFormat)
include(KDEGitCommitHooks)

ecm_setup_version(${RELEASE_SERVICE_VERSION}
                  VARIABLE_PREFIX ARK
                  VERSION_HEADER "ark_version.h")

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX KERFUFFLE)

option(BUILD_DOC "Whether to build the documentation" ON)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED
    Concurrent
    Core
    Gui
    Widgets)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Config
                                                        Crash
                                                        DBusAddons
                                                        FileMetaData
                                                        I18n
                                                        KIO
                                                        Service
                                                        Parts
                                                        WidgetsAddons
                                                        WindowSystem
                                                        IconThemes)
if (BUILD_DOC)
    find_package(KF6DocTools ${KF_MIN_VERSION})
    set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
        "Tools to generate documentation"
        TYPE REQUIRED
    )
endif()

if(NOT WIN32)
    find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Pty)
endif()

find_package(Qt6Test ${QT_MIN_VERSION} CONFIG QUIET)
set_package_properties(Qt6Test PROPERTIES
    PURPOSE "Required for tests"
    TYPE OPTIONAL)

if(NOT TARGET Qt6::Test)
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
# To remove once we can depend on LibZip 1.6+
if("${LibZip_VERSION}" VERSION_GREATER_EQUAL 1.6)
    set(LIBZIP_CANCELATION 1)
endif()
configure_file(config.h.in config.h)

option(WITH_TEST_COVERAGE "Build with test coverage support" OFF)
if(WITH_TEST_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

set(SUPPORTED_ARK_MIMETYPES "")

add_definitions(-DTRANSLATION_DOMAIN="ark")

ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 6.14.0
)


add_subdirectory(plugins)
add_subdirectory(kerfuffle)
add_subdirectory(part)
add_subdirectory(app)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

if(BUILD_DOC)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()
ki18n_install(po)

ecm_qt_install_logging_categories(EXPORT ARK FILE ark.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
