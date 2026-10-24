include(doctest.cmake)

add_executable(stereo_frustum_test stereo_frustum.cpp)
target_include_directories(stereo_frustum_test PRIVATE ${CMAKE_SOURCE_DIR}/shared/include)
target_link_libraries(stereo_frustum_test PRIVATE doctest::doctest glm::glm)
set_target_properties(stereo_frustum_test PROPERTIES CXX_STANDARD 20)
add_test(NAME stereo_frustum COMMAND stereo_frustum_test)
