cmake_minimum_required(VERSION 3.20)
project(texteditor CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COVERAGE "Enable coverage instrumentation" OFF)
option(ASAN "Enable AddressSanitizer" OFF)

set(EDITOR_SOURCES
    src/buffer/buffer.cpp
    src/editor/editor.cpp
)

# texteditor won't link until src/main.cpp has a main() — use --target tests during early dev
add_executable(texteditor
    src/main.cpp
    src/terminal/input.cpp
    src/terminal/renderer.cpp
    ${EDITOR_SOURCES}
)
target_include_directories(texteditor PRIVATE src)

# --- tests ---
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")

if(TEST_SOURCES)
    find_package(GTest REQUIRED)
    enable_testing()

    add_executable(tests ${TEST_SOURCES} ${EDITOR_SOURCES})
    target_include_directories(tests PRIVATE src)
    target_link_libraries(tests PRIVATE GTest::GTest GTest::Main)
    add_test(NAME all_tests COMMAND tests)

    if(COVERAGE)
        target_compile_options(tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(tests PRIVATE -fprofile-instr-generate)
    endif()

    if(ASAN)
        target_compile_options(tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(tests PRIVATE -fsanitize=address)
    endif()
else()
    message(STATUS "No test files found in tests/ — skipping tests target")
endif()
