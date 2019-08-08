set(RPIMEMMGR_INCLUDE_DIRS "${BCM_HOST_INCLUDE_DIRS}" "${VCSM_INCLUDE_DIRS}"
                           "${MAILBOX_INCLUDE_DIRS}")
set(RPIMEMMGR_CFLAGS_OTHER "${BCM_HOST_CFLAGS_OTHER}" "${VCSM_CFLAGS_OTHER}"
                           "${MAILBOX_CFLAGS_OTHER}")
set(RPIMEMMGR_LDFLAGS "${BCM_HOST_LDFLAGS}" "${VCSM_LDFLAGS}"
                      "${MAILBOX_LDFLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -W -Wall -Wextra -O2 -g")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/include"
                           "${CMAKE_CURRENT_BINARY_DIR}/../include")

add_executable(speed speed.c)
target_include_directories(speed PRIVATE "${RPIMEMMGR_INCLUDE_DIRS}")
target_compile_options(speed PRIVATE "${RPIMEMMGR_CFLAGS_OTHER}")
target_link_libraries(speed rpimemmgr "${RPIMEMMGR_LDFLAGS}")
add_test(Speed speed)

add_executable(addr addr.c)
target_include_directories(addr PRIVATE "${RPIMEMMGR_INCLUDE_DIRS}")
target_compile_options(addr PRIVATE "${RPIMEMMGR_CFLAGS_OTHER}")
target_link_libraries(addr rpimemmgr "${RPIMEMMGR_LDFLAGS}")
add_test(Addr addr)
