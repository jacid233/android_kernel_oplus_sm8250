/************************************************************************************
** File:  oppo_p922x.h
** OPLUS_FEATURE_CHG_BASIC
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
** 
** Description: 
**      for wireless charge
** 
** Version: 1.0
** Date created: 21:03:46,06/11/2018
** Author: Lin Shangbo
** 
** --------------------------- Revision History: ------------------------------------------------------------
* <version>       <date>        <author>              			<desc>
* Revision 1.0    2018-11-06    Lin Shangbo   		Created for wireless charge
************************************************************************************************************/
#ifndef __OPPO_P922X_H__
#define __OPPO_P922X_H__

#define SUPPORT_WPC

enum {
WPC_CHG_STATUS_DEFAULT,
WPC_CHG_STATUS_READY_FOR_FASTCHG,
WPC_CHG_STATUS_WAIT_IOUT_READY,
WPC_CHG_STATUS_WAITING_FOR_TX_INTO_FASTCHG,
WPC_CHG_STATUS_INCREASE_VOLTAGE_FOR_CHARGEPUMP,
WPC_CHG_STATUS_INCREASE_CURRENT_FOR_CHARGER,
WPC_CHG_STATUS_ADJUST_VOL_AFTER_INC_CURRENT,
WPC_CHG_STATUS_FAST_CHARGING_FROM_CHARGER,
WPC_CHG_STATUS_FAST_CHARGING_FROM_CHGPUMP,
WPC_CHG_STATUS_READY_FOR_EPP,
WPC_CHG_STATUS_CHARGER_FASTCHG_INIT,
WPC_CHG_STATUS_CHAREPUMP_FASTCHG_INIT,
WPC_CHG_STATUS_EPP_WORKING,
WPC_CHG_STATUS_READY_FOR_FTM,
WPC_CHG_STATUS_FTM_WORKING,
WPC_CHG_STATUS_DECREASE_IOUT_TO_200MA,
WPC_CHG_STATUS_DECREASE_VOUT_TO_12V,
WPC_CHG_STATUS_DECREASE_VOUT_FOR_RESTART,
WPC_CHG_STATUS_CHARGEPUMP_INIT,
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

typedef enum
{
FASTCHARGE_LEVEL_UNKNOW,
FASTCHARGE_LEVEL_1,
FASTCHARGE_LEVEL_2,
FASTCHARGE_LEVEL_3,
FASTCHARGE_LEVEL_4,
FASTCHARGE_LEVEL_5,
FASTCHARGE_LEVEL_6,
FASTCHARGE_LEVEL_7,
FASTCHARGE_LEVEL_8,
FASTCHARGE_LEVEL_9,
FASTCHARGE_LEVEL_NUM,
}E_FASTCHARGE_LEVEL;

typedef enum
{
RX_RUNNING_MODE_UNKNOW,
RX_RUNNING_MODE_EPP,
RX_RUNNING_MODE_BPP,
RX_RUNNING_MODE_OTHERS,
}E_RX_MODE;

typedef enum
{
ADAPTER_POWER_NUKNOW,
ADAPTER_POWER_20W,
ADAPTER_POWER_30W,
ADAPTER_POWER_50W,
ADAPTER_POWER_65W,
ADAPTER_POWER_MAX = ADAPTER_POWER_65W,
}E_ADAPTER_POWER;

typedef enum {
    WPC_DISCHG_STATUS_OFF,
    WPC_DISCHG_STATUS_ON,
    WPC_DISCHG_IC_READY,
    WPC_DISCHG_IC_PING_DEVICE,
    WPC_DISCHG_IC_TRANSFER,
    WPC_DISCHG_IC_ERR_TX_RXAC,
    WPC_DISCHG_IC_ERR_TX_OCP,
    WPC_DISCHG_IC_ERR_TX_OVP,
    WPC_DISCHG_IC_ERR_TX_LVP,
    WPC_DISCHG_IC_ERR_TX_FOD,
    WPC_DISCHG_IC_ERR_TX_OTP,
    WPC_DISCHG_IC_ERR_TX_CEPTIMEOUT,
    WPC_DISCHG_IC_ERR_TX_RXEPT,
    WPC_DISCHG_STATUS_UNKNOW,
}E_WPC_DISCHG_STATUS;
#define FASTCHG_CURR_MAX_UA         1500000

#define P922X_REG_RTX_STATUS                                 0x78
#define P922X_RTX_READY                                     BIT(0)
#define P922X_RTX_DIGITALPING                               BIT(1)
#define P922X_RTX_ANALOGPING                                BIT(2)
#define P922X_RTX_TRANSFER                                  BIT(3)

#define P922X_REG_RTX_ERR_STATUS                            0x79
#define P922X_REG_RTX_ERR_TX_RXAC                           BIT(0)
#define P922X_REG_RTX_ERR_TX_OCP                            BIT(1)
#define P922X_REG_RTX_ERR_TX_OVP                            BIT(2)
#define P922X_REG_RTX_ERR_TX_LVP                            BIT(3)
#define P922X_REG_RTX_ERR_TX_FOD                            BIT(4)
#define P922X_REG_RTX_ERR_TX_OTP                            BIT(5)
#define P922X_REG_RTX_ERR_TX_CEPTIMEOUT                     BIT(6)
#define P922X_REG_RTX_ERR_TX_RXEPT                          BIT(7)


#define P922X_PRE2CC_CHG_THD_LO                             3400
#define P922X_PRE2CC_CHG_THD_HI                             3420

#define P922X_CC2CV_CHG_THD_LO                              4350
#define P922X_CC2CV_CHG_THD_HI                              4370

/*The message to WPC DOCK */
#define P9221_CMD_INDENTIFY_ADAPTER							0xA1 
#define P9221_CMD_INTO_FASTCHAGE							0xA2
#define P9221_CMD_INTO_USB_CHARGE							0xA3
#define P9221_CMD_INTO_NORMAL_CHARGE						0xA4
#define P9221_CMD_SET_FAN_WORK								0xA5
#define P9221_CMD_SET_FAN_SILENT							0xA6
#define P9221_CMD_RESET_SYSTEM								0xA9
#define P9221_CMD_ENTER_TEST_MODE							0xAA
#define P9221_CMD_GET_FW_VERSION							0xAB
#define P9221_CMD_SET_LED_BRIGHTNESS						0xAC
#define P9221_CMD_SET_CEP_TIMEOUT							0xAD
#define P9221_CMD_SET_PWM_PULSE								0xAE
#define P9221_CMD_SEND_1ST_RANDOM_DATA						0xB1
#define P9221_CMD_SEND_2ND_RANDOM_DATA						0xB2
#define P9221_CMD_SEND_3RD_RANDOM_DATA						0xB3
#define P9221_CMD_GET_1ST_ENCODE_DATA						0xB4
#define P9221_CMD_GET_2ND_ENCODE_DATA						0xB5
#define P9221_CMD_GET_3RD_ENCODE_DATA						0xB6
#define P9221_CMD_NULL										0x00

/*The message from WPC DOCK */
#define P9237_RESPONE_ADAPTER_TYPE							0xF1
#define P9237_RESPONE_INTO_FASTCHAGE						0xF2
#define P9237_RESPONE_INTO_USB_CHARGE						0xF3
#define P9237_RESPONE_INTO_NORMAL_CHARGER					0xF4
#define P9237_RESPONE_FAN_WORK								0xF5
#define P9237_RESPONE_FAN_SILENT							0xF6
#define P9237_RESPONE_WORKING_IN_EPP						0xF8
#define P9237_RESPONE_RESET_SYSTEM							0xF9
#define P9237_RESPONE_ENTER_TEST_MODE						0xFA
#define P9237_RESPONE_FW_VERSION							0xFB
#define P9237_RESPONE_LED_BRIGHTNESS						0xFC
#define P9237_RESPONE_CEP_TIMEOUT							0xFD
#define P9237_RESPONE_PWM_PULSE								0xFE
#define P9237_RESPONE_RX_1ST_RANDOM_DATA					0xE1
#define P9237_RESPONE_RX_2ND_RANDOM_DATA					0xE2
#define P9237_RESPONE_RX_3RD_RANDOM_DATA					0xE3
#define P9237_RESPONE_SEND_1ST_DECODE_DATA					0xE4
#define P9237_RESPONE_SEND_2ND_DECODE_DATA					0xE5
#define P9237_RESPONE_SEND_3RD_DECODE_DATA					0xE6
#define P9237_RESPONE_NULL									0x00

#define ADAPTER_TYPE_UNKNOW									0
#define ADAPTER_TYPE_VOOC									1
#define ADAPTER_TYPE_SVOOC									2
#define ADAPTER_TYPE_USB									3
#define ADAPTER_TYPE_NORMAL_CHARGE							4
#define ADAPTER_TYPE_EPP									5
#define ADAPTER_TYPE_SVOOC_50W								6

#define WPC_CHARGE_TYPE_DEFAULT								0
#define WPC_CHARGE_TYPE_FAST								1
#define WPC_CHARGE_TYPE_USB									2
#define WPC_CHARGE_TYPE_NORMAL								3
#define WPC_CHARGE_TYPE_EPP									4

#define P922X_TASK_INTERVAL							round_jiffies_relative(msecs_to_jiffies(500))	
#define P922X_CEP_INTERVAL							round_jiffies_relative(msecs_to_jiffies(200))
#define P922X_UPDATE_INTERVAL							round_jiffies_relative(msecs_to_jiffies(3000))
#define P922X_UPDATE_RETRY_INTERVAL						round_jiffies_relative(msecs_to_jiffies(500))
#define WPC_DISCHG_WAIT_READY_EVENT						round_jiffies_relative(msecs_to_jiffies(200))
#define WPC_DISCHG_WAIT_DEVICE_EVENT						round_jiffies_relative(msecs_to_jiffies(90*1000))

#define TEMPERATURE_COLD_LOW_LEVEL							-20
#define TEMPERATURE_COLD_HIGH_LEVEL							0
#define TEMPERATURE_NORMAL_LOW_LEVEL						TEMPERATURE_COLD_HIGH_LEVEL
#define TEMPERATURE_NORMAL_HIGH_LEVEL						440
#define TEMPERATURE_HOT_LOW_LEVEL							TEMPERATURE_NORMAL_HIGH_LEVEL
#define TEMPERATURE_HOT_HIGH_LEVEL							530

#define WPC_CHARGE_FFC_TEMP_MIN								120
#define WPC_CHARGE_FFC_TEMP_MED								350
#define WPC_CHARGE_FFC_TEMP_MAX								420

#define WPC_FASTCHG_TEMP_MIN								0
#define WPC_FASTCHG_TEMP_MAX								450

#define WPC_CHARGE_TEMP_MIN									TEMPERATURE_COLD_LOW_LEVEL
#define WPC_CHARGE_TEMP_MAX									TEMPERATURE_HOT_HIGH_LEVEL

#define TEMPERATURE_STATUS_UNKNOW							0
#define TEMPERATURE_STATUS_FFC_1							1
#define TEMPERATURE_STATUS_FFC_2							2
#define TEMPERATURE_STATUS_OTHER							3

#define TEMPERATURE_STATUS_CHANGE_TIMEOUT					10		//about 10s
#define WPC_CHARGE_CURRENT_LIMIT_300MA						300
#define WPC_CHARGE_CURRENT_LIMIT_200MA						200    
#define WPC_CHARGE_CURRENT_ZERO								0		//0mA
#define WPC_CHARGE_CURRENT_200MA							200
#define WPC_CHARGE_CURRENT_DEFAULT							500		//500mA
#define WPC_CHARGE_CURRENT_BPP_INIT							150
#define WPC_CHARGE_CURRENT_BPP								1000
#define WPC_CHARGE_CURRENT_EPP_INIT							200
#define WPC_CHARGE_CURRENT_EPP								800
#define WPC_CHARGE_CURRENT_EPP_SPEC							300
#define WPC_CHARGE_CURRENT_FASTCHG_INT                      300
#define WPC_CHARGE_CURRENT_FASTCHG_END						700		//300mA
#define WPC_CHARGE_CURRENT_FASTCHG_MID						800		//800mA
#define WPC_CHARGE_CURRENT_FASTCHG							1200	//1500mA
#define WPC_CHARGE_CURRENT_CHANGE_STEP_200MA				200		//200mA
#define WPC_CHARGE_CURRENT_CHANGE_STEP_50MA					50		//50mA
#define WPC_CHARGE_CURRENT_FFC_TO_CV						1000	//1000mA
#define WPC_CHARGE_CURRENT_CHGPUMP_TO_CHARGER				1000

#define WPC_CHARGE_CURRENT_OFFSET							50

#define WPC_CHARGE_VOLTAGE_DEFAULT							5000    //5V

#define WPC_CHARGE_VOLTAGE_FASTCHG_INIT						12000
#define WPC_CHARGE_VOLTAGE_CHGPUMP_TO_CHARGER				12000
#define WPC_CHARGE_VOLTAGE_FTM								16200
#define WPC_CHARGE_VOLTAGE_FASTCHG							12000
#define WPC_CHARGE_VOLTAGE_FASTCHG_MIN						12000
#define WPC_CHARGE_VOLTAGE_FASTCHG_MAX						15000
#define WPC_CHARGE_VOLTAGE_CHANGE_STEP_1V					1000
#define WPC_CHARGE_VOLTAGE_CHANGE_STEP_100MV				100
#define WPC_CHARGE_VOLTAGE_CHANGE_STEP_20MV					20

#define WPC_CHARGE_VOLTAGE_CHGPUMP_MAX						20100
#define WPC_CHARGE_VOLTAGE_CHGPUMP_MIN						5000

#define WPC_TERMINATION_VOLTAGE_FFC							4500
#define WPC_TERMINATION_VOLTAGE_FFC1						4445
#define WPC_TERMINATION_VOLTAGE_FFC2						4445
#define WPC_TERMINATION_VOLTAGE_OTHER						4500
#define WPC_TERMINATION_VOLTAGE_CV  						4435

#define WPC_TERMINATION_CURRENT_FFC1						200
#define WPC_TERMINATION_CURRENT_FFC2						250
#define WPC_TERMINATION_CURRENT_OTHER						0

#define WPC_TERMINATION_CURRENT								100
#define WPC_TERMINATION_VOLTAGE								4500

#define WPC_RECHARGE_VOLTAGE_OFFSET							200
#define WPC_CHARGER_INPUT_CURRENT_LIMIT_INIT				200
#define WPC_CHARGER_INPUT_CURRENT_LIMIT_DEFAULT				500

#define DCP_TERMINATION_CURRENT								600
#define DCP_TERMINATION_VOLTAGE								4380
#define DCP_RECHARGE_VOLTAGE_OFFSET							200
#define DCP_PRECHARGE_CURRENT								300
#define DCP_CHARGER_INPUT_CURRENT_LIMIT_DEFAULT				1000
#define DCP_CHARGE_CURRENT_DEFAULT							1500

#define WPC_BATT_FULL_CNT									4
#define WPC_RECHARGE_CNT									5

#define WPC_INCREASE_CURRENT_DELAY                          2   
#define WPC_ADJUST_CV_DELAY									10
#define WPC_ADJUST_CURRENT_DELAY							0
#define WPC_SKEW_DETECT_DELAY								2

#define FAN_PWM_PULSE_IN_SILENT_MODE						60
#define FAN_PWM_PULSE_IN_FASTCHG_MODE						93

#define BPP_CURRENT_INCREASE_TIME							20

#define CHARGEPUMP_DETECT_CNT								40

enum wireless_mode {
	WIRELESS_MODE_NULL,
	WIRELESS_MODE_TX,
	WIRELESS_MODE_RX,
};
enum FASTCHG_STARTUP_STEP {
	FASTCHG_EN_CHGPUMP1_STEP,
	FASTCHG_EN_PMIC_CHG_STEP,
	FASTCHG_WAIT_PMIC_STABLE_STEP,
	FASTCHG_SET_CHGPUMP2_VOL_STEP,
	FASTCHG_SET_CHGPUMP2_VOL_AGAIN_STEP,
	FASTCHG_EN_CHGPUMP2_STEP,
	FASTCHG_CHECK_CHGPUMP2_STEP,
	FASTCHG_CHECK_CHGPUMP2_AGAIN_STEP,
};
enum WLCHG_TEMP_REGION_TYPE {
	WLCHG_BATT_TEMP_COLD = 0,
	WLCHG_BATT_TEMP_LITTLE_COLD,
	WLCHG_BATT_TEMP_COOL,
	WLCHG_BATT_TEMP_LITTLE_COOL,
	WLCHG_BATT_TEMP_PRE_NORMAL,
	WLCHG_BATT_TEMP_NORMAL,
	WLCHG_BATT_TEMP_WARM,
	WLCHG_BATT_TEMP_HOT,
	WLCHG_TEMP_REGION_MAX,
};

struct wpc_chg_param_t{
	int pre_input_ma;
	int iout_ma;
	int bpp_input_ma;
	int epp_input_ma;
	int epp_temp_warm_input_ma;
	int epp_input_step_ma;
	int vooc_input_ma;
	int vooc_iout_ma;
	int svooc_input_ma;
	int svooc_65w_iout_ma;
	int svooc_50w_iout_ma;
	int bpp_temp_cold_fastchg_ma;						// -2
	int vooc_temp_little_cold_fastchg_ma;		// 0
	int svooc_temp_little_cold_iout_ma;
	int svooc_temp_little_cold_fastchg_ma;
	int bpp_temp_little_cold_fastchg_ma;
	int epp_temp_little_cold_fastchg_ma;
	int vooc_temp_cool_fastchg_ma;				// 5
	int svooc_temp_cool_iout_ma;
	int svooc_temp_cool_fastchg_ma;
	int bpp_temp_cool_fastchg_ma;
	int epp_temp_cool_fastchg_ma;
	int vooc_temp_little_cool_fastchg_ma;		// 12
	int svooc_temp_little_cool_fastchg_ma;
	int bpp_temp_little_cool_fastchg_ma;
	int epp_temp_little_cool_fastchg_ma;
	int vooc_temp_normal_fastchg_ma;		// 16
	int svooc_temp_normal_fastchg_ma;
	int bpp_temp_normal_fastchg_ma;
	int epp_temp_normal_fastchg_ma;
	int vooc_temp_warm_fastchg_ma;		// 45
	int svooc_temp_warm_fastchg_ma;
	int bpp_temp_warm_fastchg_ma;
	int epp_temp_warm_fastchg_ma;
	int curr_cp_to_charger;
};

//extern struct oppo_chg_chip *p922x_chip;
struct wpc_data{
	char				charge_status;
	E_WPC_DISCHG_STATUS	wpc_dischg_status;
	enum FASTCHG_STARTUP_STEP fastchg_startup_step;
	enum WLCHG_TEMP_REGION_TYPE temp_region;
	struct wpc_chg_param_t wpc_chg_param;
	bool tx_online;
	bool tx_present;
	bool				charge_online;
	int					send_message;
	int 				send_msg_timer;
	int					send_msg_cnt;
	int					adapter_type;
	int					charge_type;
	int					dock_version;
	int					charge_voltage;
	int					charge_current;
	int					cc_value_limit;
	int					vout;
	int					vrect;
	int					iout;
	int					terminate_voltage;
	int					terminate_current;
	int					epp_current_limit;
	int					epp_current_step;
	bool				epp_chg_allow;
	bool				check_fw_update;
	bool				idt_fw_updating;
	bool				CEP_ready;
	bool				fastchg_ing;
	bool				epp_working;
	bool				vbatt_full;
	bool				ftm_mode;
	bool				wpc_reach_stable_charge;
	bool				wpc_reach_4370mv;
	bool				wpc_reach_4500mv;
	bool				wpc_ffc_charge;
	bool				wpc_skewing_proc;
	E_FASTCHARGE_LEVEL	fastcharge_level;
	int					iout_stated_current;
	int					fastchg_current_limit;
	int 				fastchg_allow;
	int					dock_fan_pwm_pulse;
	int					dock_led_pwm_pulse;
	int					BPP_fastchg_current_ma;
	int					BPP_current_limit;
	int					BPP_current_step_cnt;
	bool				has_reach_max_temperature;	
	char				CEP_value;
	E_RX_MODE			rx_runing_mode;
	E_ADAPTER_POWER		adapter_power;
	int freq_threshold;
	int freq_check_count;
	bool freq_thr_inc;
	bool is_deviation;
	bool deviation_check_done;
	bool vbat_too_high;
	bool curr_limit_mode;
	bool vol_set_ok;
	bool curr_set_ok;
	bool vol_set_start;
	bool fastchg_mode;
	bool startup_fast_chg;
	bool cep_err_flag;
	bool cep_exit_flag;
	bool cep_check;
	/* Exit fast charge, check ffc condition */
	bool ffc_check;
	/* Record if the last current needs to drop */
	bool curr_need_dec;
	bool vol_not_step;
	bool is_power_changed;
	int max_current;
	int target_curr;
	int target_vol;
	int vol_set;
	bool vout_debug_mode;
	bool iout_debug_mode;
	bool work_silent_mode;
	bool call_mode;
	bool wpc_self_reset;
	bool idt_adc_test_enable;
	bool idt_adc_test_result;
	bool need_doublecheck_to_cp;
	bool doublecheck_ok;
	char FOD_parameter;
	bool skewing_info;
	int cep_info;
};

struct oppo_p922x_ic{
	struct i2c_client				 *client;
	struct device					 *dev;

