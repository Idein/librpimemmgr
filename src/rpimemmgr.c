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
#include <mailbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <search.h>
#include <sys/stat.h>
#include <sys/types.h>

struct rpimemmgr_priv {
    _Bool is_vcsm_inited;
    int fd_mb, fd_mem;
    void *root;
};

struct mem_elem {
    enum mem_elem_type{
        MEM_TYPE_VCSM    = 1<<0,
        MEM_TYPE_MAILBOX = 1<<1,
    } type;
    size_t size;
    uint32_t handle, busaddr;
    void *usraddr;
};

static int mem_elem_compar(const void *pa, const void *pb)
{
    const struct mem_elem *na = (const struct mem_elem*) pa,
                          *nb = (const struct mem_elem*) pb;
    if (na->busaddr < nb->busaddr)
        return -1;
    if (na->busaddr > nb->busaddr)
        return 1;
    return 0;
}

static int free_elem(struct mem_elem *ep, struct rpimemmgr *sp)
{
    void *node;

    node = tdelete(ep, &sp->priv->root, mem_elem_compar);
    if (node == NULL) {
        print_error("Node not found\n");
        return 1;
    }

    switch (ep->type) {
        case MEM_TYPE_VCSM:
            return free_mem_vcsm(ep->handle, ep->usraddr);
        case MEM_TYPE_MAILBOX:
            return free_mem_mailbox(sp->priv->fd_mb, ep->size, ep->handle,
                    ep->busaddr, ep->usraddr);
        default:
            print_error("Unknown memory type: 0x%08x\n", ep->type);
            return 1;
    }

    free(ep);
}

static int free_all_elems(struct rpimemmgr *sp)
{
    int err_sum = 0;
    while (sp->priv->root != NULL) {
        int err = free_elem(*(struct mem_elem**) sp->priv->root, sp);
        if (err) {
            err_sum = err;
            /* Continue finalization. */
        }
    }
    return err_sum;
}

static int register_mem(const enum mem_elem_type type, const size_t size,
        const uint32_t handle, const uint32_t busaddr,
        void * const usraddr, struct rpimemmgr *sp)
{
    struct mem_elem *ep, *ep_ret;

    ep = malloc(sizeof(*ep));
    if (ep == NULL) {
        print_error("malloc: %s\n", strerror(errno));
        return 1;
    }

    ep->type = type;
    ep->size = size;
    ep->handle = handle;
    ep->busaddr = busaddr;
    ep->usraddr = usraddr;

    ep_ret = tsearch(ep, &sp->priv->root, mem_elem_compar);
    if (ep_ret == NULL) {
        print_error("Internal error in tsearch\n");
        goto clean_ep;
    }
    if (*(struct mem_elem**) ep_ret != ep) {
        print_error("Duplicate usraddr (internal error)\n");
        goto clean_ep;
    }

    return 0;

clean_ep:
    free(ep);
    return 1;
}

int rpimemmgr_init(struct rpimemmgr *sp)
{
    struct rpimemmgr_priv *priv;

    if (sp == NULL) {
        print_error("sp is NULL\n");
        return 1;
    }

    priv = malloc(sizeof(*priv));
    if (priv == NULL) {
        print_error("malloc: %s\n", strerror(errno));
        return 1;
    }

    priv->is_vcsm_inited = 0;
    priv->fd_mb = -1;
    priv->fd_mem = -1;
    priv->root = NULL;
    sp->priv = priv;
    return 0;
}

int rpimemmgr_finalize(struct rpimemmgr *sp)
{
    int err, err_sum = 0;

    if (sp == NULL) {
        print_error("sp is NULL\n");
        return 1;
    }

    err = free_all_elems(sp);
    if (err) {
        err_sum = err;
        /* Continue finalization. */
    }

    if (sp->priv->is_vcsm_inited)
        vcsm_exit();

    if (sp->priv->fd_mb != -1) {
        err = mailbox_close(sp->priv->fd_mb);
        if (err) {
            print_error("Failed to close Mailbox\n");
            err_sum = err;
            /* Continue finalization. */
        }
    }

    if (sp->priv->fd_mem != -1) {
        err = close(sp->priv->fd_mem);
        if (err) {
            print_error("close: %s\n", strerror(errno));
            err_sum = err;
            /* Continue finalization. */
        }
    }

    free(sp->priv);
    return err_sum;
}

int rpimemmgr_alloc_vcsm(const size_t size, const size_t align,
        const VCSM_CACHE_TYPE_T cache_type, void **usraddrp, uint32_t *busaddrp,
        struct rpimemmgr *sp)
{
    uint32_t handle, busaddr;
    void *usraddr;
    int err;

