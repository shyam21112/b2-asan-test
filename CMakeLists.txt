cmake_minimum_required(VERSION 3.18)
project(test_b2_asan C CXX)

set(WALLET_CORE_DIR "${CMAKE_SOURCE_DIR}/wallet-core" CACHE PATH "wallet-core source dir")
set(WALLET_CORE_BUILD "${WALLET_CORE_DIR}/build" CACHE PATH "wallet-core build dir")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

include_directories(${WALLET_CORE_DIR}/include)
include_directories(${WALLET_CORE_DIR}/build/local/include)

find_library(WALLET_CORE_LIB
    NAMES TrustWalletCore
    PATHS ${WALLET_CORE_BUILD} ${WALLET_CORE_BUILD}/local/lib
    NO_DEFAULT_PATH
)

if(NOT WALLET_CORE_LIB)
    message(FATAL_ERROR "wallet-core library not found in ${WALLET_CORE_BUILD}")
endif()

message(STATUS "Found wallet-core: ${WALLET_CORE_LIB}")

add_executable(test_b2_asan test_b2_asan.c)
target_link_libraries(test_b2_asan ${WALLET_CORE_LIB} stdc++ pthread)
