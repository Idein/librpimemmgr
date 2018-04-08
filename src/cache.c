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
#include <interface/vcsm/user-vcsm.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/param.h>

#define MAX_CACHE_OP_ENTRIES 8

/* op0, user0, size0, op1, user1, size1, ... */
int rpimemmgr_cache_op_multiple(unsigned op_count, ...)
{
    unsigned i;
    va_list ap;

    va_start(ap, op_count);
    for (; ; op_count -= MAX_CACHE_OP_ENTRIES) {
        int err;

        uint8_t *buf[sizeof(struct vcsm_user_clean_invalid2_s) +
                sizeof(struct vcsm_user_clean_invalid2_block_s) *
                        MAX_CACHE_OP_ENTRIES];
        struct vcsm_user_clean_invalid2_s *s =
                (struct vcsm_user_clean_invalid2_s*) buf;

        const unsigned count = MIN(op_count, MAX_CACHE_OP_ENTRIES);
        s->op_count = count;

        for (i = 0; i < count; i ++) {
            const enum rpimemmgr_cache_op op =
                    va_arg(ap, enum rpimemmgr_cache_op);
            unsigned mode;

            switch (op) {
                case RPIMEMMGR_CACHE_OP_INVALIDATE:
                    mode = 1;
                    break;
                case RPIMEMMGR_CACHE_OP_CLEAN:
                    mode = 2;
                    break;
                default:
                    print_error("Invalid op: %d\n", op);
                    return 1;
            }

            s->s[i].invalidate_mode = mode;
            s->s[i].block_count = 1;
            s->s[i].start_address = va_arg(ap, void*);
            s->s[i].block_size = va_arg(ap, size_t);
            s->s[i].inter_block_stride = 0;
        }

        err = vcsm_clean_invalid2(s);
        if (err) {
            print_error("Failed to sync cache: %d\n", err);
            return err;
        }

        if (op_count < MAX_CACHE_OP_ENTRIES)
            break;
    }
    va_end(ap);

    return 0;
}

int rpimemmgr_cache_op(const enum rpimemmgr_cache_op op, void * const p,
        const size_t size)
{
    return rpimemmgr_cache_op_multiple(1, op, p, size);
}

/* op0, user0, block_count0, block_size0, stride0, ... */
int rpimemmgr_cache_op_2_multiple(unsigned op_count, ...)
{
    unsigned i;
    va_list ap;

    va_start(ap, op_count);
    for (; ; op_count -= MAX_CACHE_OP_ENTRIES) {
        int err;

        uint8_t *buf[sizeof(struct vcsm_user_clean_invalid2_s) +
                sizeof(struct vcsm_user_clean_invalid2_block_s) *
                        MAX_CACHE_OP_ENTRIES];
        struct vcsm_user_clean_invalid2_s *s =
                (struct vcsm_user_clean_invalid2_s*) buf;

        const unsigned count = MIN(op_count, MAX_CACHE_OP_ENTRIES);
        s->op_count = count;

        for (i = 0; i < count; i ++) {
            const enum rpimemmgr_cache_op op =
                    va_arg(ap, enum rpimemmgr_cache_op);
            unsigned mode;

            switch (op) {
                case RPIMEMMGR_CACHE_OP_INVALIDATE:
                    mode = 1;
                    break;
                case RPIMEMMGR_CACHE_OP_CLEAN:
                    mode = 2;
                    break;
                default:
                    print_error("Invalid op: %d\n", op);
                    return 1;
            }

            s->s[i].invalidate_mode = mode;
            s->s[i].start_address = va_arg(ap, void*);
            s->s[i].block_count = va_arg(ap, size_t);
            s->s[i].block_size = va_arg(ap, size_t);
            s->s[i].inter_block_stride = va_arg(ap, size_t);
        }

        err = vcsm_clean_invalid2(s);
        if (err) {
            print_error("Failed to sync cache: %d\n", err);
            return err;
        }

        if (op_count < MAX_CACHE_OP_ENTRIES)
            break;
    }
    va_end(ap);

    return 0;
}

int rpimemmgr_cache_op_2(const enum rpimemmgr_cache_op op, void * const p,
        const size_t block_count, const size_t block_size, const size_t stride)
{
    return rpimemmgr_cache_op_2_multiple(1, op, p, block_count, block_size,
            stride);
}
