cmake_minimum_required(VERSION 3.18)
project(test_b2_asan C CXX)

set(WALLET_CORE_DIR "${CMAKE_SOURCE_DIR}/wallet-core" CACHE PATH "wallet-core source dir")
set(WALLET_CORE_BUILD "${WALLET_CORE_DIR}/build" CACHE PATH "wallet-core build dir")
set(WALLET_CORE_LOCAL "${WALLET_CORE_DIR}/build/local" CACHE PATH "wallet-core local deps")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

include_directories(${WALLET_CORE_DIR}/include)
include_directories(${WALLET_CORE_LOCAL}/include)

# Link wallet-core and ALL its bundled dependencies
link_directories(${WALLET_CORE_BUILD})
link_directories(${WALLET_CORE_LOCAL}/lib)

find_library(WALLET_CORE_LIB NAMES TrustWalletCore PATHS ${WALLET_CORE_BUILD} NO_DEFAULT_PATH)
find_library(protobuf_LIB NAMES protobuf PATHS ${WALLET_CORE_BUILD} ${WALLET_CORE_LOCAL}/lib NO_DEFAULT_PATH)

add_executable(test_b2_asan test_b2_asan.c)
target_link_libraries(test_b2_asan
    ${WALLET_CORE_LIB}
    ${protobuf_LIB}
    ssl
    crypto
    pthread
    dl
)
