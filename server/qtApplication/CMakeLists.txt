cmake_minimum_required(VERSION 3.16)

project(qtApplication VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(qtApplication
    src/main.cpp
    inc/mainWindow.hpp
    src/mainWindow.cpp
    inc/topBar.hpp
    src/topBar.cpp
    inc/sideBar.hpp
    src/sideBar.cpp
)

target_include_directories(qtApplication PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

qt_add_qml_module(qtApplication
    URI qtApplication
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

qt_add_resources(qtApplication "assets"
    PREFIX "/"
    FILES assets/icons/cat.png
)

set_target_properties(qtApplication PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
) 

target_link_libraries(qtApplication
    PRIVATE Qt6::Quick
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

include(GNUInstallDirs)
install(TARGETS qtApplication
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
