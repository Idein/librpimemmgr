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
#include <mailbox.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

/*
 * hello_fft-recommended parameters
 * (GPU_FFT_USE_VC4_L2_CACHE=0 means "with CPU cache".):
 * +------------+-------------------------+------------------+
 * |            | BCM2835 with CPU cache  | BCM2836, BCM2837 |
 * |            | enabled    | disabled   |                  |
 * +------------+------------+------------+------------------+
 * | flags      |        0x4 |        0xc |              0x4 |
 * | map_offset | 0x20000000 | 0x00000000 |       0x00000000 |
 * +------------+------------+------------+------------------+
 * So the most significant 3 bits of bus address on BCM2835 is used for cache
 * aliasing.  That's why BCM2835 cannot have memory more than 512MiB!
 * map_offset is used as: phyaddr = BUS_TO_PHYS(busaddr + map_offset)
 *
 * Note that on VC6 (BCM2711) no bit in an address is dedicated for memory type
 * since an MMU is introduced also on GPU.
 *
 * The most significant 3 bits of bus address with VCSM will be:
 * +-------------+---------+------------------+
 * |             | BCM2835 | BCM2836, BCM2837 |
 * +-------------+---------+------------------+
 * | NONE        |   0x110 |            0b11x |
 * | HOST        |   0x110 |            0b11x |
 * | VC          |   0x000 |            0b00x |
 * | HOST_AND_VC |   0x000 |            0b00x |
 * +-------------+---------+------------------+
 *
 * The most significant 3 bits of bus address with Mailbox will be:
 * +------------------+---------+------------------+
 * |                  | BCM2835 | BCM2836, BCM2837 |
 * +------------------+---------+------------------+
 * | NORMAL           |   0b00  |            0b00x |
 * | DIRECT           |   0b01  |            0b11x |
 * | COHERENT         |   0b10  |            0b10x |
 * | L1_NONALLOCATING |   0b11  |            0b10x |
 * +------------------+---------+------------------+
 */

/* Derived from hello_fft: http://www.aholme.co.uk/GPU_FFT/Main.htm . */
#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define MEM_FLAG_MASK MEM_FLAG_L1_NONALLOCATING

int get_processor_by_fd(const int fd_mb) {
    uint32_t board_revision;
    int err;

    err = mailbox_get_board_revision(fd_mb, &board_revision);
    if (err) {
        print_error("Failed to get board revision\n");
        return -1;
    }

    /*
     * Refer to
     * https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes
     * for the meaning of the bits.
     */

    if (!(board_revision >> 23 & 1)) {
        print_error("Revision code style is old. " \
                "Please upgrade the firmware\n");
        return -1;
    }

    return board_revision >> 12 & 0xf;
}

int alloc_mem_mailbox(const int fd_mb, const int fd_mem, const size_t size,
        const size_t align, const uint32_t flags, uint32_t *handlep,
        uint32_t *busaddrp, void **usraddrp)
{
    uint32_t handle, busaddr;
    void *usraddr;
    uint32_t map_offset = 0;
    const bool do_mapping = (usraddrp != NULL);

    if (handlep == NULL && busaddrp == NULL && usraddrp == NULL) {
        print_error("Cannot return pointer\n");
        return 1;
    }

    const int processor = get_processor_by_fd(fd_mb);
    if (processor < -1)
        return 1;

    if (do_mapping) {
        if (processor == 0) { /* BCM2835 */
            switch (flags & MEM_FLAG_MASK) {
                case MEM_FLAG_DIRECT:
                    map_offset = 0x20000000;
                    break;
                case MEM_FLAG_L1_NONALLOCATING:
                    map_offset = 0x00000000;
                    break;
                case MEM_FLAG_NORMAL:
                case MEM_FLAG_COHERENT:
                default:
                    print_error("flags must be one of these on BCM2835: " \
                            "DIRECT, L1_NONALLOCATING\n");
                    return 1;
            }
        } else { /* BCM2836, BCM2837, BCM2711 */
            if ((flags & MEM_FLAG_MASK) != MEM_FLAG_DIRECT) {
                print_error("flags must be DIRECT on " \
                        "BCM2836, BCM2837, and BCM2711\n");
                return 1;
            }
            map_offset = 0x00000000;
        }
    }

    handle = mailbox_mem_alloc(fd_mb, size, align, flags);
    if (!handle) {
        print_error("Failed to allocate memory with Mailbox\n");
        return 1;
    }

    busaddr = mailbox_mem_lock(fd_mb, handle);
    if (!busaddr) {
        print_error("Failed to lock memory with Mailbox\n");
        goto clean_alloc;
    }

    if (do_mapping) {
        if (processor == 0 && (busaddr & 0x20000000)) {
            print_error("The third significant bit is set to busaddr " \
                    "on BCM2835: 0x%08x\n", busaddr);
            goto clean_lock;
        }

        usraddr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem,
                BUS_TO_PHYS(busaddr + map_offset));
        if (usraddr == MAP_FAILED) {
            print_error("Failed to map Mailbox memory to userland: %s\n",
                    strerror(errno));
            goto clean_lock;
        }
    }

    *handlep = handle;
    *busaddrp = busaddr;
    if (usraddrp != NULL)
        *usraddrp = usraddr;
    return 0;

clean_lock:
    (void) mailbox_mem_unlock(fd_mb, busaddr);
clean_alloc:
    (void) mailbox_mem_free(fd_mb, handle);
    return 1;
}

int free_mem_mailbox(const int fd_mb, const size_t size, const uint32_t handle,
        const uint32_t busaddr, void *usraddr)
{
    int err, err_sum = 0;

    if (usraddr != NULL) {
        err = munmap(usraddr, size);
        if (err) {
            print_error("munmap: %s\n", strerror(errno));
            err_sum = err;
            /* Continue finalization. */
        }
    }

    err = mailbox_mem_unlock(fd_mb, busaddr);
    if (err) {
        print_error("Failed to unlock memory with Mailbox\n");
        err_sum = err;
        /* Continue finalization. */
    }

    err = mailbox_mem_free(fd_mb, handle);
    if (err) {
        print_error("Failed to free memory with Mailbox\n");
        err_sum = err;
        /* Continue finalization. */
    }

    return err_sum;
}
