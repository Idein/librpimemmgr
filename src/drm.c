/*
 * Copyright (c) 2019 Idein Inc. ( http://idein.jp/ )
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#include "rpimemmgr.h"
#include "local.h"
#include "v3d_drm.h"
#include <drm.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

int alloc_mem_drm(const int fd_drm, const size_t size, uint32_t *handlep,
        uint32_t *busaddrp, void **usraddrp)
{
    uint32_t handle = 0;
    uint32_t busaddr = 0;
    void* usraddr = NULL;

    {
        struct drm_v3d_create_bo create_bo = {
            .size = size,
            .flags = 0,
        };
        int res = ioctl(fd_drm, DRM_IOCTL_V3D_CREATE_BO, &create_bo);
        if (res < 0) {
            print_error("Failed to allocate memory with DRM: %s\n", strerror(errno));
            return 1;
        }
        handle = create_bo.handle;
        busaddr = create_bo.offset;
    }

    if (usraddrp != NULL) {
        struct drm_v3d_mmap_bo mmap_bo = {
            .handle = handle,
            .flags = 0,
        };
        int res = ioctl(fd_drm, DRM_IOCTL_V3D_MMAP_BO, &mmap_bo);
        if (res < 0) {
            print_error("Failed to map DRM memory to userland: %s\n", strerror(errno));
            goto clean_alloc;
        }
        usraddr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_drm, mmap_bo.offset);
        if (usraddr == MAP_FAILED) {
            print_error("Failed to map DRM memory to userland: %s\n", strerror(errno));
            goto clean_alloc;
        }
        *usraddrp = usraddr;
    }

    *handlep = handle;
    *busaddrp = busaddr;
    return 0;

clean_alloc:
    free_mem_drm(fd_drm, size, handle, usraddr);
    return 1;
}

int free_mem_drm(const int fd_drm, const size_t size, const uint32_t handle, void *usraddr)
{
    int err, err_sum = 0;

    if (usraddr != NULL) {
        err = munmap(usraddr, size);
        if (err) {
            print_error("munmap: %s\n", strerror(errno));
            err_sum = err;
            /* Continue finalization. */
        }
    }

    struct drm_gem_close gem_close = {
        .handle = handle,
    };
    err = ioctl(fd_drm, DRM_IOCTL_GEM_CLOSE, &gem_close);
    if (err < 0) {
        print_error("Failed to free memory with DRM\n");
        err_sum = errno;
    }

    return err_sum;
}
