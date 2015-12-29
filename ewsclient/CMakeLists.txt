
set(EWSCLIENT_SRCS
  ewsclient.cpp
  ewsfindfolderrequest.cpp
  ewsfinditemrequest.cpp
  ewsfolder.cpp
  ewsfoldershape.cpp
  ewsgetfolderrequest.cpp
  ewsid.cpp
  ewsitem.cpp
  ewsitembase.cpp
  ewsitemshape.cpp
  ewsjobqueue.cpp
  ewspropertyfield.cpp
  ewsrequest.cpp
  ewsrequestwriter.cpp
  ewstypes.cpp
  ewsclient_debug.cpp)

add_library(ewsclient STATIC ${EWSCLIENT_SRCS})
target_link_libraries(ewsclient Qt5::Network KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM KF5::Codecs)
