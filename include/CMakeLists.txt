include(CheckLibraryExists)

configure_file(rpimemmgr.h.in ${CMAKE_CURRENT_BINARY_DIR}/rpimemmgr.h @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rpimemmgr.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
