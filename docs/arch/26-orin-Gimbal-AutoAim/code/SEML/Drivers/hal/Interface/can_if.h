/**
  ******************************************************************************
  * @copyright (c) 2023 - ~, Singularity
  * @file   : can_if.h
  * @author : SY7_yellow
  * @brief  : CAN接口文件
  * @date   : 2023-8-8
  * @par Change Log：
  * <table>
  * <tr><th>Date           <th>Version     <th>Author      <th>Description
  * <tr><td>2023-08-08     <td>0.1         <td>SY7_yellow  <td>实现了基础功能
	* <tr><td>2023-08-08     <td>1.0         <td>SY7_yellow  <td>简化初始化所需代码量
  * </table>
  *@details :
  * ============================================================================
  *                       How to use this driver  
  * ============================================================================
  * 1. 配置cubeMX时使能CAN接收中断,使能can的注册回调
	* 2. 调用SEML_CAN_Init()对seml的can接口进行初始化
	* 3. 调用HAL_CAN_RegisterCallback()注册SEML_CAN_RxCallback为接收回调函数。
	* 4. 对HAL库的can进行初始化,配置好过滤器,注意需要先注册回调后再初始化
  ******************************************************************************
  * @attention:
  * 
  * 文件编码：UTF-8,出现乱码请勿上传! \n
  * 修改后测试没问题记得修改版本号,未经过测试的请使用加上后缀alpha,beta...并且请
  * 勿合并到master. \n
  * 
  * 防御性编程,对输入参数做有效性检查,并返回错误号. \n
  * 不要包含太多不相关的头文件. \n
  * 若发现bug请提交issue,详细描述现象(必须)和复现条件(选填),以便对应的负责人能
  * 够准确定位修复. \n
  * 若是存在多线程同时编辑的情况，请使用互斥锁防止某进程编辑时候被其他进程访问。
  * File encoding:UTF-8,Do not upload garbled code!\n
  * Please remember to change the version number. If you have not been tested, 
  * please use the suffix alpha,beta... And do not merge to master. \n
  * Defensive programming, where input arguments are checked for validity and
  * an error number is returned. \n
  * Don't include too many irrelevant headers. \n
  * If you find a bug, file an issue with a detailed description of the 
  * phenomenon (required) and conditions for reoccurrence (optional) so that 
  * the appropriate owner can locate the correct fix. \n
  * In the case of simultaneous editing by multiple threads, use a mutex to 
  * prevent one process from being accessed by other processes while it is 
  * editing. \n
  ******************************************************************************
  */
#ifndef _CAN_IF_H_
#define _CAN_IF_H_
#include "SEML_common.h"
#include "message.h"
#ifndef CAN_BUFFER_SIZE
#define CAN_BUFFER_SIZE 3
#endif
#define CAN_TIMEOUT  1000
typedef CAN_HandleTypeDef CAN_IF_t;

typedef struct 
{
	CAN_IF_t *hcan;
	message_List_t Rxmessage_list;
	CAN_TxHeaderTypeDef Txmessage_Buffer[CAN_BUFFER_SIZE];
	uint16_t Txmessage_front;
	uint16_t Txmessage_rear;
	void *next;
} Can_Handle_t;

#include "Robo_Common.h"

/**
 * @brief can初始化函数
 * 
 * @param can_handle can句柄
 * @param hcan 底层can配置句柄
 * @param id_buffer id缓存数组指针
 * @param size 缓存数组大小
 */
void SEML_CAN_Init(Can_Handle_t *can_handle,void *hcan, message_Pack_t *id_buffer, uint16_t size);
/**
 * @brief can发送函数
 * @param can_handle can句柄
 * @param ID 报文ID
 * @param aData 数据指针
 * @param size 数据长度
 * @param message_type 消息类型
 * @return 函数执行状态
 * @retval SEML_OK 发送完成
 * @retval SEML_ERROR 发送失败
 */
SEML_StatusTypeDef SEML_CAN_Send(Can_Handle_t *can_handle, uint16_t ID, uint8_t aData[], uint8_t size, uint16_t message_type);

/**
 * @brief can接收回调函数注册
 * 
 * @param can_handle can句柄
 * @param ID 报文ID
 * @param fun 回调函数
 * @param config 回调函数配置
 */
void SEML_CAN_Rxmessage_Register(Can_Handle_t *can_handle,uint16_t ID,message_callback_fun_t fun,void* config);

/**
 * @brief can接收回调函数
 * @param hcan 底层can配置句柄
 */
void SEML_CAN_RxCallback(CAN_IF_t *hcan);
#endif
