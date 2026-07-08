/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       usb_task.c/h
  * @brief      usb outputs the error message.usbÊä³ö´íÎóÐÅÏ¢
  * @note       
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     -     			CodeAlan        1. solve trajectory
  *  V1.1.0		Jan-2-2024		Xindi Lv		2. complete air resistance model
  *	 V2.0.0		Jan-4-2024		HansonZhang		3. done
  *	 V2.0.1		Jan-5-2024		HansonZhang		4. bug fixed: PC communication failed when auto aiming enabled
  *	 V2.0.2		Jan-19-2024		HansonZhang		5. bug fixed: added restriction to auto aiming control speed
  *	 V3.0.0		Mar-19-2024		HansonZhang		6. deploy in event os
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */



#include <math.h>
#include <stdio.h>

#include "Robo_AA.h"
#include "gpio.h"

#include "main.h"
#include "Robo_Common.h"
#include "Robo_USB.h"
#include "pid.h"



void AA_Init()
{
    
}

void AA_Task(void *config)
{
    
}
