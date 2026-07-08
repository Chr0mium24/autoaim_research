#ifndef _ROBO_GIMBAL_H_
#define _ROBO_GIMBAL_H_
#include "SEML_common.h"
#include "Robo_common.h"
#include "Dji_Motor.h"
#include "PID.h"
#include "EzTuner.h"
typedef struct 
{
	Motor_t *motor;
	PIDConfig_t speed_PID;
	PIDConfig_t position_PID;
	PIDConfig_t AA_PID;

	float send_data;
	float expect_speed;
	float expect_angle;
	float expect_AA;
}Gimbal_Joint_t;

typedef struct 
{
	Gimbal_Joint_t yaw;
	Gimbal_Joint_t pitch;
	const Euler_Data_t *imu;
}Task_Gimbal_t;

/**
 * @brief 云台任务初始化
 * 该函数执行云台任务相关的初始化操作
 * @note 该函数定义为弱函数，可以依此为模板定义新的同名函数替换
 */
void Gimbal_Init(void);

/**
 * @brief 云台任务
 * @note 该函数定义为弱函数，可以依此为模板定义新的同名函数替换
 */
void Gimbal_Task(void *conifg);

#endif
