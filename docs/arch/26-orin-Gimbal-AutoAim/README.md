# Welcome to DragoPass EC Group!!!
```
 git clone this repo for template files
```

#### Policies
This assignment allows collaboration, online searching, generative AI. But please cite the origin of those content in your solution with comment. 

:black_medium_square: Citing others' work will not affect the result.

:x: Using other sources without citing will cause punishment.

---

## 开发

在开始开发前，请注意：在该模板中，你的所有修改应该在./SEML目录下进行，请仔细阅读关于SEML库的介绍。

在本次开发中，请关注以下几个文件及其对应的头文件：

```
Robo_Gimbal.c
Robo_AA.c
Robo_USB.c
Dji_Motor.c
```

你可以通过修改、补充或完成这些部分的代码来完成此次项目；或者，你也可以构建自己的框架，这取决于你的开发习惯。

## 程序结构
在系统抽象的基础上，我们可以搭建了gimbal、USB和AA六个进程。其中gimbal进程负责云台控制，USB进程负责与上位机通信，AA进程负责自瞄。

每个进程都有一个Init函数和一个Task函数。在main.c中，Init函数在进程启动时执行一次，Task函数在进程运行时循环执行。代码内容我们提供了如下参考：

### Gimbal （Robo_gimbal.c/.h）
云台进程负责从全局变量中获取预期的云台朝向，然后利用电机抽象使用电流控制电机。

云台进程拥有一个句柄结构体"Task_Gimbal_t gimbal"，其中包含了云台控制所需的所有参数。

#### Init
流程如下：
1. 初始化电机
2. 初始化PID（yaw和pitch电机各有速度环、角度环双环PID）

#### Task
Task函数流程类似OOP的类方法，首先获取数据，然后处理数据，最后输出数据。

流程如下：
1. 从全局变量中获取预期云台朝向角度
2. 使用角度环PID通过角度控制电机速度
3. 使用速度环PID通过速度控制电机电流
4. 发送电流

### USB （Robo_USB.c/.h）
USB进程负责与上位机通信，接收上位机发送的数据包，然后根据数据包类型执行相应的操作。

在这里，你需要自行定义`SendPacket`和`ReceivePacket`两个结构题，用于装载收发数据包

#### Init
1. 初始化USB
2. 初始化发送数据包

#### Task
1. 接收
2. 发送

#### receive
流程如下：
1. 设定单次传输数据包长度（只要大于数据包长度即可）
2. 调用USB_CDC抽象接收数据包
3. 检查数据包头
4. 检查CRC校验和

#### send
流程如下：
1. 计算CRC校验和
2. 调用USB_CDC抽象发送数据包

### AA （Robo_AA.c/.h）
自瞄进程负责使用目标位置进行弹道解算，然后在全局变量中发布云台预期角度。

#### Init
自定义

#### Task
流程如下：
1. 将IMU和自瞄目标位置传入弹道解算函数
2. 在全局变量中发布云台预期角度


## 系统抽象

我们的系统提供了一些数据类型抽象接口，提供给大家使用

### 全局变量抽象
Event OS系统提供了全局变量抽象，可以在任何地方通过tag访问全局变量，并且不限制读写的变量类型。例如：
```c
//在power.c中
int power = 100;
Robo_Push_Message_Cmd("power", power);
```
这行代码将power变量推送到全局变量中，tag为"power"。然后在任何地方都可以通过tag访问这个变量。
```c
//在main.c中
int power;
Robo_Get_Message_Cmd("power", power);
```
这行代码将全局变量中tag为"power"的变量赋值给power。

### 一些常用的硬件抽象
#### 电机
#### Motor_Send_Data (Motor.c)
```c
/**
 * @brief 电机句柄发送函数
 * @param motor 电机句柄
 * @param data 发送数据
 */
void Motor_Send_Data(Motor_t *motor, float *data)
```
其中motor是电机句柄，data是发送的电流值，单位为毫安。例如：
```c
Motor_Send_Data(motor, 10000);
```
这行代码向motor发送了10000mA的电流。

