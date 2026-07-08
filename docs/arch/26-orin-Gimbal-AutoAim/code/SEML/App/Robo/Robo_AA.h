/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       auto_aiming_task.c/h
  * @brief      Get auto aiming parameters, solve trajectory, then offer feedback to gimbal aiming.
  * @note       
  *
  @verbatim
	@endverbatim
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */

#include <math.h>
#include <stdio.h>
#include "stdint.h"
#include "main.h"
#include "Robo_USB.h"
#include "Dji_Motor.h"
#include "EzTuner.h"


#ifndef __AUTOAIMING_TASK_H__
#define __AUTOAIMING_TASK_H__

typedef struct
{
	//Output to Bot
	float expect_delta_yaw;
	float expect_delta_pitch;
	
	//Output to PC
	ReceivePacket send;
	
	//Input from PC
	SendPacket receive;
	
	//PID
	PIDConfig_t* yaw_pid;
	PIDConfig_t* pitch_pid;
	
}auto_aiming_control_t;


extern auto_aiming_control_t  auto_aiming_control;

/**
  * @brief          auto_aiming task, created by main function
  * @param[in]      none
  * @retval         none
  */
extern void AA_Init();
extern void AA_Task(void *conifg);

#endif /*__AUTOAIMING_TASK_H__*/