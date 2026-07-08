# 电机库

该电机库"motor.h"的目的是为了完全和底层解耦，让使用电机就跟用printf一样简单，只需对底层进行简单的配置即可使用。

电机库的用户接口如下：

```c
/**
 * @brief 电机速度环控制
 * @param motor 电机句柄
 * @param data 发送数据
 */
void Motor_Speed_Ctrl(Motor_t *motor, float data);
/**
 * @brief 电机位置环控制
 * @param motor 电机句柄
 * @param data 发送数据
 */
void Motor_Position_Ctrl(Motor_t *motor, float data);
/**
 * @brief 电机句柄发送函数
 * @param motor 电机句柄
 * @param data 发送数据
 */
void Motor_Send_Data(Motor_t *motor, float data);
/**
 * @brief 获取电机位置(deg)
 */
#define Get_Motor_Position_Data(motor) ((motor)->position)

/**
 * @brief 获取电机速度(deg/s)
 */
#define Get_Motor_Speed_Data(motor) ((motor)->speed)

/**
 * @brief 获取电机转矩
 */
#define Get_Motor_Torque_Data(motor) ((motor)->torque)

/**
 * @brief 获取电机温度
 */
#define Get_Motor_Temperature_Data(motor) ((motor)->temperature)
```

对于用户来说，使用电机库只需调用对应电机类型进行初始化，接收到电机句柄后即可使用该库，详情见下方各个库的使用。

如果你是驱动开发人员，有如下配置接口：

```c
/**
 * @brief 电机句柄初始化
 * 该函数由各个电机驱动调用,无需用户调用
 * @param motor 电机句柄
 * @param Motor_Send_Data 电机发送回调函数
 * @param motor_config 电机配置
 */
void Motor_Init(Motor_t *motor, Motor_Send_Data_t Motor_Send, void *motor_config);

/**
 * @brief 电机速度环初始化
 * 该函数由各个电机驱动调用,无需用户调用
 * @param motor 电机句柄
 * @param Motor_Position_Loop 电机速度环设置回调函数
 * @param motor_position_loop_config 电机速度环句柄
 */
void Motor_Speed_Loop_Config(Motor_t *motor, Motor_Send_Data_t Motor_Position_Loop, void *motor_position_loop_config);

/**
 * @brief 电机位置环初始化
 * 该函数由各个电机驱动调用,无需用户调用
 * @param motor 电机句柄
 * @param Motor_Position_Loop 电机位置环设置回调函数
 * @param motor_position_loop_config 电机位置环句柄
 */
void Motor_Position_Loop_Config(Motor_t *motor, Motor_Send_Data_t Motor_Speed_Loop, void *motor_speed_loop_config);

/**
 * @brief 电机速度环控制器
 * 该函数由各个电机驱动调用,无需用户调用
 * @param motor 电机句柄
 * @param data 期望速度
 */
void Motor_Speed_Loop_Controller(Motor_t *motor, float data);

/**
 * @brief 电机位置环控制器
 * 该函数由各个电机驱动调用,无需用户调用
 * @param motor 电机句柄
 * @param data 期望位置
 */
void Motor_Position_Loop_Controller(Motor_t *motor, float data);

/**
 * @brief 电机位置更新函数
 * 该函数由各个电机驱动调用,无需用户调用
 * @param motor 电机句柄
 * @param new_data 新数据
 */
void Position_Update(Motor_t *motor, float new_data);
```

## 大疆电机库

### 大疆电机库的使用

#### 初始化can接口

电机库的接收功能是在SEML的can接口的基础上进行订阅电机报文id来实现的，所以需要先初始化can接口：

列出需要用到的函数原型：

