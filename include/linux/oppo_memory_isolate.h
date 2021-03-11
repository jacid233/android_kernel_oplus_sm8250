/*
 *
 * Copyright (c) 2020  Guangdong OPPO Mobile Comm Corp., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __OPPO_MEMORY_ISOLATE__
#define __OPPO_MEMORY_ISOLATE__

#define OPPO2_ORDER 2

#define is_oppo2_order(order) \
		unlikely(OPPO2_ORDER == order)

#define is_migrate_oppo2(mt) \
		unlikely(mt == MIGRATE_OPPO2)

extern void setup_zone_migrate_oppo(struct zone *zone, int reserve_migratetype);

#endif /*__OPPO_MEMORY_ISOLATE__*/
