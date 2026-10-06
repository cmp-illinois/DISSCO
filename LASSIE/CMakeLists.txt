cmake_minimum_required(VERSION 3.25)

project(LASSIE)
message(STATUS "== LASSIE ==")

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Core Xml Network)
if(WIN32 AND Qt6_VERSION VERSION_LESS "6.8.1")
    message(FATAL_ERROR
        "Windows builds require Qt 6.8.1 or newer. Qt 6.8.0 can substitute "
        "the wrong font family for the Windows system font.")
endif()

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
    src/core/ProjectXmlWriter.cpp src/core/ProjectXmlWriter.hpp
    src/core/RandomOrderIntId.cpp src/core/RandomOrderIntId.hpp
    src/core/CmodEnvironment.cpp src/core/CmodEnvironment.hpp
    src/core/ModifierUsageQtAdapter.cpp src/core/ModifierUsageQtAdapter.hpp
    src/dialogs/FunctionGenerator.cpp src/dialogs/FunctionGenerator.hpp src/ui/FunctionGenerator.ui
    src/dialogs/FunctionXmlFormat.cpp src/dialogs/FunctionXmlFormat.hpp
    src/dialogs/PartialModifierDialog.cpp src/dialogs/PartialModifierDialog.hpp
    src/dialogs/ModifierDetailsDialog.cpp src/dialogs/ModifierDetailsDialog.hpp
    src/dialogs/ModifierRulesDialog.cpp src/dialogs/ModifierRulesDialog.hpp
    src/dialogs/PartialModifierFormat.cpp src/dialogs/PartialModifierFormat.hpp
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
    src/dialogs/functions/impl/RandomOrderIntFunction.cpp src/dialogs/functions/impl/RandomOrderIntFunction.hpp
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
    src/widgets/NoteModifierSelection.cpp src/widgets/NoteModifierSelection.hpp
    src/dialogs/FileNewObject.cpp src/dialogs/FileNewObject.hpp src/ui/FileNewObject.ui
    src/widgets/LayerBox.cpp src/widgets/LayerBox.hpp
    src/widgets/Partials.cpp src/widgets/Partials.hpp
    src/widgets/Modifiers.cpp src/widgets/Modifiers.hpp src/ui/Modifiers.ui
    src/widgets/Stochos.cpp src/widgets/Stochos.hpp
    src/widgets/Select.cpp src/widgets/Select.hpp
    src/widgets/MakeEnvelopeRow.cpp src/widgets/MakeEnvelopeRow.hpp
    src/widgets/generic/FunctionEntryRow.cpp src/widgets/generic/FunctionEntryRow.hpp
    src/widgets/ComboBoxWheelGuard.cpp src/widgets/ComboBoxWheelGuard.hpp
    src/widgets/TextOverflowDisplayPolicy.cpp src/widgets/TextOverflowDisplayPolicy.hpp
    src/widgets/WindowShortcutPolicy.cpp src/widgets/WindowShortcutPolicy.hpp
    src/core/Updater.cpp src/core/Updater.hpp
)

target_precompile_headers(LASSIE PRIVATE <QtWidgets> <QtCore> <QtXml>)

target_link_libraries(LASSIE PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Xml
    Qt6::Network
    LASS
    DISSCO::ModifierUsage
)
# Note: LASSIE invokes the CMOD executable as a subprocess (see CMOD_BINARY
# below and the QProcess::start call in MainWindow::runProject), so the
# executable itself is intentionally NOT in the link list. Both executables
# instead share the Qt-free DISSCO::ModifierUsage library defined by CMOD.
# LASS, which LASSIE was previously pulling in transitively through CMOD's
# PUBLIC interface, is now linked directly.
# LASSIE does its own XML handling with Qt's QtXml (QDomDocument), so it
# needs no separate XML library.

include(GNUInstallDirs)
# LASS include path comes from the LASS target.
target_include_directories(LASSIE PRIVATE
    ${CMAKE_SOURCE_DIR}/LASS/src
)

if(NOT APPLE)
    qt_add_resources(LASSIE "app_icons"
        PREFIX "/icons"
        BASE "${CMAKE_SOURCE_DIR}/packaging/linux"
        FILES "${CMAKE_SOURCE_DIR}/packaging/linux/LASSIE.png"
    )
endif()

if(WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/packaging/windows/LASSIE.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/LASSIE.rc" @ONLY)
    target_sources(LASSIE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/LASSIE.rc")
endif()

set(LASSIE_ICON_MACOS "${CMAKE_SOURCE_DIR}/packaging/macos/LASSIE.icns")
if(APPLE AND EXISTS "${LASSIE_ICON_MACOS}")
    target_sources(LASSIE PRIVATE "${LASSIE_ICON_MACOS}")
    set_source_files_properties("${LASSIE_ICON_MACOS}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

set_target_properties(LASSIE PROPERTIES
    OUTPUT_NAME "lassie"
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

if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/LASSIE.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications" COMPONENT Runtime)
    install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/LASSIE.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps" COMPONENT Runtime)
endif()

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

if(BUILD_TESTING)
    target_compile_definitions(LASSIE PRIVATE DISSCO_ENABLE_UI_LAYOUT_TESTS)
    if(NOT APPLE)
        add_test(NAME LASSIE.ApplicationIdentity COMMAND LASSIE)
        set_tests_properties(LASSIE.ApplicationIdentity PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1;DISSCO_TEST_APPLICATION_IDENTITY=1"
            TIMEOUT 10
        )
    endif()
    if(WIN32)
        add_test(NAME LASSIE.WindowsSystemFont COMMAND LASSIE)
        set_tests_properties(LASSIE.WindowsSystemFont PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_FORCE_STDERR_LOGGING=1;DISSCO_TEST_WINDOWS_FONT=1"
            SKIP_RETURN_CODE 77
            TIMEOUT 10
        )
    endif()
    add_test(NAME LASSIE.EnvelopeLegendLayout
        COMMAND LASSIE
    )
    set_tests_properties(LASSIE.EnvelopeLegendLayout PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISSCO_TEST_ENVELOPE_LAYOUT=1"
        TIMEOUT 10
    )

    add_test(NAME LASSIE.ModifierAddRefreshesMoveButtons
        COMMAND LASSIE
    )
    set_tests_properties(LASSIE.ModifierAddRefreshesMoveButtons PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DISSCO_TEST_MODIFIER_ADD_REFRESH=1"
        TIMEOUT 10
    )

    # Tag every test registered in this folder (including future ones) with
    # the "LASSIE" label, so CI can run and report each part of DISSCO as its
    # own step (ctest -L '^LASSIE$'). -- Tony Chai, 2026
    get_property(_lassie_tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${_lassie_tests} PROPERTIES LABELS LASSIE)
endif()

message(STATUS "DONE!")
