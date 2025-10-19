cmake_minimum_required(VERSION 3.10)
project(cdma LANGUAGES C)

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED ON)
SET(CMAKE_C_EXTENSIONS OFF)

OPTION(CDMA_DEBUG_MESSAGES "Turn on all debug messages from CDMA." OFF)
OPTION(BUILD_WARN_AS_ERROR "Treat all warnings as compile errors." OFF)
OPTION(BUILD_USE_SANITIZER "Enable to build with default sanitizers." OFF)

FUNCTION(SETUP_TARGET target)
    # Determine compiler.
    SET(is_gcc OFF)
    SET(is_clang OFF)
    SET(is_msvc OFF)

    IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        SET(is_gcc ON)
    ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        SET(is_clang ON)
    ELSEIF(MSVC)
        SET(is_msvc ON)
    ENDIF()

    # Setup warnings.
    IF(is_msvc)
        TARGET_COMPILE_OPTIONS(${target} PRIVATE /W4 /permissive-)
    ELSEIF(is_clang OR is_gcc)
        TARGET_COMPILE_OPTIONS(${target} PRIVATE -Wall -Wextra -pedantic
                                                 -Wold-style-cast -Wextra-semi
                                                 -Woverloaded-virtual
                                                 -Wzero-as-null-pointer-constant
                                                 -Wnull-dereference
        )
        IF(is_clang)
            TARGET_COMPILE_OPTIONS(${target} PRIVATE -Wimplicit-fallthrough -Wshadow-uncaptured-local -Wloop-analysis)
        ELSE()
            TARGET_COMPILE_OPTIONS(${target} PRIVATE -Wimplicit-fallthrough=5 -Wshadow=compatible-local -Wduplicated-branches -Wduplicated-cond -Wsuggest-override)
        ENDIF()
    ENDIF()

    IF(BUILD_WARN_AS_ERROR)
        IF(is_msvc)
            TARGET_COMPILE_OPTIONS(${target} PRIVATE /WX)
        ELSE()
            TARGET_COMPILE_OPTIONS(${target} PRIVATE -Werror)
        ENDIF()
    ENDIF()

    # Setup sanitizers.
    IF(BUILD_USE_SANITIZER)
        IF(is_msvc)
            TARGET_COMPILE_OPTIONS(${target} PRIVATE /fsanitize=address)
            TARGET_COMPILE_DEFINITIONS(${target} PRIVATE _DISABLE_STRING_ANNOTATION=1 _DISABLE_VECTOR_ANNOTATION=1)
        ELSE()
            IF(WIN32)
                TARGET_COMPILE_OPTIONS(${target} PRIVATE -fsanitize=address)
                TARGET_LINK_OPTIONS(${target} PRIVATE -fsanitize=address)
            ELSE()
                TARGET_COMPILE_OPTIONS(${target} PRIVATE -fsanitize=address,leak,undefined)
                TARGET_LINK_OPTIONS(${target} PRIVATE -fsanitize=address,leak,undefined)
            ENDIF()
            TARGET_COMPILE_OPTIONS(${target} PRIVATE -fno-sanitize-recover=all -fno-optimize-sibling-calls -fno-omit-frame-pointer)
        ENDIF()
    ENDIF()

    IF(WIN32)
        TARGET_COMPILE_DEFINITIONS(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    ENDIF()

    IF(CDMA_DEBUG_MESSAGES)
        TARGET_COMPILE_DEFINITIONS(${target} PRIVATE CDMA_DEBUG)
    ENDIF()
ENDFUNCTION()

ENABLE_TESTING()

FOREACH(t IN ITEMS "god-cat-ham-sun" "xxxx-yyyy-zzzz" "eight-words")
    SET(n "test-${t}")
    ADD_EXECUTABLE(${n} "test-driver.c" "${n}.c" "cdma.c" "cdma.h")
    SETUP_TARGET(${n})
    ADD_TEST(${n} ${n})
ENDFOREACH()
