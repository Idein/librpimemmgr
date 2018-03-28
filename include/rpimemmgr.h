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

    /*
     * IMPORTANT NOTE: You can specify NULL as busaddr.  In this case, busaddr
     * is not passed to you here.  Use rpimemmgr_usraddr_to_busaddr() if you
     * need that.
     */
    void* rpimemmgr_alloc_vcsm(const size_t size, const size_t align,
            const VCSM_CACHE_TYPE_T cache_type, uint32_t *busaddrp,
            struct rpimemmgr *sp);
    void* rpimemmgr_alloc_mailbox(const size_t size, const size_t align,
            const uint32_t flags, uint32_t *busaddrp, struct rpimemmgr *sp);

    int rpimemmgr_free(void * const usraddr, struct rpimemmgr *sp);

    /* op0, usraddr0, size0, ... */
    int rpimemmgr_cache_op_multiple(const unsigned op_count, ...);
    int rpimemmgr_cache_op(const enum rpimemmgr_cache_op op, void * const p,
            const size_t size);

    /* op0, usraddr0, block_count0, block_size0, stride0, ... */
    int rpimemmgr_cache_op_2_multiple(const unsigned op_couint, ...);
    int rpimemmgr_cache_op_2(const enum rpimemmgr_cache_op op, void * const p,
            const size_t block_count, const size_t block_size,
            const size_t stride);

    uint32_t rpimemmgr_usraddr_to_busaddr(const void *usraddr);

#endif /* RPIMEMMGR_H_ */
