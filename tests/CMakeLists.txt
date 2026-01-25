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
)

set(CORE_HEADERS
    ../src/core/item_id.h
    ../src/core/item_ref.h
    ../src/core/item_store.h
    ../src/core/scene_controller.h
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

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_item_store
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
