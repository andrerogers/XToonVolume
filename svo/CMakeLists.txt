cmake_minimum_required(VERSION 3.20)
project(XToonSVO LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(SVO_BUILD_APP "Build the interactive OpenGL viewer (needs GLFW)" ON)
option(SVO_BUILD_TESTS "Build the unit tests" ON)

include(FetchContent)

# glm: prefer the system package (libglm-dev), otherwise fetch it.
find_package(glm 0.9.9 QUIET CONFIG)
if(NOT TARGET glm::glm)
  FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glm)
endif()

find_package(Threads REQUIRED)

# --- Core: octree build, traversal, BVH, animation, CPU reference renderer (no GL) ---
add_library(svo_core STATIC
  src/core/animation.cpp
  src/core/bvh.cpp
  src/core/demo.cpp
  src/core/image.cpp
  src/core/octree.cpp
  src/core/render.cpp
  src/core/scene.cpp
  src/core/traverse.cpp
  src/core/voxelize.cpp)
target_include_directories(svo_core PUBLIC src/core)
target_link_libraries(svo_core PUBLIC glm::glm Threads::Threads)
target_compile_definitions(svo_core PUBLIC GLM_ENABLE_EXPERIMENTAL)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(svo_core PRIVATE -Wall -Wextra)
endif()

# Headless CPU renderer: writes PNGs, handy for checking changes without a GPU.
add_executable(svo_cli src/tools/svo_cli.cpp)
target_link_libraries(svo_cli PRIVATE svo_core)

if(SVO_BUILD_APP)
  find_package(OpenGL REQUIRED)
  find_package(glfw3 3.3 QUIET)
  if(NOT TARGET glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw)
  endif()

  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.5 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(imgui)
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

  add_library(glad STATIC third_party/glad/src/glad.c)
  target_include_directories(glad PUBLIC third_party/glad/include)
  target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

  add_executable(xtoon_svo src/app/main.cpp src/gpu/gl_renderer.cpp)
  target_include_directories(xtoon_svo PRIVATE src)
  target_link_libraries(xtoon_svo PRIVATE svo_core glad glfw imgui OpenGL::GL)
  target_compile_definitions(xtoon_svo PRIVATE
    SVO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    SVO_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
endif()

if(SVO_BUILD_TESTS)
  enable_testing()
  add_executable(svo_tests tests/tests.cpp)
  target_link_libraries(svo_tests PRIVATE svo_core)
  target_compile_definitions(svo_tests PRIVATE SVO_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
  add_test(NAME svo_tests COMMAND svo_tests)
endif()
