cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(soci_wrapper)

# Options decl.
option(SOCI_EMPTY OFF)
option(SW_CPPTEST_COVERAGE "Cpp Test Coverage" OFF)
option(SW_BUILD_TESTS "Build tests" OFF)
option(SW_SQLITE3 "Link against sqlite3" ON)

set(SOCI_TESTS OFF)

# Definitions decl.
add_compile_definitions($<$<CONFIG:Debug>:DEBUG=1>)
add_compile_definitions($<$<BOOL:${SW_SQLITE3}>:SOCI_WRAPPER_SQLITE=1>)

# Check prereqs
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
find_program(DOXYGEN_PATH doxygen)
find_program(CLANGFORMAT_PATH clang-format)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

add_subdirectory(3rdParty/soci)

if(SW_BUILD_TESTS)
    message(STATUS "Building tests")
    enable_testing()
    set(SOCI_TESTS ON)
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()

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
        $<BUILD_INTERFACE:soci_core>
        $<BUILD_INTERFACE:$<$<BOOL:${SW_SQLITE3}>:soci_sqlite3>>
)

# Decl. doc target
add_custom_target(
    doc
    COMMAND ${DOXYGEN_PATH} ${PROJECT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/soci-wrapper/
    COMMENT "Generating API Documentation"
    VERBATIM
)

# Exclude the doc target from all
set_target_properties(doc PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Decl. coverage target
add_custom_target(
    coverage
    rm -f app.info
    COMMAND ${LCOV_PATH} --directory ${PROJECT_BINARY_DIR} --capture --output-file app.info
    COMMAND ${GENHTML_PATH} --output-directory ${PROJECT_SOURCE_DIR}/coverage app.info
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generating Coverage HTML report"
    VERBATIM
)

# Exclude the coverage target from all
set_target_properties(coverage PROPERTIES EXLCLUDE_FROM_ALL TRUE)
file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/include/*.hpp)

# Decl. clang-format target
add_custom_target(
    ${PROJECT_NAME}_clangformat
    COMMAND ${CLANGFORMAT_PATH} -i -style=WebKit ${SRCS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include
    COMMENT "Fomatting the code with ${CLANGFORMAT_PATH}"
    VERBATIM
)

# Exclude the coverage target from all
set_target_properties(${PROJECT_NAME}_clangformat PROPERTIES EXLCLUDE_FROM_ALL TRUE)

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
message(STATUS "SW_CPPTEST_COVERAGE:.........${SW_CPPTEST_COVERAGE}")
message(STATUS "CMAKE_BUILD_TYPE:............${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_VERSION:...............${CMAKE_VERSION}")
message(STATUS "CMAKE_C_COMPILER:............${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:..........${CMAKE_CXX_COMPILER}")
message(STATUS "GCOV_PATH:...................${GCOV_PATH}")
message(STATUS "LCOV_PATH:...................${LCOV_PATH}")
message(STATUS "GENHTML_PATH:................${GENHTML_PATH}")
message(STATUS "DOXYGEN_PATH:................${DOXYGEN_PATH}")
message(STATUS "-------------------------------------------------------")
