#ifndef _OPPO_WPC_H_
#define _OPPO_WPC_H_

#include <linux/workqueue.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#include <linux/wakelock.h>
#endif
#include <linux/timer.h>
#include <linux/slab.h>
#include <soc/oppo/device_info.h>
#include <soc/oppo/oppo_project.h>
#include <linux/firmware.h>


typedef enum{
    CHARGE_LEVEL_UNKNOW,
    CHARGE_LEVEL_ABOVE_41_DEGREE,
    CHARGE_LEVEL_39_TO_41_DEGREE,
    CHARGE_LEVEL_BELOW_39_DEGREE,
}E_CHARGE_LEVEL;

typedef enum{
    OPPO_WPC_NO_CHARGER_MODE,
    OPPO_WPC_NORMAL_MODE,
    OPPO_WPC_FASTCHG_MODE,
    OPPO_WPC_UNKNOW_MODE,
}E_WPC_CHG_MODE;

typedef enum{
	OPPO_WPC_SWITCH_DEFAULT_MODE,
	OPPO_WPC_SWITCH_WIRED_CHG_MODE,
	OPPO_WPC_SWITCH_WPC_CHG_MODE,
	OPPO_WPC_SWITCH_WIRED_OTG_MODE,
	OPPO_WPC_SWITCH_WPC_OTG_MODE,
	OPPO_WPC_SWITCH_OTHER_MODE,
};

struct wpc_gpio_ctrl{
	int			    wpc_en_gpio;  //idt en
	int				wpc_con_gpio; // wpc connect
	int				wpc_con_irq;  // wpc connect irq
	int				wpc_int_gpio;
	int				wpc_int_irq;
	int				vbat_en_gpio;
	int				booster_en_gpio;
	int             wired_conn_gpio;
	int             wired_conn_irq;

    int             otg_en_gpio;
    int             wrx_en_gpio;
    int             wrx_otg_gpio;

    struct pinctrl                       *pinctrl;
    struct pinctrl_state        *wpc_en_active;
    struct pinctrl_state        *wpc_en_sleep;
    struct pinctrl_state        *wpc_en_default;
	struct pinctrl_state 		*wpc_con_active;
	struct pinctrl_state 		*wpc_con_sleep;
	struct pinctrl_state 		*wpc_con_default;
	struct pinctrl_state 		*wpc_int_active;
	struct pinctrl_state 		*wpc_int_sleep;
	struct pinctrl_state 		*wpc_int_default;
	struct pinctrl_state 		*vbat_en_active;
	struct pinctrl_state 		*vbat_en_sleep;
	struct pinctrl_state 		*vbat_en_default;
	struct pinctrl_state 		*booster_en_active;
	struct pinctrl_state 		*booster_en_sleep;
	struct pinctrl_state 		*booster_en_default;

	struct pinctrl_state        *wired_conn_active;
	struct pinctrl_state        *wired_conn_sleep;

    struct pinctrl_state        *otg_en_active;
    struct pinctrl_state        *otg_en_sleep;
    struct pinctrl_state        *otg_en_default;

    struct pinctrl_state        *wrx_en_active;
    struct pinctrl_state        *wrx_en_sleep;
    struct pinctrl_state        *wrx_en_default;

    struct pinctrl_state        *wrx_otg_active;
    struct pinctrl_state        *wrx_otg_sleep;
};



struct oppo_wpc_chip {
	struct i2c_client				 *client;
	struct device					 *dev;

    struct wpc_gpio_ctrl            wpc_gpios;
    struct oppo_wpc_operations      *wpc_ops;
    struct oppo_wpc_operations      *wpc_cp_ops;

    char    charge_status;
    bool    charge_online;
    int     send_message;
    int     adapter_type;
    int     charge_type;
    char    tx_command;
    char    tx_data;
    int     charge_voltage;
    int     charge_current;
    int     cc_value_limit;
    int     vout;
    int     vrect;
    int     iout;
    int     iout_array[10];
    int     temperature_status;
    int     termination_status;
    int     terminate_voltage;
    int     terminate_current;
    int     work_mode;
    bool    check_fw_update;
    bool    wpc_fw_updating;
    bool    CEP_is_OK;
    bool    CEP_now_OK;
    bool    fastchg_ing;
    bool	epp_working;
    bool	vbatt_full;
    bool	ftm_mode;
    bool    wpc_otg_switch;
    bool    wpc_chg_enable;
    bool    wired_chg_enable;
    bool    wired_otg_enable;
    bool    wpc_fastchg_allow;
    bool    cp_support;
    E_CHARGE_LEVEL charge_level;
    atomic_t                         suspended;

    struct delayed_work idt_con_work;
    struct delayed_work wpc_task_work;
    struct delayed_work p922x_CEP_work;
    struct delayed_work p922x_update_work;
    struct delayed_work idt_event_int_work;
    struct delayed_work wpc_connect_int_work;

};



