cmake_minimum_required(VERSION 3.25)

project(LASSIE)
message(STATUS "== LASSIE ==")

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Xml)

if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(_lassie_claim_default ON)
else()
    set(_lassie_claim_default OFF)
endif()
option(LASSIE_CLAIM_DISSCO
    "Register LASSIE as the OS-level handler for .dissco files (Info.plist on macOS, .desktop+mime on Linux). Off by default for non-release builds to avoid worktree/debug builds fighting over the file association."
    ${_lassie_claim_default})
message(STATUS "LASSIE_CLAIM_DISSCO = ${LASSIE_CLAIM_DISSCO}")

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
    src/win/file_association.cpp src/win/file_association.hpp
)

target_precompile_headers(LASSIE PRIVATE <QtWidgets> <QtCore> <QtXml>)

target_link_libraries(LASSIE PRIVATE 
    Qt6::Widgets
    Qt6::Core
    Qt6::Xml
    CMOD
)

include(GNUInstallDirs)
target_include_directories(LASSIE PRIVATE
    ${XercesC_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/LASS/src
    ${CMAKE_SOURCE_DIR}/CMOD/src
)

set_target_properties(LASSIE PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "LASSIE"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.dissco.LASSIE"
    MACOSX_BUNDLE_BUNDLE_VERSION "${DISSCO_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${DISSCO_VERSION}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${DISSCO_VERSION}"
)

if(APPLE AND LASSIE_CLAIM_DISSCO)
    set_target_properties(LASSIE PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/packaging/macos/MacOSXBundleInfo.plist.in"
    )
endif()

if(LASSIE_CLAIM_DISSCO)
    target_compile_definitions(LASSIE PRIVATE LASSIE_CLAIM_DISSCO)
endif()

if(WIN32)
    # SHChangeNotify (shell32) + Reg* (advapi32) — usually pulled in implicitly,
    # but list them so non-default Windows toolchains link cleanly.
    target_link_libraries(LASSIE PRIVATE shell32 advapi32)
endif()

install(TARGETS LASSIE
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(LINUX AND LASSIE_CLAIM_DISSCO)
    # .in templates live in the project-root packaging/ tree. configure_file
    # drops the .in suffix and lets us substitute @VAR@ if we ever need to
    # (right now neither file uses substitutions, but the .in convention
    # signals "this is a generated install artifact, edit the template").
    configure_file(${CMAKE_SOURCE_DIR}/packaging/linux/LASSIE.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/LASSIE.desktop @ONLY)
    configure_file(${CMAKE_SOURCE_DIR}/packaging/linux/dissco-mime.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/dissco-mime.xml @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LASSIE.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dissco-mime.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages)
endif()

# Windows has no CMake-installed packaging asset — LASSIE.exe self-registers
# via the helper in src/win/file_association.cpp. The .reg under
# packaging/windows/ is a manual-install fallback users can edit + import; it
# isn't wired into the build.

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
