project(KDAB_Qt_Common)

find_package(
    Qt6 6.8.2
    COMPONENTS Core
               Widgets
               Quick
               QuickWidgets
               Gui
               QuickControls2
               Qml
               3DExtras
    REQUIRED
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)

# Lib
qt_add_library(
    ${PROJECT_NAME}
    STATIC
    util_qt.h
    side_menu.h
    main_window.h
    main_window.cpp
    renderer_initializer.h
    window_event_watcher.h
    stereo_output.h
    stereo_output.cpp
    ${CMAKE_SOURCE_DIR}/assets/resources.qrc
)

# The desktop's declaration of a full side by side surface (Sparky Stereo OS's libstereo-declare); without it
# the side by side path is unavailable and the program says so
find_package(StereoDeclare 1.1 CONFIG QUIET)
if(TARGET StereoDeclare::stereo-declare)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WaylandClient IMPORTED_TARGET wayland-client)
    find_package(Qt6 6.8.2 COMPONENTS GuiPrivate QUIET) # the window's native surface, from the platform interface
    if(TARGET PkgConfig::WaylandClient AND TARGET Qt6::GuiPrivate)
        target_compile_definitions(${PROJECT_NAME} PUBLIC ALLEGIANCE_STEREO_DECLARE)
        target_link_libraries(${PROJECT_NAME} PRIVATE StereoDeclare::stereo-declare PkgConfig::WaylandClient Qt6::GuiPrivate)
    endif()
endif()
add_feature_info(StereoDeclare "TARGET StereoDeclare::stereo-declare" "full side by side declared to the desktop without a stereo context")

add_library(
    KDAB::QtCommon ALIAS ${PROJECT_NAME}
)

# Schneider QML module
add_subdirectory(qml/Schneider)

# Main Module for Root QML File
qt_add_qml_module(
    ${PROJECT_NAME}
    URI
    common
    QML_FILES
    "qml/side_menu.qml"
    RESOURCE_PREFIX
    "/qt/qml" # Set to default prefix for elements to be in a know import path
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC Qt6::Gui
           Qt6::Widgets
           Qt6::QuickWidgets
           Qt6::QuickControls2
           shared
           KDAB_QT_Common_Schneider_QML # To access the Singleton Types
           KDAB_QT_Common_Schneider_QMLplugin # To allow QML Imports to work since plugin is static
)

# Avoid conflicts between keywords such as emit that also exist in KDBindings
target_compile_definitions(${PROJECT_NAME} PUBLIC QT_NO_KEYWORDS)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)
