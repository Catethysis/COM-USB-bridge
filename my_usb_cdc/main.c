
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define TESTRESULT_ADDRESS         0x080FFFFC
#define ALLTEST_PASS               0x00000000
#define ALLTEST_FAIL               0x55555555

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment = 4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;
  
uint16_t PrescalerValue = 0;

__IO uint32_t TimingDelay;
__IO uint8_t DemoEnterCondition = 0x00;
__IO uint8_t UserButtonPressed = 0x00;

uint8_t Buffer[6];

/* Private function prototypes -----------------------------------------------*/
static void Demo_Exec(void);


void USART2_Init( void ) {
    USART_ClockInitTypeDef USART_ClockInitStruct;
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;            // Rx Pin
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Make sure you use 'GPIO_PinSource2' and NOT 'GPIO_Pin_2'.  Using the
    // latter will not work!
    GPIO_PinAFConfig( GPIOA, GPIO_PinSource2, GPIO_AF_USART2 );
    GPIO_PinAFConfig( GPIOA, GPIO_PinSource3, GPIO_AF_USART2 );

    // Make sure syncro clock is turned off.
    USART_ClockStructInit( &USART_ClockInitStruct );
    USART_ClockInit( USART2, &USART_ClockInitStruct  );

    USART_StructInit( &USART_InitStructure );
    // Initialize USART
    USART_InitStructure.USART_BaudRate = 9600;    
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init( USART2, &USART_InitStructure );
    //USART2->BRR = 364;
    USART_Cmd( USART2, ENABLE );
    NVIC_EnableIRQ(USART2_IRQn);
}

void print(const char* str)
{
    int i = 0; while(str[i]) {
        USART_SendData(USART2, (uint8_t) str[i++]);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET) {}
    }    
}

void send(const char dat)
{
    USART_SendData(USART2, (uint8_t) dat);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET) {}
}

void usart_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; //Структура содержащая настройки порта
  USART_InitTypeDef USART_InitStructure; //Структура содержащая настройки USART
 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //Включаем тактирование порта A
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); //Включаем тактирование порта USART2
 
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //Подключаем PA3 к TX USART2
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //Подключаем PA2 к RX USART2
 
  //Конфигурируем PA2 как альтернативную функцию -> TX UART. Подробнее об конфигурации можно почитать во втором уроке.
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
 
  //Конфигурируем PA2 как альтернативную функцию -> RX UART. Подробнее об конфигурации можно почитать во втором уроке.
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
 
  USART_StructInit(&USART_InitStructure); //Инициализируем UART с дефолтными настройками: скорость 9600, 8 бит данных, 1 стоп бит
 
  USART_Init(USART2, &USART_InitStructure);
  USART_Cmd(USART2, ENABLE); //Включаем UART
    __enable_irq();
   NVIC_EnableIRQ(USART2_IRQn);
   
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  
  /* Initialize LEDs and User_Button on STM32F4-Discovery --------------------*/ 
  STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);//при нажатии кнопки, в порт выводится "HELLO"
  
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);
  
  /* SysTick end of count event each 1ms */
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);  
  
  DemoEnterCondition = 0x00;
    
  /* Reset UserButton_Pressed variable */
  UserButtonPressed = 0x00;
  
  /* USB configuration */
  USBD_Init(&USB_OTG_dev,
        USB_OTG_FS_CORE_ID,
        &USR_desc, 
        &USBD_CDC_cb, 
        &USR_cb);
  
  usart_init();
  
  DemoEnterCondition = 0x01;
  //дальше все на прерываниях
  while (1) {}
  //при передаче в порт символов A и S - светодиод загорается и гаснет 
  //обработка идет в функции VCP_DataRx 
  //при нажатии кнопки на DISCOVERY по прерыванию в порт передается "HELLO"
}




/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}




#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
