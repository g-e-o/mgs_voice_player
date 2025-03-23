cmake_minimum_required(VERSION 3.16)

project(vox_editor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets Multimedia)

qt_standard_project_setup(REQUIRES 6.5)

set_source_files_properties(Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(appvox_editor
    main.cpp
    app_icon.rc
)

qt_add_resources(appvox_editor resources.qrc)

qt_add_qml_module(appvox_editor
    URI vox_editor
    VERSION 1.0
    SOURCES
        VagConverter.h
        VagConverter.cpp
        VoxManager.h
        VoxManager.cpp
    QML_FILES
        Main.qml
        VoxPlayer.qml
        widgets/CodecChara.qml

    RESOURCES
        resources.qrc
        SOURCES font.h

)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appvox_editor PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appvox_editor
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appvox_editor
    PRIVATE Qt6::Quick Qt6::Widgets Qt6::Multimedia
)

include(GNUInstallDirs)
install(TARGETS appvox_editor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
