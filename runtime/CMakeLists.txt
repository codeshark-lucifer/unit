add_executable(runtime
    src/main.cpp
)

target_link_libraries(runtime PRIVATE
    engine
)

target_include_directories(runtime PRIVATE
    ${PROJECT_SOURCE_DIR}/engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)