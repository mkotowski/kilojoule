cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(kilojoule LANGUAGES CXX)

# Project version
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

find_program(CMAKE_CXX_CLANG_TIDY clang-tidy 
    NAMES clang-tidy HINTS $ENV{PROGRAMFILES}/clang-tidy
)

if(CMAKE_CXX_CLANG_TIDY)
    list(APPEND CMAKE_CXX_CLANG_TIDY
        "--extra-arg-before=-std=c++17"
        "-header-filter=."
        "-checks=-*,clang-analyzer-*,modernize-*,readability-*,-modernize-use-trailing-return-type,-modernize-avoid-bind"
    )
endif()

find_program(CMAKE_CXX_CPPCHECK cppcheck 
    NAMES cppcheck HINTS $ENV{PROGRAMFILES}/cppcheck
)

if(CMAKE_CXX_CPPCHECK)
    include(ProcessorCount)
    ProcessorCount(CPU_CORES)

    list(APPEND CMAKE_CXX_CPPCHECK
        "-j ${CPU_CORES}"
        "--quiet"
        "--std=c++17"
        "${CMAKE_SOURCE_DIR}"
    )
endif()

find_program(PATH_INCLUDE_WHAT_YOU_USE iwyu 
    NAMES iwyu HINTS
        $ENV{PROGRAMFILES}/iwyu
        /usr/bin/iwyu
)

set(CMAKE_LINK_WHAT_YOU_USE TRUE)

file(GLOB executable_source_files CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

file(GLOB executable_header_files CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
)

set(TARGET_NAME kj)

add_executable(${TARGET_NAME}
    ${executable_header_files} ${executable_source_files}
)

if(PATH_INCLUDE_WHAT_YOU_USE)	
	set_property(TARGET ${TARGET_NAME} 
    	PROPERTY CXX_INCLUDE_WHAT_YOU_USE
    		"${PATH_INCLUDE_WHAT_YOU_USE}"
	)
endif()

target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET ${TARGET_NAME} 
    PROPERTY FOLDER "${CMAKE_PROJECT_NAME}"
)

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(${TARGET_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /utf-8>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELEASE>>:/O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:RELEASE>>:-O3>
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:DEBUG>>:-O0>
)

if(MSVC)
    # https://discourse.cmake.org/t/cmake-cxx-clang-tidy-in-msvc/890/3
    set_target_properties(${TARGET_NAME} PROPERTIES
        VS_GLOBAL_RunCodeAnalysis true
        VS_GLOBAL_EnableMicrosoftCodeAnalysis off

        VS_STARTUP_PROJECT ${TARGET_NAME}

        # Use clangtidy
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
        VS_GLOBAL_ClangTidyChecks -checks=-*,modernize-*,-modernize-use-trailing-return-type
    )
endif(MSVC)
