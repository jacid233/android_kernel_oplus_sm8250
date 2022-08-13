#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/soc/qcom/smem.h>
#include <linux/seq_file.h>
#include <soc/oppo/oppo_project.h>
#include <linux/io.h>

#define SMEM_PROJECT	135

#define OCDT_VERSION_1_0	(1)
#define OCDT_VERSION_2_0	(2)

#define UINT2Ptr(n)		(uint32_t *)(n)
#define Ptr2UINT32(p)	(uint32_t)(p)

#define PROJECT_VERSION			(0x1)
#define PCB_VERSION				(0x2)
#define RF_INFO					(0x3)
#define MODEM_TYPE				(0x4)
#define OPPO_BOOTMODE			(0x5)
#define SECURE_TYPE				(0x6)
#define SECURE_STAGE			(0x7)
#define OCP_NUMBER				(0x8)
#define SERIAL_NUMBER			(0x9)
#define ENG_VERSION				(0xA)
#define CONFIDENTIAL_STATUS		(0xB)
#define CDT_INTEGRITY			(0xC)
#define OPPO_FEATURE			(0xD)
#define OPERATOR_NAME			(0xE)
#define PROJECT_TEST			(0x1F)

static ProjectInfoCDTType *g_project = NULL;
static EngInfoType *g_enginfo = NULL;

static struct pcb_match pcb_str[] = {
	{.version=PRE_EVB1, .str="PRE_EVB1"},
	{.version=PRE_EVB2, .str="PRE_EVB2"},
	{.version=EVB1, .str="EVB1"},
	{.version=EVB2, .str="EVB2"},
	{.version=T0, .str="T0"},
	{.version=T1, .str="T1"},
	{.version=T2, .str="T2"},
	{.version=T3, .str="T3"},
	{.version=EVT1, .str="EVT1"},
	{.version=EVT2, .str="EVT2"},
	{.version=EVT3, .str="EVT3"},
	{.version=EVT4, .str="EVT4"},
	{.version=DVT1, .str="DVT1"},
	{.version=DVT2, .str="DVT2"},
	{.version=DVT3, .str="DVT3"},
	{.version=DVT4, .str="DVT4"},
	{.version=PVT1, .str="PVT1"},
	{.version=PVT2, .str="PVT2"},
	{.version=PVT3, .str="PVT3"},
	{.version=MP1, .str="MP1"},
	{.version=MP2, .str="MP2"},
	{.version=MP3, .str="MP3"},
};

static void init_project_version(void)
{
	size_t smem_size;
	void *smem_addr;

	if (g_project)
		return;

	smem_addr = qcom_smem_get(QCOM_SMEM_HOST_ANY,
		SMEM_PROJECT,
		&smem_size);
	if (IS_ERR(smem_addr)) {
		pr_err("unable to acquire smem SMEM_PROJECT entry\n");
		return;
	}

	g_project = (ProjectInfoCDTType *)smem_addr;
	if (g_project == ERR_PTR(-EPROBE_DEFER)) {
		g_project = NULL;
		return;
	}

	pr_err("KE Project:%d, Audio:%d, nRF:%d, PCB:%s\n",
		g_project->nProject,
		g_project->nAudio,
		g_project->nRF,
		sizeof(pcb_str)/sizeof(struct pcb_match) > g_project->nPCB?
		(pcb_str[g_project->nPCB].version == g_project->nPCB?pcb_str[g_project->nPCB].str:NULL):NULL);
	pr_err("OCP: %d 0x%x %c %d 0x%x %c\n",
		g_project->nPmicOcp[0],
		g_project->nPmicOcp[1],
		g_project->nPmicOcp[2],
		g_project->nPmicOcp[3],
		g_project->nPmicOcp[4],
		g_project->nPmicOcp[5]);
}

static bool cdt_integrity = false;
static int __init cdt_setup(char *str)
{
	if (str[0] == '1')
		cdt_integrity = true;

	return 1;
}
__setup("cdt_integrity=", cdt_setup);

unsigned int get_project(void)
{
	init_project_version();

	return g_project? g_project->nProject : 0;
}

int get_PCB_Version(void)
{
	init_project_version();
		
	return g_project? g_project->nPCB:-EINVAL;
}
EXPORT_SYMBOL(get_PCB_Version);

unsigned char get_Oppo_Boot_Mode(void)
{
	init_project_version();

	return g_project?g_project->nOppoBootMode:0;
}

/*
* @deprecated: used for upgrade projects: P->Q
*/
int32_t get_Operator_Version(void)
{
	init_project_version();

	if (g_project->nVerison == OCDT_VERSION_1_0)
		return g_project?g_project->reserved[0]:OPERATOR_UNKOWN;

	return OPERATOR_UNKOWN;
}
EXPORT_SYMBOL(get_Operator_Version);

int32_t get_Modem_Version(void)
{
	init_project_version();

	if (g_project->nVerison == OCDT_VERSION_1_0)
		return g_project?g_project->reserved[1]:-EINVAL;
	else
		return g_project?g_project->nRF:-EINVAL;

	return -EINVAL;
}