```c
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
  * @brief  注册can的接收回调
  *         To be used instead of the weak predefined callback
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for CAN module
  * @param  CallbackID ID of the callback to be registered
  *         This parameter can be one of the following values:
  *           @arg @ref HAL_CAN_TX_MAILBOX0_COMPLETE_CB_ID Tx Mailbox 0 Complete callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX1_COMPLETE_CB_ID Tx Mailbox 1 Complete callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX2_COMPLETE_CB_ID Tx Mailbox 2 Complete callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX0_ABORT_CB_ID Tx Mailbox 0 Abort callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX1_ABORT_CB_ID Tx Mailbox 1 Abort callback ID
  *           @arg @ref HAL_CAN_TX_MAILBOX2_ABORT_CB_ID Tx Mailbox 2 Abort callback ID
  *           @arg @ref HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID Rx fifo 0 接收完成回调
  *           @arg @ref HAL_CAN_RX_FIFO0_FULL_CB_ID Rx Fifo 0 full callback ID
  *           @arg @ref HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID Rx Fifo 1 message pending callback ID
  *           @arg @ref HAL_CAN_RX_FIFO1_FULL_CB_ID Rx Fifo 1 full callback ID
  *           @arg @ref HAL_CAN_SLEEP_CB_ID Sleep callback ID
  *           @arg @ref HAL_CAN_WAKEUP_FROM_RX_MSG_CB_ID Wake Up from Rx message callback ID
  *           @arg @ref HAL_CAN_ERROR_CB_ID Error callback ID
  *           @arg @ref HAL_CAN_MSPINIT_CB_ID MspInit callback ID
  *           @arg @ref HAL_CAN_MSPDEINIT_CB_ID MspDeInit callback ID
  * @param  pCallback pointer to the Callback function
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CAN_RegisterCallback(CAN_HandleTypeDef *hcan, HAL_CAN_CallbackIDTypeDef CallbackID, void (* pCallback)(CAN_HandleTypeDef *_hcan));

```

其他的按照注释进行传入，以hal库为例对消息列表进行初始化:

```C
// 消息列表缓存数组
message_Pack_t message_buffer[10];
// can句柄
Can_Handle_t can1;
// can接口初始化
SEML_CAN_Init(&can1,&hcan1,robo_middleware.can1_rx_buffer,10);
HAL_CAN_RegisterCallback(&hcan1,HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID,SEML_CAN_RxCallback);
// 使能can
CAN_FilterTypeDef sFilterConfig;

sFilterConfig.FilterBank = 0; // 设置过滤器组编号
sFilterConfig.SlaveStartFilterBank = 0;
sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 掩码模式
sFilterConfig.FilterMaskIdHigh = 0x00 << 5;		  // 掩码符寄存器ID高十六位
sFilterConfig.FilterMaskIdLow = 0x00 << 5;		  // 掩码符寄存器ID低十六位
sFilterConfig.FilterIdHigh = 0x00 << 5;			  // 标识符寄存器ID高十六位
sFilterConfig.FilterIdLow = 0x00 << 5;			  // 标识符寄存器ID低十六位
sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT; // 16位宽
sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 过滤器组关联到FIFO0
sFilterConfig.FilterActivation = ENABLE;		   // �????????活过滤器
if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
{
	Error_Handler();
}
if (HAL_CAN_Start(&hcan1) != HAL_OK)
{ //启动CAN
	Error_Handler();
}
if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
{ // 启动CAN的FIFO0接收中断
	Error_Handler();
}
```

这样，can接口初始化完成了。

#### 初始化电机句柄

初始化完can接口后需要对电机句柄进行简单的配置：

列出需要用到的函数原型：

```c
/**
 * @brief 大疆电机初始化
 *
 * @param[in,out] motor 大疆电机句柄(不能为临时变量)
 * @param[in] motor_type 电机类型
 * @param[in] ID 电机设置的id
 * @param[in,out] can_handle can句柄
 * @return 电机句柄
 */
Motor_t* DJI_Motor_Init(Dji_Motor_t *motor, DJI_motor_type motor_type,uint16_t ID, Can_Handle_t *can_handle);
```

电机类型共有如下几个：

```c
typedef enum
{
	DJI_Motor_C620,		/**< C620电调 适配M3508电机 */
	DJI_Motor_C610,		/**< C610电调 适配M2006电机 */
	DJI_Motor_GM6020, /**< GM6020电机 */
	DJI_Motor_820R,		/**< 820R电调 适配M3510,M2310,M2006电机 */
} DJI_motor_type;
```

初始化调用`DJI_Motor_Init`,他会返回一个motor_t的句柄：

```c
//注意 句柄初始化的时候需要声明成静态变量或者全局变量
static Motor_t *pitch;
static Dji_Motor_t dji_pitch;
pitch = DJI_Motor_Init(&pitch_motor, DJI_Motor_GM6020, 2, &can2);
```

随后可以使用大疆电机库也可以使用电机库的函数，使用电机库需要传入pitch。下列以电机库的为例：

```c
// 向pitch轴电机发送扭矩电流2000
Motor_Send_Data(pitch,2000);
```

#### 速度闭环和位置闭环配置

由于大疆电机只能发送电流环，如果想使用速度环和位置环需要再进一步进行初始化。

