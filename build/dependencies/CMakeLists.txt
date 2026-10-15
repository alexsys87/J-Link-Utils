cmake_minimum_required(VERSION 3.25)
project(JLinkOpenSourceDependencies LANGUAGES C CXX)
if(NOT MSVC OR NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
  message(FATAL_ERROR "Use Build-WindowsDependencies.ps1 with a Visual Studio generator")
endif()
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Let the installed packages, not a second hand-maintained list of .lib names,
# resolve Debug/Release and all Windows system/transitive libraries.
find_package(SDL2 CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(unofficial-brotli CONFIG REQUIRED)

add_executable(dependency_probe WIN32 probe.cpp)
target_compile_features(dependency_probe PRIVATE cxx_std_17)
target_link_libraries(dependency_probe PRIVATE SDL2::SDL2main SDL2::SDL2-static
  Freetype::Freetype PNG::PNG BZip2::BZip2 ZLIB::ZLIB
  unofficial::brotli::brotlidec unofficial::brotli::brotlienc)
enable_testing()
add_test(NAME dependency_probe COMMAND dependency_probe)
