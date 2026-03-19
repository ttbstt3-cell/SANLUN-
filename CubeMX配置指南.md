# RoboMaster C 板 CubeMX 配置指南

## 替换关系说明
- 原系统: STM32F407最小系统板 + 外部舵机控制板 + SBUS接收机
- 新系统: RoboMaster C板 (内置7路PWM + DBUS接口)

---

## CubeMX 配置步骤

### 1. 新建项目
- 芯片选择: **STM32F427IIH6**

### 2. System Core 配置
- **RCC**: HSE = Crystal/Ceramic Resonator
- **SYS**: Debug = Serial Wire

### 3. 外设配置

#### TIM1 或 TIM8 (舵机PWM)
- Mode: PWM Generation CH1 CH2 CH3 CH4
- Prescaler: 83
- Counter Period: 19999
- PWM 模式: Edge-aligned mode, Up counter

#### UART3 (DBUS/SBUS接收)
- Mode: Asynchronous
- Baud Rate: 100000
- Word Length: 8 Bits
- Parity: Even
- Stop Bits: 2

#### USART1 (调试串口，可选)
- Mode: Asynchronous
- Baud Rate: 115200

### 4. 时钟配置
- PLL Source: HSE
- PLL M: 8
- PLL N: 168
- PLL P: 2
- System Clock Mux: PLLCLK
- APB1 Prescaler: /4
- APB2 Prescaler: /2

### 5. 项目设置
- Project Name: sanlun_robomaster
- Toolchain: MDK-ARM
- 点击 GENERATE CODE

---

## 引脚参考 (RoboMaster C板)
- DBUS接口: UART3 (已固定)
- PWM输出: TIM1/TIM8 (可在Pinout中选择具体引脚)
- 调试串口: USART1 (已固定)

配置完成后，把项目文件夹发给我帮你修改main.c。
