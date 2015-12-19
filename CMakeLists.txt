cmake_minimum_required(VERSION 3.1.0)

project(akonadi-ews)

find_package(ECM 5.16.0 NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

set(REQUIRED_QT_VERSION 5.4.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)
include(FeatureSummary)

find_package(KF5KIO)
find_package(KF5Akonadi)
find_package(Xsltproc)
find_package(KF5Mime)
find_package(KF5CalendarCore)
find_package(KF5Contacts)

add_subdirectory(ewsclient)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ewsclient)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_ews_resource\")

set(ewsresource_SRCS
    configdialog.cpp
    ewsresource.cpp)

ki18n_wrap_ui(ewsresource_SRCS configdialog.ui)

kconfig_add_kcfg_files(ewsresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ewsresource.kcfg org.kde.Akonadi.EWS.Settings)
qt5_add_dbus_adaptor(ewsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.EWS.Settings.xml settings.h Settings
)

add_executable(akonadi_ews_resource ${ewsresource_SRCS})

target_link_libraries(akonadi_ews_resource
    KF5::AkonadiAgentBase
    KF5::AkonadiCore
    KF5::KIOCore
    KF5::Mime
    KF5::CalendarCore
    KF5::Contacts
    ewsclient)

add_subdirectory(test)

