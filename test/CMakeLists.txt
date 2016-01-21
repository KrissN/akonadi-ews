include_directories(../)

add_executable(ewstest ewstest.cpp)

target_link_libraries(ewstest Qt5::Core ewsclient)

add_subdirectory(unittests)
