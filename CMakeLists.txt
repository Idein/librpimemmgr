cmake_minimum_required(VERSION 3.0)

project(librpimemmgr)
include(CTest)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_CONTACT "Sugizaki Yukimasa <ysugi@idein.jp>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A memory manager for Raspberry Pi")

# See https://semver.org/ and https://www.pakin.org/~scott/ltver.html .
set(CPACK_PACKAGE_VERSION_MAJOR 4)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)

if (CPACK_PACKAGING_INSTALL_PREFIX STREQUAL "")
    set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif ()
include(CPack)

find_package(PkgConfig)

pkg_check_modules(VCSM vcsm=1)
if (NOT VCSM_FOUND)
    message(STATUS "Adding /opt/vc/lib/pkgconfig to PKG_CONFIG_PATH")
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/vc/lib/pkgconfig")
    pkg_check_modules(VCSM vcsm=1)
    if (NOT VCSM_FOUND)
        message (FATAL_ERROR "vcsm not found even in /opt/vc/lib. "
                             "Building on non-RPi host? "
                             "Please specify PKG_CONFIG_PATH.")
    endif ()
endif ()

pkg_check_modules(MAILBOX REQUIRED libmailbox>=2.0.0)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(test)

configure_file(librpimemmgr.pc.in librpimemmgr.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/librpimemmgr.pc"
        DESTINATION lib/pkgconfig)
