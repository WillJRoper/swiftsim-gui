cmake_minimum_required(VERSION 3.16)
project(swift_gui VERSION 0.1 LANGUAGES CXX)

# ─── Suppress CMake developer warnings ───────────────
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)

# ─── yaml-cpp options ──────────────────────────────────
set(YAML_CPP_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS      OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONVERTORS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_SAMPLES    OFF CACHE BOOL "" FORCE)

# 1) Try to find an installed yaml-cpp first
find_package(yaml-cpp QUIET)

# 2) If not found, pull it from GitHub
if (NOT yaml-cpp_FOUND)
  message(STATUS "yaml-cpp not found: downloading via FetchContent")
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        yaml-cpp-0.7.0
  )
  FetchContent_MakeAvailable(yaml-cpp)
endif()

# 3) Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)

# 4) Include dirs
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 5) Sources & headers
set(SOURCES
    src/main.cpp
    src/SimulationController.cpp
    src/MainView.cpp
    src/CommandLineParser.cpp
    src/ScaledPixmapLabel.cpp
    src/HomeTabWidget.cpp
    src/LogTabWidget.cpp
    src/VizTabWidget.cpp
    src/DiagTabWidget.cpp
    src/RuntimeOptions.cpp
)

set(HEADERS
    include/SimulationController.h
    src/MainView.h
    src/CommandLineParser.h
    src/ScaledPixmapLabel.h
    src/HomeTabWidget.h
    src/LogTabWidget.h
    src/VizTabWidget.h
    src/DiagTabWidget.h
    src/RuntimeOptions.h
)

# 6) QRCs (Qt5 style)
qt5_add_resources(IMAGE_RES resources/images.qrc)
qt5_add_resources(STYLE_QRC resources/styles.qrc)

# 7) Executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${IMAGE_RES}
)
target_sources(${PROJECT_NAME} PRIVATE ${STYLE_QRC})

# 8) Link
target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt5::Widgets
      Qt5::Core
      Qt5::Gui
      yaml-cpp
)

# 9) Runtime output
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
