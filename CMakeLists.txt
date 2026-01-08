cmake_minimum_required(VERSION 4.0)

set(DIALUP_INSTALL_DIR "$ENV{LOCALAPPDATA}/DialUp")
include("${DIALUP_INSTALL_DIR}/build-tools/PreProject.cmake")

project(DialUp-SDKGen VERSION 1.3 LANGUAGES CXX)

option(DEPLOY_TO_DEV "Copy plugin and PDB to dev plugins folder after build" OFF)

include("${DIALUP_INSTALL_DIR}/build-tools/GameBinaryDir.cmake")

set(GAME "default" CACHE STRING "Name of the game config to compile with")
set(LOG_DIR "${CMAKE_SOURCE_DIR}" CACHE STRING "Directory to create the log file in")
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/config" CONFIG_DIR)

# fixme: make work with Makefile
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/../sdk-plugin/generated" SDK_OUTPUT_DIR)

file(TO_CMAKE_PATH "${CONFIG_DIR}/${GAME}" GAME_CONFIG_DIR)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/runtime" ENGINE_DIR)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" LOG_DIR)
file(TO_CMAKE_PATH "${DIALUP_INSTALL_DIR}" DIALUP_DIR)

message("Using game: ${GAME}")
message("Using schema from: ${CONFIG_DIR}/${GAME}")

# for AST walking Schema
set(CLANG_INCLUDE_DIR "$ENV{ProgramFiles}/LLVM/include")
set(CLANG_LIBRARY "$ENV{ProgramFiles}/LLVM/lib/libclang.lib")

set(SOURCES
    "${GAME_CONFIG_DIR}/Schema.cpp"
    ${ENGINE_DIR}/Runtime.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/RuntimeGen.cpp
    ${CMAKE_SOURCE_DIR}/src/model/ObjectStore.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set(INCLUDES
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/model
    ${CMAKE_SOURCE_DIR}/src/model/abstract
    ${CMAKE_SOURCE_DIR}/src/model/base
    ${CMAKE_SOURCE_DIR}/src/model/helpers
    ${CMAKE_SOURCE_DIR}/src/model/properties
    ${CMAKE_SOURCE_DIR}/src/schema
    ${DIALUP_INSTALL_DIR}/include/v1
    ${DIALUP_INSTALL_DIR}/include/external

    ${GAME_CONFIG_DIR}
    ${ENGINE_DIR}

)

set(EXTERNAL_INCLUDES
    ${DIALUP_INSTALL_DIR}/include/external/fkYAML/include
    ${DIALUP_INSTALL_DIR}/include/external/fmt/include
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CLANG_INCLUDE_DIR}
    ${INCLUDES}
    ${EXTERNAL_INCLUDES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${CLANG_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    LOG_DIR="${LOG_DIR}"
    CONFIG_DIR="${CONFIG_DIR}"
    SDK_OUTPUT_DIR="${SDK_OUTPUT_DIR}"
    GAME_CONFIG_DIR="${GAME_CONFIG_DIR}"
    GAME_NAME="${GAME}"
    ENGINE_DIR="${ENGINE_DIR}"
    DIALUP_DIR="${DIALUP_DIR}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE SDK_BUILD)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

include("${DIALUP_INSTALL_DIR}/build-tools/PostProject.cmake")
