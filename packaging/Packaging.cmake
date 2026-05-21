# Release packaging for DISSCO (LASSIE GUI + CMOD CLI).
#
# Produces:
#   - macOS: a signed-by-default .dmg containing LASSIE.app (with CMOD
#     embedded in Contents/MacOS/) via CPack's DragNDrop generator. Qt
#     frameworks are bundled by an install(CODE) step that invokes
#     macdeployqt.
#   - Linux: an AppImage built by packaging/linux/build-appimage.sh, wired
#     up as a CMake custom target named `appimage` (not part of CPack,
#     since AppImage's tooling lives outside it).
#
# Trigger:
#   cmake --build build --target package        # macOS DMG (via CPack)
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
