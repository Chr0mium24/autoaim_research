#include "robo_Common.h"
#include "can.h"
#include "Mahony.h"
#include "BMI088driver.h"
#include "button.h"
message_List_t robo_message;
Can_Handle_t can1,can2;
AHRS_t AHRS;
Control_Flag_t robo_control_flag;

Robo_Middleware_t robo_middleware;

void Robo_Middleware_Init(void)
{
	extern BMI088_Data_t BMI088;
	// 消息总线初始化
  message_List_Init(&robo_message,robo_middleware.robo_message_buffer,40,1000);
  // for(int i = 1;i < 15;i++)
  //   message_List_Register(&robo_message,i,NULL,NULL);
	float temp = 180;
	Robo_Push_Message_Cmd("Set_Yaw_Angle",temp);
	// can初始化
    SEML_CAN_Init(&can1,&hcan1,robo_middleware.can1_rx_buffer,10);
	HAL_CAN_RegisterCallback(&hcan1,HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID,SEML_CAN_RxCallback);
    SEML_CAN_Init(&can2,&hcan2,robo_middleware.can2_rx_buffer,10);
	HAL_CAN_RegisterCallback(&hcan2,HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID,SEML_CAN_RxCallback);
	// 按钮中断初始化
	SEML_Timer_Create(&robo_middleware.button_timer, Timer_Cycle, 10, Button_Scan, NULL);
	// AHRS初始化
	AHRS_Init(&AHRS,0.001f,Mahony_AHRS_Update,BMI088_Read,&BMI088,NULL,NULL);
	//Calibrate_IMU_Offset(&AHRS);
	SEML_Timer_Create(&robo_middleware.timer_AHRS,Timer_Cycle,1,AHRS_Update,&AHRS);
}

#define WORD_NUM ROBO_TOPIC_NUM
#define WORD_SIZE 30
/**
 * @brief 获取话题字符串的id
 * @param str 话题字符串
 * @return 话题id 
 */
uint16_t Get_Topic_ID(char *str)
{
	static char dictionary[WORD_NUM][WORD_SIZE] = {0};
	static uint16_t temp = 0, hit = 0,dictionary_num;
	temp = 0, hit = 0;
	// 求字符串哈希值
	for (int i = 0; str[i] != '\0'; i++)
	{
		temp ^= str[i];
	}
	// 开始遍历
	temp %= WORD_NUM;
	for (int i = 0; i <= WORD_NUM; i++)
	{
		// 查找不到进行添加
		if (dictionary[temp][0] == '\0')
			break;
		for (int j = 0; j < 30; j++)
		{
			// 字符串与字典匹配
			if (str[j] == '\0')
			{
				hit = 1;
				return temp;
			}
			if (str[j] != dictionary[temp][j])
				break;
		}
		temp ++;
		temp %= WORD_NUM;
	}
	dictionary_num ++;
	if(dictionary_num >= WORD_NUM)
		assert_param(0);
	for (int i = 0; str[i] != '\0'; i++)
	{
		dictionary[temp][i] = str[i];
	}
	return temp;
}