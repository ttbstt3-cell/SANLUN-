/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    servo.h
  * @brief   舵机控制头文件
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SERVO_H__
#define __SERVO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
// PWM周期为20000 (20ms, 50Hz)
#define SERVO_PWM_PERIOD 20000
// 180度舵机脉宽范围：500-2500us (对应0.5ms-2.5ms)
#define SERVO_180_MIN_PULSE 500   // 0度
#define SERVO_180_MAX_PULSE 2500  // 180度
#define SERVO_180_MID_PULSE 1500  // 90度
// 360度连续旋转舵机脉宽范围：500-2500us
// 1500us = 停止, <1500us = 反转, >1500us = 正转
#define SERVO_360_STOP_PULSE 1500
#define SERVO_360_MIN_PULSE 500   // 最大反转速度
#define SERVO_360_MAX_PULSE 2500  // 最大正转速度

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Servo_Init(void);
void Servo_Set180(uint8_t servo_num, uint16_t pulse_us);
void Servo_Set360(uint8_t servo_num, int16_t speed);  // speed: -1000到+1000
void Servo_Stop360(uint8_t servo_num);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __SERVO_H__ */

