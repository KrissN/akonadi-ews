#
#  Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>
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

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

set(EWSCLIENT_SRCS
  ewsattachment.cpp
  ewsattendee.cpp
  ewsclient.cpp
  ewscreatefolderrequest.cpp
  ewscreateitemrequest.cpp
  ewsdeletefolderrequest.cpp
  ewsdeleteitemrequest.cpp
  ewseffectiverights.cpp
  ewseventrequestbase.cpp
  ewsfindfolderrequest.cpp
  ewsfinditemrequest.cpp
  ewsfolder.cpp
  ewsfoldershape.cpp
  ewsgeteventsrequest.cpp
  ewsgetstreamingeventsrequest.cpp
  ewsgetfolderrequest.cpp
  ewsgetitemrequest.cpp
  ewsid.cpp
  ewsitem.cpp
  ewsitembase.cpp
  ewsitemshape.cpp
  ewsjob.cpp
  ewsmailbox.cpp
  ewsmovefolderrequest.cpp
  ewsmoveitemrequest.cpp
  ewsoccurrence.cpp
  ewspoxautodiscoverrequest.cpp
  ewspropertyfield.cpp
  ewsrecurrence.cpp
  ewsrequest.cpp
  ewsserverversion.cpp
  ewssubscriberequest.cpp
  ewssyncfolderhierarchyrequest.cpp
  ewssyncfolderitemsrequest.cpp
  ewstypes.cpp
  ewsunsubscriberequest.cpp
  ewsupdatefolderrequest.cpp
  ewsupdateitemrequest.cpp
  ewsxml.cpp
  ewsclient_debug.cpp)

ecm_qt_declare_logging_category(EWSCLIENT_SRCS
  HEADER ewscli_debug.h
  IDENTIFIER EWSCLI_LOG
  CATEGORY_NAME org.kde.pim.ews.client)
ecm_qt_declare_logging_category(EWSCLIENT_SRCS
  HEADER ewscli_proto_debug.h
  IDENTIFIER EWSCLI_PROTO_LOG
  CATEGORY_NAME org.kde.pim.ews.client.proto
  DEFAULT_SEVERITY Warning)
ecm_qt_declare_logging_category(EWSCLIENT_SRCS
  HEADER ewscli_req_debug.h
  IDENTIFIER EWSCLI_REQUEST_LOG
  CATEGORY_NAME org.kde.pim.ews.client.request
  DEFAULT_SEVERITY Warning)
ecm_qt_declare_logging_category(EWSCLIENT_SRCS
  HEADER ewscli_failedreq_debug.h
  IDENTIFIER EWSCLI_FAILEDREQUEST_LOG
  CATEGORY_NAME org.kde.pim.ews.client.failedrequest
  DEFAULT_SEVERITY Warning)

add_library(ewsclient STATIC ${EWSCLIENT_SRCS})
target_link_libraries(ewsclient 
  Qt5::Network
  KF5::KIOCore
  KF5::KIOFileWidgets
  KF5::KIOWidgets
  KF5::KIONTLM
  KF5::Codecs
  KF5::Mime
  KF5::CalendarCore)
