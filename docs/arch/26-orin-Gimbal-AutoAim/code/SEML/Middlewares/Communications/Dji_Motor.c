#include "Dji_Motor.h"
#include "stdlib.h"
/**
 * @brief 电机控制数据最大值
 */
const int16_t dji_motor_control_max[4] =
		{
				16384, // DJI_Motor_C620
				10000, // DJI_Motor_C610
				30000, // DJI_Motor_GM6020
				32767  // DJI_Motor_820R
};
const float dji_motor_torque_constant[4] =
		{
				0.3,	 // DJI_Motor_M3508
				0.18,	 // DJI_Motor_M2006
				0.741,   // DJI_Motor_GM6020
				0.202	 // DJI_Motor_M3510
};
/**
 * @brief 电机减速比
 */
const float dji_motor_gear_ratio[4] =
		{
				19,	 // DJI_Motor_M3508
				36,	 // DJI_Motor_M2006
				4,   // DJI_Motor_GM6020
				19   // DJI_Motor_M3510
};
DJI_Motor_Tx_Buffer_t DJI_Motor_Tx[MAX_SEND_ID_NUM];

static void DJI_Motor_Timeout_Callback(Dji_Motor_t *config)
{
	DJI_Motor_Send(config, 0);
	config->motor.position = 0;
	config->motor.speed = 0;
	config->motor.torque = 0;
	config->motor.temperature = 0;
	config->motor.position_gear = 0;
	config->motor.speed_gear = 0;
}

/**
 * @brief 大疆电调的接收回调函数
 * @param[out] motor 大疆电机句柄
 * @param[in] id id号
 * @param[in] data 接收到的数据
 */
void DJI_Motor_Callback(Dji_Motor_t *motor, message_Pack_t const *const message_pack)
{
	uint8_t *data, longth;
	longth = Get_message_Pack_Data(message_pack, (void**)&data);
	if (longth != 8)
		return;
	Position_Update(&motor->motor,((int16_t)(data[0] << 8) | data[1]) * 2 * PI / 8192.0f);
	motor->motor.speed = RPM2Rads((int16_t)(data[2] << 8) | data[3]);
	motor->motor.torque = ((int16_t)(data[4] << 8) | data[5]) * (dji_motor_torque_constant[motor->motor_type] / dji_motor_gear_ratio[motor->motor_type] * motor->motor.gear_ratio) * 0.0001f;
	motor->motor.temperature = data[6];
	motor->motor.speed_gear = motor->motor.speed / motor->motor.gear_ratio;
	SEML_Feed_WitchDog(motor);
}

/**
 * @brief 大疆电调的发送缓存初始化函数
 * @param[in] motor 大疆电机句柄
 * @param[in] tx_ID 发送id
 * @return 发送缓存指针
 * @comments: 该函数会根据发送句柄和发送id号来分配缓存
 */
static DJI_Motor_Tx_Buffer_t *DJI_Motor_TxBuffer_Init(Dji_Motor_t *motor, uint16_t tx_ID)
{
	// 发送配置和发送id融合后进行遍历
	uint32_t temp = ((uint32_t)motor->tx_config.can_handle & 0x0000ffff) | (tx_ID << 16);
	for (uint16_t i = 0; i < MAX_SEND_ID_NUM; i++)
	{
		// 命中或者未找到
		if (DJI_Motor_Tx[i].hash == temp || DJI_Motor_Tx[i].hash == NULL)
		{
			DJI_Motor_Tx[i].hash = temp;
			return &DJI_Motor_Tx[i];
		}
	}
	return NULL;
}

/**
 * @brief 大疆电机位置环初始化
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] position_controller 位置环控制器(不能为临时变量)
 * @return 电机句柄
 */
void DJI_Motor_Position_Loop_Init(Dji_Motor_t *motor, SISO_Controller_t *position_controller)
{
	assert_param(position_controller != NULL);
	assert_param(position_controller->Callback_Fun != NULL);
	Motor_Position_Loop_Config(&motor->motor, (Motor_Send_Data_t)Motor_Position_Loop_Controller, position_controller);
}

/**
 * @brief 大疆电机速度环初始化
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] Speed_Controller 速度环控制器(不能为临时变量)
 * @return 电机句柄
 */
void DJI_Motor_Speed_Loop_Init(Dji_Motor_t *motor, SISO_Controller_t *speed_controller)
{
	// TODO
}

/**
 * @brief 获取电机发送报文id
 * @param[in] motor_type 电机类型
 * @param[in] ID id号
 * @param[out] tx_num 在发送缓存数组位置
 * @return 发送报文id
 * @note 该函数会对电机类型和id进行断言
 * @comments: e.g uint8_t tx_num; uint16_t tx_id=Get_DJI_Motor_TX_ID(DJI_Motor_C620,1,&tx_num);
 */
static uint16_t Get_DJI_Motor_TX_ID(DJI_motor_type motor_type, uint16_t ID, uint8_t *tx_num)
{
	// TODO
	;
}

/**
 * @brief 大疆电机初始化
 *
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] motor_type 电机类型
 * @param[in] ID 电机设置的id
 * @param[in,out] can_handle can句柄
 * @return 电机句柄
 * @comments: e.g Dji_Motor_t motor; DJI_Motor_Init(&motor,DJI_Motor_C620,1,&can_handle);
 */
Motor_t *DJI_Motor_Init(Dji_Motor_t *motor, DJI_motor_type motor_type, uint16_t ID, Can_Handle_t *can_handle)
{
	assert_param(motor != NULL);
	assert_param(can_handle != NULL);
	assert_param(can_handle->hcan != NULL);
	assert_param(IS_DJI_Motor_type(motor_type));
	motor->motor_type = motor_type;

	// TODO: 订阅报文消息 Hint: SEML_CAN_Rxmessage_Register()
	



	// TODO: 配置can句柄
	
	// TODO: 按照id和发送句柄来分配缓存
	
	// 注册看门狗
	SEML_WitchDog_Register(motor, DJI_MOTOR_TIMEOUT, DJI_Motor_Timeout_Callback);
	// 初始化电机数据结构体
	Motor_Init(&motor->motor, (Motor_Send_Data_t)DJI_Motor_Send, motor);
	return &motor->motor;
}

/**
 * @brief 向大疆电机发送数据
 * @param[in,out] motor 大疆电机句柄
 * @param[in] data 电流值
 * @return 函数执行状态
 * @retval SEML_OK 发送完成
 * @retval SEML_ERROR 发送失败
 * @comments: e.g DJI_Motor_Send(&motor,1000);
 */
SEML_StatusTypeDef DJI_Motor_Send(Dji_Motor_t *motor, float data)
{
	// Hint: SEML_CAN_Send()
	;
}
