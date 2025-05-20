cmake_minimum_required(VERSION 3.16)
project(swift_gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. Find the Qt6 Widgets module (and core/gui)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# 2. Your include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 3. List all of your .cpp/.h pairs, including the new tab widgets:
set(SOURCES
    src/main.cpp
    src/SimulationController.cpp

    # Tab widget implementations
    src/HomeTabWidget.cpp
    src/LogTabWidget.cpp
    src/VizTabWidget.cpp
    src/DiagTabWidget.cpp
)

set(HEADERS
    include/SimulationController.h

    # Main window header 
    src/MainView.h

    # Tab widget headers
    src/HomeTabWidget.h
    src/LogTabWidget.h
    src/VizTabWidget.h
    src/DiagTabWidget.h
)

# 4. Qt resource file for your logo
qt6_add_resources(IMAGE_RES resources/images.qrc)

# 5. Define the executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${IMAGE_RES}
)

# 6. Link against Qt6::Widgets (and Core/Gui)
target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui
)

# 7. Drop the binary beside your source dir
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