列出需要用到的函数原型：

```c
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
```

速度环初始化：初始化完大疆电机句柄后，调用`DJI_Motor_Speed_Loop_Init`配置好控制器后即可调用：

```c
static SISO_Controller_t pitch_s_controller;
static PIDConfig_t pitch_speed_PID;
float temp;
// 绑定电机速度闭环
pitch_s_controller.Callback_Fun = Basic_PID_Controller;
pitch_s_controller.config = &pitch_speed_PID;
DJI_Motor_Speed_Loop_Init(&pitch_motor,&pitch_s_controller);
// 配置pid
temp = Get_DJI_Motor_Control_Max(&pitch_motor);
PID_Init(&pitch_speed_PID, 70, 450, 0, temp, -temp, 0.001f);
```

位置环初始化：初始化完由于使用串级pid，**需要先对速度环进行初始化**，初始化速度环后调用`DJI_Motor_Position_Loop_Init`配置好控制器后即可调用：

```c
static SISO_Controller_t pitch_l_controller，pitch_s_controller;
static PIDConfig_t pitch_location_PID，pitch_speed_PID;
float temp;
// 绑定电机速度闭环
pitch_s_controller.Callback_Fun = Basic_PID_Controller;
pitch_s_controller.config = &pitch_speed_PID;
DJI_Motor_Speed_Loop_Init(&pitch_motor,&pitch_s_controller);
// 绑定电机位置闭环
pitch_l_controller.Callback_Fun = Basic_PID_Controller;
pitch_l_controller.config = &pitch_location_PID;
DJI_Motor_Position_Loop_Init(&pitch_motor,&pitch_l_controller);
// 配置pid
temp = Get_DJI_Motor_Control_Max(&pitch_motor);
PID_Init(&pitch_speed_PID, 70, 450, 0, temp, -temp, 0.001f);
PID_Init(&pitch_speed_PID, 15, 0, 0, 360, -360, 0.001f);
```

#### 如何使用速度闭环和位置闭环

列出需要用到的函数原型：

```c
/**
 * @brief 电机速度环控制
 * @param motor 电机句柄
 * @param data 发送数据
 */
void Motor_Speed_Ctrl(Motor_t *motor, float data);
/**
 * @brief 电机位置环控制
 * @param motor 电机句柄
 * @param data 发送数据
 */
void Motor_Position_Ctrl(Motor_t *motor, float data);
```

速度闭环直接调用`Motor_Speed_Ctrl`即可：

```c
Motor_Speed_Ctrl(pitch,expect_speed);
```

位置闭环直接调用`Motor_Position_Ctrl`即可,位置闭环已经进行了零点处理,无需对数据进行处理：

```c
Motor_Position_Ctrl(pitch,expect_location);
```

## 达妙电机库

跟大疆电机一样，需要先初始化好can接口。

初始化完can接口后需要对电机句柄进行简单的配置：

列出需要用到的函数原型：

```c
/**
 * @brief 达妙电机初始化
 * @param dm_motor 达妙电机句柄
 * @param ctrl_mode 控制模式
 * @param master_id 电机反馈报文主id
 * @param motor_id 电机的控制报文id
 */
Motor_t *DM_Motor_Init(DM_Motor_t *dm_motor, DM_Motor_Ctrl_Mode_t ctrl_mode, uint16_t master_id, uint16_t motor_id, Can_Handle_t *can_handle, float max_speed);
```

电机模式如下：

```c
typedef enum
{
	DM_Motor_MIT_Mode = 0,	/**< MIT模式 */
	DM_Motor_PosSpeed_Mode, /**< 位置速度模式 */
	DM_Motor_Speed_Mode		/**< 速度模式 */
} DM_Motor_Ctrl_Mode_t;
```

电机模式,反馈报文id和控制报文id由达妙电机调试助手进行设置。注意电机模式需要和电机调试助手设置一样。

随后进行初始化:

```c
//注意 句柄初始化的时候需要声明成静态变量或者全局变量
static Motor_t *yaw;
static DM_Motor_t dm_yaw;
yaw = DM_Motor_Init(&dm_yaw,DM_Motor_Speed_Mode,0x300,0x01,&hcan2,1000);
```

注意，使用MIT模式只能使用扭矩环`Motor_Send_Data`，使用位置速度模式只能使用位置环`Motor_Position_Ctrl`,使用速度模式只能使用速度环`Motor_Speed_Ctrl`。

