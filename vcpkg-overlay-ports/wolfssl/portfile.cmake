vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO wolfssl/wolfssl
    REF "v${VERSION}-stable"
    SHA512 b3953692a87aada84d77a26aac3ee1791344af3cf6e3d0b4fa9913095bc0892dd4cfe1491a893b469469bdfba511fe067ee80d3c0beab8df5ac5e174fa5f5577
    HEAD_REF master
    PATCHES
)

vcpkg_cmake_get_vars(cmake_vars_file)
include("${cmake_vars_file}")

set(LOCAL_C_FLAGS_RELEASE "${VCPKG_COMBINED_C_FLAGS_RELEASE}")
set(LOCAL_C_FLAGS_DEBUG "${VCPKG_COMBINED_C_FLAGS_DEBUG}")

if(VCPKG_TARGET_IS_LINUX)
  message(STATUS "Applying workaround for wolfSSL stringop-overflow warning on Linux.")
  set(LOCAL_C_FLAGS_RELEASE "${LOCAL_C_FLAGS_RELEASE} -Wno-error=stringop-overflow")
  set(LOCAL_C_FLAGS_DEBUG "${LOCAL_C_FLAGS_DEBUG} -Wno-error=stringop-overflow")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
      -DWOLFSSL_BUILD_OUT_OF_TREE=yes
      -DWOLFSSL_EXAMPLES=no
      -DWOLFSSL_CRYPT_TESTS=no
      -DWOLFSSL_CURL=yes
    OPTIONS_RELEASE
      -DCMAKE_C_FLAGS=${LOCAL_C_FLAGS_RELEASE}
    OPTIONS_DEBUG
      -DCMAKE_C_FLAGS=${LOCAL_C_FLAGS_DEBUG}
      -DWOLFSSL_DEBUG=yes
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/wolfssl)

if(VCPKG_TARGET_IS_IOS OR VCPKG_TARGET_IS_OSX)
  vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/lib/pkgconfig/wolfssl.pc" "Libs.private: " "Libs.private: -framework CoreFoundation -framework Security ")
  if(NOT VCPKG_BUILD_TYPE)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/wolfssl.pc" "Libs.private: " "Libs.private: -framework CoreFoundation -framework Security ")
  endif()
endif()
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
