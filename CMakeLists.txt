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
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

# Find Qt SVG module (optional, for SVG export functionality)
set(HAVE_QT_SVG FALSE)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Svg QUIET)
if(Qt${QT_VERSION_MAJOR}Svg_FOUND)
    set(HAVE_QT_SVG TRUE)
    message(STATUS "Qt SVG module found - SVG export enabled")
else()
    message(STATUS "Qt SVG module not found - SVG export disabled")
endif()

# Find Qt PDF module (optional, for PDF viewing functionality)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Pdf QUIET)
if(Qt${QT_VERSION_MAJOR}Pdf_FOUND)
    message(STATUS "Qt PDF module found - PDF viewing enabled")
    add_definitions(-DHAVE_QT_PDF)
else()
    message(STATUS "Qt PDF module not found - PDF viewing disabled")
endif()

# Find Qt WebEngineWidgets module (optional, for KaTeX LaTeX rendering)
set(HAVE_QT_WEBENGINE FALSE)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS WebEngineWidgets QUIET)
if(Qt${QT_VERSION_MAJOR}WebEngineWidgets_FOUND)
    set(HAVE_QT_WEBENGINE TRUE)
    message(STATUS "Qt WebEngineWidgets found - KaTeX LaTeX rendering enabled")
else()
    message(STATUS "Qt WebEngineWidgets not found - using Unicode fallback for LaTeX")
endif()

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

# Always include UI icon resources
set(UI_RESOURCES resources/ui_icons.qrc)

# Add Qt resources if WebEngine is available (KaTeX assets)
if(HAVE_QT_WEBENGINE)
    set(KATEX_RESOURCES resources/resources.qrc)
endif()

# Set up executable or library depending on platform and Qt version
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_FILES}
        ${UI_RESOURCES}
        ${KATEX_RESOURCES}
    )
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_FILES}
            ${UI_RESOURCES}
            ${KATEX_RESOURCES}
        )
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_FILES}
            ${UI_RESOURCES}
            ${KATEX_RESOURCES}
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
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Link Qt SVG if available
if(HAVE_QT_SVG)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_QT_SVG)
endif()

# Link Qt PDF if available
if(Qt${QT_VERSION_MAJOR}Pdf_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Pdf)
endif()

# Link Qt WebEngineWidgets if available
if(HAVE_QT_WEBENGINE)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_QT_WEBENGINE)
endif()

# Set properties for macOS, if needed
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

# Finalize the executable for Qt6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# Add tests subdirectory (optional, controlled by BUILD_TESTING)
option(BUILD_TESTING "Build unit tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
