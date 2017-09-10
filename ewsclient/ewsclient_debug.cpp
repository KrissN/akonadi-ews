/*
    Copyright (C) 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "ewsclient_debug.h"

QTemporaryDir ewsLogDir("/tmp/akonadi-ews-XXXXXXX");

Q_LOGGING_CATEGORY(EWSRES_LOG, "log_ews_resource")

Q_LOGGING_CATEGORY(EWSRES_PROTO_LOG, "log_ews_resource_proto", QtWarningMsg)

Q_LOGGING_CATEGORY(EWSRES_REQUEST_LOG, "log_ews_resource_request", QtWarningMsg)

Q_LOGGING_CATEGORY(EWSRES_FAILEDREQUEST_LOG, "log_ews_resource_failedrequest", QtWarningMsg)
