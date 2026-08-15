cmake_minimum_required(VERSION 3.20)
project(AuraLite LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
  message(FATAL_ERROR "AuraLite Phase-1 targets Windows only.")
endif()

set(AURALITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Match VS OutDir: bin|lib/<Platform>/<Configuration>/
if(CMAKE_GENERATOR_PLATFORM)
  set(AURALITE_PLATFORM "${CMAKE_GENERATOR_PLATFORM}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(AURALITE_PLATFORM "x64")
else()
  set(AURALITE_PLATFORM "Win32")
endif()
# VS uses "Win32"; empty / "x86" normalize to Win32.
if(AURALITE_PLATFORM STREQUAL "" OR AURALITE_PLATFORM STREQUAL "x86")
  set(AURALITE_PLATFORM "Win32")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${AURALITE_ROOT}/bin/${AURALITE_PLATFORM}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${AURALITE_ROOT}/lib/${AURALITE_PLATFORM}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${AURALITE_ROOT}/lib/${AURALITE_PLATFORM}/$<CONFIG>")

include("${AURALITE_ROOT}/cmake/LegacySources.cmake")

# ---- Direct2D canvas (next-gen) ----
add_library(auralite_d2d STATIC
  auralite/canvas_d2d.cpp
)
add_library(AuraLite::D2D ALIAS auralite_d2d)

target_include_directories(auralite_d2d PUBLIC "${AURALITE_ROOT}")
target_compile_definitions(auralite_d2d
  PUBLIC
    NOMINMAX
    WINVER=0x0601
    _WIN32_WINNT=0x0601
    NTDDI_VERSION=0x06010000
    UNICODE
    _UNICODE
)
target_link_libraries(auralite_d2d PUBLIC d2d1 dwrite windowscodecs ole32)
if(MSVC)
  target_compile_options(auralite_d2d PRIVATE /W3 /utf-8 /permissive-)
endif()

# ---- Legacy Views libs (Direct2D paint; still used by 1.x / master) ----
option(AURALITE_BUILD_LEGACY "Build legacy AuraLite.Base + AuraLite.UI" ON)
if(AURALITE_BUILD_LEGACY)
  add_library(AuraLite.Base STATIC ${AURALITE_BASE_SOURCES})
  add_library(AuraLite::Base ALIAS AuraLite.Base)
  target_include_directories(AuraLite.Base PUBLIC "${AURALITE_ROOT}")
  target_compile_definitions(AuraLite.Base
    PUBLIC
      AURALITE_STATIC
      NOMINMAX
      WINVER=0x0601
      _WIN32_WINNT=0x0601
      NTDDI_VERSION=0x06010000
      UNICODE
      _UNICODE
      _CRT_RAND_S
  )
  # Legacy sources are C++14-era; keep C++14 for this target.
  set_target_properties(AuraLite.Base PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  if(MSVC)
    target_compile_options(AuraLite.Base PRIVATE /W3)
  endif()
  if(MSVC)
    # zlib .c files
    set_source_files_properties(
      ${AURALITE_ROOT}/rfc_algorithm/zlib/adler32.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/compress.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/crc32.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/deflate.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/gzio.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/infback.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/inffast.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/inflate.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/inftrees.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/trees.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/uncompr.c
      ${AURALITE_ROOT}/rfc_algorithm/zlib/zutil.c
      PROPERTIES LANGUAGE C
    )
  endif()

  add_library(AuraLite.UI STATIC ${AURALITE_UI_SOURCES})
  add_library(AuraLite::UI ALIAS AuraLite.UI)
  target_include_directories(AuraLite.UI PUBLIC "${AURALITE_ROOT}")
  target_compile_definitions(AuraLite.UI
    PUBLIC
      AURALITE_STATIC
      NOMINMAX
      WINVER=0x0601
      _WIN32_WINNT=0x0601
      NTDDI_VERSION=0x06010000
      UNICODE
      _UNICODE
  )
  set_target_properties(AuraLite.UI PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(AuraLite.UI
    PUBLIC
      AuraLite::Base
      d2d1
      dwrite
      windowscodecs
      msimg32
      comctl32
      ole32
      oleaut32
      uuid
      shell32
      shlwapi
      imm32
      dwmapi
      uxtheme
      oleacc
  )
  if(MSVC)
    target_compile_options(AuraLite.UI PRIVATE /W3)
  endif()
endif()

# ---- yaml-cpp (declarative UI loader) ----
include(FetchContent)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

# ---- Next-gen declarative UI (auralite::ui) ----
add_library(auralite_ui STATIC
  auralite/ui/node.cpp
  auralite/ui/column.cpp
  auralite/ui/row.cpp
  auralite/ui/tile.cpp
  auralite/ui/tab.cpp
  auralite/ui/absolute.cpp
  auralite/ui/label.cpp
  auralite/ui/button.cpp
  auralite/ui/image_view.cpp
  auralite/ui/image_button.cpp
  auralite/ui/text_field.cpp
  auralite/ui/checkbox.cpp
  auralite/ui/radio.cpp
  auralite/ui/switch_control.cpp
  auralite/ui/progress_bar.cpp
  auralite/ui/slider.cpp
  auralite/ui/combo.cpp
  auralite/ui/text_area.cpp
  auralite/ui/scroll_view.cpp
  auralite/ui/list_view.cpp
  auralite/ui/split_view.cpp
  auralite/ui/context_menu.cpp
  auralite/ui/window.cpp
  auralite/ui/application.cpp
  auralite/ui/factory.cpp
  auralite/ui/yaml_loader.cpp
)
add_library(AuraLite::UINext ALIAS auralite_ui)
target_include_directories(auralite_ui PUBLIC "${AURALITE_ROOT}")
target_compile_features(auralite_ui PUBLIC cxx_std_20)
target_compile_definitions(auralite_ui
  PUBLIC
    NOMINMAX
    UNICODE
    _UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
target_link_libraries(auralite_ui
  PUBLIC
    AuraLite::D2D
    yaml-cpp
    imm32
  PRIVATE
    user32
    gdi32
)
if(MSVC)
  target_compile_options(auralite_ui PRIVATE /W3 /utf-8 /permissive-)
endif()

# ---- D2D demo + legacy smoke tests (same OutDir as library.sln) ----
option(AURALITE_BUILD_D2D_DEMO "Build Direct2D smoke demo" ON)
if(AURALITE_BUILD_D2D_DEMO)
  add_executable(d2d_demo WIN32 examples/d2d_demo/main.cpp)
  target_link_libraries(d2d_demo PRIVATE AuraLite::D2D)
  target_compile_definitions(d2d_demo PRIVATE
    NOMINMAX UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
  )
  if(MSVC)
    target_compile_options(d2d_demo PRIVATE /W3 /utf-8 /permissive-)
  endif()
endif()

option(AURALITE_BUILD_UI_SMOKE "Build auralite::ui smoke demo" ON)
if(AURALITE_BUILD_UI_SMOKE)
  add_executable(ui_smoke WIN32 examples/ui_smoke/main.cpp)
  target_link_libraries(ui_smoke PRIVATE AuraLite::UINext)
  target_compile_definitions(ui_smoke PRIVATE
    NOMINMAX UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
  )
  if(MSVC)
    target_compile_options(ui_smoke PRIVATE /W3 /utf-8 /permissive-)
  endif()
endif()

option(AURALITE_BUILD_UI_DEMOS "Build Phase-2 login_demo + ui_gallery" ON)
if(AURALITE_BUILD_UI_DEMOS)
  add_executable(login_demo WIN32 examples/login/main.cpp)
  target_link_libraries(login_demo PRIVATE AuraLite::UINext)
  target_compile_definitions(login_demo PRIVATE
    NOMINMAX UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
  )
  if(MSVC)
    target_compile_options(login_demo PRIVATE /W3 /utf-8 /permissive-)
  endif()
  add_custom_command(TARGET login_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${AURALITE_ROOT}/examples/login/login_window.yaml"
      "$<TARGET_FILE_DIR:login_demo>/login_window.yaml"
    COMMENT "Copy login_window.yaml next to login_demo"
  )

  add_executable(ui_gallery WIN32 examples/ui_gallery/main.cpp)
  target_link_libraries(ui_gallery PRIVATE AuraLite::UINext)
  target_compile_definitions(ui_gallery PRIVATE
    NOMINMAX UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
  )
  if(MSVC)
    target_compile_options(ui_gallery PRIVATE /W3 /utf-8 /permissive-)
  endif()
  add_custom_command(TARGET ui_gallery POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${AURALITE_ROOT}/examples/ui_gallery/gallery.yaml"
      "$<TARGET_FILE_DIR:ui_gallery>/gallery.yaml"
    COMMENT "Copy gallery.yaml next to ui_gallery"
  )

  add_executable(ui_layout_test examples/ui_layout_test/main.cpp)
  target_link_libraries(ui_layout_test PRIVATE AuraLite::UINext)
  target_compile_definitions(ui_layout_test PRIVATE
    NOMINMAX UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
  )
  if(MSVC)
    target_compile_options(ui_layout_test PRIVATE /W3 /utf-8 /permissive-)
  endif()
endif()

option(AURALITE_BUILD_TESTS "Build test_base / test_view" ON)
if(AURALITE_BUILD_TESTS AND AURALITE_BUILD_LEGACY)
  add_executable(test_base test_base/main.cpp)
  target_link_libraries(test_base PRIVATE AuraLite::Base)
  set_target_properties(test_base PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  if(MSVC)
    target_compile_options(test_base PRIVATE /W3)
  endif()

  add_executable(test_view WIN32
    test_view/main.cpp
    test_view/test_view.rc
  )
  target_link_libraries(test_view PRIVATE AuraLite::UI AuraLite::Base)
  set_target_properties(test_view PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  if(MSVC)
    target_compile_options(test_view PRIVATE /W3)
  endif()

  add_executable(test_dwrite test_dwrite/main.cpp)
  target_link_libraries(test_dwrite PRIVATE AuraLite::UI AuraLite::Base)
  set_target_properties(test_dwrite PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  if(MSVC)
    target_compile_options(test_dwrite PRIVATE /W3)
  endif()

  add_executable(test_wic_bitmap test_wic_bitmap/main.cpp)
  target_link_libraries(test_wic_bitmap PRIVATE AuraLite::UI AuraLite::Base)
  set_target_properties(test_wic_bitmap PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  if(MSVC)
    target_compile_options(test_wic_bitmap PRIVATE /W3)
  endif()
endif()

message(STATUS "AuraLite outputs: bin|lib/${AURALITE_PLATFORM}/<Config>/")
message(STATUS "AuraLite Phase-1: D2D canvas + optional legacy Base/UI via CMake")
