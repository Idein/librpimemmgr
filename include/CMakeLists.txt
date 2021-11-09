include(CheckLibraryExists)

set(CMAKE_REQUIRED_FLAGS ${VCSM_CFLAGS})
set(CMAKE_REQUIRED_LIBRARIES ${VCSM_LDFLAGS})
check_library_exists(vcsm vcsm_init_ex "${VCSM_LIBDIR}" RPIMEMMGR_VCSM_HAS_CMA)
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LIBRARIES)

configure_file(rpimemmgr.h.in ${CMAKE_CURRENT_BINARY_DIR}/rpimemmgr.h @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rpimemmgr.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
