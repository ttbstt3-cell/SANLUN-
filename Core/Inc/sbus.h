/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sbus.h
  * @brief   SBUS协议解析头文件
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SBUS_H__
#define __SBUS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
#define SBUS_CHANNEL_NUMBER 16
#define SBUS_FRAME_SIZE 25
#define SBUS_START_BYTE 0x0F
#define SBUS_END_BYTE 0x00

/* SBUS数据结构 */
typedef struct {
    uint16_t channels[SBUS_CHANNEL_NUMBER];  // 16个通道值 (172-1811)
    uint8_t failsafe;                        // 故障保护标志
    uint8_t frame_lost;                      // 帧丢失标志
    uint8_t valid;                           // 数据有效标志
} SBUS_Data_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void SBUS_Init(void);
void SBUS_Receive_Callback(uint8_t *data, uint16_t len);
void SBUS_Process(void);
uint16_t SBUS_GetChannel(uint8_t channel);
uint8_t SBUS_IsValid(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __SBUS_H__ */