	struct power_supply *wireless_psy;
	enum power_supply_type wireless_type;
	enum wireless_mode wireless_mode;
	bool disable_charge;
	int quiet_mode_need;


	int				idt_en_gpio;		//for WPC
	int				idt_con_gpio;	//for WPC
	int				idt_con_irq;	//for WPC
	int				idt_int_gpio;	//for WPC
	int				idt_int_irq;	//for WPC
	int				vbat_en_gpio;	//for WPC
	int				booster_en_gpio;	//for WPC
	int             ext1_wired_otg_en_gpio;
	int             ext2_wireless_otg_en_gpio;
	int             cp_ldo_5v_gpio;
    struct pinctrl                       *pinctrl;
	struct pinctrl_state 		*idt_con_active;	//for WPC
	struct pinctrl_state 		*idt_con_sleep;		//for WPC
	struct pinctrl_state 		*idt_con_default;	//for WPC
	struct pinctrl_state 		*idt_int_active;	//for WPC
	struct pinctrl_state 		*idt_int_sleep;		//for WPC
	struct pinctrl_state 		*idt_int_default;	//for WPC
	struct pinctrl_state 		*vbat_en_active;	//for WPC
	struct pinctrl_state 		*vbat_en_sleep;		//for WPC
	struct pinctrl_state 		*vbat_en_default;	//for WPC
	struct pinctrl_state 		*booster_en_active;	//for WPC
	struct pinctrl_state 		*booster_en_sleep;		//for WPC
	struct pinctrl_state 		*booster_en_default;	//for WPC
	struct pinctrl_state 		*idt_con_out_active;	//for WPC
	struct pinctrl_state 		*idt_con_out_sleep;		//for WPC
	struct pinctrl_state 		*idt_con_out_default;	//for WPC

