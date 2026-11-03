cmake_minimum_required(VERSION 3.16)
project(cpp_embedded_practice LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

function(enable_project_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endfunction()

add_executable(foundation_practice
    src/foundation_practice.cpp
)
enable_project_warnings(foundation_practice)
target_link_libraries(foundation_practice PRIVATE Threads::Threads)

add_executable(gateway_resilience
    src/gateway_resilience.cpp
)
enable_project_warnings(gateway_resilience)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(linux_timer_epoll
        src/linux_timer_epoll.cpp
    )
    enable_project_warnings(linux_timer_epoll)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME foundation_practice COMMAND foundation_practice)
    add_test(NAME gateway_resilience COMMAND gateway_resilience)
endif()
