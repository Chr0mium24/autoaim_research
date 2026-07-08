/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       usb_task.c/h
  * @brief      usb outputs the error message.usb输出错误信息
  * @note       
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */
#include "Robo_USB.h"
#include "gpio.h"
#include "Robo_Common.h"

#include "stdint.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

//Prototypes of functions
static void send(ReceivePacket *packet);
static void receive(void);
static void init_packet(ReceivePacket *packet);
//End of prototypes


ReceivePacket send_packet;
SendPacket received_packet;

void USB_Init()
{
    MX_USB_DEVICE_Init();
	init_packet(&send_packet);
}

void USB_Task(void *conifg)
{
	receive();
	send(&send_packet);
}

static void send(ReceivePacket *packet)
{
	
}

/* 
 * @brief 接收函数
 */
static void receive(void)
{
	
}

/*
 * @brief 初始化发送数据包
 * @param packet 待初始化的数据包指针
*/
static void init_packet(ReceivePacket *packet)
{
	
}
