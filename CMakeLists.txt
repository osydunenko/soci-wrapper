cmake_minimum_required(VERSION 3.16)

project(soci_wrapper VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options decl.
option(SW_CPPTEST_COVERAGE "Cpp Test Coverage" OFF)
option(SW_BUILD_TESTS "Build tests" OFF)
option(SW_SQLITE "Build for SQLite" OFF)

set(SOCI_EMPTY OFF CACHE BOOL "SOCI Emtpy")
set(SOCI_TESTS OFF CACHE BOOL "SOCI Tests")
set(SOCI_STATIC OFF CACHE BOOL "SOCI static libraries")
set(SOCI_SHARED OFF CACHE BOOL "SOCI shared libraries")

# Check prereqs
find_package(Boost 1.74.0 REQUIRED COMPONENTS unit_test_framework)
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
find_program(DOXYGEN_PATH doxygen)
find_program(CLANGFORMAT_PATH clang-format)
find_package(SQLite3)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

if(SW_BUILD_TESTS)
    enable_testing()
    set(SOCI_TESTS ON)
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()

add_subdirectory(3rdParty/soci)

if (SOCI_STATIC AND SOCI_SHARED)
    message(FATAL_ERROR "Cannot have both STATIC and SHARED compilation mode activated simultaneously")
endif()

# Definitions decl.
target_compile_definitions(${PROJECT_NAME}
    INTERFACE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<AND:$<BOOL:${SQLite3_FOUND}>,$<BOOL:${SW_SQLITE}>>:SW_SQLITE>)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/soci/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/3rdParty/soci/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        $<$<AND:$<BOOL:${SOCI_STATIC}>,$<BOOL:${SW_SQLITE}>>:Soci::sqlite3_static>
        $<$<AND:$<BOOL:${SOCI_SHARED}>,$<BOOL:${SW_SQLITE}>>:Soci::sqlite3>
        $<$<BOOL:${SOCI_STATIC}>:Soci::core_static>
        $<$<BOOL:${SOCI_SHARED}>:Soci::core>)

# Decl. doc target
add_custom_target(${PROJECT_NAME}_doc
    COMMAND ${DOXYGEN_PATH} ${PROJECT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/soci-wrapper/
    COMMENT "Generating API Documentation..."
    VERBATIM)

add_custom_target(${PROJECT_NAME}_debug
    COMMAND ${CMAKE_COMMAND} -DSW_SQLITE=ON -DSW_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug ${PROJECT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}
    VERBATIM
)

# Exclude the doc & debug targets from all
set_target_properties(${PROJECT_NAME}_doc PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(${PROJECT_NAME}_debug PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Decl. coverage target
add_custom_target(${PROJECT_NAME}_coverage
    rm -f app.info
    COMMAND ${LCOV_PATH} --directory ${PROJECT_BINARY_DIR} --capture --output-file app.info
    COMMAND ${GENHTML_PATH} --output-directory ${PROJECT_SOURCE_DIR}/coverage app.info
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generating Coverage HTML report..."
    VERBATIM)

# Exclude the coverage target from all
set_target_properties(${PROJECT_NAME}_coverage PROPERTIES EXLCLUDE_FROM_ALL TRUE)
file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/include/*.hpp ${PROJECT_SOURCE_DIR}/tests/*.cpp)

# Decl. clang-format target
add_custom_target(${PROJECT_NAME}_clangformat
    COMMAND ${CLANGFORMAT_PATH} -i -style=WebKit ${SRCS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include
    COMMENT "Fomatting the code with ${CLANGFORMAT_PATH}"
    VERBATIM)

# Exclude the coverage target from all
set_target_properties(${PROJECT_NAME}_clangformat PROPERTIES EXLCLUDE_FROM_ALL TRUE)

# Installation
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)
