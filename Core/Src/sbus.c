/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sbus.c
  * @brief   SBUS协议解析实现文件
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "sbus.h"
#include <string.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define SBUS_RX_BUFFER_SIZE 32

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
uint8_t sbus_rx_buffer[1];  // 改为全局，供HAL回调使用
static uint8_t sbus_frame[SBUS_FRAME_SIZE];
static uint8_t sbus_frame_index = 0;
static uint32_t sbus_last_frame_time = 0;
static SBUS_Data_t sbus_data = {0};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void SBUS_ParseFrame(uint8_t *frame);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  SBUS初始化
  * @retval None
  */
void SBUS_Init(void)
{
    sbus_frame_index = 0;
    sbus_last_frame_time = HAL_GetTick();  // 初始化为当前时间，避免立即超时
    memset(&sbus_data, 0, sizeof(SBUS_Data_t));
    
    // 启动UART5接收
    HAL_UART_Receive_IT(&huart5, sbus_rx_buffer, 1);
}

/**
  * @brief  SBUS接收回调函数（在UART中断中调用）
  * @param  data: 接收到的数据指针
  * @param  len: 数据长度
  * @retval None
  */
void SBUS_Receive_Callback(uint8_t *data, uint16_t len)
{
    static uint8_t byte;
    
    if (len > 0)
    {
        byte = data[0];
        
        // 检查起始字节
        if (byte == SBUS_START_BYTE)
        {
            sbus_frame_index = 0;
            sbus_frame[sbus_frame_index++] = byte;
        }
        else if (sbus_frame_index > 0 && sbus_frame_index < SBUS_FRAME_SIZE)
        {
            sbus_frame[sbus_frame_index++] = byte;
            
            // 检查是否接收到完整帧
            if (sbus_frame_index == SBUS_FRAME_SIZE)
            {
                // 检查结束字节
                if (sbus_frame[SBUS_FRAME_SIZE - 1] == SBUS_END_BYTE)
                {
                    SBUS_ParseFrame(sbus_frame);
                    sbus_last_frame_time = HAL_GetTick();
                }
                sbus_frame_index = 0;
            }
        }
        else
        {
            sbus_frame_index = 0;
        }
    }
    
    // 注意：接收继续在HAL_UART_RxCpltCallback中处理
}

/**
  * @brief  解析SBUS帧数据
  * @param  frame: SBUS帧数据指针
  * @retval None
  */
static void SBUS_ParseFrame(uint8_t *frame)
{
    // 解析16个通道数据
    // 每个通道11位，共22字节存储16个通道
    sbus_data.channels[0]  = ((frame[1]    | frame[2]  << 8)                        & 0x07FF);
    sbus_data.channels[1]  = ((frame[2]>>3 | frame[3]  << 5)                        & 0x07FF);
    sbus_data.channels[2]  = ((frame[3]>>6 | frame[4]  << 2 | frame[5] << 10)       & 0x07FF);
    sbus_data.channels[3]  = ((frame[5]>>1 | frame[6]  << 7)                        & 0x07FF);
    sbus_data.channels[4]  = ((frame[6]>>4 | frame[7]  << 4)                        & 0x07FF);
    sbus_data.channels[5]  = ((frame[7]>>7 | frame[8]  << 1 | frame[9] << 9)        & 0x07FF);
    sbus_data.channels[6]  = ((frame[9]>>2 | frame[10] << 6)                        & 0x07FF);
    sbus_data.channels[7]  = ((frame[10]>>5| frame[11] << 3)                        & 0x07FF);
    sbus_data.channels[8]  = ((frame[12]   | frame[13] << 8)                        & 0x07FF);
    sbus_data.channels[9]  = ((frame[13]>>3| frame[14] << 5)                        & 0x07FF);
    sbus_data.channels[10] = ((frame[14]>>6| frame[15] << 2 | frame[16] << 10)      & 0x07FF);
    sbus_data.channels[11] = ((frame[16]>>1| frame[17] << 7)                        & 0x07FF);
    sbus_data.channels[12] = ((frame[17]>>4| frame[18] << 4)                        & 0x07FF);
    sbus_data.channels[13] = ((frame[18]>>7| frame[19] << 1 | frame[20] << 9)        & 0x07FF);
    sbus_data.channels[14] = ((frame[20]>>2| frame[21] << 6)                        & 0x07FF);
    sbus_data.channels[15] = ((frame[21]>>5| frame[22] << 3)                        & 0x07FF);
    
    // 解析标志字节（第23字节）
    sbus_data.frame_lost = (frame[23] >> 2) & 0x01;
    sbus_data.failsafe = (frame[23] >> 3) & 0x01;
    
    // 数据有效标志
    sbus_data.valid = !sbus_data.failsafe && !sbus_data.frame_lost;
}

/**
  * @brief  SBUS处理函数（在主循环中调用）
  * @retval None
  */
void SBUS_Process(void)
{
    uint32_t current_time = HAL_GetTick();
    // 检查超时（超过30ms没有接收到新帧则认为丢失，SBUS帧周期约14ms）
    // 使用无符号数减法处理溢出情况
    if (current_time - sbus_last_frame_time > 30)
    {
        sbus_data.valid = 0;
    }
}

/**
  * @brief  获取指定通道的值
  * @param  channel: 通道号 (1-16)
  * @retval 通道值 (172-1811)
  */
uint16_t SBUS_GetChannel(uint8_t channel)
{
    if (channel >= 1 && channel <= SBUS_CHANNEL_NUMBER)
    {
        return sbus_data.channels[channel - 1];
    }
    return 992;  // 返回中位值
}

/**
  * @brief  检查SBUS数据是否有效
  * @retval 1-有效, 0-无效
  */
uint8_t SBUS_IsValid(void)
{
    return sbus_data.valid;
}

/* USER CODE END 0 */

