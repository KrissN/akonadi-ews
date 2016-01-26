#  This file is part of Akonadi EWS Resource
#  Copyright (C) 2015-2016 Krzysztof Nowicki <krissn@op.pl>
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public License
#  along with this library; see the file COPYING.LIB.  If not, write to
#  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301, USA.

cmake_minimum_required(VERSION 3.1.0)

project(akonadi-ews)

enable_testing()

find_package(ECM 5.16.0 NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

set(REQUIRED_QT_VERSION 5.4.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)
include(FeatureSummary)

find_package(KF5KIO)
find_package(KF5Akonadi)
find_package(KF5AkonadiMime)
find_package(Xsltproc)
find_package(KF5Mime)
find_package(KF5CalendarCore)
find_package(KF5Contacts)
find_package(KF5Codecs)
find_package(KF5KDELibs4Support)

add_subdirectory(ewsclient)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ewsclient)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_ews_resource\")

set(ewsresource_SRCS
    configdialog.cpp
    ewsfetchitemsjob.cpp
    ewsfetchitemdetailjob.cpp
    ewsfetchmaildetailjob.cpp
    ewsfetchcalendardetailjob.cpp
    ewsresource.cpp
    ewssubscriptionmanager.cpp)

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
    KF5::AkonadiMime
    KF5::KIOCore
    KF5::Mime
    KF5::CalendarCore
    KF5::Contacts
    KF5::KDELibs4Support
    ewsclient)

add_subdirectory(test)

