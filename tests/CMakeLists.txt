add_executable(testLayouts test_layouts.cpp)
target_link_libraries(testLayouts Qt5::Core Qt5::Test)
target_link_libraries(testLayouts ${LIB_NAME})
add_test(TestLayouts testLayouts)
