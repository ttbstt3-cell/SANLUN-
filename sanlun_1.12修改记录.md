# sanlun_1.12 main.c 修改记录

## 项目信息
- **MCU**：STM32F407VET6 (LQFP100)
- **主频**：168MHz
- **文件路径**：`C:\Users\ttbstt\Desktop\sanlun_1.12\sanlun_1.12\Core\Src\main.c`
- **开发工具**：Keil MDK-ARM V5

---

## 硬件资源

| 外设 | 引脚 | 用途 |
|------|------|------|
| UART5 | PC12(TX), PD2(RX) | 接收SBUS遥控信号 |
| TIM1_CH1 | PE9 | 1号舵机 PWM（360度，63mm轮） |
| TIM1_CH2 | PE11 | 2号舵机 PWM（180度，转向） |
| TIM1_CH3 | PE13 | 3号舵机 PWM（360度，50mm轮） |
| TIM1_CH4 | PE14 | 4号舵机 PWM（360度，50mm轮） |
| SWD | PA13, PA14 | 调试/烧录接口 |

---

## 通道分配

| 通道 | 模式1 | 模式2 |
|------|-------|-------|
| 通道2 | 控制三个驱动舵机（1、3、4号）同步 | 单独控制3号舵机 |
| 通道3 | 控制2号转向舵机 | 单独控制4号舵机 |
| 通道4 | 未使用 | 单独控制1号舵机 |
| 通道9 | 模式切换（<1000=模式1，>=1000=模式2） | 同左 |

---

## 修改历史

### 修改1：降低转向舵机灵敏度
- **位置**：main.c 模式1转向控制部分
- **原因**：转向过于灵敏，不易控制
- **修改内容**：
  - 脉宽范围从 500-2500us 缩小到 **900-2100us**（灵敏度降低约40%）
  - 增加 **±30 死区**，防止摇杆中位抖动
- **可调参数**：
```c
#define STEER_MIN_PULSE  900   // 最小脉宽（增大此值降低左转幅度）
#define STEER_MAX_PULSE  2100  // 最大脉宽（减小此值降低右转幅度）
#define STEER_DEADZONE   30    // 中位死区（0-100，越大越不灵敏）
```

### 修改2：模式2增加舵机3独立控制（通道2）
- **位置**：main.c 模式2控制部分
- **原因**：用户需要在模式2中单独控制3号舵机
- **问题历程**：
  - 最初尝试用通道5控制，但通道5没有绑定到遥控器摇杆，值异常导致舵机自动运行
  - 改用**通道2**（模式2中空闲），采用三区域判断法，无需精确中位值
- **控制方式**：三区域判断（低位反转/中位停止/高位正转）
```c
#define CH2_LOW_THRESHOLD  600   // 低于此值为反转区
#define CH2_HIGH_THRESHOLD 1400  // 高于此值为正转区
// 中位区（600-1400）：停止
```

### 修改3：模式2增加舵机4独立控制（通道3）
- **位置**：main.c 模式2控制部分
- **原因**：用户需要在模式2中单独控制4号舵机
- **控制通道**：通道3（模式2中空闲）
- **控制方式**：三区域判断
```c
#define CH3_LOW_THRESHOLD  600   // 低于此值为反转区
#define CH3_HIGH_THRESHOLD 1400  // 高于此值为正转区
// 中位区（600-1400）：停止
```

---

## 当前完整逻辑说明

### 模式1（通道9 < 1000）：三轮车协同控制
- **通道2**：同步控制1、3、4号驱动舵机
  - SBUS中位值：1002
  - 1号舵机速度按比例调整：`speed × 50/63`（补偿63mm与50mm轮径差异）
- **通道3**：控制2号转向舵机（180度）
  - SBUS中位值：1004
  - 脉宽范围：900-2100us（已降低灵敏度）
  - 死区：±30

### 模式2（通道9 >= 1000）：三舵机独立控制
- **通道4**：控制1号舵机（63mm轮，360度）
  - 中位区：600-1200（停止）
- **通道2**：控制3号舵机（50mm轮，360度）
  - 中位区：600-1400（停止）
- **通道3**：控制4号舵机（50mm轮，360度）
  - 中位区：600-1400（停止）
- **2号转向舵机**：自动回到中位

### SBUS数据无效时
- 所有驱动舵机停止（1、3、4号）

---

## 三区域控制法说明

模式2采用三区域控制，**无需知道精确中位值**：

```
SBUS值:  200          600         1400        1800
          |<--反转区-->|<---停止区--->|<--正转区-->|
速度:   -1000         0            0          +1000
```

- 低位区（< 600）：舵机反转，速度从 0 到 -1000 线性映射
- 中位区（600-1400）：舵机停止，速度 = 0
- 高位区（> 1400）：舵机正转，速度从 0 到 +1000 线性映射

通道4的中位区为 600-1200（较窄），通道2和通道3的中位区为 600-1400（较宽，更保险）。

---

## 当前 main.c USER CODE 完整内容

### USER CODE BEGIN Includes
```c
#include "sbus.h"
#include "servo.h"
```

### USER CODE BEGIN 2
```c
SBUS_Init();
Servo_Init();
#define DEBUG_MODE 0
```

