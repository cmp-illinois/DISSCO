cmake_minimum_required(VERSION 3.25)

project(LASSIE)
message(STATUS "== LASSIE ==")

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Xml)
find_package(XercesC REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

### This will need to be updated as we add more windows/classes
qt_add_executable(LASSIE
    src/main.cpp
    src/windows/MainWindow.cpp src/windows/MainWindow.hpp src/ui/mainwindow.ui
    src/windows/MarkovModelLibraryWindow.cpp src/windows/MarkovModelLibraryWindow.hpp
    src/windows/EnvelopeLibraryWindow.cpp src/windows/EnvelopeLibraryWindow.hpp 
    src/windows/PostWindow.cpp src/windows/PostWindow.hpp 
    src/widgets/EnvLibDrawingArea.cpp src/widgets/EnvLibDrawingArea.hpp 
    src/core/EnvelopeLibraryEntry.cpp src/core/EnvelopeLibraryEntry.hpp
    src/dialogs/FunctionGenerator.cpp src/dialogs/FunctionGenerator.hpp src/ui/FunctionGenerator.ui
    src/widgets/REVChannel.cpp src/widgets/REVChannel.hpp
    src/widgets/SPAChannel.cpp src/widgets/SPAChannel.hpp
    src/dialogs/ProjectPropertiesDialog.cpp src/dialogs/ProjectPropertiesDialog.hpp src/ui/ProjectPropertiesDialog.ui
    src/inst.hpp
    src/widgets/ProjectViewController.cpp src/widgets/ProjectViewController.hpp
    src/core/project_struct.cpp src/core/project_struct.hpp
    src/widgets/PaletteViewController.cpp src/widgets/PaletteViewController.hpp
    src/widgets/EventAttributesViewController.cpp src/widgets/EventAttributesViewController.hpp src/ui/Attributes.ui
    src/dialogs/FileNewObject.cpp src/dialogs/FileNewObject.hpp src/ui/FileNewObject.ui
    src/widgets/LayerBox.cpp src/widgets/LayerBox.hpp
    src/widgets/Partials.cpp src/widgets/Partials.hpp
    src/widgets/Modifiers.cpp src/widgets/Modifiers.hpp src/ui/Modifiers.ui
    src/widgets/Stochos.cpp src/widgets/Stochos.hpp
    src/widgets/Select.cpp src/widgets/Select.hpp
    src/widgets/generic/FunctionEntryRow.cpp src/widgets/generic/FunctionEntryRow.hpp
)

target_precompile_headers(LASSIE PRIVATE <QtWidgets> <QtCore> <QtXml>)

target_link_libraries(LASSIE PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Xml
    LASS
    XercesC::XercesC
)
# Note: LASSIE invokes CMOD as a subprocess (see CMOD_BINARY below and the
# QProcess::start call in MainWindow::runProject). No symbols are shared, so
# CMOD is intentionally NOT in the link list. Linking against an executable
# target requires an import library on Windows, which CMOD doesn't produce
# (no dllexport / WINDOWS_EXPORT_ALL_SYMBOLS), and the dependency was never
# load-bearing anyway. LASS and XercesC, which LASSIE was previously pulling
# in transitively through CMOD's PUBLIC interface, are now linked directly.

include(GNUInstallDirs)
# XercesC include path comes from the XercesC::XercesC imported target's
# INTERFACE_INCLUDE_DIRECTORIES; LASS comes from the LASS target.
target_include_directories(LASSIE PRIVATE
    ${CMAKE_SOURCE_DIR}/LASS/src
)

set_target_properties(LASSIE PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

install(TARGETS LASSIE
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LASSIE)
endif()

target_compile_definitions(LASSIE PUBLIC "CMOD_BINARY=\"${CMOD_BINARY}\"")

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_BRANCH)
    set(GIT_BRANCH "unknown")
endif()
target_compile_definitions(LASSIE PRIVATE "GIT_BRANCH=\"${GIT_BRANCH}\"")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(LASSIE PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

message(STATUS "DONE!")
