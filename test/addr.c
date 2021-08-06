/*
 * Copyright (c) 2018 Idein Inc. ( http://idein.jp/ )
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#include "rpimemmgr.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <inttypes.h>

static int test_vcsm_gpu(const VCSM_CACHE_TYPE_T cache_type)
{
    uint32_t busaddr;
    int err;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    err = rpimemmgr_alloc_vcsm(4096, 4096, cache_type, NULL, &busaddr, &st);
    if (err)
        return err;

    printf("busaddr=0x%08" PRIx32 "\n", busaddr);

    err = rpimemmgr_free_by_busaddr(busaddr, &st);
    if (err)
        return err;

    return rpimemmgr_finalize(&st);
}

#ifdef RPIMEMMGR_VCSM_HAS_CMA

static int test_vcsm_cma(const VCSM_CACHE_TYPE_T cache_type)
{
    uint32_t busaddr;
    int err;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    st.vcsm_use_cma = 1;

    err = rpimemmgr_alloc_vcsm(4096, 4096, cache_type, NULL, &busaddr, &st);
    if (err)
        return err;

    printf("busaddr=0x%08" PRIx32 "\n", busaddr);

    err = rpimemmgr_free_by_busaddr(busaddr, &st);
    if (err)
        return err;

    return rpimemmgr_finalize(&st);
}

#endif /* RPIMEMMGR_VCSM_HAS_CMA */

static int test_mailbox(const uint32_t flags)
{
    uint32_t busaddr;
    int err;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    err = rpimemmgr_alloc_mailbox(4096, 4096, flags, NULL, &busaddr, &st);
    if (err)
        return err;

    printf("busaddr=0x%08" PRIx32 "\n", busaddr);

    err = rpimemmgr_free_by_busaddr(busaddr, &st);
    if (err)
        return err;

    return rpimemmgr_finalize(&st);
}

static int test_drm()
{
    uint32_t busaddr;
    int err;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    err = rpimemmgr_alloc_drm(4096, NULL, &busaddr, &st);
    if (err)
        return err;

    printf("busaddr=0x%08" PRIx32 "\n", busaddr);

    err = rpimemmgr_free_by_busaddr(busaddr, &st);
    if (err)
        return err;

    return rpimemmgr_finalize(&st);
}

int main(void)
{
    struct rpimemmgr st;
    int processor;
    int err;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    processor = rpimemmgr_get_processor(&st);
    if (processor < 0)
        err = 1;

    err |= rpimemmgr_finalize(&st);
    if (err)
        return err;

    printf("VCSM (GPU): NONE:             ");
    err = test_vcsm_gpu(VCSM_CACHE_TYPE_NONE);
    if (err)
        return err;
    printf("VCSM (GPU): HOST:             ");
    err = test_vcsm_gpu(VCSM_CACHE_TYPE_HOST);
    if (err)
        return err;
    printf("VCSM (GPU): VC:               ");
    err = test_vcsm_gpu(VCSM_CACHE_TYPE_VC);
    if (err)
        return err;
    printf("VCSM (GPU): HOST_AND_VC:      ");
    err = test_vcsm_gpu(VCSM_CACHE_TYPE_HOST_AND_VC);
    if (err)
        return err;

#ifdef RPIMEMMGR_VCSM_HAS_CMA

    printf("VCSM (CMA): NONE:             ");
    err = test_vcsm_cma(VCSM_CACHE_TYPE_NONE);
    if (err)
        return err;
    printf("VCSM (CMA): HOST:             ");
    err = test_vcsm_cma(VCSM_CACHE_TYPE_HOST);
    if (err)
        return err;
    printf("VCSM (CMA): VC:               ");
    err = test_vcsm_cma(VCSM_CACHE_TYPE_VC);
    if (err)
        return err;
    printf("VCSM (CMA): HOST_AND_VC:      ");
    err = test_vcsm_cma(VCSM_CACHE_TYPE_HOST_AND_VC);
    if (err)
        return err;

#endif /* RPIMEMMGR_VCSM_HAS_CMA */

    printf("Mailbox:    NORMAL:           ");
    err = test_mailbox(MEM_FLAG_NORMAL);
    if (err)
        return err;
    printf("Mailbox:    DIRECT:           ");
    err = test_mailbox(MEM_FLAG_DIRECT);
    if (err)
        return err;
    printf("Mailbox:    COHERENT:         ");
    err = test_mailbox(MEM_FLAG_COHERENT);
    if (err)
        return err;
    printf("Mailbox:    L1_NONALLOCATING: ");
    err = test_mailbox(MEM_FLAG_L1_NONALLOCATING);
    if (err)
        return err;

    if (processor == 3) { /* BCM2711 */
        printf("DRM:                          ");
        err = test_drm();
        if (err)
            return err;
    }

    return 0;
}
