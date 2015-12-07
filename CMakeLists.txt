cmake_minimum_required(VERSION 3.1.0)

project(akonadi-ews CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Network)
find_package(KF5KIO)

add_subdirectory(ewsclient)
add_subdirectory(test)

