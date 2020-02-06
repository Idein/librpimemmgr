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
    int fd_mb, fd_mem, fd_drm;
    void *busaddr_based_root;
    void *usraddr_based_root;
};

struct mem_elem {
    enum mem_elem_type{
        MEM_TYPE_VCSM    = 1<<0,
        MEM_TYPE_MAILBOX = 1<<1,
        MEM_TYPE_DRM     = 1<<2,
    } type;
    size_t size;
    uint32_t handle, busaddr;
    const void *usraddr;
};

static int mem_elem_busaddr_compar(const void *pa, const void *pb)
{
    const struct mem_elem *na = (const struct mem_elem*) pa,
                          *nb = (const struct mem_elem*) pb;
    if (na->busaddr < nb->busaddr)
        return -1;
    if (na->busaddr > nb->busaddr)
        return 1;
    return 0;
}

static int mem_elem_usraddr_compar(const void *pa, const void *pb)
{
    const struct mem_elem *na = (const struct mem_elem*) pa,
                          *nb = (const struct mem_elem*) pb;
    if (na->usraddr < nb->usraddr)
        return -1;
    if (na->usraddr > nb->usraddr)
        return 1;
    return 0;
}

static int free_elem(struct mem_elem *ep, struct rpimemmgr *sp)
{
    void *node_from_busaddr_based;
    void *node_from_usraddr_based;

    node_from_busaddr_based = tdelete(ep, &sp->priv->busaddr_based_root,
            mem_elem_busaddr_compar);
    node_from_usraddr_based = tdelete(ep, &sp->priv->usraddr_based_root,
            mem_elem_usraddr_compar);
    if (node_from_busaddr_based == NULL || node_from_usraddr_based == NULL) {
        print_error("Node not found\n");
        return 1;
    }

    switch (ep->type) {
        case MEM_TYPE_VCSM:
            return free_mem_vcsm(ep->handle, (void*)ep->usraddr);
        case MEM_TYPE_MAILBOX:
            return free_mem_mailbox(sp->priv->fd_mb, ep->size, ep->handle,
                    ep->busaddr, (void*)ep->usraddr);
        case MEM_TYPE_DRM:
            return free_mem_drm(sp->priv->fd_drm, ep->size, ep->handle,
                    (void*)ep->usraddr);
        default:
            print_error("Unknown memory type: 0x%08x\n", ep->type);
            return 1;
    }

    free(ep);
}

