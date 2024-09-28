vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
OUT_SOURCE_PATH SOURCE_PATH
REPO ocornut/imgui
REF 29cff2be06860cadbc7557cc64f29159468213e2
SHA512 45f3f9f7589faa85d37b4c23b512ef93366ec63af6bb43f19c94fc43561c1d34ef87ad33cabde82819d21bdfe785cff0dd31b4b2e422d9cab6fa45b104860a5c
HEAD_REF master
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/imgui-config.cmake.in" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES 
    dx11-binding                IMGUI_BUILD_DX11_BINDING
    win32-binding               IMGUI_BUILD_WIN32_BINDING
    freetype                    IMGUI_FREETYPE
    wchar32                     IMGUI_USE_WCHAR32
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
    OPTIONS_DEBUG
        -DIMGUI_SKIP_HEADERS=ON
)

vcpkg_cmake_install()

if ("freetype" IN_LIST FEATURES)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/imconfig.h" "//#define IMGUI_ENABLE_FREETYPE" "#define IMGUI_ENABLE_FREETYPE")
endif()
if ("wchar32" IN_LIST FEATURES)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/imconfig.h" "//#define IMGUI_USE_WCHAR32" "#define IMGUI_USE_WCHAR32")
endif()

vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