int rpmb_is_enable(void)
{
#define RPMB_KEY_PROVISIONED 0x00780178

	unsigned int rmpb_rd = 0;
	void __iomem *rpmb_addr = NULL;
	static unsigned int rpmb_enable = 0;

	if (rpmb_enable)
		return rpmb_enable;

	rpmb_addr = ioremap(RPMB_KEY_PROVISIONED , 4);	
	if (rpmb_addr) {
		rmpb_rd = __raw_readl(rpmb_addr);
		iounmap(rpmb_addr);
		rpmb_enable = (rmpb_rd >> 24) & 0x01;
	} else {
		rpmb_enable = 0;
	}

	return rpmb_enable;
}
EXPORT_SYMBOL(rpmb_is_enable);

static void init_eng_version(void)
{
	size_t smem_size = 0;
	void *smem_addr = NULL;

	if (g_enginfo)
		return;

	smem_addr = qcom_smem_get(QCOM_SMEM_HOST_ANY,
		SMEM_PROJECT,
		&smem_size);
	if (IS_ERR(smem_addr)) {
		pr_err("unable to acquire smem SMEM_PROJECT entry\n");
		return;
	}

	g_enginfo = (EngInfoType *)(smem_addr + ALIGN4(ProjectInfoCDTType));
	if (g_enginfo == ERR_PTR(-EPROBE_DEFER)) {
		g_enginfo = NULL;
		return;
	}

	pr_err("eng info: version(%d), confidential(%d).\n", g_enginfo->version, g_enginfo->is_confidential);
}

int get_eng_version(void)
{
	if (!g_enginfo)
		init_eng_version();

	return g_enginfo ? g_enginfo->version : RELEASE;
}
EXPORT_SYMBOL(get_eng_version);

extern char *saved_command_line;

bool oppo_daily_build(void)
{
    static int daily_build = -1;
    int eng_version = 0;

    if (daily_build != -1)
        return daily_build;

    if (strstr(saved_command_line, "buildvariant=userdebug") ||
        strstr(saved_command_line, "buildvariant=eng")) {
        daily_build = true;
    } else {
        daily_build = false;
    }

    eng_version = get_eng_version();
    if ((ALL_NET_CMCC_TEST == eng_version) || (ALL_NET_CMCC_FIELD == eng_version) ||
        (ALL_NET_CU_TEST == eng_version) || (ALL_NET_CU_FIELD == eng_version) ||
        (ALL_NET_CT_TEST == eng_version) || (ALL_NET_CT_FIELD == eng_version)) {
        daily_build = false;
    }

    return daily_build;
}
EXPORT_SYMBOL(oppo_daily_build);

bool is_confidential(void)
{
	if (!g_enginfo)
		init_eng_version();

	return g_enginfo ? g_enginfo->is_confidential : true;
}
EXPORT_SYMBOL(is_confidential);

uint32_t get_oppo_feature(enum f_index index)
{
	init_project_version();

	if (index < 1 || index > FEATURE_COUNT)
		return 0;

	return g_project?g_project->nFeature[index-1]:0;
}

static void dump_ocp_info(struct seq_file *s)
{
	init_project_version();

	if (!g_project)
		return;

	seq_printf(s, "ocp: %d 0x%x %d 0x%x %c %c",
		g_project->nPmicOcp[0],
		g_project->nPmicOcp[1],
		g_project->nPmicOcp[3],
		g_project->nPmicOcp[4],
		g_project->nPmicOcp[2],
		g_project->nPmicOcp[5]);
}

static void dump_serial_info(struct seq_file *s)
{
#define QFPROM_RAW_SERIAL_NUM 0x00786134

	void __iomem *serial_id_addr = NULL;
	unsigned int serial_id = 0xFFFFFFFF;

	serial_id_addr = ioremap(QFPROM_RAW_SERIAL_NUM , 4);
	if (serial_id_addr) {
		serial_id = __raw_readl(serial_id_addr);
		iounmap(serial_id_addr);
	}

	seq_printf(s, "0x%x", serial_id);
}

static void dump_project_test(struct seq_file *s)
{
	return;
}

static void dump_oppo_feature(struct seq_file *s)
{
	seq_printf(s, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		g_project->nFeature[0],
		g_project->nFeature[1],
		g_project->nFeature[2],
		g_project->nFeature[3],
		g_project->nFeature[4],
		g_project->nFeature[5],
		g_project->nFeature[6],
		g_project->nFeature[7],
		g_project->nFeature[8],
		g_project->nFeature[9]);
	return;
}

static void dump_eng_version(struct seq_file *s)
{
	seq_printf(s, "%d", get_eng_version());
	return;
}

static void dump_confidential_status(struct seq_file *s)
{
	seq_printf(s, "%d", is_confidential());
	return;
}

static void dump_secure_type(struct seq_file *s)
{
#define OEM_SEC_BOOT_REG 0x780350

	void __iomem *oem_config_base = NULL;
	uint32_t secure_oem_config = 0;

	oem_config_base = ioremap(OEM_SEC_BOOT_REG, 4);
	if (oem_config_base) {
		secure_oem_config = __raw_readl(oem_config_base);
		iounmap(oem_config_base);
	}

	seq_printf(s, "%d", secure_oem_config);	
}

