add_library(engine SHARED
    src/engine.cpp
    src/vk/instance.cpp
    src/vk/device.cpp
    src/vk/debug.cpp
    src/vk/swapchain.cpp
    src/vk/pipeline.cpp
    src/vk/framebuffer.cpp
    src/vk/buffer.cpp
)

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engine
    PUBLIC
        Vulkan::Vulkan
    PRIVATE
        ${PROJECT_SOURCE_DIR}/lib/libSDL3.a
)

target_compile_definitions(engine PRIVATE
    ENGINE_BUILD
    $<$<CONFIG:Debug>:DEBUG>
)

if(WIN32)
    target_compile_definitions(engine PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
    )
    target_link_libraries(engine PRIVATE
        winmm
        setupapi
        imm32
        version
        cfgmgr32
        hid
    )
elseif(APPLE)
    target_compile_definitions(engine PRIVATE
        VK_USE_PLATFORM_MACOS_MVK
    )
    target_link_libraries(engine PRIVATE
        "-framework Metal"
        "-framework Cocoa"
    )
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(engine PRIVATE
        VK_USE_PLATFORM_XCB_KHR
    )
endif()

# Place the DLL and import library in the root bin directory
set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)