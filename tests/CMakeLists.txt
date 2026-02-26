# Test CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find Qt Test component
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# Core source files needed for testing
set(CORE_SOURCES
    ../src/core/item_store.cpp
    ../src/core/scene_controller.cpp
    ../src/core/layer.cpp
    ../src/core/project_serializer.cpp
)

set(CORE_HEADERS
    ../src/core/item_id.h
    ../src/core/item_ref.h
    ../src/core/item_store.h
    ../src/core/scene_controller.h
    ../src/core/layer.h
    ../src/core/project_serializer.h
)

# ItemStore tests
add_executable(test_item_store
    test_item_store.cpp
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(test_item_store PRIVATE
    ../src/core
)

target_link_libraries(test_item_store PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

# Enable automatic MOC for test executable
set_target_properties(test_item_store PROPERTIES
    AUTOMOC ON
)

# Add test
add_test(NAME TestItemStore COMMAND test_item_store)

# ProjectSerializer tests
add_executable(test_project_serializer
    test_project_serializer.cpp
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(test_project_serializer PRIVATE
    ../src/core
)

target_link_libraries(test_project_serializer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_project_serializer PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestProjectSerializer COMMAND test_project_serializer)

# ImageFilters tests
add_executable(test_image_filters
    test_image_filters.cpp
    ../src/core/image_filters.cpp
    ../src/core/image_filters.h
)

target_include_directories(test_image_filters PRIVATE
    ../src/core
)

target_link_libraries(test_image_filters PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_image_filters PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestImageFilters COMMAND test_image_filters)

# PerspectiveTransform tests
add_executable(test_perspective_transform
    test_perspective_transform.cpp
    ../src/widgets/perspective_transform_dialog.cpp
    ../src/widgets/perspective_transform_dialog.h
)

target_include_directories(test_perspective_transform PRIVATE
    ../src/widgets
)

target_link_libraries(test_perspective_transform PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_perspective_transform PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestPerspectiveTransform COMMAND test_perspective_transform)

# SnapEngine tests
add_executable(test_snap_engine
    test_snap_engine.cpp
    ../src/core/snap_engine.cpp
    ../src/core/snap_engine.h
)

target_include_directories(test_snap_engine PRIVATE
    ../src/core
)

target_link_libraries(test_snap_engine PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_snap_engine PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestSnapEngine COMMAND test_snap_engine)

# TextWhitespace tests
add_executable(test_text_whitespace
    test_text_whitespace.cpp
)

target_link_libraries(test_text_whitespace PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_text_whitespace PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestTextWhitespace COMMAND test_text_whitespace)

# LatexUnicode tests
add_executable(test_latex_unicode
    test_latex_unicode.cpp
)

target_link_libraries(test_latex_unicode PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_latex_unicode PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestLatexUnicode COMMAND test_latex_unicode)

# BusySpinnerOverlay tests
add_executable(test_busy_spinner_overlay
    test_busy_spinner_overlay.cpp
    ../src/widgets/busy_spinner_overlay.cpp
    ../src/widgets/busy_spinner_overlay.h
)

target_include_directories(test_busy_spinner_overlay PRIVATE
    ../src/widgets
)

target_link_libraries(test_busy_spinner_overlay PRIVATE
# BrushTip tests
add_executable(test_brush_tip
    test_brush_tip.cpp
    ../src/core/brush_tip.cpp
    ../src/core/brush_tip.h
)

target_include_directories(test_brush_tip PRIVATE
    ../src/core
)

target_link_libraries(test_brush_tip PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_busy_spinner_overlay PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestBusySpinnerOverlay COMMAND test_busy_spinner_overlay)
set_target_properties(test_brush_tip PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestBrushTip COMMAND test_brush_tip)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_item_store test_project_serializer test_image_filters test_perspective_transform test_snap_engine test_text_whitespace test_latex_unicode test_busy_spinner_overlay
    DEPENDS test_item_store test_project_serializer test_image_filters test_perspective_transform test_snap_engine test_text_whitespace test_latex_unicode test_brush_tip
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
