cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(psvgamesd)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format -Wno-strict-aliasing -Wno-attributes -std=gnu99 -iquote ${CMAKE_CURRENT_SOURCE_DIR}/substitute/lib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
 .
)

link_directories(
  ${CMAKE_SOURCE_DIR}/extra/
)

add_definitions(-DNO_DYNAMIC_LINKER_STUFF)
add_definitions(-DNO_PTHREADS)

add_executable(psvgamesd
  psvgamesd.c
  cmd56_key.c
  mmc_emu.c
  global_log.c
)

target_link_libraries(psvgamesd
  gcc
  SceSysmemForDriver_stub
  SceSysclibForDriver_stub
  SceIofilemgrForDriver_stub
  SceSdifForDriver_stub
  SceMsifForDriver_stub
  SceThreadmgrForDriver_stub
  SceModulemgrForKernel_stub
  SceKernelSuspendForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self(psvgamesd.skprx psvgamesd
  CONFIG ${CMAKE_SOURCE_DIR}/exports.yml
  UNSAFE
)
vita_create_stubs(psvgamesd-stubs psvgamesd ${CMAKE_SOURCE_DIR}/exports.yml
  KERNEL
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/psvgamesd-stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)

install(FILES psvgamesd_api.h
  DESTINATION include
)
