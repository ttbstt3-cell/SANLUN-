/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sbus.h"
#include "servo.h"
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
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  // 初始化SBUS和舵机
  SBUS_Init();
  Servo_Init();

  // 调试模式开关：设置为1启用调试，设置为0禁用
  // 调试模式下，通过串口输出SBUS通道值（需要额外配置USART1）
  // 或者使用LED/舵机动作来指示通道值
  #define DEBUG_MODE 0

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 处理SBUS数据
    SBUS_Process();
    
    // 如果SBUS数据有效，控制舵机
    if (SBUS_IsValid())
    {
        uint16_t channel2, channel3, channel4, channel5, channel9;
        uint16_t mode_switch;

        // 获取通道值
        channel2 = SBUS_GetChannel(2);  // 模式1:驱动 / 模式2:控制3号舵机
        channel3 = SBUS_GetChannel(3);  // 模式1:转向 / 模式2:控制4号舵机
        channel4 = SBUS_GetChannel(4);  // 模式2:控制1号舵机
        channel5 = SBUS_GetChannel(5);  // 备用
        channel9 = SBUS_GetChannel(9);  // 模式切换通道
        
        // 判断模式：通道9 < 1000 为模式1（原逻辑），>= 1000 为模式2（新模式）
        // 通道9值200左右 = 模式1，1800左右 = 模式2
        mode_switch = channel9;
        
        if (mode_switch < 1000)
        {
            // 模式1：原控制逻辑
            // 通道2控制三个驱动舵机，通道3控制转向舵机
            int16_t drive_speed;
            uint16_t steer_pulse;
            
            // 将通道2值转换为驱动速度 (-1000到+1000)
            // SBUS范围: 200-1800, 中位值: 1002
            // 映射到速度: -1000到+1000
            if (channel2 <= 1002)
            {
                // 反转: 200->-1000, 1002->0
                if (channel2 <= 200)
                {
                    drive_speed = -1000;
                }
                else
                {
                    drive_speed = -1000 + ((channel2 - 200) * 1000) / (1002 - 200);
                }
            }
            else
            {
                // 正转: 1002->0, 1800->+1000
                if (channel2 >= 1800)
                {
                    drive_speed = 1000;
                }
                else
                {
                    drive_speed = ((channel2 - 1002) * 1000) / (1800 - 1002);
                }
            }
            
            // 限制速度范围（双重保险）
            if (drive_speed < -1000) drive_speed = -1000;
            if (drive_speed > 1000) drive_speed = 1000;
            
            // 同步控制三个360度驱动舵机 (1, 3, 4)
            // 1号舵机轮子直径63mm，3号和4号舵机轮子直径50mm
            // 为了同步前进，1号舵机转速 = 其他舵机转速 × (50/63) ≈ 0.794
            // 先乘后除以保持精度
            int16_t servo1_speed = (drive_speed * 50) / 63;
            
            Servo_Set360(1, servo1_speed);  // 1号舵机（63mm轮子，速度按比例调整）
            Servo_Set360(3, drive_speed);   // 3号舵机（50mm轮子）
            Servo_Set360(4, drive_speed);   // 4号舵机（50mm轮子）
            
            // 转向舵机灵敏度控制
            // SBUS范围: 200-1800, 中位值: 1004

            // 方案1: 缩小转动范围 (原500-2500us, 现900-2100us，灵敏度降低40%)
            // 调整这两个值可以改变灵敏度：数值越接近1500，灵敏度越低
            #define STEER_MIN_PULSE  900   // 最小脉宽 (原500, 增大此值降低左转幅度)
            #define STEER_MAX_PULSE  2100  // 最大脉宽 (原2500, 减小此值降低右转幅度)
            #define STEER_DEADZONE   30    // 中位死区 (0-100, 越大死区越大)

            // 计算与中位的偏差
            int16_t deviation = channel3 - 1004;

            // 应用死区：中位附近小幅度摇杆移动不响应
            if (deviation > -STEER_DEADZONE && deviation < STEER_DEADZONE)
            {
                deviation = 0;  // 死区内，偏差归零
            }
            else if (deviation >= STEER_DEADZONE)
            {
                deviation -= STEER_DEADZONE;  // 减去死区
            }
            else if (deviation <= -STEER_DEADZONE)
            {
                deviation += STEER_DEADZONE;  // 减去死区
            }

            // 计算调整后的通道值
            uint16_t adjusted_channel3 = 1004 + deviation;

            // 映射到缩小后的脉宽范围
            steer_pulse = STEER_MIN_PULSE + ((adjusted_channel3 - 200) * (STEER_MAX_PULSE - STEER_MIN_PULSE)) / (1800 - 200);

            // 限制脉宽范围
            if (steer_pulse < STEER_MIN_PULSE) steer_pulse = STEER_MIN_PULSE;
            if (steer_pulse > STEER_MAX_PULSE) steer_pulse = STEER_MAX_PULSE;

            // 控制180度转向舵机 (2号)
            Servo_Set180(2, steer_pulse);
        }
        else
        {
            // 模式2：通道4控制1号舵机，通道2控制3号舵机，通道3控制4号舵机
            // 使用区域判断法，无需精确中位值
            int16_t servo1_speed, servo3_speed, servo4_speed;

            // === 通道4控制1号舵机 ===
            // 三区域控制：低位区（反转）、中位区（停止）、高位区（正转）
            #define CH4_LOW_THRESHOLD  600   // 低于此值为反转区
            #define CH4_HIGH_THRESHOLD 1200  // 高于此值为正转区

            if (channel4 < CH4_LOW_THRESHOLD)
            {
                // 低位区：反转，200->-1000, 600->0
                if (channel4 <= 200)
                {
                    servo1_speed = -1000;
                }
                else
                {
                    servo1_speed = -1000 + ((channel4 - 200) * 1000) / (CH4_LOW_THRESHOLD - 200);
                }
            }
            else if (channel4 > CH4_HIGH_THRESHOLD)
            {
                // 高位区：正转，1200->0, 1800->+1000
                if (channel4 >= 1800)
                {
                    servo1_speed = 1000;
                }
                else
                {
                    servo1_speed = ((channel4 - CH4_HIGH_THRESHOLD) * 1000) / (1800 - CH4_HIGH_THRESHOLD);
                }
            }
            else
            {
                // 中位区（600-1200）：停止
                servo1_speed = 0;
            }

            // === 通道2控制3号舵机 ===
            // 三区域控制：低位区（反转）、中位区（停止）、高位区（正转）
            #define CH2_LOW_THRESHOLD  600   // 低于此值为反转区
            #define CH2_HIGH_THRESHOLD 1400  // 高于此值为正转区

            if (channel2 < CH2_LOW_THRESHOLD)
            {
                // 低位区：反转，200->-1000, 600->0
                if (channel2 <= 200)
                {
                    servo3_speed = -1000;
                }
                else
                {
                    servo3_speed = -1000 + ((channel2 - 200) * 1000) / (CH2_LOW_THRESHOLD - 200);
                }
            }
            else if (channel2 > CH2_HIGH_THRESHOLD)
            {
                // 高位区：正转，1400->0, 1800->+1000
                if (channel2 >= 1800)
                {
                    servo3_speed = 1000;
                }
                else
                {
                    servo3_speed = ((channel2 - CH2_HIGH_THRESHOLD) * 1000) / (1800 - CH2_HIGH_THRESHOLD);
                }
            }
            else
            {
                // 中位区（600-1400）：停止
                servo3_speed = 0;
            }

            // === 通道3控制4号舵机 ===
            // 三区域控制：低位区（反转）、中位区（停止）、高位区（正转）
            #define CH3_LOW_THRESHOLD  600   // 低于此值为反转区
            #define CH3_HIGH_THRESHOLD 1400  // 高于此值为正转区

            if (channel3 < CH3_LOW_THRESHOLD)
            {
                // 低位区：反转，200->-1000, 600->0
                if (channel3 <= 200)
                {
                    servo4_speed = -1000;
                }
                else
                {
                    servo4_speed = -1000 + ((channel3 - 200) * 1000) / (CH3_LOW_THRESHOLD - 200);
                }
            }
            else if (channel3 > CH3_HIGH_THRESHOLD)
            {
                // 高位区：正转，1400->0, 1800->+1000
                if (channel3 >= 1800)
                {
                    servo4_speed = 1000;
                }
                else
                {
                    servo4_speed = ((channel3 - CH3_HIGH_THRESHOLD) * 1000) / (1800 - CH3_HIGH_THRESHOLD);
                }
            }
            else
            {
                // 中位区（600-1400）：停止
                servo4_speed = 0;
            }

            // 限制速度范围
            if (servo1_speed < -1000) servo1_speed = -1000;
            if (servo1_speed > 1000) servo1_speed = 1000;
            if (servo3_speed < -1000) servo3_speed = -1000;
            if (servo3_speed > 1000) servo3_speed = 1000;
            if (servo4_speed < -1000) servo4_speed = -1000;
            if (servo4_speed > 1000) servo4_speed = 1000;

            // 控制三个舵机
            Servo_Set360(1, servo1_speed);  // 通道4控制1号舵机
            Servo_Set360(3, servo3_speed);  // 通道2控制3号舵机
            Servo_Set360(4, servo4_speed);  // 通道3控制4号舵机

            // 转向舵机回到中位
            Servo_Set180(2, SERVO_180_MID_PULSE);  // 2号舵机回到中位
        }
    }
    else
    {
        // SBUS数据无效，停止所有驱动舵机
        Servo_Stop360(1);
        Servo_Stop360(3);
        Servo_Stop360(4);
    }
    
    // 延时，避免CPU占用过高
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
