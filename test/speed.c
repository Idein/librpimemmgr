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

static double get_time(void)
{
    struct timespec t;
    (void) clock_gettime(CLOCK_MONOTONIC, &t);
    return (double) t.tv_sec + t.tv_nsec * 1e-9;
}

static void test_speed_copy(const size_t size, void *dst, void *src)
{
    double start, end, elapsed;
    unsigned i;
    const unsigned n_warmup = 16, n_measure = 16;
    /* const unsigned n_cooldown = ...; */

    for (i = 0; i < n_warmup; i ++)
        (void) memcpy(dst, src, size);

    start = get_time();
    for (i = 0; i < n_measure; i ++)
        (void) memcpy(dst, src, size);
    end = get_time();

    elapsed = (end - start) / n_measure;
    printf("%e [s], %e [B/s]\n", elapsed, size / elapsed);
}

static int test_vcsm(const size_t size, const VCSM_CACHE_TYPE_T cache_type,
        struct rpimemmgr *sp)
{
    void *dst, *src;
    int err_sum = 0;

    dst = rpimemmgr_alloc_vcsm(size, 4096, cache_type, NULL, sp);
    if (dst == NULL)
        return 1;

    src = rpimemmgr_alloc_vcsm(size, 4096, cache_type, NULL, sp);
    if (src == NULL)
        return 1;

    test_speed_copy(size, dst, src);

    err_sum |= rpimemmgr_free(src, sp);
    err_sum |= rpimemmgr_free(dst, sp);
    return err_sum;
}

static int test_mailbox(const size_t size, const uint32_t flags,
        struct rpimemmgr *sp)
{
    void *dst, *src;
    int err_sum = 0;

    dst = rpimemmgr_alloc_mailbox(size, 4096, flags, NULL, sp);
    if (dst == NULL)
        return 1;

    src = rpimemmgr_alloc_mailbox(size, 4096, flags, NULL, sp);
    if (src == NULL)
        return 1;

    test_speed_copy(size, dst, src);

    err_sum |= rpimemmgr_free(src, sp);
    err_sum |= rpimemmgr_free(dst, sp);
    return err_sum;
}

int main(void)
{
    const size_t size = 1UL<<24; /* 16 MiB */
    struct rpimemmgr st;
    int err;

    err = rpimemmgr_init(&st);
    if (err)
        return err;

    printf("VCSM:    NONE:             ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_NONE,        &st);
    if (err)
        return err;
    printf("VCSM:    HOST:             ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_HOST,        &st);
    if (err)
        return err;
    printf("VCSM:    VC:               ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_VC,          &st);
    if (err)
        return err;
    printf("VCSM:    HOST_AND_VC:      ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_HOST_AND_VC, &st);
    if (err)
        return err;

    /*
    printf("Mailbox: NORMAL:           ");
    err = test_mailbox(size, MEM_FLAG_NORMAL,           &st);
    if (err)
        return err;
    */
    printf("Mailbox: DIRECT:           ");
    err = test_mailbox(size, MEM_FLAG_DIRECT,           &st);
    if (err)
        return err;
    /*
    printf("Mailbox: COHERENT:         ");
    err = test_mailbox(size, MEM_FLAG_COHERENT,         &st);
    if (err)
        return err;
    */
    /*
    printf("Mailbox: L1_NONALLOCATING: ");
    err = test_mailbox(size, MEM_FLAG_L1_NONALLOCATING, &st);
    if (err)
        return err;
    */

    return rpimemmgr_finalize(&st);
}
