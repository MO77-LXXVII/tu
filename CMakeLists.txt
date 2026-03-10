cmake_minimum_required(VERSION 3.20)

project(bank_app)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(platform_src platform/windows.cpp)
else()
    set(platform_src platform/linux.cpp)
endif()

add_executable(bank_app
    main.cpp
    ${platform_src}
)

target_include_directories(bank_app PRIVATE .)

set_target_properties(bank_app PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

configure_file(
    ${CMAKE_SOURCE_DIR}/tu/config.hpp.in
    ${CMAKE_SOURCE_DIR}/tu/config.hpp
)