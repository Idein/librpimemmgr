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

static int test_vcsm(const VCSM_CACHE_TYPE_T cache_type, struct rpimemmgr *sp) {
    void *usraddr;
    uint32_t busaddr;

    usraddr = rpimemmgr_alloc_vcsm(4096, 4096, cache_type, &busaddr, sp);
    if (usraddr == NULL)
        return 1;

    printf("busaddr=0x%08" PRIx32 "\n", busaddr);

    return rpimemmgr_free(usraddr, sp);
}

static int test_mailbox(const uint32_t flags, struct rpimemmgr *sp)
{
    void *usraddr;
    uint32_t busaddr;

    usraddr = rpimemmgr_alloc_mailbox(4096, 4096, flags, &busaddr, sp);
    if (usraddr == NULL)
        return 1;

    printf("busaddr=0x%08" PRIx32 "\n", busaddr);

    return rpimemmgr_free(usraddr, sp);
}

int main(void)
{
    struct rpimemmgr st;
    int err;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    printf("VCSM:    NONE:             ");
    err = test_vcsm(VCSM_CACHE_TYPE_NONE,        &st);
    if (err)
        return err;
    printf("VCSM:    HOST:             ");
    err = test_vcsm(VCSM_CACHE_TYPE_HOST,        &st);
    if (err)
        return err;
    printf("VCSM:    VC:               ");
    err = test_vcsm(VCSM_CACHE_TYPE_VC,          &st);
    if (err)
        return err;
    printf("VCSM:    HOST_AND_VC:      ");
    err = test_vcsm(VCSM_CACHE_TYPE_HOST_AND_VC, &st);
    if (err)
        return err;

    /*
    printf("Mailbox: NORMAL:           ");
    err = test_mailbox(MEM_FLAG_NORMAL,           &st);
    if (err)
        return err;
    */
    printf("Mailbox: DIRECT:           ");
    err = test_mailbox(MEM_FLAG_DIRECT,           &st);
    if (err)
        return err;
    /*
    printf("Mailbox: COHERENT:         ");
    err = test_mailbox(MEM_FLAG_COHERENT,         &st);
    if (err)
        return err;
    */
    /*
    printf("Mailbox: L1_NONALLOCATING: ");
    err = test_mailbox(MEM_FLAG_L1_NONALLOCATING, &st);
    if (err)
        return err;
    */

    return rpimemmgr_finalize(&st);
}
