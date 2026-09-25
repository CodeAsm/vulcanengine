cmake_minimum_required(VERSION 3.20)
project(model_viewer CXX C)
 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
 
find_package(Vulkan REQUIRED)
 
include(FetchContent)
 
# ---- GLFW (windowing / input) ----
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
 
# ---- GLM (math) ----
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1)
FetchContent_MakeAvailable(glm)
 
# ---- Dear ImGui (UI, source only, we compile it ourselves) ----
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.1)
FetchContent_MakeAvailable(imgui)
 
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
 
# ---- tinyobjloader (header-only, OBJ/MTL) ----
FetchContent_Declare(tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
  GIT_TAG        release)

FetchContent_MakeAvailable(tinyobjloader)
 
# ---- stb (header-only, images) ----
FetchContent_Declare(stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master)
FetchContent_MakeAvailable(stb)
 
# ---- Assimp (optional, only used for FBX import) ----
find_package(assimp QUIET)
if(assimp_FOUND)
  message(STATUS "Assimp found -> FBX import enabled")
else()
  message(WARNING "Assimp NOT found -> FBX import disabled, OBJ still works. Install libassimp-dev to enable FBX.")
endif()
 
add_executable(model_viewer
  src/main.cpp
  src/Application.cpp
  src/VulkanBase.cpp
  src/Pipeline.cpp
  src/Buffer.cpp
  src/Texture.cpp
  src/ModelLoader.cpp
  src/Camera.cpp
  src/UI.cpp
)
 
target_include_directories(model_viewer PRIVATE
  src
  ${stb_SOURCE_DIR}
  ${tinyobjloader_SOURCE_DIR}
)
 
target_link_libraries(model_viewer PRIVATE
  Vulkan::Vulkan
  glfw
  glm::glm
  imgui
  tinyobjloader
)
 
if(assimp_FOUND)
  target_compile_definitions(model_viewer PRIVATE HAVE_ASSIMP)
  target_link_libraries(model_viewer PRIVATE assimp::assimp)
endif()
 
target_compile_definitions(model_viewer PRIVATE
  $<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)
 
# ---- Shader compilation (glslc, ships with the Vulkan SDK / vulkan-tools) ----
find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/bin)
if(NOT GLSLC_EXECUTABLE)
  message(FATAL_ERROR "glslc not found. Install vulkan-tools / the Vulkan SDK (it provides glslc).")
endif()
 
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})
 
add_custom_command(
  OUTPUT ${SHADER_BIN_DIR}/shader.vert.spv
  COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC_DIR}/shader.vert -o ${SHADER_BIN_DIR}/shader.vert.spv
  DEPENDS ${SHADER_SRC_DIR}/shader.vert
  COMMENT "Compiling shader.vert")
 
add_custom_command(
  OUTPUT ${SHADER_BIN_DIR}/shader.frag.spv
  COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC_DIR}/shader.frag -o ${SHADER_BIN_DIR}/shader.frag.spv
  DEPENDS ${SHADER_SRC_DIR}/shader.frag
  COMMENT "Compiling shader.frag")
 
add_custom_target(shaders DEPENDS ${SHADER_BIN_DIR}/shader.vert.spv ${SHADER_BIN_DIR}/shader.frag.spv)
add_dependencies(model_viewer shaders)
 
target_compile_definitions(model_viewer PRIVATE SHADER_DIR="${SHADER_BIN_DIR}/")
 
