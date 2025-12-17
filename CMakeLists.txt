cmake_minimum_required(VERSION 4.0)
project(mouse_highlight LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(${PROJECT_NAME} SHARED ${CMAKE_SOURCE_DIR}/src/library.cpp)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        src/*.cpp
        src/*.h
)

set(THIRD_PARTY_SOURCES
        third-party/imgui/imgui.cpp
        third-party/imgui/imgui_draw.cpp
        third-party/imgui/imgui_tables.cpp
        third-party/imgui/imgui_widgets.cpp
)

target_sources(${PROJECT_NAME} PRIVATE
        ${SRC_FILES}
        ${THIRD_PARTY_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party
)

if (MSVC)
    # Define DEBUG macro in Debug builds
    target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:DEBUG>)

    # Keep warning level modest to avoid churn; enable multi-processor compilation
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /MP)

    # Compiler optimizations
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/O1 /GL /DNDEBUG>
    )

    # Linker optimizations
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )

    # Use the DLL runtime library
    set_property(TARGET ${PROJECT_NAME} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif ()

if (WIN32)
    add_custom_target(generate_version ALL
            COMMAND cmd /C "${CMAKE_SOURCE_DIR}/version.bat"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    add_dependencies(${PROJECT_NAME} generate_version)
endif ()

if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "C:/Guild Wars 2/addons/${PROJECT_NAME}.dll.update"
            COMMAND ${CMAKE_COMMAND} -E echo "Copied ${PROJECT_NAME} to Guild Wars 2 addons folder."
            VERBATIM
    )
endif ()