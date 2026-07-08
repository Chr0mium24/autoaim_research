#include "can_if.h"
#if USE_HAL_DRIVER

uint8_t data[8];

uint8_t recving_supcap = 0;
/**
 * @brief CAN邮箱过滤配置
 * @param hcan can句柄
 */
SEML_StatusTypeDef __CAN_Filter_Config(CAN_IF_t *hcan)
{
	uint8_t status = 0;
	CAN_FilterTypeDef sFilterConfig;

	sFilterConfig.FilterBank = 0; // 设置过滤器组编号
	sFilterConfig.SlaveStartFilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 掩码模式
	sFilterConfig.FilterMaskIdHigh = 0x00 << 5;				// 掩码符寄存器ID高十六位
	sFilterConfig.FilterMaskIdLow = 0x00 << 5;				// 掩码符寄存器ID低十六位
	sFilterConfig.FilterIdHigh = 0x00 << 5;						// 标识符寄存器ID高十六位
	sFilterConfig.FilterIdLow = 0x00 << 5;						// 标识符寄存器ID低十六位
	sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT; // 16位宽
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 过滤器组关联到FIFO0
	sFilterConfig.FilterActivation = ENABLE;					 // 激活活过滤器
	status |= HAL_CAN_ConfigFilter(hcan, &sFilterConfig);
	status |= HAL_CAN_Start(hcan);
	status |= HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
	return (SEML_StatusTypeDef)status;
}
#if (USE_HAL_CAN_REGISTER_CALLBACKS == 1)
#define __SEML_CAN_MSP_Init(can) do{\
	HAL_CAN_RegisterCallback((can)->hcan,HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID,SEML_CAN_RxCallback);\
	__CAN_Filter_Config((can)->hcan);\
} while (0)

#else
#warning HAL is not enabled to register callbacks, SEML_CAN_RxCallback needs to be put into can to receive interrupts.(can_if.c)
#define __SEML_CAN_MSP_Init(can)   __CAN_Filter_Config((can)->hcan);
#endif
#endif


Can_Handle_t *can_handle_root;

/**
 * @brief can初始化函数
 * 
 * @param can_handle can句柄
 * @param hcan 底层can配置句柄
 * @param id_buffer id缓存数组指针
 * @param size 缓存数组大小
 */
void SEML_CAN_Init(Can_Handle_t *can_handle,void *hcan, message_Pack_t *id_buffer, uint16_t size)
{
	Can_Handle_t *can_handle_temp = can_handle_root;
	can_handle->hcan = hcan;
	can_handle->Txmessage_front = 0;
	can_handle->Txmessage_rear = 0;
	message_List_Init(&can_handle->Rxmessage_list,id_buffer,size,CAN_TIMEOUT);
	if(can_handle_root != NULL)
	{
		while(can_handle_temp->next != NULL)
		{
			// 链表中查找到不再添加
			if(can_handle_temp == can_handle)
				return;
			can_handle_temp = can_handle_temp->next;
		}
		can_handle_temp->next = can_handle;
	}
	else
		can_handle_root = can_handle;
	__SEML_CAN_MSP_Init(can_handle);
}

/**
 * @brief can接收回调函数
 * @param hcan 底层can配置句柄
 */
void SEML_CAN_RxCallback(CAN_IF_t *hcan)
{
	Can_Handle_t *can_handle_temp = can_handle_root;
	CAN_RxHeaderTypeDef RxMessage;
	HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&RxMessage,data);

	if (RxMessage.StdId == 0x111 && data[0] == 0xFF)
	{
		uint16_t power_limit, current_power;
		power_limit = (uint16_t)(data[1] << 8 | data[2]);
		current_power = (uint16_t)(data[3] << 8 | data[4]);
		Robo_Push_Message_Cmd("Power Limit", power_limit);
		Robo_Push_Message_Cmd("Current Power", current_power);
	}


	while(can_handle_temp != NULL)
	{
		if(can_handle_temp->hcan == hcan)
		{
			message_List_Update(&can_handle_temp->Rxmessage_list,RxMessage.StdId,RxMessage.RTR,data,RxMessage.DLC);
			return;
		}
		can_handle_temp = can_handle_temp->next;
	}
}

/**
 * @brief can接收回调函数注册
 * 
 * @param can_handle can句柄
 * @param ID 报文ID
 * @param fun 回调函数
 * @param config 回调函数配置
 */
