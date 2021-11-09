include(CTest)

if (NOT BUILD_TESTING)
    return()
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_CURRENT_BINARY_DIR}/../include)
add_compile_options(-pipe -W -Wall -Wextra -O2 -g)

foreach (test IN ITEMS addr speed)
    add_executable(${test} ${test}.c)
    target_include_directories(${test} PUBLIC ${VCSM_INCLUDE_DIRS}
                                              ${MAILBOX_INCLUDE_DIRS})
    target_compile_options(${test} PUBLIC ${VCSM_CFLAGS_OTHER}
                                          ${MAILBOX_CFLAGS_OTHER})
    target_link_libraries(${test} rpimemmgr ${VCSM_LDFLAGS} ${MAILBOX_LDFLAGS})
    add_test(${test} ${test})
endforeach ()
