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
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

typedef struct {
    uint32_t size;
    uint32_t flags;
    uint32_t handle;
    uint32_t offset;
} drm_v3d_create_bo;

typedef struct {
    uint32_t handle;
    uint32_t flags;
    uint64_t offset;
} drm_v3d_mmap_bo;

typedef struct {
    uint32_t  handle;
    uint32_t  pad;
} gem_close;

#define DRM_IOCTL_BASE   'd'
#define DRM_COMMAND_BASE 0x40
#define DRM_GEM_CLOSE    0x09

#define DRM_V3D_WAIT_BO    (DRM_COMMAND_BASE + 0x01)
#define DRM_V3D_CREATE_BO  (DRM_COMMAND_BASE + 0x02)
#define DRM_V3D_MMAP_BO    (DRM_COMMAND_BASE + 0x03)

#define IOCTL_GEM_CLOSE      _IOW(DRM_IOCTL_BASE, DRM_GEM_CLOSE, gem_close)
#define IOCTL_V3D_CREATE_BO  _IOWR(DRM_IOCTL_BASE, DRM_V3D_CREATE_BO, drm_v3d_create_bo)
#define IOCTL_V3D_MMAP_BO    _IOWR(DRM_IOCTL_BASE, DRM_V3D_MMAP_BO, drm_v3d_mmap_bo)

int alloc_mem_drm(const int fd_drm, const size_t size, uint32_t *handlep,
        uint32_t *busaddrp, void **usraddrp)
{
    const _Bool do_mapping = (usraddrp != NULL);

    uint32_t handle = 0;
    uint32_t busaddr = 0;
    void *usraddr = NULL;

    {
        drm_v3d_create_bo create_bo;
        create_bo.size = size;
        create_bo.flags = 0;
        int res = ioctl(fd_drm, IOCTL_V3D_CREATE_BO, &create_bo);
        if (res < 0) {
            print_error("Failed to allocate memory with DRM: %s\n", strerror(errno));
            return 1;
        }
        handle = create_bo.handle;
        busaddr = create_bo.offset;
    }

    if (do_mapping) {
        drm_v3d_mmap_bo mmap_bo;
        mmap_bo.handle = handle;
        mmap_bo.flags = 0;
        int res = ioctl(fd_drm, IOCTL_V3D_MMAP_BO, &mmap_bo);
        if (res < 0) {
            print_error("Failed to map DRM memory to userland: %s\n", strerror(errno));
            goto clean_alloc;
        }
        void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_drm, mmap_bo.offset);
        if (addr == MAP_FAILED) {
            print_error("Failed to map DRM memory to userland: %s\n", strerror(errno));
            goto clean_alloc;
        }
        usraddr = addr;
    }

    *handlep = handle;
    *busaddrp = busaddr;
    *usraddrp = usraddr;
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

    gem_close cl;
    cl.handle = handle;
    err = ioctl(fd_drm, IOCTL_GEM_CLOSE, &cl);
    if (err < 0) {
        print_error("Failed to free memory with DRM\n");
        err_sum = errno;
    }

    return err_sum;
}
