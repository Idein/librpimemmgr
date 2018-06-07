/*
 * Copyright (c) 2018 Idein Inc. ( http://idein.jp/ )
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#ifndef RPIMEMMGR_H_
#define RPIMEMMGR_H_

#include <interface/vcsm/user-vcsm.h>
#include <mailbox.h>
#include <sys/types.h>

    struct rpimemmgr {
        struct rpimemmgr_priv *priv;
    };

    enum rpimemmgr_cache_op {
        RPIMEMMGR_CACHE_OP_INVALIDATE,
        RPIMEMMGR_CACHE_OP_CLEAN
    };

    int rpimemmgr_init(struct rpimemmgr *sp);
    int rpimemmgr_finalize(struct rpimemmgr *sp);

    _Bool rpimemmgr_is_bcm2835(void);

    /*
     * If usraddrp is NULL, then memory will NOT be mapped to userland.  This is
     * very useful with Mailbox because that skips access to /dev/mem i.e. you
     * don't need to be root.
     *
     * If busaddrp is NULL, then busaddr is not passed to you here.  Use
     * rpimemmgr_usraddr_to_busaddr() if you need that.
     */
    int rpimemmgr_alloc_vcsm(const size_t size, const size_t align,
            const VCSM_CACHE_TYPE_T cache_type, void **usraddrp,
            uint32_t *busaddrp, struct rpimemmgr *sp);
    int rpimemmgr_alloc_mailbox(const size_t size, const size_t align,
            const uint32_t flags, void **usraddrp, uint32_t *busaddrp,
            struct rpimemmgr *sp);

    int rpimemmgr_free_by_usraddr(void * const usraddr, struct rpimemmgr *sp);
    int rpimemmgr_free_by_busaddr(const uint32_t busaddr, struct rpimemmgr *sp);

    /* op0, usraddr0, size0, ... */
    int rpimemmgr_cache_op_multiple(const unsigned op_count, ...);
    int rpimemmgr_cache_op(const enum rpimemmgr_cache_op op, void * const p,
            const size_t size);

    /* op0, usraddr0, block_count0, block_size0, stride0, ... */
    int rpimemmgr_cache_op_2_multiple(const unsigned op_couint, ...);
    int rpimemmgr_cache_op_2(const enum rpimemmgr_cache_op op, void * const p,
            const size_t block_count, const size_t block_size,
            const size_t stride);

    uint32_t rpimemmgr_usraddr_to_busaddr(const void * const usraddr,
            struct rpimemmgr *sp);

    void unif_set_uint(uint32_t *p, const uint32_t u);
    void unif_set_float(uint32_t *p, const float f);
    void unif_add_uint(const uint32_t u, uint32_t **pp);
    void unif_add_float(const float f, uint32_t **pp);

#endif /* RPIMEMMGR_H_ */