struct oppo_wpc_operations {
    int (*init_irq_registers)(struct oppo_wpc_chip *chip);
    int (*get_vrect_iout)(struct oppo_wpc_chip *chip);
    int (*set_charger_type_dect)(struct oppo_wpc_chip *chip);
    int (*set_tx_charger_mode)(struct oppo_wpc_chip *chip);
    int (*set_voltage_out)(struct oppo_wpc_chip *chip);
    int (*set_dischg_enable)(struct oppo_wpc_chip *chip, bool is_on);
    void (*set_rx_charge_voltage)(struct oppo_wpc_chip *chip, int vol);
};

static void oppo_wpc_reset_variables(struct oppo_wpc_chip *chip);


enum {
WPC_CHG_STATUS_DEFAULT,
WPC_CHG_STATUS_READY_FOR_VOOC,
WPC_CHG_STATUS_READY_FOR_SVOOC,
WPC_CHG_STATUS_WAITING_FOR_TX_INTO_FASTCHG,
WPC_CHG_STATUS_INCREASE_VOLTAGE,
WPC_CHG_STATUS_INCREASE_CURRENT,
WPC_CHG_STATUS_ADJUST_VOL_AFTER_INC_CURRENT,
WPC_CHG_STATUS_FAST_CHARGING,
WPC_CHG_STATUS_STANDARD_CHARGING,
WPC_CHG_STATUS_FAST_CHARGING_FROM_CHGPUMP,
WPC_CHG_STATUS_READY_FOR_EPP,
WPC_CHG_STATUS_EPP_1A,
WPC_CHG_STATUS_EPP_WORKING,
WPC_CHG_STATUS_READY_FOR_FTM,
WPC_CHG_STATUS_FTM_WORKING,
};

enum {
WPC_TERMINATION_STATUS_UNDEFINE,
WPC_TERMINATION_STATUS_DEFAULT,
WPC_TERMINATION_STATUS_FFC,
WPC_TERMINATION_STATUS_COLD,
WPC_TERMINATION_STATUS_COOL,
WPC_TERMINATION_STATUS_NORMAL,
WPC_TERMINATION_STATUS_WARM,
WPC_TERMINATION_STATUS_HOT,
WPC_TERMINATION_STATUS_ABNORMAL,
};


#define P922X_PRE2CC_CHG_THD_LO                             3400
#define P922X_PRE2CC_CHG_THD_HI                             3420

#define P922X_CC2CV_CHG_THD_LO                              4350
#define P922X_CC2CV_CHG_THD_HI                              4370

#define P9221_CMD_INDENTIFY_ADAPTER							0xA1 
#define P9221_CMD_INTO_FASTCHAGE							0xA2
#define P9221_CMD_INTO_USB_CHARGE							0xA3
#define P9221_CMD_INTO_NORMAL_CHARGE						0xA4
#define P9221_CMD_NULL										0x00

#define P9237_RESPONE_ADAPTER_TYPE							0xF1
#define P9237_RESPONE_INTO_FASTCHAGE						0xF2
#define P9237_RESPONE_INTO_USB_CHARGE						0xF3
#define P9237_RESPONE_INTO_NORMAL_CHARGER					0xF4
#define P9237_COMMAND_READY_FOR_EPP							0xFA
#define P9237_COMMAND_WORKING_IN_EPP						0xFB
#define P9237_RESPONE_NULL									0x00

#define ADAPTER_TYPE_UNKNOW									0
#define ADAPTER_TYPE_FASTCHAGE_VOOC							1
#define ADAPTER_TYPE_FASTCHAGE_SVOOC						2
#define ADAPTER_TYPE_USB									3
#define ADAPTER_TYPE_NORMAL_CHARGE							4
#define ADAPTER_TYPE_EPP									5

#define WPC_CHARGE_TYPE_DEFAULT								0
#define WPC_CHARGE_TYPE_FAST								1
#define WPC_CHARGE_TYPE_USB									2
#define WPC_CHARGE_TYPE_NORMAL								3
#define WPC_CHARGE_TYPE_EPP									4

#define WPC_TASK_INTERVAL									round_jiffies_relative(msecs_to_jiffies(500))	
#define P922X_CEP_INTERVAL									round_jiffies_relative(msecs_to_jiffies(200))
#define P922X_UPDATE_INTERVAL								round_jiffies_relative(msecs_to_jiffies(3000))
#define P922X_UPDATE_RETRY_INTERVAL							round_jiffies_relative(msecs_to_jiffies(500))

#define TEMPERATURE_COLD_LOW_LEVEL							-30
#define TEMPERATURE_COLD_HIGH_LEVEL							0
#define TEMPERATURE_COOL_LOW_LEVEL							TEMPERATURE_COLD_HIGH_LEVEL
#define TEMPERATURE_COOL_HIGH_LEVEL							165
#define TEMPERATURE_NORMAL_LOW_LEVEL						TEMPERATURE_COOL_HIGH_LEVEL
#define TEMPERATURE_NORMAL_HIGH_LEVEL						410
#define TEMPERATURE_WARM_LOW_LEVEL							TEMPERATURE_NORMAL_HIGH_LEVEL
#define TEMPERATURE_WARM_HIGH_LEVEL							450
#define TEMPERATURE_HOT_LOW_LEVEL							TEMPERATURE_WARM_HIGH_LEVEL
#define TEMPERATURE_HOT_HIGH_LEVEL							550

