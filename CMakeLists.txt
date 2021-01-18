cmake_minimum_required(VERSION 3.16)

project(soci_wrapper)

option(BUILD_EXAMPLES "Build examples" ON)

add_compile_definitions("$<$<CONFIG:Debug>:DEBUG=1>")
add_compile_definitions("SOCI_WRAPPER_SQLITE=1")

set(CMAKE_CXX_STANDARD 17)
option(SOCI_EMPTY OFF)
add_subdirectory(3rdParty/soci)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package(Boost REQUIRED)

include(GNUInstallDirs)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdParty/soci/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/3rdParty/soci/include>
        $<INSTALL_INTERFACE:Boost::headers>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    INTERFACE
        soci_core
        soci_sqlite3
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

if(BUILD_EXAMPLES)
    add_subdirectory("${PROJECT_SOURCE_DIR}/examples/simple_db")
endif()

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION include)
