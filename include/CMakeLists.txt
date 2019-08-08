include(CheckLibraryExists)
check_library_exists(vcsm vcsm_init_ex "${VCSM_LIBDIR}" RPIMEMMGR_VCSM_HAS_CMA)
configure_file(rpimemmgr.h.in "${CMAKE_CURRENT_BINARY_DIR}/rpimemmgr.h" @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rpimemmgr.h" DESTINATION include)
