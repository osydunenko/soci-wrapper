cmake_minimum_required(VERSION 3.16)

project(soci_wrapper)

option(SW_BUILD_TESTS "Build tests" OFF)

add_compile_definitions("$<$<CONFIG:Debug>:DEBUG=1>")
add_compile_definitions("SOCI_WRAPPER_SQLITE=1")

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

option(SOCI_TESTS OFF)
if(SW_BUILD_TESTS)
    message(STATUS "Building tests")
    enable_testing()
    set(SOCI_TESTS ON)
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
option(SOCI_EMPTY OFF)
add_subdirectory(3rdParty/soci)

add_subdirectory(3rdParty/range-v3)

include(GNUInstallDirs)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdParty/soci/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/3rdParty/soci/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/3rdParty/range-v3/include>
        $<INSTALL_INTERFACE:Boost::headers>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    INTERFACE
        soci_core
        soci_sqlite3
        range-v3
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION include)

message(STATUS "-------------------------------------------------------")
message(STATUS "SW_BUILD_TESTS:..............${SW_BUILD_TESTS}")
message(STATUS "CMAKE_VERSION:...............${CMAKE_VERSION}")
message(STATUS "CMAKE_C_COMPILER:............${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:..........${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_BUILD_TYPE:............${CMAKE_BUILD_TYPE}")
message(STATUS "-------------------------------------------------------")