### USER CODE BEGIN 3（主循环）
```c
SBUS_Process();

if (SBUS_IsValid())
{
    uint16_t channel2, channel3, channel4, channel5, channel9;
    uint16_t mode_switch;

    channel2 = SBUS_GetChannel(2);  // 模式1:驱动 / 模式2:控制3号舵机
    channel3 = SBUS_GetChannel(3);  // 模式1:转向 / 模式2:控制4号舵机
    channel4 = SBUS_GetChannel(4);  // 模式2:控制1号舵机
    channel5 = SBUS_GetChannel(5);  // 备用
    channel9 = SBUS_GetChannel(9);  // 模式切换通道

    mode_switch = channel9;

    if (mode_switch < 1000)
    {
        // 模式1：协同控制
        int16_t drive_speed;
        uint16_t steer_pulse;

        if (channel2 <= 1002)
        {
            if (channel2 <= 200) drive_speed = -1000;
            else drive_speed = -1000 + ((channel2 - 200) * 1000) / (1002 - 200);
        }
        else
        {
            if (channel2 >= 1800) drive_speed = 1000;
            else drive_speed = ((channel2 - 1002) * 1000) / (1800 - 1002);
        }

        if (drive_speed < -1000) drive_speed = -1000;
        if (drive_speed > 1000) drive_speed = 1000;

        int16_t servo1_speed = (drive_speed * 50) / 63;
        Servo_Set360(1, servo1_speed);
        Servo_Set360(3, drive_speed);
        Servo_Set360(4, drive_speed);

        #define STEER_MIN_PULSE  900
        #define STEER_MAX_PULSE  2100
        #define STEER_DEADZONE   30

        int16_t deviation = channel3 - 1004;
        if (deviation > -STEER_DEADZONE && deviation < STEER_DEADZONE) deviation = 0;
        else if (deviation >= STEER_DEADZONE) deviation -= STEER_DEADZONE;
        else if (deviation <= -STEER_DEADZONE) deviation += STEER_DEADZONE;

        uint16_t adjusted_channel3 = 1004 + deviation;
        steer_pulse = STEER_MIN_PULSE + ((adjusted_channel3 - 200) * (STEER_MAX_PULSE - STEER_MIN_PULSE)) / (1800 - 200);
        if (steer_pulse < STEER_MIN_PULSE) steer_pulse = STEER_MIN_PULSE;
        if (steer_pulse > STEER_MAX_PULSE) steer_pulse = STEER_MAX_PULSE;
        Servo_Set180(2, steer_pulse);
    }
    else
    {
        // 模式2：独立控制
        int16_t servo1_speed, servo3_speed, servo4_speed;

        // 通道4 -> 1号舵机
        #define CH4_LOW_THRESHOLD  600
        #define CH4_HIGH_THRESHOLD 1200
        if (channel4 < CH4_LOW_THRESHOLD)
        {
            if (channel4 <= 200) servo1_speed = -1000;
            else servo1_speed = -1000 + ((channel4 - 200) * 1000) / (CH4_LOW_THRESHOLD - 200);
        }
        else if (channel4 > CH4_HIGH_THRESHOLD)
        {
            if (channel4 >= 1800) servo1_speed = 1000;
            else servo1_speed = ((channel4 - CH4_HIGH_THRESHOLD) * 1000) / (1800 - CH4_HIGH_THRESHOLD);
        }
        else servo1_speed = 0;

        // 通道2 -> 3号舵机
        #define CH2_LOW_THRESHOLD  600
        #define CH2_HIGH_THRESHOLD 1400
        if (channel2 < CH2_LOW_THRESHOLD)
        {
            if (channel2 <= 200) servo3_speed = -1000;
            else servo3_speed = -1000 + ((channel2 - 200) * 1000) / (CH2_LOW_THRESHOLD - 200);
        }
        else if (channel2 > CH2_HIGH_THRESHOLD)
        {
            if (channel2 >= 1800) servo3_speed = 1000;
            else servo3_speed = ((channel2 - CH2_HIGH_THRESHOLD) * 1000) / (1800 - CH2_HIGH_THRESHOLD);
        }
        else servo3_speed = 0;

        // 通道3 -> 4号舵机
        #define CH3_LOW_THRESHOLD  600
        #define CH3_HIGH_THRESHOLD 1400
        if (channel3 < CH3_LOW_THRESHOLD)
        {
            if (channel3 <= 200) servo4_speed = -1000;
            else servo4_speed = -1000 + ((channel3 - 200) * 1000) / (CH3_LOW_THRESHOLD - 200);
        }
        else if (channel3 > CH3_HIGH_THRESHOLD)
        {
            if (channel3 >= 1800) servo4_speed = 1000;
            else servo4_speed = ((channel3 - CH3_HIGH_THRESHOLD) * 1000) / (1800 - CH3_HIGH_THRESHOLD);
        }
        else servo4_speed = 0;

        if (servo1_speed < -1000) servo1_speed = -1000;
        if (servo1_speed > 1000) servo1_speed = 1000;
        if (servo3_speed < -1000) servo3_speed = -1000;
        if (servo3_speed > 1000) servo3_speed = 1000;
        if (servo4_speed < -1000) servo4_speed = -1000;
        if (servo4_speed > 1000) servo4_speed = 1000;

        Servo_Set360(1, servo1_speed);
        Servo_Set360(3, servo3_speed);
        Servo_Set360(4, servo4_speed);
        Servo_Set180(2, SERVO_180_MID_PULSE);
    }
}
else
{
    Servo_Stop360(1);
    Servo_Stop360(3);
    Servo_Stop360(4);
}

HAL_Delay(10);
```

---

## 新会话使用说明

在新的Claude对话窗口中，可以这样提示：

> 我有一个STM32F407VET6的三轮车项目，项目路径是 `C:\Users\ttbstt\Desktop\sanlun_1.12\sanlun_1.12\Core\Src\main.c`。
> 请先阅读这份修改记录文档（`C:\Users\ttbstt\Desktop\sanlun_1.12\sanlun_1.12修改记录.md`）了解项目背景，然后帮我继续修改。
