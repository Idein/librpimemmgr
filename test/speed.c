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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>


#define barrier() __asm__ volatile ("" : : : "memory")
#define barrier_data(ptr) __asm__ volatile ("" : : "r" (ptr) : "memory")


static inline
double get_time(void)
{
    struct timespec t;
    (void) clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (double) t.tv_sec + t.tv_nsec * 1e-9;
}

static void test_speed_copy(const size_t size, void *dst, void *src)
{
    double start, end, elapsed;
    unsigned i;
    const unsigned n_warmup = 16, n_measure = 32;
    /* const unsigned n_cooldown = ...; */

    memset(src, 0x55, size);
    memset(dst, 0xaa, size);
    barrier_data(src);
    barrier_data(dst);

    for (i = 0; i < n_warmup; i ++) {
        (void) memcpy(dst, src, size);
        barrier_data(src);
        barrier_data(dst);
    }

    barrier();
    start = get_time();
    barrier();
    for (i = 0; i < n_measure; i ++) {
        (void) memcpy(dst, src, size);
        barrier_data(src);
        barrier_data(dst);
    }
    barrier();
    end = get_time();
    barrier();

    elapsed = (end - start) / n_measure;
    printf("%e [s], %e [B/s]\n", elapsed, size / elapsed);
}

static int test_malloc(const size_t size)
{
    void *dst, *src;
    int err = 0;

    err = posix_memalign(&dst, 4096, size);
    if (err) {
        fprintf(stderr, "Failed to allocate dst\n");
        goto clean_none;
    }

    err = posix_memalign(&src, 4096, size);
    if (err) {
        fprintf(stderr, "Failed to allocate src\n");
        goto clean_dst;
    }

    test_speed_copy(size, dst, src);

    free(src);
clean_dst:
    free(dst);
clean_none:
    return err;
}

static int test_vcsm(const size_t size, const VCSM_CACHE_TYPE_T cache_type)
{
    void *dst, *src;
    int err = 0;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        goto clean_none;

    err = rpimemmgr_alloc_vcsm(size, 4096, cache_type, &dst, NULL, &st);
    if (err)
        goto clean_init;

    err = rpimemmgr_alloc_vcsm(size, 4096, cache_type, &src, NULL, &st);
    if (err)
        goto clean_dst;

    test_speed_copy(size, dst, src);

    err |= rpimemmgr_free_by_usraddr(src, &st);
clean_dst:
    err |= rpimemmgr_free_by_usraddr(dst, &st);
clean_init:
    err |= rpimemmgr_finalize(&st);
clean_none:
    return err;
}

#ifdef RPIMEMMGR_VCSM_HAS_CMA

static int test_vcsm_cma(const size_t size, const VCSM_CACHE_TYPE_T cache_type)
{
    void *dst, *src;
    int err = 0;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        goto clean_none;

    st.vcsm_use_cma = 1;

    err = rpimemmgr_alloc_vcsm(size, 4096, cache_type, &dst, NULL, &st);
    if (err)
        goto clean_init;

    err = rpimemmgr_alloc_vcsm(size, 4096, cache_type, &src, NULL, &st);
    if (err)
        goto clean_dst;

    test_speed_copy(size, dst, src);

    err |= rpimemmgr_free_by_usraddr(src, &st);
clean_dst:
    err |= rpimemmgr_free_by_usraddr(dst, &st);
clean_init:
    err |= rpimemmgr_finalize(&st);
clean_none:
    return err;
}

#endif /* RPIMEMMGR_VCSM_HAS_CMA */

static int test_mailbox(const size_t size, const uint32_t flags)
{
    void *dst, *src;
    int err = 0;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        goto clean_none;

    err = rpimemmgr_alloc_mailbox(size, 4096, flags, &dst, NULL, &st);
    if (err)
        goto clean_init;

    err = rpimemmgr_alloc_mailbox(size, 4096, flags, &src, NULL, &st);
    if (err)
        goto clean_dst;

    test_speed_copy(size, dst, src);

    err |= rpimemmgr_free_by_usraddr(src, &st);
clean_dst:
    err |= rpimemmgr_free_by_usraddr(dst, &st);
clean_init:
    err |= rpimemmgr_finalize(&st);
clean_none:
    return err;
}

static int test_drm(const size_t size)
{
    void *dst, *src;
    int err = 0;
    struct rpimemmgr st;

    err = rpimemmgr_init(&st);
    if (err)
        goto clean_none;

    err = rpimemmgr_alloc_drm(size, &dst, NULL, &st);
    if (err)
        goto clean_init;

    err = rpimemmgr_alloc_drm(size, &src, NULL, &st);
    if (err)
        goto clean_dst;

    test_speed_copy(size, dst, src);

    err |= rpimemmgr_free_by_usraddr(src, &st);
clean_dst:
    err |= rpimemmgr_free_by_usraddr(dst, &st);
clean_init:
    err |= rpimemmgr_finalize(&st);
clean_none:
    return err;
}

int main(void)
{
    const size_t size = 1ULL << 24; /* 16 MiB */
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

    printf("malloc:                       ");
    err = test_malloc(size);
    if (err)
        return err;

    printf("VCSM (GPU): NONE:             ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_NONE);
    if (err)
        return err;
    printf("VCSM (GPU): HOST:             ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_HOST);
    if (err)
        return err;
    printf("VCSM (GPU): VC:               ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_VC);
    if (err)
        return err;
    printf("VCSM (GPU): HOST_AND_VC:      ");
    err = test_vcsm(size, VCSM_CACHE_TYPE_HOST_AND_VC);
    if (err)
        return err;

#ifdef RPIMEMMGR_VCSM_HAS_CMA

    printf("VCSM (CMA): NONE:             ");
    err = test_vcsm_cma(size, VCSM_CACHE_TYPE_NONE);
    if (err)
        return err;
    printf("VCSM (CMA): HOST:             ");
    err = test_vcsm_cma(size, VCSM_CACHE_TYPE_HOST);
    if (err)
        return err;
    printf("VCSM (CMA): VC:               ");
    err = test_vcsm_cma(size, VCSM_CACHE_TYPE_VC);
    if (err)
        return err;
    printf("VCSM (CMA): HOST_AND_VC:      ");
    err = test_vcsm_cma(size, VCSM_CACHE_TYPE_HOST_AND_VC);
    if (err)
        return err;

#endif /* RPIMEMMGR_VCSM_HAS_CMA */

    printf("Mailbox:    DIRECT:           ");
    err = test_mailbox(size, MEM_FLAG_DIRECT);
    if (err)
        return err;
    if (processor == 0) { /* BCM2835 */
        printf("Mailbox:    L1_NONALLOCATING: ");
        err = test_mailbox(size, MEM_FLAG_L1_NONALLOCATING);
        if (err)
            return err;
    }

    if (processor == 3) { /* BCM2711 */
        printf("DRM:                          ");
        err = test_drm(size);
        if (err)
            return err;
    }

    return 0;
}
