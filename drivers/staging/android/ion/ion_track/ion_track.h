/* Kui.Zhang@BSP.Kernel.MM, 2020-05-20, record the ion device, used for
 * travel all ion bufers.
 */
static struct ion_device *internal_dev = NULL;

static inline void update_internal_dev(struct ion_device *dev)
{
	if (!internal_dev)
		internal_dev = dev;
}
