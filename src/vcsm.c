/*
 * Copyright (c) 2018 Idein Inc. ( http://idein.jp/ )
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#include "rpimemmgr.h"
#include "local.h"

int alloc_mem_vcsm(const size_t size, size_t align,
        const VCSM_CACHE_TYPE_T cache_type, uint32_t *handlep,
        uint32_t *busaddrp, void **usraddrp)
{
    uint32_t handle, busaddr;
    void *usraddr;

    if (align > 4096) {
        print_error("Alignment %zu (>4096) is not supported by the driver\n",
                align);
        return 1;
    }
    if (align <= 0)
        align = 1;

    handle = vcsm_malloc_cache(size, cache_type, "rpimemmgr");
    if (!handle) {
        print_error("Failed to allocate memory with VCSM\n");
        return 1;
    }

    usraddr = vcsm_lock(handle);
    if (!usraddr) {
        print_error("Failed to lock VCSM memory\n");
        goto clean_alloc;
    }

    busaddr = vcsm_vc_addr_from_hdl(handle);
    if (!busaddr) {
        print_error("Failed to get bus addr from VCSM\n");
        goto clean_lock;
    }
    if (busaddr & (align-1)) {
        print_error("The VCSM driver did not align the memory. "
                "Contact the author of librpimemmgr.\n");
        goto clean_lock;
    }

    *handlep = handle;
    *busaddrp = busaddr;
    *usraddrp = usraddr;
    return 0;

clean_lock:
    (void) vcsm_unlock_ptr(usraddr);
clean_alloc:
    vcsm_free(handle);
    return 1;
}

int free_mem_vcsm(const uint32_t handle, void *usraddr)
{
    int err, err_sum = 0;

    err = vcsm_unlock_ptr(usraddr);
    if (err) {
        print_error("Failed to unlock memory with VCSM\n");
        err_sum = err;
        /* Continue finalization. */
    }

    vcsm_free(handle);

    return err_sum;
}
