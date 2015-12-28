
set(EWSCLIENT_SRCS
  ewscalendarfolder.cpp
  ewsclient.cpp
  ewscontactsfolder.cpp
  ewsfindfolderrequest.cpp
  ewsfolderbase.cpp
  ewsfoldershape.cpp
  ewsgetfolderrequest.cpp
  ewsid.cpp
  ewsitemshape.cpp
  ewsjobqueue.cpp
  ewsmailfolder.cpp
  ewspropertyfield.cpp
  ewsrequest.cpp
  ewsrequestwriter.cpp
  ewssearchfolder.cpp
  ewstasksfolder.cpp
  ewstypes.cpp
  ewsclient_debug.cpp)

add_library(ewsclient STATIC ${EWSCLIENT_SRCS})
target_link_libraries(ewsclient Qt5::Network KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)
