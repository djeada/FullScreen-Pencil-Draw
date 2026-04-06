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
    ../src/widgets/latex_text_item.cpp
    ../src/widgets/latex_text_item.h
    ../src/widgets/text_on_path_item.cpp
    ../src/widgets/text_on_path_item.h
)

target_include_directories(test_item_store PRIVATE
    ../src/core
    ../src/widgets
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
    ../src/widgets/latex_text_item.cpp
    ../src/widgets/latex_text_item.h
    ../src/widgets/text_on_path_item.cpp
    ../src/widgets/text_on_path_item.h
)

target_include_directories(test_project_serializer PRIVATE
    ../src/core
    ../src/widgets
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
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

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

# RotationDialog tests
add_executable(test_rotation_dialog
    test_rotation_dialog.cpp
    ../src/widgets/rotation_dialog.cpp
    ../src/widgets/rotation_dialog.h
)

target_include_directories(test_rotation_dialog PRIVATE
    ../src/widgets
)

target_link_libraries(test_rotation_dialog PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_rotation_dialog PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestRotationDialog COMMAND test_rotation_dialog)

# AlignmentDialog tests
add_executable(test_alignment_dialog
    test_alignment_dialog.cpp
    ../src/widgets/alignment_dialog.cpp
    ../src/widgets/alignment_dialog.h
)

target_include_directories(test_alignment_dialog PRIVATE
    ../src/widgets
)

target_link_libraries(test_alignment_dialog PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_alignment_dialog PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestAlignmentDialog COMMAND test_alignment_dialog)

# TransformAction tests
add_executable(test_transform_action
    test_transform_action.cpp
    ../src/core/transform_action.cpp
    ../src/core/transform_action.h
    ../src/core/action.cpp
    ../src/core/action.h
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ../src/widgets/latex_text_item.cpp
    ../src/widgets/latex_text_item.h
    ../src/widgets/mermaid_text_item.cpp
    ../src/widgets/mermaid_text_item.h
    ../src/widgets/text_on_path_item.cpp
    ../src/widgets/text_on_path_item.h
)

target_include_directories(test_transform_action PRIVATE
    ../src/core
    ../src/widgets
)

target_link_libraries(test_transform_action PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_transform_action PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestTransformAction COMMAND test_transform_action)

# Group clipboard round-trip tests
add_executable(test_group_clipboard
    test_group_clipboard.cpp
)

target_link_libraries(test_group_clipboard PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_group_clipboard PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestGroupClipboard COMMAND test_group_clipboard)

# SideTabBar tests
add_executable(test_side_tab_bar
    test_side_tab_bar.cpp
    ../src/widgets/side_tab_bar.cpp
    ../src/widgets/side_tab_bar.h
)

target_include_directories(test_side_tab_bar PRIVATE
    ../src/widgets
)

target_link_libraries(test_side_tab_bar PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_side_tab_bar PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestSideTabBar COMMAND test_side_tab_bar)

# Architecture elements tests
add_executable(test_architecture_elements
    test_architecture_elements.cpp
    ../src/widgets/architecture_elements.cpp
    ../src/widgets/architecture_elements.h
)

target_include_directories(test_architecture_elements PRIVATE
    ../src/widgets
)

target_link_libraries(test_architecture_elements PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_architecture_elements PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestArchitectureElements COMMAND test_architecture_elements)

# Electronics elements tests
add_executable(test_electronics_elements
    test_electronics_elements.cpp
    ../src/widgets/electronics_elements.cpp
    ../src/widgets/electronics_elements.h
    ../src/widgets/wire_item.cpp
    ../src/widgets/wire_item.h
    ../src/core/theme_manager.cpp
)

target_include_directories(test_electronics_elements PRIVATE
    ../src/widgets
    ../src
)

target_link_libraries(test_electronics_elements PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_electronics_elements PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestElectronicsElements COMMAND test_electronics_elements)

# Wire item and pin tests
add_executable(test_wire_item
    test_wire_item.cpp
    ../src/widgets/electronics_elements.cpp
    ../src/widgets/electronics_elements.h
    ../src/widgets/wire_item.cpp
    ../src/widgets/wire_item.h
    ../src/core/theme_manager.cpp
)

target_include_directories(test_wire_item PRIVATE
    ../src/widgets
    ../src
)

target_link_libraries(test_wire_item PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_wire_item PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestWireItem COMMAND test_wire_item)

# UndoRedoManager tests
add_executable(test_undo_redo_manager
    test_undo_redo_manager.cpp
    ../src/core/undo_redo_manager.cpp
    ../src/core/undo_redo_manager.h
    ../src/core/action.cpp
    ../src/core/action.h
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ../src/widgets/latex_text_item.cpp
    ../src/widgets/latex_text_item.h
    ../src/widgets/mermaid_text_item.cpp
    ../src/widgets/mermaid_text_item.h
    ../src/widgets/text_on_path_item.cpp
    ../src/widgets/text_on_path_item.h
)

target_include_directories(test_undo_redo_manager PRIVATE
    ../src/core
    ../src/widgets
)

target_link_libraries(test_undo_redo_manager PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_undo_redo_manager PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestUndoRedoManager COMMAND test_undo_redo_manager)

# Canvas interaction tests
set(CANVAS_TEST_SOURCES
    ../src/core/action.cpp
    ../src/core/auto_save_manager.cpp
    ../src/core/brush_tip.cpp
    ../src/core/fill_utils.cpp
    ../src/core/image_filters.cpp
    ../src/core/item_store.cpp
    ../src/core/katex_renderer.cpp
    ../src/core/layer.cpp
    ../src/core/mermaid_renderer.cpp
    ../src/core/pdf_document.cpp
    ../src/core/pdf_overlay.cpp
    ../src/core/project_serializer.cpp
    ../src/core/recent_files_manager.cpp
    ../src/core/scene_controller.cpp
    ../src/core/snap_engine.cpp
    ../src/core/theme_manager.cpp
    ../src/core/transform_action.cpp
    ../src/core/undo_redo_manager.cpp
    ../src/widgets/alignment_dialog.cpp
    ../src/widgets/animated_button.cpp
    ../src/widgets/architecture_elements.cpp
    ../src/widgets/busy_spinner_overlay.cpp
    ../src/widgets/canvas.cpp
    ../src/widgets/color_curves_dialog.cpp
    ../src/widgets/dock_title_bar.cpp
    ../src/widgets/electronics_elements.cpp
    ../src/widgets/image_size_dialog.cpp
    ../src/widgets/latex_text_item.cpp
    ../src/widgets/layer_panel.cpp
    ../src/widgets/page_thumbnail_panel.cpp
    ../src/widgets/pdf_search_bar.cpp
    ../src/widgets/pdf_viewer.cpp
    ../src/widgets/mermaid_text_item.cpp
    ../src/widgets/perspective_transform_dialog.cpp
    ../src/widgets/resize_canvas_dialog.cpp
    ../src/widgets/rotation_dialog.cpp
    ../src/widgets/scale_dialog.cpp
    ../src/widgets/scan_document_dialog.cpp
    ../src/widgets/side_tab_bar.cpp
    ../src/widgets/text_on_path_item.cpp
    ../src/widgets/transform_handle_item.cpp
    ../src/widgets/wire_item.cpp
    ../src/widgets/brush_preview.cpp
    ../src/widgets/brush_stroke_item.cpp
    ../src/widgets/element_bank_panel.cpp
    ../src/widgets/tool_panel.cpp
    ../src/windows/main_window.cpp
    ../src/tools/arrow_tool.cpp
    ../src/tools/bezier_tool.cpp
    ../src/tools/circle_tool.cpp
    ../src/tools/eraser_tool.cpp
    ../src/tools/fill_tool.cpp
    ../src/tools/highlighter_tool.cpp
    ../src/tools/lasso_selection_tool.cpp
    ../src/tools/line_tool.cpp
    ../src/tools/mermaid_tool.cpp
    ../src/tools/pan_tool.cpp
    ../src/tools/pen_tool.cpp
    ../src/tools/rectangle_tool.cpp
    ../src/tools/selection_tool.cpp
    ../src/tools/shape_tool.cpp
    ../src/tools/text_on_path_tool.cpp
    ../src/tools/text_tool.cpp
    ../src/tools/tool.cpp
    ../src/tools/tool_manager.cpp
)

add_executable(test_canvas_interaction
    test_canvas_interaction.cpp
    ${CANVAS_TEST_SOURCES}
    ../resources/ui_icons.qrc
    ../resources/resources.qrc
)

target_include_directories(test_canvas_interaction PRIVATE
    ../src/core
    ../src/widgets
    ../src/tools
    ../src/windows
)

target_link_libraries(test_canvas_interaction PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

if(HAVE_QT_SVG)
    target_link_libraries(test_canvas_interaction PRIVATE
        Qt${QT_VERSION_MAJOR}::Svg
    )
endif()

if(Qt${QT_VERSION_MAJOR}Pdf_FOUND)
    target_link_libraries(test_canvas_interaction PRIVATE
        Qt${QT_VERSION_MAJOR}::Pdf
    )
endif()

if(HAVE_QT_WEBENGINE)
    target_link_libraries(test_canvas_interaction PRIVATE
        Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    )
endif()

set_target_properties(test_canvas_interaction PROPERTIES
    AUTOMOC ON
)

add_test(NAME TestCanvasInteraction COMMAND test_canvas_interaction)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_item_store test_project_serializer test_image_filters test_perspective_transform test_snap_engine test_text_whitespace test_latex_unicode test_busy_spinner_overlay test_brush_tip test_rotation_dialog test_alignment_dialog test_transform_action test_group_clipboard test_side_tab_bar test_undo_redo_manager test_canvas_interaction
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