    if (sp == NULL) {
        print_error("sp is NULL\n");
        return 1;
    }

    if (!sp->priv->is_vcsm_inited) {
        err = vcsm_init();
        if (err) {
            print_error("Failed to initialize VCSM\n");
            return err;
        }
        sp->priv->is_vcsm_inited = !0;
    }

    err = alloc_mem_vcsm(size, align, cache_type, &handle, &busaddr, &usraddr);
    if (err)
        return err;

    err = register_mem(MEM_TYPE_VCSM, size, handle, busaddr, usraddr, sp);
    if (err) {
        (void) free_mem_vcsm(handle, usraddr);
        return err;
    }

    if (usraddrp)
        *usraddrp = usraddr;
    if (busaddrp)
        *busaddrp = busaddr;
    return 0;
}

int rpimemmgr_alloc_mailbox(const size_t size, const size_t align,
        const uint32_t flags, void **usraddrp, uint32_t *busaddrp,
        struct rpimemmgr *sp)
{
    uint32_t handle, busaddr;
    const _Bool do_mapping = (usraddrp != NULL);
    int err;

    if (sp == NULL) {
        print_error("sp is NULL\n");
        return 1;
    }

    if (sp->priv->fd_mb == -1) {
        const int fd = mailbox_open();
        if (fd == -1) {
            print_error("Failed to open Mailbox\n");
            return fd;
        }
        sp->priv->fd_mb = fd;
    }

    if (do_mapping && sp->priv->fd_mem == -1) {
        /*
         * This fd will be used only for mapping Mailbox memory, which is
         * non-cached.  That's why we specify O_SYNC here.
         */
        const int fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd == -1) {
            print_error("open: %s\n", strerror(errno));
            goto clean_mb;
        }
        sp->priv->fd_mem = fd;
    }

    err = alloc_mem_mailbox(sp->priv->fd_mb, sp->priv->fd_mem, size, align,
            flags, &handle, &busaddr, usraddrp);
    if (err)
        goto clean_mem;

    err = register_mem(MEM_TYPE_MAILBOX, size, handle, busaddr,
            usraddrp != NULL ? *usraddrp : NULL, sp);
    if (err)
        goto clean_alloc;

    if (busaddrp)
        *busaddrp = busaddr;
    return 0;

clean_alloc:
    (void) free_mem_mailbox(sp->priv->fd_mb, size, handle, busaddr,
            usraddrp != NULL ? *usraddrp : NULL);
clean_mem:
    if (do_mapping)
        (void) close(sp->priv->fd_mem);
    sp->priv->fd_mem = -1;
clean_mb:
    (void) mailbox_close(sp->priv->fd_mb);
    sp->priv->fd_mb = -1;
    return 1;
}

int rpimemmgr_free_by_busaddr(const uint32_t busaddr, struct rpimemmgr *sp)
{
    void *node;
    struct mem_elem elem_key = {
        .busaddr = busaddr
    };

    node = tfind(&elem_key, &sp->priv->root, mem_elem_compar);
    if (node == NULL) {
        print_error("No such mem_elem: busaddr=0x%08x\n", busaddr);
        return 1;
    }

    return free_elem(*(struct mem_elem**) node, sp);
}

int rpimemmgr_free_by_usraddr(void * const usraddr, struct rpimemmgr *sp)
{
    uint32_t busaddr;

    busaddr = rpimemmgr_usraddr_to_busaddr(usraddr, sp);
    if (!busaddr)
        return 1;

    return rpimemmgr_free_by_busaddr(busaddr, sp);
}

#define IN_RANGE(val, lo, hi) ((lo) <= (val) && (val) < (hi))

static void *usraddr_to_find = NULL;
static uint32_t busaddr_found = 0;
static void action_find_busaddr_by_usraddr(const void *nodep, const VISIT which,
        const int depth)
{
    const struct mem_elem *p = *(const struct mem_elem**) nodep;

    (void) depth;

    if (!usraddr_to_find) {
        print_error("busaddr_to_find is not set (internal error)\n");
        return;
    }
    if (busaddr_found)
        return;

    if ((which == preorder || which == leaf)
            && IN_RANGE(usraddr_to_find, p->usraddr, p->usraddr + p->size))
        busaddr_found = p->busaddr + (usraddr_to_find - p->usraddr);
}

uint32_t rpimemmgr_usraddr_to_busaddr(void * const usraddr,
        struct rpimemmgr *sp)
{
    usraddr_to_find = usraddr;
    busaddr_found = 0;
    twalk(sp->priv->root, action_find_busaddr_by_usraddr);
    if (!busaddr_found) {
        print_error("usraddr=%p is not found\n", usraddr);
        return 0;
    }

    return busaddr_found;
}
