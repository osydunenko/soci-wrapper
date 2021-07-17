cmake_minimum_required(VERSION 3.16)

project(soci_wrapper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options decl.
option(SW_BUILD_TESTS "Build tests" OFF)
option(CPPTEST_COVERAGE "Cpp Test Coverage" OFF)

# Definitions decl.
add_compile_definitions("$<$<CONFIG:Debug>:DEBUG=1>")
add_compile_definitions("SOCI_WRAPPER_SQLITE=1")

# Check prereqs
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
find_program(DOXYGEN_PATH doxygen)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

option(SOCI_EMPTY OFF)
add_subdirectory(3rdParty/soci)

if(SW_BUILD_TESTS)
    message(STATUS "Building tests")
    enable_testing()
    set(SOCI_TESTS ON)
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
else()
    option(SOCI_TESTS OFF)
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
        soci_core
        soci_sqlite3
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
message(STATUS "CPPTEST_COVERAGE:............${CPPTEST_COVERAGE}")
message(STATUS "CMAKE_BUILD_TYPE:............${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_VERSION:...............${CMAKE_VERSION}")
message(STATUS "CMAKE_C_COMPILER:............${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:..........${CMAKE_CXX_COMPILER}")
message(STATUS "GCOV_PATH:...................${GCOV_PATH}")
message(STATUS "LCOV_PATH:...................${LCOV_PATH}")
message(STATUS "GENHTML_PATH:................${GENHTML_PATH}")
message(STATUS "DOXYGEN_PATH:................${DOXYGEN_PATH}")
message(STATUS "-------------------------------------------------------")
