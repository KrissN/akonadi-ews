include_directories(../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fakeserver)

add_library(isolatedtestcommon STATIC isolatedtestbase.cpp statemonitor.cpp)
 target_link_libraries(isolatedtestcommon
   KF5::AkonadiCore
   Qt5::Core
 )

macro(add_akonadi_isolated_test _source)
  get_filename_component(_targetName ${_source} NAME_WE)
  set(_srcList ${_source})
  qt5_add_resources(_srcList ${_targetName}.qrc)

  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../ewsresource.kcfg org.kde.Akonadi.Ews.Settings)
  set(ewssettingsinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Settings.xml)
  qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/../settings.h org.kde.Akonadi.Ews.Wallet.xml OPTIONS -a )
  set(ewswalletinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Wallet.xml)

  qt5_add_dbus_interface(_srcList ${ewssettingsinterface_xml} ewssettings)
  qt5_add_dbus_interface(_srcList ${ewswalletinterface_xml} ewswallet)

  add_executable(${_targetName} ${_srcList})
  target_link_libraries(${_targetName}
    Qt5::Test
    KF5::AkonadiCore
    KF5::Mime
    Qt5::Network
    Qt5::Widgets
    Qt5::DBus
    fakeewsserver
    isolatedtestcommon
  )

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_targetName} LOCATION )
    set(_executable ${_loc}.bat)
  else()
    set(_executable ${CMAKE_CURRENT_BINARY_DIR}/${_targetName})
  endif()
  if (UNIX)
    if (APPLE)
      set(_executable ${_executable}.app/Contents/MacOS/${_targetName})
    else()
      set(_executable ${_executable})
    endif()
  endif()

  find_program(_testrunner akonaditest)

  add_test( ${_targetName} ${_testrunner} -c ${CMAKE_CURRENT_BINARY_DIR}/config.xml ${_executable} )
endmacro(add_akonadi_isolated_test)


add_executable(ewstest ewstest.cpp)

target_link_libraries(ewstest Qt5::Core ewsclient)

add_subdirectory(unittests)
add_subdirectory(fakeserver)

file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/xdgdata EWS_TMP_XDG_DATA_DIR)
file(TO_NATIVE_PATH ${EWS_TMP_XDG_DATA_DIR}/akonadi/agents EWS_TMP_AGENTS_DIR)
file(MAKE_DIRECTORY ${EWS_TMP_XDG_DATA_DIR})
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/../ewsresource.desktop DESTINATION ${EWS_TMP_AGENTS_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testenv/config.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.xml)

add_akonadi_isolated_test(basictest.cpp)

