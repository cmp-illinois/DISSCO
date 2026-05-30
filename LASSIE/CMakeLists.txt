cmake_minimum_required(VERSION 3.25)

project(LASSIE)
message(STATUS "== LASSIE ==")

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Xml Network)

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
    src/dialogs/functions/FunctionWidget.cpp src/dialogs/functions/FunctionWidget.hpp
    src/dialogs/functions/FunctionRegistry.cpp src/dialogs/functions/FunctionRegistry.hpp
    src/dialogs/functions/MultiEntryFunction.cpp src/dialogs/functions/MultiEntryFunction.hpp
    src/dialogs/functions/impl/InverseFunction.hpp
    src/dialogs/functions/impl/LNFunction.hpp
    src/dialogs/functions/impl/FibonacciFunction.hpp
    src/dialogs/functions/impl/ChooseLFunction.hpp
    src/dialogs/functions/impl/MarkovFunction.hpp
    src/dialogs/functions/impl/RandomFunction.hpp
    src/dialogs/functions/impl/RandomIntFunction.hpp
    src/dialogs/functions/impl/RandomOrderIntFunction.hpp
    src/dialogs/functions/impl/RandomizerFunction.hpp
    src/dialogs/functions/impl/RandomDensityFunction.hpp
    src/dialogs/functions/impl/EnvLibFunction.hpp
    src/dialogs/functions/impl/MakeListFunction.hpp
    src/dialogs/functions/impl/SpectrumGenFunction.hpp
    src/dialogs/functions/impl/ReadFileFunctions.hpp
    src/dialogs/functions/impl/StaticConstantFunctions.hpp
    src/dialogs/functions/impl/DecayFunction.cpp src/dialogs/functions/impl/DecayFunction.hpp
    src/dialogs/functions/impl/GetPatternFunction.cpp src/dialogs/functions/impl/GetPatternFunction.hpp
    src/dialogs/functions/impl/ExpandPatternFunction.cpp src/dialogs/functions/impl/ExpandPatternFunction.hpp
    src/dialogs/functions/impl/MakeFilterFunction.cpp src/dialogs/functions/impl/MakeFilterFunction.hpp
    src/dialogs/functions/impl/MakePatternFunction.hpp
    src/dialogs/functions/impl/MakeEnvelopeFunction.cpp src/dialogs/functions/impl/MakeEnvelopeFunction.hpp
    src/dialogs/functions/impl/MakeSieveFunction.cpp src/dialogs/functions/impl/MakeSieveFunction.hpp
    src/dialogs/functions/impl/ValuePickFunction.cpp src/dialogs/functions/impl/ValuePickFunction.hpp
    src/dialogs/functions/impl/SelectFunction.cpp src/dialogs/functions/impl/SelectFunction.hpp
    src/dialogs/functions/impl/StochosFunction.cpp src/dialogs/functions/impl/StochosFunction.hpp
    src/dialogs/functions/impl/SPAFunction.cpp src/dialogs/functions/impl/SPAFunction.hpp
    src/dialogs/functions/impl/RevFunctionBase.cpp src/dialogs/functions/impl/RevFunctionBase.hpp
    src/dialogs/functions/impl/RevFunctions.hpp
    src/dialogs/functions/FunctionRegistrations.cpp
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
    src/widgets/MakeEnvelopeRow.cpp src/widgets/MakeEnvelopeRow.hpp
    src/widgets/generic/FunctionEntryRow.cpp src/widgets/generic/FunctionEntryRow.hpp
    src/core/Updater.cpp src/core/Updater.hpp
)

target_precompile_headers(LASSIE PRIVATE <QtWidgets> <QtCore> <QtXml>)

target_link_libraries(LASSIE PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Xml
    Qt6::Network
    LASS
)
# Note: LASSIE invokes CMOD as a subprocess (see CMOD_BINARY below and the
# QProcess::start call in MainWindow::runProject). No symbols are shared, so
# CMOD is intentionally NOT in the link list. Linking against an executable
# target requires an import library on Windows, which CMOD doesn't produce
# (no dllexport / WINDOWS_EXPORT_ALL_SYMBOLS), and the dependency was never
# load-bearing anyway. LASS, which LASSIE was previously pulling in
# transitively through CMOD's PUBLIC interface, is now linked directly.
# LASSIE does its own XML handling with Qt's QtXml (QDomDocument), so it
# needs no separate XML library.

include(GNUInstallDirs)
# LASS include path comes from the LASS target.
target_include_directories(LASSIE PRIVATE
    ${CMAKE_SOURCE_DIR}/LASS/src
)

set(LASSIE_ICON_MACOS "${CMAKE_SOURCE_DIR}/packaging/macos/LASSIE.icns")
if(APPLE AND EXISTS "${LASSIE_ICON_MACOS}")
    target_sources(LASSIE PRIVATE "${LASSIE_ICON_MACOS}")
    set_source_files_properties("${LASSIE_ICON_MACOS}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

set_target_properties(LASSIE PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "LASSIE"
    MACOSX_BUNDLE_GUI_IDENTIFIER "edu.illinois.dissco.lassie"
    MACOSX_BUNDLE_BUNDLE_VERSION "${DISSCO_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${DISSCO_VERSION}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "DISSCO ${DISSCO_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) DISSCO authors. Licensed under GPL-3.0."
    MACOSX_BUNDLE_ICON_FILE "LASSIE.icns"
)

install(TARGETS LASSIE
    BUNDLE  DESTINATION . COMPONENT Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LASSIE)
endif()

target_compile_definitions(LASSIE PUBLIC "CMOD_BINARY=\"${CMOD_BINARY}\"")

# Version info — sourced from DISSCOVersions.txt via the root CMakeLists.
# Used by the in-app updater to compare against the latest GitHub release.
target_compile_definitions(LASSIE PRIVATE
    "DISSCO_VER_MAJOR=${DISSCO_VER_MAJOR}"
    "DISSCO_VER_MINOR=${DISSCO_VER_MINOR}"
    "DISSCO_VER_PATCH=${DISSCO_VER_PATCH}"
    "DISSCO_VERSION_STR=\"${DISSCO_VERSION}\""
)

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

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_COMMIT_SHA)
    set(GIT_COMMIT_SHA "unknown")
endif()
target_compile_definitions(LASSIE PRIVATE "GIT_COMMIT_SHA=\"${GIT_COMMIT_SHA}\"")

# Canonical upstream repo for the updater. Kept in sync with the URLs in
# packaging/Packaging.cmake (CPACK_NSIS_HELP_LINK / URL_INFO_ABOUT).
set(DISSCO_GITHUB_REPO "cmp-illinois/DISSCO-2.2.0")
target_compile_definitions(LASSIE PRIVATE "DISSCO_GITHUB_REPO=\"${DISSCO_GITHUB_REPO}\"")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(LASSIE PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

message(STATUS "DONE!")