#define TEMPERATURE_STATUS_COLD								0
#define TEMPERATURE_STATUS_COOL								1
#define TEMPERATURE_STATUS_NORMAL							2
#define TEMPERATURE_STATUS_WARM								3
#define TEMPERATURE_STATUS_HOT								4
#define TEMPERATURE_STATUS_ABNORMAL							5

#define TEMPERATURE_STATUS_CHANGE_TIMEOUT					10		//about 10s

#define WPC_CHARGE_CURRENT_LIMIT_200MA						200
#define WPC_CHARGE_CURRENT_ZERO								0		//0mA
#define WPC_CHARGE_CURRENT_200MA							200
#define WPC_CHARGE_CURRENT_DEFAULT							500		//500mA
#define WPC_CHARGE_CURRENT_FASTCHG_INT                      300
#define WPC_CHARGE_CURRENT_FASTCHG_END						700		//300mA
#define WPC_CHARGE_CURRENT_FASTCHG_MID						800		//800mA
#define WPC_CHARGE_CURRENT_FASTCHG							1500	//1500mA
#define WPC_CHARGE_CURRENT_CHANGE_STEP_200MA				200		//200mA
#define WPC_CHARGE_CURRENT_CHANGE_STEP_50MA					50		//50mA
#define WPC_CHARGE_CURRENT_FFC_TO_CV						1000	//1000mA
#define WPC_CHARGE_CURRENT_CHGPUMP_TO_CHARGER				1000

#define WPC_CHARGE_1A_UPPER_LIMIT							1050
#define WPC_CHARGE_1A_LOWER_LIMIT							950
#define WPC_CHARGE_800MA_UPPER_LIMIT						850
#define WPC_CHARGE_800MA_LOWER_LIMIT						750
#define WPC_CHARGE_500MA_UPPER_LIMIT						550
#define WPC_CHARGE_500MA_LOWER_LIMIT						450

#define WPC_CHARGE_VOLTAGE_DEFAULT							5000    //5V

#define WPC_CHARGE_VOLTAGE_VOOC_INIT						9000	//9V
#define WPC_CHARGE_VOLTAGE_CHGPUMP_TO_CHARGER				12000//15000
#define WPC_CHARGE_VOLTAGE_CHGPUMP_CV						18000
#define WPC_CHARGE_VOLTAGE_SVOOC_INIT						14000	//14V
#define WPC_CHARGE_VOLTAGE_FTM								16200
#define WPC_CHARGE_VOLTAGE_FASTCHG							12000
#define WPC_CHARGE_VOLTAGE_FASTCHG_MAX						15000
#define WPC_CHARGE_VOLTAGE_CHANGE_STEP_1V					1000
#define WPC_CHARGE_VOLTAGE_CHANGE_STEP_100MV				100
#define WPC_CHARGE_VOLTAGE_CHANGE_STEP_20MV					20

#define WPC_CHARGE_VOLTAGE_CHGPUMP_MAX						20100
#define WPC_CHARGE_VOLTAGE_CHGPUMP_MIN						5000

#define WPC_CHARGE_IOUT_HIGH_LEVEL							1050	//1050mA
#define WPC_CHARGE_IOUT_LOW_LEVEL							950		//950mA

#define WPC_TERMINATION_VOLTAGE_DEFAULT						4370
#define WPC_TERMINATION_VOLTAGE_ABNORMAL					3980
#define WPC_TERMINATION_VOLTAGE_FCC							4450
#define WPC_TERMINATION_VOLTAGE_CV_COLD                   	3980
#define WPC_TERMINATION_VOLTAGE_CV_COOL                   	4350
#define WPC_TERMINATION_VOLTAGE_CV_NORMAL                   4370
#define WPC_TERMINATION_VOLTAGE_CV_WARM                   	4350
#define WPC_TERMINATION_VOLTAGE_CV_HOT                   	4080
#define WPC_TERMINATION_VOLTAGE_OFFSET						50

#define WPC_TERMINATION_CURRENT								200
#define WPC_TERMINATION_VOLTAGE								WPC_TERMINATION_VOLTAGE_DEFAULT
#define WPC_RECHARGE_VOLTAGE_OFFSET							200
#define WPC_PRECHARGE_CURRENT								300
#define WPC_CHARGER_INPUT_CURRENT_LIMIT_DEFAULT				1000

#define DCP_TERMINATION_CURRENT								600
#define DCP_TERMINATION_VOLTAGE								4380
#define DCP_RECHARGE_VOLTAGE_OFFSET							200
#define DCP_PRECHARGE_CURRENT								300
#define DCP_CHARGER_INPUT_CURRENT_LIMIT_DEFAULT				1000
#define DCP_CHARGE_CURRENT_DEFAULT							1500

#define WPC_BATT_FULL_CNT									5
#define WPC_RECHARGE_CNT									5

#define WPC_INCREASE_CURRENT_DELAY                          2   
#define WPC_ADJUST_CV_DELAY									10
#define WPC_CEP_NONZERO_DELAY								1
				
int oppo_wpc_get_status(void);


#endif

