set(OUTPUT_NAME simple_db)

add_executable(${OUTPUT_NAME} main.cpp)

target_include_directories(
    ${OUTPUT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(
    ${OUTPUT_NAME}
    PRIVATE
        soci_wrapper::soci_wrapper
)
