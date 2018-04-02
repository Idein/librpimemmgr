set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra -pipe -O2 -g
                   ${BCM_HOST_CFLAGS} ${VCSM_CFLAGS} ${MAILBOX_CFLAGS}")
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(speed speed.c)
target_link_libraries(speed LINK_PUBLIC rpimemmgr ${BCM_HOST_LDFLAGS}
                                        ${VCSM_LDFLAGS} ${MAILBOX_LDFLAGS})
add_test(Speed speed)

add_executable(addr addr.c)
target_link_libraries(addr LINK_PUBLIC rpimemmgr ${BCM_HOST_LDFLAGS}
                                       ${VCSM_LDFLAGS} ${MAILBOX_LDFLAGS})
add_test(Addr addr)
