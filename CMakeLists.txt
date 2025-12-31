cmake_minimum_required(VERSION 3.10)
project(Arena VERSION 1.0 LANGUAGES C)

# Flags de compilation stricts
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Dossiers de sortie
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

# Librairie statique
add_library(ledger_lib STATIC src/ledger.c)
set_target_properties(ledger_lib PROPERTIES OUTPUT_NAME "ledger")

# Exécutable
add_executable(ledger_exe 
    src/main.c
    src/ledger.c
)

set_target_properties(ledger_exe PROPERTIES OUTPUT_NAME "ledger")

# Linkage
target_link_libraries(ledger_exe PRIVATE ledger_lib)