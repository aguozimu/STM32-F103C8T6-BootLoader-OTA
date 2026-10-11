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
#include "iwdg.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Mydef.h"
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
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

	FREQ2_4G_ReadInit();
uint8_t RXData[12]={0};
uint8_t RXData_BUFF[12]={0};
uint8_t TXData[1]={0xF5};

ReadClc_Data Data={0};

Data.RXData_P0=RXData;
Data.TXData=TXData;
Data.TXLen=1;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//	  Delay_ms(50);
	HAL_IWDG_Refresh(&hiwdg);
	if(FREQ2_4G_ReadClc(&Data)==HAL_OK){
		/*查看同步位*/
		if(RXData[3]==1){
		

		
			/*拷贝一份*/
			for(int i=0;i<12;i++){
			RXData_BUFF[i]=RXData[i];
			}
			
			
		if(Data.RX_PIN==0){//通道0发起同步

		
		/*一直等待1通道发来消息*/
		while(Data.RX_PIN==1){FREQ2_4G_ReadClc(&Data);};
		/*看模式位*/	
		if(RXData[7]==0){//转移1~2bit位
		RXData_BUFF[1]=RXData[1];
		RXData_BUFF[2]=	RXData[2];
		
		
		}else if(RXData[7]==2){//转移4~6bit，8~10bit
		RXData_BUFF[4]=RXData[4];
		RXData_BUFF[5]=RXData[5];
		RXData_BUFF[6]=RXData[6];
			
		RXData_BUFF[8]=RXData[8];			
		RXData_BUFF[9]=RXData[9];
		RXData_BUFF[10]=RXData[10];		
		};
		}else if(Data.RX_PIN==1){//通道1发起同步
			
		/*一直等待0通道发来消息*/
		while(Data.RX_PIN==0){FREQ2_4G_ReadClc(&Data);};
		/*看模式位*/	
		if(RXData[7]==0){//转移1~2bit位
		RXData_BUFF[1]=RXData[1];
		RXData_BUFF[2]=	RXData[2];
		
		
		}else if(RXData[7]==2){//转移4~6bit，8~10bit
		RXData_BUFF[4]=RXData[4];
		RXData_BUFF[5]=RXData[5];
		RXData_BUFF[6]=RXData[6];
			
		RXData_BUFF[8]=RXData[8];			
		RXData_BUFF[9]=RXData[9];
		RXData_BUFF[10]=RXData[10];		
		};		
		
		};
			

		
		};
	
		/*处理完发送*/
	  HAL_UART_Transmit(&huart1,RXData,Data.RXLen,HAL_MAX_DELAY); 

	};

	
//	uint8_t TX;
//	uint8_t RX[6];
//	
//	SPI_CSN(0);
//	TX=SI24R1_ReadRegister(SI24R1_SETUP_RETR);
//	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX[0],1,HAL_MAX_DELAY);
//	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX[1],1,HAL_MAX_DELAY);
//	SPI_CSN(1);
//	
//	 HAL_UART_Transmit(&huart1,RX,6,HAL_MAX_DELAY); 
//	
//	
//	


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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
