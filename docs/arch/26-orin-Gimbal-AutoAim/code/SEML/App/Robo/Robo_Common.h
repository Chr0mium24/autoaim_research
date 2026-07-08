#ifndef _ROBO_COMMON_H_
#define _ROBO_COMMON_H_
#include "SEML_common.h"
#include "message.h"
#include "timer.h"
#include "can_if.h"
#include "AHRS.h"

#define ROBO_TOPIC_NUM 50
typedef struct 
{
	message_Pack_t can1_rx_buffer[10];
	message_Pack_t can2_rx_buffer[10];
	message_Pack_t robo_message_buffer[ROBO_TOPIC_NUM];
	Soft_Timer_t button_timer, timer_AHRS;
}Robo_Middleware_t;

typedef struct
{
	uint16_t remote_off : 1;
} Control_Flag_t;

typedef struct
{
	enum
	{
		Yaw_Follow_Mode = 1,
		Rotate_Made,
		Chassis_Normal_Mode
	} rotate : 2; /**< 底盘运动模式 */
} Chassis_Mode_t;

typedef enum
{
	Shoot_Off = 0,
	Shoot_Ready,
	Shoot_Single_Shot,
	Shoot_Fire
} Shoot_Status_t;

extern message_List_t robo_message;
extern Can_Handle_t can1, can2;
extern AHRS_t AHRS;
extern Control_Flag_t robo_control_flag;

#define Robo_Init()         \
	do                        \
	{                         \
		Robo_Middleware_Init(); \
	} while (0)

void Robo_Middleware_Init(void);

/**
 * @brief 向机器人消息列表发送控制码
 * @param[in] topic 话题
 * @param[in] cmd 控制码
 */
#define Robo_Push_Message_Cmd(topic, cmd) message_List_Update_Cmd(&robo_message, Get_Topic_ID(topic), *(Cmd_t *)&(cmd))
/**
 * @brief 向机器人消息列表发送数据
 * @param[in] topic 话题
 * @param[in] data 数据指针
 * @param[in] longth 数据长度
 */
#define Robo_Push_message_Data(topic, data, longth) message_List_Update_Data(&robo_message, Get_Topic_ID(topic), data, longth)

/**
 * @brief 从机器人消息列表读取控制码
 * @param[in] topic 话题
 * @param[out] cmd 控制码
 */
#define Robo_Get_Message_Cmd(topic, cmd) *(Cmd_t *)&(cmd) = Get_message_ID_Cmd(&robo_message, Get_Topic_ID(topic))

/**
 * @brief 从机器人消息列表读取数据
 * @param[in] topic 话题
 * @param[out] pdat 数据指针
 * @return 数据长度
 */
#define Robo_Get_message_Data(topic, pdat) Get_message_ID_Data(&robo_message, Get_Topic_ID(topic), pdat)

/**
 * @brief 获取话题字符串的id
 * @param str 话题字符串
 * @return 话题id
 */
uint16_t Get_Topic_ID(char *str);
#endif
