/**
 ******************************************************************************
 * @copyright (c) 2023 - ~, Singularity
 * @file   : DJI_Motor.h
 * @author : SY7_yellow
 * @brief  : 大疆电机通讯模块
 * @date   : 2023-8-8
 * @details :
 * ============================================================================
 *                       How to use this driver
 * ============================================================================
 * 1. 电机接收报文基于消息列表,所以需要先正确初始化消息列表才能使用.详细初始化
 *    方式见"message.h".
 * 2. 在初始化时候调用DJI_Motor_Init,需要向电机发送数据时候用DJI_Motor_Send.
 ******************************************************************************
 */
#ifndef DJI_MOTOR_H
#define DJI_MOTOR_H
#include "SEML_common.h"
#include "message.h"
#include "Motor.h"
#include "can_if.h"
#include "timer.h"
/**
 * @addtogroup 通讯库
 * @{
 */
/**
 * @addtogroup 电机库
 * @{
 */
/**
 * @addtogroup 大疆电机库
 * @{
 */
/// @brief 缓存数组最大id数量
#define MAX_SEND_ID_NUM 10

/// @brief 电机接收报文超时
#define DJI_MOTOR_TIMEOUT 100
extern const int16_t dji_motor_control_max[4];
extern const float dji_motor_torque_constant[4];
/**
 * @brief 电调类型
 */
typedef enum
{
	DJI_Motor_C620,		        /**< C620电调 适配M3508电机 */
	DJI_Motor_C610,		        /**< C610电调 适配M2006电机 */
	DJI_Motor_GM6020,           /**< GM6020电机 */
	DJI_Motor_820R,		        /**< 820R电调 适配M3510,M2310,M2006电机 */
} DJI_motor_type;
/// @brief 电调类型断言
#define IS_DJI_Motor_type(x) ((x) == DJI_Motor_C620 || (x) == DJI_Motor_C610 || (x) == DJI_Motor_GM6020 || (x) == DJI_Motor_820R)
/// @brief 常用的电机类型对应的电调
#define DJI_Motor_M2006 DJI_Motor_C610
#define DJI_Motor_M3508 DJI_Motor_C620
#define DJI_Motor_M3510 DJI_Motor_820R
/// @brief C620,C610,GM6020电调id断言
#define IS_DJI_C620_C610_GM6020_ID(x) ((x) >= 1 && (x) <= 8)
/// @brief 820R电调id断言
#define IS_DJI_820R_ID(x) ((x) >= 1 && (x) <= 4)

typedef struct 
{
	uint32_t hash;
	uint8_t buffer[8];
	uint32_t tx_tick;
}DJI_Motor_Tx_Buffer_t;

/**
 * @brief 大疆电机发送配置结构体
 */
typedef struct
{
	Can_Handle_t *can_handle;				 /**< 消息发送句柄 */
	DJI_Motor_Tx_Buffer_t *buffer; /**< 发送缓存数组 */
	uint8_t num;										 /**< 发送缓存数组序号 */
	uint16_t id;										 /**< 发送数据包id */
} DJI_Motor_tx_t;
/**
 * @brief 大疆电机句柄
 */
typedef struct
{
	Motor_t motor;

	DJI_motor_type motor_type; /**< 电机类型 */
	DJI_Motor_tx_t tx_config;	 /**< 发送配置 */

	Soft_WitchDog_t witchdog; /**< 软件看门狗 */
} Dji_Motor_t;

/**
 * @brief 大疆电机初始化
 *
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] motor_type 电机类型
 * @param[in] ID 电机设置的id
 * @param[in,out] can_handle can句柄
 * @return 电机句柄
 */
Motor_t *DJI_Motor_Init(Dji_Motor_t *motor, DJI_motor_type motor_type, uint16_t ID, Can_Handle_t *can_handle);

/**
 * @brief 大疆电机位置环初始化
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] position_controller 位置环控制器(不能为临时变量)
 * @return 电机句柄
 */
void DJI_Motor_Position_Loop_Init(Dji_Motor_t *motor, SISO_Controller_t *position_controller);

/**
 * @brief 大疆电机速度环初始化
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] Speed_Controller 速度环控制器(不能为临时变量)
 * @return 电机句柄
 */
void DJI_Motor_Speed_Loop_Init(Dji_Motor_t *motor, SISO_Controller_t *Speed_Controller);

/**
 * @brief 向大疆电机发送数据
 * @param[in,out] motor 大疆电机句柄
 * @param[in] data 电流值
 * @return 函数执行状态
 * @retval SEML_OK 发送完成
 * @retval SEML_ERROR 发送失败
 */
SEML_StatusTypeDef DJI_Motor_Send(Dji_Motor_t *motor, float data);

/**
 * @brief 获取大疆电机位置(deg)
*/
#define Get_DJI_Motor_Position_Data(handle) Get_Motor_Position_Data(&(handle)->motor)
/**
 * @brief 获取大疆电机速度(deg/s)
*/
#define Get_DJI_Motor_Speed_Data(handle) Get_Motor_Speed_Data(&(handle)->motor)
/**
 * @brief 获取大疆电机转矩
*/
#define Get_DJI_Motor_Torque_Data(handle) Get_Motor_Torque_Data(&(handle)->motor)
/**
 * @brief 获取大疆电机温度
*/
#define Get_DJI_Motor_Temperature_Data(handle) Get_Motor_Temperature_Data(&(handle)->motor)
/**
 * @brief 获取电机控制数据最大值
 */
#define Get_DJI_Motor_Control_Max(handle) (dji_motor_control_max[(handle)->motor_type])
/// @} 大疆电机库结束
/// @} 电机库结束
/// @} 通讯库结束

#endif
