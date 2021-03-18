include(CTest)

file(GLOB TST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

foreach(tst_src ${TST_SRCS})
    get_filename_component(tst_name ${tst_src} NAME_WE)

    message(STATUS "Adding test 'tst_${tst_name}'")

    add_executable(tst_${tst_name} ${tst_src})

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

