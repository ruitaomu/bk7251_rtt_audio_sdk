#ifndef _MANUAL_PS_PUB_H_
#define _MANUAL_PS_PUB_H_

//#define BK_DEEP_SLEEP_DEBUG
#ifdef  BK_DEEP_SLEEP_DEBUG
#define BK_DEEP_SLEEP_PRT  rt_kprintf
#else
#define BK_DEEP_SLEEP_PRT   os_null_printf
#endif

typedef enum { 
	PS_DEEP_WAKEUP_GPIO = 1,
	PS_DEEP_WAKEUP_RTC = 2, 
	PS_DEEP_WAKEUP_USB = 4,
} PS_DEEP_WAKEUP_WAY;

typedef struct  ps_deep_ctrl{

	/*deep_sleep 模式下唤醒方式*/
    PS_DEEP_WAKEUP_WAY wake_up_way;
	
	/** @brief	Request deep sleep,and wakeup by gpio.
	 *
	 *	@param	gpio_index_lo_map:The gpio bitmap which set 1 enable wakeup deep sleep.
	 *				gpio_index_lo_map is hex and every bits is map to gpio0-gpio31.
	 *				ps:gpio1 as uart RX pin must be wake up from falling
	 *			gpio_edge_lo_map:The gpio edge bitmap for wakeup gpios,
	 *				gpio_edge_lo_map is hex and every bits is map to gpio0-gpio31.
	 *				0:rising,1:falling.
     *		gpio_stay_lo_map:The gpio bitmap which need stay ,not change in deep sleep.
	 * 			 gpio_index_lo_map is hex and every bits is map to gpio0-gpio31.
     *		gpio_index_hi_map:The gpio bitmap which set 1 enable wakeup deep sleep.
	 * 			 gpio_index_lo_map is hex and every bits is map to gpio32-gpio39.
	 * 		gpio_edge_hi_map:The gpio edge bitmap for wakeup gpios,
	 * 			 gpio_edge_lo_map is hex and every bits is map to gpio32-gpio39.
	 * 			 0:rising,1:falling.
  	 *		gpio_stay_hi_map:The gpio bitmap which need stay ,not change in deep sleep.
     * 			 gpio_index_lo_map is hex and every bits is map to gpio32-gpio39.
	 */
	
	UINT32 gpio_index_lo_map;
	UINT32 gpio_edge_lo_map;
	UINT32 gpio_stay_lo_map;
	UINT32 gpio_index_hi_map;
	UINT32 gpio_edge_hi_map;
	UINT32 gpio_stay_hi_map;
	
	UINT32 sleep_time;
}PS_DEEP_CTRL_PARAM;

typedef enum { 
	RESET_SOURCE_POWERON = 0,
	RESET_SOURCE_REBOOT = 1,
	RESET_SOURCE_DEEPPS_GPIO = 2, 
	RESET_SOURCE_DEEPPS_RTC = 3, 
	RESET_SOURCE_DEEPPS_USB = 4,
	RESET_SOURCE_ABNORMAL = 5,
} RESET_SOURCE_STATUS;

#define     PS_SUPPORT_MANUAL_SLEEP     0
typedef void (*ps_wakeup_cb)(void);
extern void bk_enter_deep_sleep_mode(PS_DEEP_CTRL_PARAM *deep_param);
extern RESET_SOURCE_STATUS sctrl_get_wake_soure_status(void);

#endif

