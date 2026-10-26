project(KDAB_Qt_Qt3D_OpenGL)

find_package(
    Qt6
    6.8.2
    COMPONENTS Gui
               Widgets
               3DExtras
               3DCore
               3DRender
               3DInput
               Qml
               Quick
               QuickWidgets
               QuickControls2
    REQUIRED
    CONFIG
)
find_package(Qt6 6.8.2 COMPONENTS LinguistTools QUIET)
find_package(Qt6 6.8.2 COMPONENTS GuiPrivate QUIET) # KDAB::QtCommon reads the window's native surface through it

qt_standard_project_setup()
qt_add_executable(${PROJECT_NAME} main.cpp)

# The strings the stereo version adds, in the languages Sparky Stereo OS ships; the .qm files are embedded
if(TARGET Qt6::lrelease)
    file(GLOB ALLEGIANCE_TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/stereo3ddemo_*.ts)
    qt_add_translations(${PROJECT_NAME} TS_FILES ${ALLEGIANCE_TS_FILES} RESOURCE_PREFIX "/i18n")
    target_compile_definitions(${PROJECT_NAME} PRIVATE ALLEGIANCE_TRANSLATIONS)
endif()

# Fake QML Module to have qmlimportscanner correctly deploy the Qt QML plugins
# when packaging (we don't need to deploy our own modules since those are static)
qt_add_qml_module(
    ${PROJECT_NAME}
    URI
    imports
    QML_FILES
    "imports.qml"
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC Qt6::Widgets KDAB::QtCommon KDAB::Qt3DRenderer
)
# The Qt Quick 3D renderer in the same program, chosen under Advanced
if(BUILD_QUICK3D_RENDERER)
    target_link_libraries(${PROJECT_NAME} PUBLIC KDAB::Quick3DRenderer)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::GuiPrivate)
    qt_import_qml_plugins(${PROJECT_NAME})
endif()

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES CXX_STANDARD 20
               AUTOMOC ON
               AUTORCC ON
               RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

install(
    TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT applications
)

if(ALLEGIANCE_DESKTOP_INSTALL)
    include(GNUInstallDirs)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME stereo3ddemo)
    install(FILES stereo3ddemo.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications COMPONENT applications)
    install(FILES stereo3ddemo.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps COMPONENT applications)
endif()

# A standalone bundle deploys Qt's QML modules beside the program; a desktop installation uses the system's
if(NOT ALLEGIANCE_DESKTOP_INSTALL)
    qt_generate_deploy_qml_app_script(
        TARGET
        ${PROJECT_NAME}
        OUTPUT_SCRIPT
        deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
    )

    install(
        SCRIPT ${deploy_script}
        COMPONENT applications
    )
endif()
