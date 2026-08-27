cmake_minimum_required(VERSION 3.21)

project(financial_reconciliation VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(XLSXWRITER_VCPKG_ROOT
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    )
    find_path(XLSXWRITER_INCLUDE_DIR
        NAMES xlsxwriter.h
        PATHS "${XLSXWRITER_VCPKG_ROOT}/include"
        NO_DEFAULT_PATH
        REQUIRED
    )
    find_library(XLSXWRITER_LIBRARY_RELEASE
        NAMES xlsxwriter
        PATHS "${XLSXWRITER_VCPKG_ROOT}/lib"
        NO_DEFAULT_PATH
        REQUIRED
    )
    find_library(XLSXWRITER_LIBRARY_DEBUG
        NAMES xlsxwriter
        PATHS "${XLSXWRITER_VCPKG_ROOT}/debug/lib"
        NO_DEFAULT_PATH
        REQUIRED
    )

    add_library(xlsxwriter_external UNKNOWN IMPORTED)
    set_target_properties(xlsxwriter_external PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"
        IMPORTED_LOCATION_DEBUG "${XLSXWRITER_LIBRARY_DEBUG}"
        IMPORTED_LOCATION_RELEASE "${XLSXWRITER_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${XLSXWRITER_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_MINSIZEREL "${XLSXWRITER_LIBRARY_RELEASE}"
        INTERFACE_INCLUDE_DIRECTORIES "${XLSXWRITER_INCLUDE_DIR}"
    )
else()
    find_path(XLSXWRITER_INCLUDE_DIR NAMES xlsxwriter.h REQUIRED)
    find_library(XLSXWRITER_LIBRARY NAMES xlsxwriter REQUIRED)

    add_library(xlsxwriter_external UNKNOWN IMPORTED)
    set_target_properties(xlsxwriter_external PROPERTIES
        IMPORTED_LOCATION "${XLSXWRITER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${XLSXWRITER_INCLUDE_DIR}"
    )
endif()

add_executable(financial_reconciliation
    src/main.c
    src/data.c
    src/formulas.c
    src/workbook.c
)

target_include_directories(financial_reconciliation PRIVATE src)
target_link_libraries(financial_reconciliation PRIVATE xlsxwriter_external)

if(MSVC)
    target_compile_options(financial_reconciliation PRIVATE /W4 /utf-8 /permissive-)
else()
    target_compile_options(financial_reconciliation PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(formula_tests tests/formula_tests.c src/formulas.c)
    target_include_directories(formula_tests PRIVATE src)
    if(MSVC)
        target_compile_options(formula_tests PRIVATE /W4 /utf-8 /permissive-)
    else()
        target_compile_options(formula_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion
        )
    endif()
    add_test(NAME formula_tests COMMAND formula_tests)
endif()
