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

find_package(yaml-cpp QUIET)
if (NOT yaml-cpp_FOUND)
  message(STATUS "yaml-cpp not found: downloading via FetchContent")
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        yaml-cpp-0.7.0
  )
  FetchContent_MakeAvailable(yaml-cpp)
endif()

# ─── Qt ─────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui SerialPort)

# ─── HDF5 ───────────────────────────────────────────────
find_package(HDF5 REQUIRED COMPONENTS CXX)
if (HDF5_FOUND)
  message(STATUS "Found HDF5: ${HDF5_CXX_LIBRARIES}")
else()
  message(FATAL_ERROR "HDF5 not found")
endif()

# ─── Include dirs ───────────────────────────────────────
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${HDF5_INCLUDE_DIRS}           # for H5Cpp.h
)

# ─── Sources & headers ──────────────────────────────────
set(SOURCES
    src/main.cpp
    src/SimulationController.cpp
    src/MainView.cpp
    src/CommandLineParser.cpp
    src/ScaledPixmapLabel.cpp
    src/HomeTabWidget.cpp
    src/LogTabWidget.cpp
    src/VizTabWidget.cpp
    src/DataWatcher.cpp
    src/StepCounter.cpp
    src/PlotWidget.cpp
    src/ImageProgressWidget.cpp
    src/SerialHandler.cpp
)

set(HEADERS
    include/SimulationController.h
    src/MainView.h
    src/CommandLineParser.h
    src/ScaledPixmapLabel.h
    src/HomeTabWidget.h
    src/LogTabWidget.h
    src/VizTabWidget.h
    src/StyledSplitter.h
    src/DataWatcher.h
    src/StepCounter.h
    src/PlotWidget.h
    src/ImageProgressWidget.h
    src/SerialHandler.h
)

# ─── QRCs ────────────────────────────────────────────────
qt6_add_resources(IMAGE_RES resources/images.qrc)
qt6_add_resources(STYLE_QRC resources/styles.qrc)

# ─── Executable ─────────────────────────────────────────
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${IMAGE_RES}
)
target_sources(${PROJECT_NAME} PRIVATE ${STYLE_QRC})

# ─── Link ────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt6::Widgets
      Qt6::Core
      Qt6::Gui
      Qt6::SerialPort
      yaml-cpp
      ${HDF5_CXX_LIBRARIES}       # HDF5 C++ API
      ${HDF5_LIBRARIES}           # HDF5 C API
)

# ─── Runtime output ─────────────────────────────────────
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