static void dump_secure_stage(struct seq_file *s)
{
#define OEM_SEC_ENABLE_ANTIROLLBACK_REG 0x78019c

	void __iomem *oem_config_base = NULL;
	uint32_t secure_oem_config = 0;

	oem_config_base = ioremap(OEM_SEC_ENABLE_ANTIROLLBACK_REG, 4);
	if (oem_config_base) {
		secure_oem_config = __raw_readl(oem_config_base);
		iounmap(oem_config_base);
	}

	seq_printf(s, "%d", secure_oem_config);
}

static void update_manifest(struct proc_dir_entry *parent)
{
	static const char* manifest_src[2] = {
		"/vendor/etc/vintf/manifest_ssss.xml",
		"/vendor/etc/vintf/manifest_dsds.xml",
	};
	mm_segment_t fs;
	char * substr = strstr(boot_command_line, "simcardnum.doublesim=");

	if(!substr)
		return;

	substr += strlen("simcardnum.doublesim=");

	fs = get_fs();
	set_fs(KERNEL_DS);

	if (parent) {
		if (substr[0] == '0')
			proc_symlink("manifest", parent, manifest_src[0]);//single sim
		else
			proc_symlink("manifest", parent, manifest_src[1]);
	}

	set_fs(fs);
}

static int project_read_func(struct seq_file *s, void *v)
{
	void *p = s->private;

	switch(Ptr2UINT32(p)) {
	case PROJECT_VERSION:
		seq_printf(s, "%d", get_project());
		break;
	case PCB_VERSION:
		seq_printf(s, "%d", get_PCB_Version());
		break;
	case OPPO_BOOTMODE:
		seq_printf(s, "%d", get_Oppo_Boot_Mode());
		break;
	case MODEM_TYPE:
	case RF_INFO:
		seq_printf(s, "%d", get_Modem_Version());
		break;
	case SECURE_TYPE:
		dump_secure_type(s);
		break;
	case SECURE_STAGE:
		dump_secure_stage(s);
		break;
	case OCP_NUMBER:
		dump_ocp_info(s);
		break;
	case SERIAL_NUMBER:
		dump_serial_info(s);
		break;
	case ENG_VERSION:
		dump_eng_version(s);
		break;
	case CONFIDENTIAL_STATUS:
		dump_confidential_status(s);
		break;
	case PROJECT_TEST:
		dump_project_test(s);
		break;
	case CDT_INTEGRITY:
		seq_printf(s, "%d", cdt_integrity);
		break;
	case OPPO_FEATURE:
		dump_oppo_feature(s);
		break;
	case OPERATOR_NAME:
		seq_printf(s, "%d", get_Operator_Version());
		break;
	default:
		seq_printf(s, "not support\n");
		break;
	}

	return 0;
}

static int get_cdt_version()
{
	init_project_version();

	return g_project?g_project->nVerison:0;
}

static int projects_open(struct inode *inode, struct file *file)
{
	return single_open(file, project_read_func, PDE_DATA(inode));
}

static const struct file_operations project_info_fops = {
	.owner = THIS_MODULE,
	.open  = projects_open,
	.read  = seq_read,
	.release = single_release,
};

static int __init oppo_project_init(void)
{
	struct proc_dir_entry *p_entry;
	static struct proc_dir_entry *oppo_info = NULL;

	oppo_info = proc_mkdir("oppoVersion", NULL);
	if (!oppo_info) {
		goto error_init;
	}

	p_entry = proc_create_data("prjName", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(PROJECT_VERSION));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("pcbVersion", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(PCB_VERSION));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("oppoBootmode", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(OPPO_BOOTMODE));
	if (!p_entry)
		goto error_init;

	if (get_cdt_version() == OCDT_VERSION_1_0) {
		p_entry = proc_create_data("modemType", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(MODEM_TYPE));
		if (!p_entry)
			goto error_init;

		p_entry = proc_create_data("operatorName", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(OPERATOR_NAME));
		if (!p_entry)
			goto error_init;
	} else {
		p_entry = proc_create_data("RFType", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(RF_INFO));
		if (!p_entry)
			goto error_init;
	}

	p_entry = proc_create_data("secureType", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(SECURE_TYPE));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("secureStage", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(SECURE_STAGE));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("ocp", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(OCP_NUMBER));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("serialID", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(SERIAL_NUMBER));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("engVersion", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(ENG_VERSION));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("isConfidential", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(CONFIDENTIAL_STATUS));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("cdt", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(CDT_INTEGRITY));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("feature", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(OPPO_FEATURE));
	if (!p_entry)
		goto error_init;

	p_entry = proc_create_data("test", S_IRUGO, oppo_info, &project_info_fops, UINT2Ptr(PROJECT_TEST));
	if (!p_entry)
		goto error_init;

	/*update single or double cards*/
	update_manifest(oppo_info);

	return 0;

error_init:
	remove_proc_entry("oppoVersion", NULL);
	return -ENOENT;
}

arch_initcall(oppo_project_init);

MODULE_DESCRIPTION("OPPO project version");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Joshua <gyx@oppo.com>");