	struct pinctrl_state 		*ext1_wired_otg_en_active;	//for WPC
	struct pinctrl_state 		*ext1_wired_otg_en_sleep;		//for WPC
	struct pinctrl_state 		*ext1_wired_otg_en_default;	//for WPC
	struct pinctrl_state 		*ext2_wireless_otg_en_active;	//for WPC
	struct pinctrl_state 		*ext2_wireless_otg_en_sleep;		//for WPC
	struct pinctrl_state 		*ext2_wireless_otg_en_default;	//for WPC
	struct pinctrl_state 		*cp_ldo_5v_active;	//for WPC
	struct pinctrl_state 		*cp_ldo_5v_sleep;		//for WPC
	struct pinctrl_state 		*cp_ldo_5v_default;	//for WPC

    struct delayed_work idt_con_work;   //for WPC
    struct delayed_work p922x_task_work;    //for WPC
    struct delayed_work p922x_CEP_work; //for WPC
    struct delayed_work p922x_update_work;  //for WPC
    struct delayed_work p922x_test_work;    //for WPC
    struct delayed_work idt_event_int_work; //for WPC
    struct delayed_work idt_connect_int_work;   //for WPC
    struct delayed_work idt_dischg_work;   //for WPC
    struct delayed_work p922x_self_reset_work;  //for WPC
    struct wpc_data p922x_chg_status;   //for WPC
    //int         batt_volt_2cell_max;    //for WPC
    //int         batt_volt_2cell_min;    //for WPC
    atomic_t                         suspended;
};


bool p922x_wpc_get_fast_charging(void);
bool p922x_wpc_get_ffc_charging(void);
bool p922x_wpc_get_normal_charging(void);
bool p922x_wpc_get_otg_charging(void);
void p922x_set_rtx_function_prepare(void);
void p922x_set_rtx_function(bool is_on);
void p922x_wpc_print_log(void);
void p922x_set_vbat_en_val(int value);
int p922x_get_vbat_en_val(void);
void p922x_set_booster_en_val(int value);
int p922x_get_booster_en_val(void);
bool p922x_wireless_charge_start(void);
void p922x_set_wireless_charge_stop(void);
bool p922x_firmware_is_updating(void);
void p922x_set_ext1_wired_otg_en_val(int value);
int p922x_get_ext1_wired_otg_en_val(void);
void p922x_set_ext2_wireless_otg_en_val(int value);
int p922x_get_ext2_wireless_otg_en_val(void);
void p922x_set_cp_ldo_5v_val(int value);
int p922x_get_cp_ldo_5v_val(void);
int p922x_wpc_get_adapter_type(void);
bool p922x_check_chip_is_null(void);
int p922x_set_tx_cep_timeout_1500ms(void);
int p922x_get_CEP_flag(struct oppo_p922x_ic * chip);
//int p922x_set_dock_fan_pwm_pulse(int pwm_pulse);
#endif

