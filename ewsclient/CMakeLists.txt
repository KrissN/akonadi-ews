
set(EWSCLIENT_SRCS
  ewsclient.cpp
  ewsgetfolderrequest.cpp
  ewsjobqueue.cpp
  ewsrequest.cpp
  ewsrequestwriter.cpp
  ewsxmlitems.cpp
  ewsclient_debug.cpp)

add_library(ewsclient STATIC ${EWSCLIENT_SRCS})
target_link_libraries(ewsclient Qt5::Network KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)
