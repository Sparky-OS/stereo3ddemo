project(KDAB_Quick3D_Renderer)

# The phase 2 renderer: the scene through Qt Quick 3D in a QQuickWindow that is the 3D window; the edition's Qt
# Quick draws it once per eye on the quad-buffer path, and the window holds one View3D per eye on the declared pair
# path. The per-eye cameras, the frustum viewer, the picking and the cursor use Qt Quick 3D's private headers
# (qt6-quick3d-private-dev), as the edition pins one Qt Quick 3D.
find_package(
    Qt6
    6.8.2
    COMPONENTS Core
               Gui
               Qml
               Quick
               Quick3D
               Quick3DPrivate
    REQUIRED
    CONFIG
)

qt_add_library(${PROJECT_NAME} STATIC)
add_library(KDAB::Quick3DRenderer ALIAS ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC ALLEGIANCE_QUICK3D QT_NO_KEYWORDS)
if(ALLEGIANCE_DESKTOP_INSTALL)
    include(GNUInstallDirs)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ALLEGIANCE_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/stereo3ddemo")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/allegiance)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 AUTOMOC ON)

qt_add_qml_module(
    ${PROJECT_NAME}
    URI Stereo3D.Quick3D
    VERSION 1.0
    SOURCES
    quick3d_renderer.h
    quick3d_renderer.cpp
    frustum_geometry.h
    frustum_geometry.cpp
    focus_area.h
    focus_area.cpp
    QML_FILES
    "qml/StereoScene.qml"
    RESOURCE_PREFIX "/qt/qml"
    OUTPUT_TARGETS quick3d_qml_targets
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Quick3D shared
    PRIVATE Qt6::Quick3DPrivate
)
# The static QML module's plugin, linked by every program that uses the renderer (which imports it with
# Q_IMPORT_QML_PLUGIN(Stereo3D_Quick3DPlugin)), so the import works without a deployed module
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}plugin ${quick3d_qml_targets})
