include(doctest.cmake)

add_executable(stereo_frustum_test stereo_frustum.cpp)
target_include_directories(stereo_frustum_test PRIVATE ${CMAKE_SOURCE_DIR}/shared/include)
target_link_libraries(stereo_frustum_test PRIVATE doctest::doctest glm::glm)
set_target_properties(stereo_frustum_test PROPERTIES CXX_STANDARD 20)
add_test(NAME stereo_frustum COMMAND stereo_frustum_test)

# The eyes' places in the 3D window's buffer and the render resolution per eye
add_executable(stereo_layout_test stereo_layout.cpp)
target_include_directories(stereo_layout_test PRIVATE ${CMAKE_SOURCE_DIR}/shared/include)
target_link_libraries(stereo_layout_test PRIVATE doctest::doctest)
set_target_properties(stereo_layout_test PROPERTIES CXX_STANDARD 20)
add_test(NAME stereo_layout COMMAND stereo_layout_test)

# The side menu's layout, loaded with the offscreen platform
if(BUILD_QT_UI)
    find_package(Qt6 6.8.2 COMPONENTS Test Widgets Quick QUIET)
    if(TARGET Qt6::Test)
        qt_add_executable(side_menu_layout_test side_menu_layout.cpp)
        target_link_libraries(side_menu_layout_test PRIVATE Qt6::Test Qt6::Widgets KDAB::QtCommon)
        set_target_properties(side_menu_layout_test PROPERTIES CXX_STANDARD 20 AUTOMOC ON)
        add_test(NAME side_menu_layout COMMAND side_menu_layout_test)
        set_tests_properties(side_menu_layout PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QML_DISABLE_DISK_CACHE=1")
    endif()
endif()
