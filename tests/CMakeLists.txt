include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

# Add test coverage if requested and possible
if (${CMAKE_CXX_COMPILER_ID} MATCHES GNU AND ${SW_CPPTEST_COVERAGE})
    if (NOT GCOV_PATH)
        message(FATAL_ERROR "gcov not found. aborting...")
    endif()

    set(COVERAGE_OPTIONS "--coverage")
    set(COVERAGE_LINK_OPTIONS "--coverage")
endif()

foreach(tst_src ${TST_SRCS})
    get_filename_component(tst_name ${tst_src} NAME_WE)

    message(STATUS "Adding test 'tst_${tst_name}'")

    add_executable(tst_${tst_name} ${tst_src})

    target_compile_options(
        tst_${tst_name}
        PRIVATE
            "${COVERAGE_OPTIONS}"
    )

    target_link_options(
        tst_${tst_name}
        PRIVATE
            "${COVERAGE_LINK_OPTIONS}"
    )

    target_link_libraries(
        tst_${tst_name}
        PRIVATE
            soci_wrapper::soci_wrapper
            Boost::unit_test_framework
    )

    set_target_properties(
        tst_${tst_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/
    )

    add_test(
        NAME tst_${tst_name}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin/
        COMMAND ${PROJECT_BINARY_DIR}/bin/tst_${tst_name}
    )
endforeach()