static int free_all_elems(struct rpimemmgr *sp)
{
    int err_sum = 0;
    while (sp->priv->busaddr_based_root != NULL) {
        int err = free_elem(*(struct mem_elem**) sp->priv->busaddr_based_root,
                sp);
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
    void *node = NULL;

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

    ep_ret = tsearch(ep, &sp->priv->busaddr_based_root,
            mem_elem_busaddr_compar);
    if (ep_ret == NULL) {
        print_error("Internal error in tsearch\n");
        goto clean_ep;
    }
    if (*(struct mem_elem**) ep_ret != ep) {
        print_error("Duplicate busaddr (internal error)\n");
        goto clean_ep;
    }

    ep_ret = tsearch(ep, &sp->priv->usraddr_based_root,
            mem_elem_usraddr_compar);
    if (ep_ret == NULL) {
        print_error("Internal error in tsearch\n");
        goto clean_and_delete_ep;
    }
    if (*(struct mem_elem**) ep_ret != ep) {
        print_error("Duplicate usraddr (internal error)\n");
        goto clean_and_delete_ep;
    }

    return 0;

clean_and_delete_ep:
    node = tdelete(ep, &sp->priv->busaddr_based_root, mem_elem_busaddr_compar);
    if (node == NULL) {
        print_error("Node not found\n");
        goto clean_ep;
    }

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
    priv->fd_drm = -1;
    priv->busaddr_based_root = NULL;
    priv->usraddr_based_root = NULL;
    sp->priv = priv;
#ifdef RPIMEMMGR_VCSM_HAS_CMA
    sp->vcsm_use_cma = 0;
#endif /* RPIMEMMGR_VCSM_HAS_CMA */
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

    if (sp->priv->fd_drm != -1) {
        err = close(sp->priv->fd_drm);
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
#ifdef RPIMEMMGR_VCSM_HAS_CMA
        err = vcsm_init_ex(sp->vcsm_use_cma, -1);
#else
        err = vcsm_init();
#endif /* RPIMEMMGR_VCSM_HAS_CMA */
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
            print_error("open: /dev/mem: %s\n", strerror(errno));
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

int rpimemmgr_alloc_drm(const size_t size, void **usraddrp, uint32_t *busaddrp, struct rpimemmgr *sp)
{
    uint32_t handle, busaddr;
    void *usraddr;
    int err;

    if (sp == NULL) {
        print_error("sp is NULL\n");
        return 1;
    }

    if (sp->priv->fd_drm == -1) {
        const int fd = open("/dev/dri/card0", O_RDWR);
        if (fd == -1) {
            print_error("open: /dev/dri/card0: %s\n", strerror(errno));
            return fd;
        }
        sp->priv->fd_drm = fd;
    }

    err = alloc_mem_drm(sp->priv->fd_drm, size, &handle, &busaddr, &usraddr);
    if (err)
        return err;

    err = register_mem(MEM_TYPE_DRM, size, handle, busaddr, usraddr, sp);
    if (err) {
        (void) free_mem_drm(handle, size, handle, usraddr);
        return err;
    }

    if (usraddrp)
        *usraddrp = usraddr;
    if (busaddrp)
        *busaddrp = busaddr;
    return 0;
}

int rpimemmgr_free_by_busaddr(const uint32_t busaddr, struct rpimemmgr *sp)
{
    void *node;
    struct mem_elem elem_key = {
        .busaddr = busaddr
    };

    node = tfind(&elem_key, &sp->priv->busaddr_based_root,
            mem_elem_busaddr_compar);
    if (node == NULL) {
        print_error("No such mem_elem: busaddr=0x%08x\n", busaddr);
        return 1;
    }

    return free_elem(*(struct mem_elem**) node, sp);
}

int rpimemmgr_free_by_usraddr(void * const usraddr, struct rpimemmgr *sp)
{
    void *node;
    struct mem_elem elem_key = {
        .usraddr = usraddr
    };

    node = tfind(&elem_key, &sp->priv->usraddr_based_root,
            mem_elem_usraddr_compar);
    if (node == NULL) {
        print_error("No such mem_elem: usraddr=%p\n", usraddr);
        return 1;
    }

    return free_elem(*(struct mem_elem**) node, sp);
}

#define IN_RANGE(val, lo, hi) ((lo) <= (val) && (val) < (hi))
static int find_busaddr_by_usraddr(const void *pa, const void *pb)
{
    const struct mem_elem *na = (const struct mem_elem*) pa;
    const struct mem_elem *nb = (const struct mem_elem*) pb;
    const struct mem_elem *key = na->size == 0 ? na : nb;
    const struct mem_elem *target = na->size == 0 ? nb : na;
    if (IN_RANGE(key->usraddr, target->usraddr,
                target->usraddr + target->size)) {
        return 0;
    } else if (key->usraddr < target->usraddr) {
        return -1;
    } else {
        return 1;
    }
}

uint32_t rpimemmgr_usraddr_to_busaddr(const void * const usraddr,
        struct rpimemmgr *sp)
{
    struct mem_elem elem_key = {
        .size = 0,
        .usraddr = usraddr,
    };
    void* found = tfind(&elem_key, &sp->priv->usraddr_based_root,
            find_busaddr_by_usraddr);
    if (found == NULL) {
        print_error("usraddr=%p is not found\n", usraddr);
        return 0;
    }
    struct mem_elem* node = *(struct mem_elem**)found;

    return node->busaddr + (usraddr - node->usraddr);
}

uint32_t rpimemmgr_usraddr_to_handle(const void * const usraddr,
        struct rpimemmgr *sp)
{
    struct mem_elem elem_key = {
        .size = 0,
        .usraddr = usraddr,
    };
    void* found = tfind(&elem_key, &sp->priv->usraddr_based_root,
            find_busaddr_by_usraddr);
    if (found == NULL) {
        print_error("usraddr=%p is not found\n", usraddr);
        return 0;
    }
    struct mem_elem* node = *(struct mem_elem**)found;

    return node->handle;
}
