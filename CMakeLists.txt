cmake_minimum_required(VERSION 3.16)

project(FullScreen-Pencil-Draw VERSION 0.1 LANGUAGES CXX)

# Enable automatic handling of Qt-related features
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt libraries (prefer Qt6, fallback to Qt5)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Svg REQUIRED)

# Explicitly specify source and header files
file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
)

file(GLOB_RECURSE PROJECT_HEADERS
    src/*.h
)

# Combine sources and headers for Qt Creator visibility
set(PROJECT_FILES
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

# Set up executable or library depending on platform and Qt version
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_FILES}
    )
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_FILES}
        )
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_FILES}
        )
    endif()
endif()

# Include the headers directory to ensure all headers are found
target_include_directories(${PROJECT_NAME} PRIVATE
    src/core
    src/tools
    src/widgets
    src/windows
)

# Link Qt Widgets library
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)

# Set properties for macOS, if needed
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# Finalize the executable for Qt6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
