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

set(EWSCLIENT_SRCS
  ewsattendee.cpp
  ewsclient.cpp
  ewseffectiverights.cpp
  ewsfindfolderrequest.cpp
  ewsfinditemrequest.cpp
  ewsfolder.cpp
  ewsfoldershape.cpp
  ewsgetfolderrequest.cpp
  ewsgetitemrequest.cpp
  ewsid.cpp
  ewsitem.cpp
  ewsitembase.cpp
  ewsitemshape.cpp
  ewsjob.cpp
  ewsmailbox.cpp
  ewsmoveitemrequest.cpp
  ewsoccurrence.cpp
  ewspropertyfield.cpp
  ewsrecurrence.cpp
  ewsrequest.cpp
  ewsserverversion.cpp
  ewssubscriberequest.cpp
  ewstypes.cpp
  ewsunsubscriberequest.cpp
  ewsupdateitemrequest.cpp
  ewsxmlreader.cpp
  ewsclient_debug.cpp)

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
