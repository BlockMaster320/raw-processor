cmake_minimum_required(VERSION 3.16)
project(raw-processor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Concurrent)

add_subdirectory(libraw)

qt_standard_project_setup()

qt_add_executable(raw-processor
    main.cpp
    utility.h
    utility.cpp
    image.h
    image.cpp
    imageprocessor.h
    imageprocessor.cpp

    adjustment.h
    adjustment.cpp
    adjustmentcell.h
    adjustmentcell.cpp

    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/imageviewer.h
    ui/imageviewer.cpp
    ui/gallerywidget.h
    ui/gallerywidget.cpp
    ui/adjustmentcellwidget.h
    ui/adjustmentcellwidget.cpp
    ui/adjustmentpanelwidget.h
    ui/adjustmentpanelwidget.cpp
    ui/PieMenu.h
    ui/PieMenu.cpp

    thumbnailloader.h
    thumbnailloader.cpp
    imagemanager.h
    imagemanager.cpp

    stb_image.h
)

target_link_libraries(raw-processor
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
        Qt6::Concurrent
        raw
)

# target_include_directories(code PRIVATE ${Qt6Core_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS})

set_target_properties(raw-processor PROPERTIES
    #WIN32_EXECUTABLE ON # this prevents a console window from appearing when running the application
    MACOSX_BUNDLE ON
)

include(GNUInstallDirs)

install(TARGETS raw-processor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET raw-processor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})