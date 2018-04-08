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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void unif_set_uint(uint32_t *p, const uint32_t u)
{
    memcpy(p, &u, sizeof(u));
}

void unif_set_float(uint32_t *p, const float f)
{
    memcpy(p, &f, sizeof(f));
}

void unif_add_uint(const uint32_t u, uint32_t **pp)
{
    unif_set_uint((*pp)++, u);
}

void unif_add_float(const float f, uint32_t **pp)
{
    unif_set_float((*pp)++, f);
}
