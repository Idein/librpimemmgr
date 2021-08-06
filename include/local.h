/*
 * Copyright (c) 2018 Idein Inc. ( http://idein.jp/ )
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#ifndef RPIMEMMGR_LOCAL_H_
#define RPIMEMMGR_LOCAL_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <interface/vcsm/user-vcsm.h>

    /* vcsm.c */
    int alloc_mem_vcsm(const size_t size, size_t align,
            const VCSM_CACHE_TYPE_T cache_type, uint32_t *handlep,
            uint32_t *busaddrp, void **usraddrp);
    int free_mem_vcsm(const uint32_t handle, void *usraddr);

    /* mailbox.c */
    int get_processor_by_fd(const int fd_mb);
    int alloc_mem_mailbox(const int fd_mb, const int fd_mem, const size_t size,
            const size_t align, const uint32_t flags, uint32_t *handlep,
            uint32_t *busaddrp, void **usraddrp);
    int free_mem_mailbox(const int fd_mb, const size_t size,
            const uint32_t handle, const uint32_t busaddr, void *usraddr);

    /* drm.c */
    int alloc_mem_drm(const int fd_drm, const size_t size, uint32_t *handlep,
            uint32_t *busaddrp, void **usraddrp);
    int free_mem_drm(const int fd_drm, const size_t size, const uint32_t handle,
            void *usraddr);

#define print_error(fmt, ...) \
        do { \
            fprintf(stderr, "%s:%d:%s: error: " fmt, \
                    __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        } while (0)

#endif /* RPIMEMMGR_LOCAL_H_ */
