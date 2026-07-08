# SEML：Singularity Embedded Middlewares Layer Library
**嵌入式软件中间件层库**

<div align=center>
	<p>
		<img src="https://img.shields.io/badge/version-alpha-blue"/>
		<img src="https://img.shields.io/badge/license-GNU3.0-green"/>
    </p>
   	<p>
		<img src="https://gitee.com/SenLinks/super-capacitor-control-board/badge/star.svg"/>
		<img src="https://gitee.com/SenLinks/super-capacitor-control-board/badge/fork.svg"/>
	</p>
</div>

## 简介

​	SEML旨在将已有的代码解耦，模块化后封装成一个服务于组织内所有软件项目的中间件层，形成实验室内部软件开发标准接口，以解决实验室代码混乱的问题。同时机器人软件开发者需要维护、升级已有的模块，而无需为了同一个功能重复造轮子，提高软件开发效率和质量。如果你认为自己对某个模块有更好的代码方案，或者发现了现有模块不再满足当前使用需求，请仔细阅读完原有代码后向实验室其他队员收集需求意见然后再进行重构。重构时请务必谨慎考虑模块接口的设置！

### 核心模块简介

该部分文档为SEML库核心模块，也算是比较复杂的重难点，可以说整个SEML库都是基于这部分模块搭建而成，搞懂了这些模块就能快速上手SEML库。

​		\- [HAL层-外设接口层](.\Drivers\hal\Interface\Interface.md)

​		\- [系统库-消息模块](.\Middlewares\System\message.md)

​		

**SEML库文件包含如下：**

### App

  软件应用层，放置了所有机器人执行器

### Drivers

​	内置所有基本驱动配置文件**BSP**文件夹和硬件抽象层**HAL**文件夹

#### 		BSP

​	BSP层为驱动配置文件，在网络可以收集到，主控与外部模块的接口层文件基于HAL层的Interface层

#### HAL

​	对BSP层和外设层进一步抽象，使其形成一个统一且通用的接口。

### Middlewares

​	内置许许多多常用软件算法和辅助代码。

#### math

​	数学支持库，主要目的是为SEML提供一个通用、快速的数学计算支持。主要提供单精度浮点数的软件计算和arm_math的算法，里面所有函数都比标准库`<math.h>`要快。如果需要双精度的可以调用`<math.h>`内的函数或者其他第三方库。

#### Solution

​	解算算法库，包括电源类、惯性运动单元解算、底盘运动模型、功率限制等解算算法。理解大致使用方法后基本都可以直接调用使用，前人搞过的解算算法都在这里面，避免重复造轮子。

#### DataStructure

​	数据结构支持库，主要为SEML数据结构支持，主要是队列和栈。

#### Control

​	控制器支持库，里面就是一些比较常用的控制算法，例如PID之类的。

#### System

​	系统库，主要是辅助代码，为了实现代码解耦,将驱动层透明化(真·让你写嵌入式不用跟底层打交道)~~甚至还想在这里实现个操作系统~~。

#### Communications

​	通讯库，主要是通讯解算代码。

