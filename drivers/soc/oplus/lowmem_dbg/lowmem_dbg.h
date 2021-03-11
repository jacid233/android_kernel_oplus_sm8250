#ifndef __LOWMEM_DBG_H
#define __LOWMEM_DBG_H

void oppo_lowmem_dbg(void );

#ifndef CONFIG_MTK_ION
inline int oppo_is_dma_buf_file(struct file *file);
#endif /* CONFIG_MTK_ION */

#endif /* __LOWMEM_DBG_H */
