# Release packaging for DISSCO (LASSIE GUI + CMOD CLI).
#
# Produces:
#   - macOS: a .dmg containing LASSIE.app (with CMOD embedded in
#     Contents/MacOS/) via CPack's DragNDrop generator. Qt frameworks are
#     bundled by an install(CODE) step that invokes macdeployqt.
#   - Windows: an NSIS installer .exe (LASSIE.exe + CMOD.exe + Qt DLLs)
#     via CPack's NSIS generator. Qt DLLs are bundled by an install(CODE)
#     step that invokes windeployqt. The installer writes registry entries
#     under HKLM\Software\Classes for the .dissco extension; these
#     complement the per-user HKCU writes done by LASSIE's runtime
#     file-association helper (see LASSIE/src/win/file_association.cpp,
#     which lives on the file-association branch).
#   - Linux: an AppImage built by packaging/linux/build-appimage.sh, wired
#     up as a CMake custom target named `appimage` (not part of CPack,
#     since AppImage's tooling lives outside it).
#
# Trigger:
#   cmake --build build --target package        # macOS DMG / Windows .exe
#   cmake --build build --target appimage       # Linux AppImage

set(CPACK_PACKAGE_NAME "DISSCO")
set(CPACK_PACKAGE_VENDOR "DISSCO Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "DISSCO: composition and sound design environment (LASSIE GUI + CMOD).")
set(CPACK_PACKAGE_VERSION_MAJOR "${DISSCO_VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${DISSCO_VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${DISSCO_VER_PATCH}")
set(CPACK_PACKAGE_VERSION "${DISSCO_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "DISSCO-${DISSCO_VERSION}-${CMAKE_SYSTEM_NAME}")

if(APPLE)
    # macdeployqt lives in Qt6's bin directory; ask the imported target where
    # that is so we don't depend on PATH.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    set(MACDEPLOYQT_EXECUTABLE "${_qt_bin_dir}/macdeployqt")

    # Bundle Qt frameworks into the installed LASSIE.app. Runs at `cmake
    # --install` time (and therefore during CPack), after the executable and
    # CMOD have been copied in.
    #
    # macdeployqt prints alarming-looking "ERROR: Cannot resolve rpath" lines
    # for weak-linked Qt modules (QtPdf, QtSvg, QtVirtualKeyboard*) that
    # aren't installed via brew's qt@6 and that LASSIE doesn't actually use.
    # These are non-fatal: macdeployqt still exits 0 and the bundle is
    # valid. We capture output and drop those known-benign lines so real
    # problems remain visible.
    install(CODE "
        message(STATUS \"Running macdeployqt on \${CMAKE_INSTALL_PREFIX}/LASSIE.app\")
        execute_process(
            COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                \"\${CMAKE_INSTALL_PREFIX}/LASSIE.app\"
                -always-overwrite
            RESULT_VARIABLE _mdq_result
            OUTPUT_VARIABLE _mdq_out
            ERROR_VARIABLE _mdq_err
        )
        set(_mdq_combined \"\${_mdq_out}\${_mdq_err}\")
        string(REGEX REPLACE \"(ERROR: Cannot resolve rpath \\\"@rpath/Qt(Pdf|Svg|VirtualKeyboard[A-Za-z]*)\\\\.framework[^\\n]*\\n)\" \"\" _mdq_filtered \"\${_mdq_combined}\")
        string(REGEX REPLACE \"(ERROR:  using QList[^\\n]*\\n)\" \"\" _mdq_filtered \"\${_mdq_filtered}\")
        if(_mdq_filtered)
            message(\"\${_mdq_filtered}\")
        endif()
        if(NOT _mdq_result EQUAL 0)
            message(FATAL_ERROR \"macdeployqt failed with exit code \${_mdq_result}\")
        endif()
    " COMPONENT Runtime)

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "DISSCO ${DISSCO_VERSION}")
    set(CPACK_DMG_FORMAT "UDZO")
    if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/macos/LASSIE.icns")
        set(CPACK_DMG_VOLUME_ICON "${CMAKE_SOURCE_DIR}/packaging/macos/LASSIE.icns")
    endif()
endif()

if(WIN32)
    # windeployqt: walks LASSIE.exe's PE imports and copies the matching Qt
    # DLLs (and platform plugins, styles, etc.) into the same directory.
    # We pull the Qt6::windeployqt imported target's path so the path
    # tracks whatever Qt the build is configured against, not whatever
    # happens to be on PATH.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        HINTS "${_qt_bin_dir}"
        REQUIRED
    )

    # Run windeployqt against the installed LASSIE.exe. Crucially this is a
    # separate invocation from any build-time windeployqt (e.g. the one in
    # LASSIE/CMakeLists.txt's POST_BUILD command, which targets the build
    # dir for dev convenience): the installer needs DLLs in CMAKE_INSTALL_PREFIX,
    # not the build dir.
    install(CODE "
        message(STATUS \"Running windeployqt on \${CMAKE_INSTALL_PREFIX}/bin/LASSIE.exe\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                --verbose 0
                --no-compiler-runtime
                --no-translations
                --no-system-d3d-compiler
                --no-quick-import
                \"\${CMAKE_INSTALL_PREFIX}/bin/LASSIE.exe\"
            RESULT_VARIABLE _wdq_result
        )
        if(NOT _wdq_result EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with exit code \${_wdq_result}\")
        endif()
    " COMPONENT Runtime)

    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "DISSCO ${DISSCO_VERSION}")
    set(CPACK_NSIS_DISPLAY_NAME "DISSCO ${DISSCO_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/cmp-illinois/DISSCO-2.2.0")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/cmp-illinois/DISSCO-2.2.0")
    set(CPACK_NSIS_CONTACT "https://github.com/cmp-illinois/DISSCO-2.2.0/issues")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "DISSCO ${DISSCO_VERSION}")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")

    # Start Menu and desktop shortcuts pointing at LASSIE.exe.
    set(CPACK_PACKAGE_EXECUTABLES "LASSIE;LASSIE")
    set(CPACK_CREATE_DESKTOP_LINKS "LASSIE")

    if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/windows/LASSIE.ico")
        # NSIS wants Windows-native backslashes in these paths.
        file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/windows/LASSIE.ico" _nsis_icon)
        string(REPLACE "\\" "\\\\" _nsis_icon "${_nsis_icon}")
        set(CPACK_NSIS_MUI_ICON "${_nsis_icon}")
        set(CPACK_NSIS_MUI_UNIICON "${_nsis_icon}")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\LASSIE.exe")
    endif()

    # File-association registry entries.
    #
    # Three layers describe the same `DISSCO.Project` ProgID, picked up by
    # Explorer in priority order:
    #
    #   1. Installer (this block) — HKLM, system-wide, written here at
    #      install time. Makes .dissco files openable immediately, no
    #      LASSIE launch required, benefits other users on the machine.
    #   2. Runtime helper — HKCU, per-user, written by LASSIE on first
    #      launch (LASSIE/src/win/file_association.cpp on the
    #      file-association branch). Naturally overrides HKLM per user;
    #      "last Release LASSIE launched wins" between installs / dev
    #      builds.
    #   3. Manual fallback — packaging/windows/dissco-association.reg.
    #      Documents the exact HKCU layout (1) and (2) write, for users
    #      doing scripted/offline self-install. Not wired into the build.
    #
    # The HKLM commands below are kept inline (not generated from the
    # .reg) so they can use $INSTDIR; the .reg file remains the canonical
    # human-readable reference.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKLM 'Software\\\\Classes\\\\.dissco' '' 'DISSCO.Project'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project' '' 'DISSCO Project'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project' 'FriendlyTypeName' 'DISSCO Project'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project\\\\DefaultIcon' '' '$INSTDIR\\\\bin\\\\LASSIE.exe,0'
        WriteRegStr HKLM 'Software\\\\Classes\\\\DISSCO.Project\\\\shell\\\\open\\\\command' '' '\\\"$INSTDIR\\\\bin\\\\LASSIE.exe\\\" \\\"%1\\\"'
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKLM 'Software\\\\Classes\\\\DISSCO.Project'
        DeleteRegKey HKLM 'Software\\\\Classes\\\\.dissco'
        System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
    ")
endif()

if(UNIX AND NOT APPLE)
    # AppImage is produced by linuxdeploy + linuxdeploy-plugin-qt operating on
    # a staged AppDir. We delegate to a shell script so the same recipe can be
    # run by hand or by CI without needing CMake. The script consumes an
    # already-`cmake --install`-ed tree.
    add_custom_target(appimage
        COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
                --prefix "${CMAKE_BINARY_DIR}/AppDir/usr"
                --component Runtime
        COMMAND ${CMAKE_COMMAND} -E env
                DISSCO_VERSION=${DISSCO_VERSION}
                APPDIR=${CMAKE_BINARY_DIR}/AppDir
                OUTPUT_DIR=${CMAKE_BINARY_DIR}
                SOURCE_DIR=${CMAKE_SOURCE_DIR}
                bash "${CMAKE_SOURCE_DIR}/packaging/linux/build-appimage.sh"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building DISSCO AppImage"
        VERBATIM
    )
endif()

include(CPack)
