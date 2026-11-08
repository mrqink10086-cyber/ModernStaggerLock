cmake_minimum_required(VERSION 3.24)

project(ModernStaggerLock VERSION 1.2.2 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(SKSEPluginAuthor "Maxsu" CACHE STRING "Plugin author")
set(COMMONLIBSSE_NG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../CommonLibSSE-NG-ng" CACHE PATH "CommonLibSSE-NG source directory")
set(DKUTIL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/DKUtil" CACHE PATH "DKUtil source directory")

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

option(ENABLE_SKYRIM_SE "Build for Skyrim 1.5.97" ON)
option(ENABLE_SKYRIM_AE "Build for Skyrim 1.6.1170" ON)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "VR is not supported by this plugin" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build CommonLibSSE tests")
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "Enable Xbyak trampoline support")
add_subdirectory("${COMMONLIBSSE_NG_PATH}" CommonLibSSE)

if(NOT EXISTS "${DKUTIL_PATH}/CMakeLists.txt")
    include(FetchContent)
    FetchContent_Declare(DKUtil
        GIT_REPOSITORY https://github.com/gottyduke/DKUtil.git
        GIT_TAG 19fcb2d2ee00a9b678bf08b24c53208ab7618fc6)
    FetchContent_MakeAvailable(DKUtil)
else()
    add_subdirectory("${DKUTIL_PATH}" DKUtil)
endif()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(xbyak CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS SimpleIni.h REQUIRED)

configure_file(cmake/Version.h.in include/Version.h @ONLY)
configure_file(cmake/version.rc.in version.rc @ONLY)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h include/*.h)
add_library(${PROJECT_NAME} SHARED ${SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include" include src ${SIMPLEINI_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE DKUtil::DKUtil
    nlohmann_json::nlohmann_json spdlog::spdlog xbyak::xbyak)
target_compile_options(${PROJECT_NAME} PRIVATE /W3 /permissive- /Zc:__cplusplus /Zc:preprocessor)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION Data/SKSE/Plugins COMPONENT Plugin)

option(MSL_BUILD_TESTS "Build standalone runtime logic tests and benchmarks" OFF)
if(MSL_BUILD_TESTS)
    enable_testing()
    add_executable(msl_runtime_tests tests/runtime_tests.cpp src/StaggerData.cpp)
    target_include_directories(msl_runtime_tests PRIVATE src)
    target_link_libraries(msl_runtime_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME msl_runtime_tests COMMAND msl_runtime_tests)
    add_executable(msl_runtime_bench EXCLUDE_FROM_ALL tests/runtime_bench.cpp src/StaggerData.cpp)
    target_include_directories(msl_runtime_bench PRIVATE src)
    target_link_libraries(msl_runtime_bench PRIVATE nlohmann_json::nlohmann_json)
endif()
