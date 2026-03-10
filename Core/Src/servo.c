/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    servo.c
  * @brief   舵机控制实现文件
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "servo.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void Servo_SetPulse(uint8_t channel, uint16_t pulse_us);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  舵机初始化
  * @retval None
  */
void Servo_Init(void)
{
    // 启动所有PWM通道
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // PE9 - 舵机1 (360度)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  // PE11 - 舵机2 (180度)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);  // PE13 - 舵机3 (360度)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);  // PE14 - 舵机4 (360度)
    
    // 初始化所有舵机到中位/停止位置
    Servo_SetPulse(TIM_CHANNEL_1, SERVO_360_STOP_PULSE);
    Servo_SetPulse(TIM_CHANNEL_2, SERVO_180_MID_PULSE);
    Servo_SetPulse(TIM_CHANNEL_3, SERVO_360_STOP_PULSE);
    Servo_SetPulse(TIM_CHANNEL_4, SERVO_360_STOP_PULSE);
}

/**
  * @brief  设置PWM脉宽（内部函数）
  * @param  channel: 定时器通道
  * @param  pulse_us: 脉宽（微秒）
  * @retval None
  */
static void Servo_SetPulse(uint8_t channel, uint16_t pulse_us)
{
    uint32_t pulse_ticks;
    
    // 将微秒转换为定时器计数值
    // APB2时钟 = 84MHz, APB2分频 = 2, 所以TIM1时钟 = 84MHz * 2 = 168MHz
    // Prescaler = 167, 所以定时器频率 = 168MHz / (167 + 1) = 1MHz
    // 因此 1us = 1个计数
    pulse_ticks = pulse_us;
    
    // 限制脉宽范围
    if (pulse_ticks > SERVO_PWM_PERIOD)
    {
        pulse_ticks = SERVO_PWM_PERIOD;
    }
    
    // 设置PWM占空比
    __HAL_TIM_SET_COMPARE(&htim1, channel, pulse_ticks);
}

/**
  * @brief  设置180度舵机角度
  * @param  servo_num: 舵机编号 (2)
  * @param  pulse_us: 脉宽（500-2500微秒）
  * @retval None
  */
void Servo_Set180(uint8_t servo_num, uint16_t pulse_us)
{
    uint8_t channel;
    
    // 限制脉宽范围
    if (pulse_us < SERVO_180_MIN_PULSE)
        pulse_us = SERVO_180_MIN_PULSE;
    if (pulse_us > SERVO_180_MAX_PULSE)
        pulse_us = SERVO_180_MAX_PULSE;
    
    // 根据舵机编号选择通道
    switch(servo_num)
    {
        case 2:
            channel = TIM_CHANNEL_2;  // PE11
            break;
        default:
            return;  // 无效的舵机编号
    }
    
    Servo_SetPulse(channel, pulse_us);
}

/**
  * @brief  设置360度连续旋转舵机速度
  * @param  servo_num: 舵机编号 (1, 3, 4)
  * @param  speed: 速度值 (-1000到+1000, 0=停止)
  * @retval None
  */
void Servo_Set360(uint8_t servo_num, int16_t speed)
{
    uint8_t channel;
    uint16_t pulse_us;
    
    // 限制速度范围
    if (speed < -1000) speed = -1000;
    if (speed > 1000) speed = 1000;
    
    // 根据舵机编号选择通道
    switch(servo_num)
    {
        case 1:
            channel = TIM_CHANNEL_1;  // PE9
            break;
        case 3:
            channel = TIM_CHANNEL_3;  // PE13
            break;
        case 4:
            channel = TIM_CHANNEL_4;  // PE14
            break;
        default:
            return;  // 无效的舵机编号
    }
    
    // 将速度值转换为脉宽
    // speed范围: -1000到+1000
    // 脉宽范围: 500到2500us
    // 1500us = 停止
    if (speed == 0)
    {
        pulse_us = SERVO_360_STOP_PULSE;
    }
    else if (speed > 0)
    {
        // 正转: 1500-2500us
        pulse_us = SERVO_360_STOP_PULSE + (speed * (SERVO_360_MAX_PULSE - SERVO_360_STOP_PULSE) / 1000);
    }
    else
    {
        // 反转: 500-1500us
        pulse_us = SERVO_360_STOP_PULSE + (speed * (SERVO_360_STOP_PULSE - SERVO_360_MIN_PULSE) / 1000);
    }
    
    Servo_SetPulse(channel, pulse_us);
}

/**
  * @brief  停止360度连续旋转舵机
  * @param  servo_num: 舵机编号 (1, 3, 4)
  * @retval None
  */
void Servo_Stop360(uint8_t servo_num)
{
    Servo_Set360(servo_num, 0);
}

/* USER CODE END 0 */

