cmake_minimum_required(VERSION 3.16.3)
project(AdvancedServer C)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

option(WITH_EXTENDED_CORE "Statically link ExtendedCore plugin" ON)
option(PLUGIN_DYNAMIC_LOADING "Load .dll/.so plugins from plugins/ at runtime" OFF)

include_directories(include/)
include_directories(enet/include)
add_subdirectory(enet/)
add_subdirectory(advancedserver/)