void SEML_CAN_Rxmessage_Register(Can_Handle_t *can_handle,uint16_t ID,message_callback_fun_t fun,void* config)
{
	message_List_Register(&can_handle->Rxmessage_list,ID,fun,config);
}

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
SEML_StatusTypeDef SEML_CAN_Send(Can_Handle_t *can_handle, uint16_t ID, uint8_t aData[], uint8_t size, uint16_t message_type)
{
#ifdef USE_HAL_DRIVER
#define CAN_AddTxMessage(handle_, TxMessage, aData) HAL_CAN_AddTxMessage(handle_->hcan, TxMessage, aData, &Tx_Mailbox)
	uint32_t Tx_Mailbox;
	CAN_TxHeaderTypeDef TxMessage;
	// 对发送报文结构体TxMessage里的成员赋值
	TxMessage.IDE = CAN_ID_STD; // 标准模式
	TxMessage.RTR = message_type;
	TxMessage.DLC = size;
	TxMessage.TransmitGlobalTime = DISABLE;
	TxMessage.StdId = ID;
	TxMessage.ExtId = 0;
#else
#error can send function is not configured.(can_if.c)
#endif
#if CAN_BUFFER_SIZE != 0
	if (CAN_AddTxMessage(can_handle, &TxMessage, aData) == HAL_OK)
	{
		// 缓存不为空将缓存内数据加入发送邮箱,成功加入将其出队
		if (can_handle->Txmessage_front != can_handle->Txmessage_rear)
			if (CAN_AddTxMessage(can_handle, &can_handle->Txmessage_Buffer[can_handle->Txmessage_front], aData) == HAL_OK)
				can_handle->Txmessage_front = (can_handle->Txmessage_front >= CAN_BUFFER_SIZE - 1) ? 0 : can_handle->Txmessage_front + 1;
	}
	else
	{
		// 发送失败进缓存，等待下次发送
		can_handle->Txmessage_Buffer[can_handle->Txmessage_rear] = TxMessage;
		// 缓存队列满覆盖最旧数据
		if (can_handle->Txmessage_front == can_handle->Txmessage_rear + 1)
			can_handle->Txmessage_front = (can_handle->Txmessage_front >= CAN_BUFFER_SIZE - 1) ? 0 : can_handle->Txmessage_front + 1;
		can_handle->Txmessage_rear = (can_handle->Txmessage_rear >= CAN_BUFFER_SIZE - 1) ? 0 : can_handle->Txmessage_rear + 1;
	}
	// 缓存队列满
	if (can_handle->Txmessage_front == can_handle->Txmessage_rear + 1)
		return SEML_ERROR;
	else
		return SEML_OK;
#else
	if (CAN_AddTxMessage(can_handle, &TxMessage, aData) == HAL_OK)
		return SEML_OK;
	else
		return SEML_ERROR;
#endif
}

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
SEML_StatusTypeDef SEML_CAN_Send_SupCap(Can_Handle_t *can_handle, uint16_t ID, uint8_t aData[], uint8_t size, uint16_t message_type)
{
#ifdef USE_HAL_DRIVER
#define CAN_AddTxMessage(handle_, TxMessage, aData) HAL_CAN_AddTxMessage(handle_->hcan, TxMessage, aData, &Tx_Mailbox)
	uint32_t Tx_Mailbox;
	CAN_TxHeaderTypeDef TxMessage;
	// 对发送报文结构体TxMessage里的成员赋值
	TxMessage.IDE = CAN_ID_STD; // 标准模式
	TxMessage.RTR = message_type;
	TxMessage.DLC = size;
	TxMessage.TransmitGlobalTime = DISABLE;
	TxMessage.StdId = ID;
	TxMessage.ExtId = 0;
#else
#error can send function is not configured.(can_if.c)
#endif
#if CAN_BUFFER_SIZE != 0
	if (CAN_AddTxMessage(can_handle, &TxMessage, aData) == HAL_OK)
	{
		// 缓存不为空将缓存内数据加入发送邮箱,成功加入将其出队
		if (can_handle->Txmessage_front != can_handle->Txmessage_rear)
			if (CAN_AddTxMessage(can_handle, &can_handle->Txmessage_Buffer[can_handle->Txmessage_front], aData) == HAL_OK)
				can_handle->Txmessage_front = (can_handle->Txmessage_front >= CAN_BUFFER_SIZE - 1) ? 0 : can_handle->Txmessage_front + 1;
	}
	else
	{
		// 发送失败进缓存，等待下次发送
		can_handle->Txmessage_Buffer[can_handle->Txmessage_rear] = TxMessage;
		// 缓存队列满覆盖最旧数据
		if (can_handle->Txmessage_front == can_handle->Txmessage_rear + 1)
			can_handle->Txmessage_front = (can_handle->Txmessage_front >= CAN_BUFFER_SIZE - 1) ? 0 : can_handle->Txmessage_front + 1;
		can_handle->Txmessage_rear = (can_handle->Txmessage_rear >= CAN_BUFFER_SIZE - 1) ? 0 : can_handle->Txmessage_rear + 1;
	}
	// 缓存队列满
	if (can_handle->Txmessage_front == can_handle->Txmessage_rear + 1)
		return SEML_ERROR;
	else
		return SEML_OK;
#else
	if (CAN_AddTxMessage(can_handle, &TxMessage, aData) == HAL_OK)
		return SEML_OK;
	else
		return SEML_ERROR;
#endif
}
