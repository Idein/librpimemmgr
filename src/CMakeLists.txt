set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -W -Wall -Wextra -pipe -O2 -g \
                   ${VCSM_CFLAGS} ${MAILBOX_CFLAGS}")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/include"
                           "${CMAKE_CURRENT_BINARY_DIR}/../include")

set(rpimemmgr_SOURCES rpimemmgr.c vcsm.c mailbox.c cache.c unif.c drm.c)
add_library(rpimemmgr        SHARED ${rpimemmgr_SOURCES})
add_library(rpimemmgr-static STATIC ${rpimemmgr_SOURCES})
set_target_properties(rpimemmgr-static PROPERTIES OUTPUT_NAME "rpimemmgr")

install(TARGETS rpimemmgr        LIBRARY DESTINATION lib)
install(TARGETS rpimemmgr-static ARCHIVE DESTINATION lib)
