cmake_minimum_required(VERSION 3.16)
project(raw-processor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Concurrent)

add_subdirectory(libraw)

qt_standard_project_setup()

qt_add_executable(raw-processor
    resources.qrc
    src/main.cpp
    src/core/utility.h
    src/core/utility.cpp
    src/core/image.h
    src/core/image.cpp
    src/processing/imageprocessor.h
    src/processing/imageprocessor.cpp
    src/processing/exporter.h
    src/processing/exporter.cpp

    src/core/adjustment.h
    src/core/adjustment.cpp
    src/core/adjustmentcell.h
    src/core/adjustmentcell.cpp
    src/core/adjustmentcelldata.h
    src/core/adjustmentcelldata.cpp
    src/core/adjustmentcellmanager.h
    src/core/adjustmentcellmanager.cpp

    src/ui/uiconstants.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/imageviewer.h
    src/ui/imageviewer.cpp
    src/ui/gallerywidget.h
    src/ui/gallerywidget.cpp
    src/ui/adjustmentcellwidget.h
    src/ui/adjustmentcellwidget.cpp
    src/ui/adjustmentpanelwidget.h
    src/ui/adjustmentpanelwidget.cpp
    src/ui/adjustmentcellmanagerwidget.h
    src/ui/adjustmentcellmanagerwidget.cpp
    src/ui/PieMenu.h
    src/ui/PieMenu.cpp

    src/core/thumbnailloader.h
    src/core/thumbnailloader.cpp
    src/core/imagemanager.h
    src/core/imagemanager.cpp

    src/core/stb_image.h

    QtAwesome/QtAwesome.h
    QtAwesome/QtAwesome.cpp
    QtAwesome/QtAwesomeAnim.h
    QtAwesome/QtAwesomeAnim.cpp
    QtAwesome/QtAwesomeEnumGenerated.h
    QtAwesome/QtAwesomeStringGenerated.h
)

target_include_directories(raw-processor PRIVATE src src/core src/processing src/ui QtAwesome)

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

set(RAW_PROCESSOR_PORTABLE_DIR "${CMAKE_BINARY_DIR}/portable-release")

install(TARGETS raw-processor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(TARGETS raw
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(WIN32)
    # Install OpenMP runtime DLL bundled with the MinGW toolchain.
    # libraw is compiled with OpenMP support, so libgomp-1.dll must ship next to the executable.
    get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    install(FILES "${_compiler_bin}/libgomp-1.dll" DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

qt_generate_deploy_app_script(
    TARGET raw-processor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

add_custom_target(portable-release
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${RAW_PROCESSOR_PORTABLE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RAW_PROCESSOR_PORTABLE_DIR}"
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${RAW_PROCESSOR_PORTABLE_DIR}"
    DEPENDS raw-processor
    USES_TERMINAL
    COMMENT "Creating portable release folder in ${RAW_PROCESSOR_PORTABLE_DIR}"
)