#### IMU
从C板的IMU获取的所有数据均存储在AHRS结构体中，include "AHRS.h"即可使用。在云台跟随中，我们常常用它来获取云台姿态
例如：
```c
current_yaw = AHRS.euler_angle.yaw;
```
其中AHRS_t结构体在AHRS.h中定义如下：
```c
/**
 * @brief AHRS句柄
 */
typedef struct
{
	Euler_Data_t euler_angle;							/**< 欧拉角 */
	Accel_Data_t accel;									/**< 加速度 */
	Accel_Data_t real_accel;							/**< 惯性坐标系下的加速度 */
	Gyro_Data_t gyro;									/**< 角速度 */
	Mag_Data_t mag;										/**< 磁场 */
	float quat[4];										/**< 姿态四元数 */
	float g_norm;										/**< 标定时的重力加速度 */
	float sample_time;									/**< 采样时间 */
	Gyro_Data_t gyro_offset;							/**< 角速度零偏校正值 */
	AHRS_Calculate_fun_t AHRS_Calculate_fun; 			/**< AHRS解算算法 */
	void *IMU_handle;									/**< IMU句柄*/
	Get_IMU_fun_t Get_IMU_fun;							/**< 获取IMU数据函数 */
	void *mag_handle;									/**< 磁力计句柄*/
	Get_Mag_fun_t Get_Mag_fun;							/**< 获取磁力计数据函数 */
	uint8_t count;
} AHRS_t;
```

### 一些常用的算法抽象
#### CRC校验
get_CRC16_check_sum方法定义在CRC8_CRC16.c中，用于计算CRC16校验和。
```c
/**
  * @brief          计算CRC16
  * @param[in]      pch_message: 数据
  * @param[in]      dw_length: 数据和校验的长度
  * @param[in]      wCRC:初始CRC16
  * @retval         计算完的CRC16
  */
uint16_t get_CRC16_check_sum(uint8_t *pch_message,uint32_t dw_length,uint16_t wCRC)
```
其中pch_message是数据，dw_length是数据长度，wCRC是初始CRC16值。例如：
```c
checksum = get_CRC16_check_sum(msg, sizeof(msg), 0xffff);
```
这行代码计算了msg的CRC16校验和。

#### 滤波器
##### RMS_filter_Init (math_filter.c)
```c
/**
 * @brief 均方根滤波器初始化
 * @param[out] RMS_filter  均方根配置结构体指针
 * @param[in] window_width 滑动窗口大小
 * @param[in] init_value 初始值
 * @param[in,out] buffer 队列缓存数组
*/
void RMS_filter_Init(RMS_filter_t *RMS_filter,const uint16_t window_width,const float init_value,float *buffer);
```
例如：
```c
RMS_filter_Init(&rms_filter, 10, 0, rms_buffer);
```
这行代码初始化了一个滑动窗口大小为10的均方根滤波器，初始值为0。

其中RMS_filter_t结构体在math_filter.h中定义如下：
```c
typedef struct 
{
    uint16_t window_width;  /**< 窗口大小 */
    float gain;
    uint16_t count;         /**< 计数值 */
    float RMS_value;        /**< rms值 */
    float square_sum;       /**< 平方和 */
    s_queue data_queue;     /**< 数据队列*/
} RMS_filter_t;
```

##### RMS_filter (math_filter.c)
```c
/**
 * @brief 均方根滤波器
 * @param[in,out] RMS_filter 滤波器指针
 * @param[in] data 输入数据
 * @return 滤波后结果
*/
_FAST float RMS_filter(RMS_filter_t *RMS_filter,const float data);
```
例如：
```c
filtered_data = RMS_filter(&rms_filter, raw_data);
```
这行代码对raw_data进行均方根滤波。

##### 其他滤波器
math_filter.c中还有滑动均值滤波器，使用方法类似。卡尔曼滤波器算法十分简单，但暂未封装。

### 其他算法
math目录下还有一些其他算法，如矩阵运算、向量运算等。需要时可以查看。