/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       usb_task.c/h
  * @brief      no action.
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */
#ifndef USB_TASK_H
#define USB_TASK_H

#include "stdint.h"

typedef struct __attribute__((packed))
{
  uint16_t cmd;

  uint8_t checksum;
}SendPacket;

typedef struct __attribute__((packed))
{
 
  uint16_t cmd;
  
  uint8_t checksum;

} ReceivePacket;

extern void USB_Init();
extern void USB_Task(void *conifg);

#endif
