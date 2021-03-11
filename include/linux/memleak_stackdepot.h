/*
 * A generic stack depot implementation
 *
 * Author: Alexander Potapenko <glider@google.com>
 * Copyright (C) 2016 Google, Inc.
 *
 * Based on code by Dmitry Chernenkov.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_MEMLEAK_STACKDEPOT_H
#define _LINUX_MEMLEAK_STACKDEPOT_H

typedef u32 ml_depot_stack_handle_t;

struct stack_trace;

ml_depot_stack_handle_t ml_depot_save_stack(struct stack_trace *trace, gfp_t flags);
void ml_depot_fetch_stack(ml_depot_stack_handle_t handle, struct stack_trace *trace);
int ml_depot_init(void);
int ml_get_depot_index(void);
#endif /* _LINUX_MEMLEAK_STACKDEPOT_H */
