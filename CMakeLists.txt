# For CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT.
cmake_minimum_required(VERSION 3.7)

project(librpimemmgr VERSION 4.1.1 LANGUAGES C
        DESCRIPTION "A memory manager for Raspberry Pi")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Yukimasa Sugizaki <ysugi@idein.jp>")
set(CPACK_GENERATOR DEB)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libdrm2, libraspberrypi-dev, libmailbox (>= 2.0.0)")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Install prefix" FORCE)
endif ()
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
message(STATUS "Install prefix is set to ${CMAKE_INSTALL_PREFIX}")

include(CPack)

find_package(PkgConfig)

pkg_check_modules(DRM REQUIRED libdrm)

pkg_check_modules(VCSM vcsm=1)
if (NOT VCSM_FOUND)
    message(STATUS "Adding /opt/vc/lib/pkgconfig to PKG_CONFIG_PATH")
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/vc/lib/pkgconfig")
    pkg_check_modules(VCSM vcsm=1)
    if (NOT VCSM_FOUND)
        message (FATAL_ERROR "vcsm is not found even in /opt/vc/lib. "
                             "Building on non-RPi host? "
                             "Please specify PKG_CONFIG_PATH.")
    endif ()
endif ()

pkg_check_modules(MAILBOX REQUIRED libmailbox>=2.0.0)

include(GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(test)

configure_file(librpimemmgr.pc.in librpimemmgr.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/librpimemmgr.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
