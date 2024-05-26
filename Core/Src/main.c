/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DataTransfer_STM_Matlab.h"
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

// buffer s uvodnim pozdravem volan pri vykonani main()
//uint8_t welcome_tx_buffer[16] = "Hello world\n\r";

// dva znaky, data jsou prenasena binarne a nemela by obsahovat ukoncovaci znak
uint16_t iD;

//delka nacitanych dat
uint16_t nData = 0;

//nactena data
uint32_t xxData[10];

//promenna pro stavovy automat nacitani dat prichazejicich po seriove lince
int rx_state = 0;

uint8_t tx_buffer[44];

//promenna pro odesilani zpetne vazby do matlabu
int periodical = 0;

//promenne casovace
uint32_t tic, toc, dma_tic, dma_toc;

//testovaci pole, zjistovani casu potrebneho ke kopirovani dat
float dma[1000];
float cpy[1000];

//promenna pro DMA kruhovy buffer
uint16_t dma_data_buffer[200];

float potenciometer;
int potenciometerInt;
uint8_t potenciometerArr[sizeof(int)];
uint8_t potenciometerArr2[sizeof(int)];



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */




/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  UNUSED(htim);

  if(htim == &htim6){
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

	// zvysovani promenne periodical a odesilani po UART
	periodical += 1;
	//HAL_UART_Transmit_DMA(&hlpuart1, (const uint8_t*)&periodical, 10);
	DataTransmit2MTLB(1010, &periodical, 1);
  }
}

char testdata[10];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
  {
    UNUSED(huart);

//    if(huart == &hlpuart1){
//    	HAL_UART_Transmit_DMA(&hlpuart1, (uint8_t*)testdata, 10);
//    	HAL_UART_Receive_DMA(&hlpuart1, (uint8_t*)testdata, 10);
//    }

  }

void DataReceive_MTLB_Callback(uint16_t iD, uint32_t * xData, uint16_t nData_in_values)
{//when data comes from matlab, this is called and here is the branching and processing
	//HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	switch(iD)
	{
	case 20:
		DataTransmit2MTLB(20, xData, nData_in_values);
		break;
	case 4001://get temperature ... some example....
	// DataTransmit2MTLB(40001,(uint8_t *) &teplota[nTeplota], 1);
	break;
	case 4002:
	// teplota[60]=nTeplota;
	// DataTransmit2MTLB(40002,(uint8_t *) teplota, 61);
	break;

	default:
	break;
	}
	//HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

void tx_send(){

	uint8_t *ptr = tx_buffer;

    // Kopírování proměnných iD a nData do tx_buffer

    memcpy(ptr, &iD, sizeof(iD));
    ptr += sizeof(iD);

    memcpy(ptr, &nData, sizeof(nData));
    ptr += sizeof(nData);

    memcpy(ptr, xxData, nData*sizeof(uint32_t));

	uint8_t tx_status = USBD_OK;
    tx_status = CDC_Transmit_FS(tx_buffer, (nData*sizeof(uint32_t))+4);

    if(tx_status == USBD_OK){
    	tx_status = tx_status;
    }

    DataTransmit2MTLB(20, tx_buffer, nData);
}

int8_t CDC_myReceive_FS(uint8_t* Buf, uint32_t *Len){
	// stavovy automat

	switch(rx_state){
	case 0:
		//nova zprava

		iD = ((uint16_t *) Buf)[0];

        rx_state = 1;
		break;

	case 1:
		//delka ocekavanych dat
	    nData = ((uint16_t *) Buf)[0] *4;

		rx_state = 2;
		break;

	case 2:
		//nacitani samotnych dat
		memcpy((uint8_t*)xxData, Buf, 4*nData);
		tx_send();
		rx_state = 0;
		break;
	}


	return 0;
}

void tx_process(void)//called from inf. loop
{
	//if(!m2s_Status) return;//the most often ....
	if(rx_state == 3)
	{
		tx_send();
		rx_state = 0;
		return;
	}
}



/* ------------------ DMA FUNKCE A CALLBACKY ------------------ */
void myDmaFunction(DMA_HandleTypeDef *_hdma){
	dma_tic = htim5.Instance->CNT;
	dma_toc = htim5.Instance->CNT;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  UNUSED(hadc);

  potenciometer = 0;

  for(int i = 0; i < 100; i++){
	  potenciometer = potenciometer + dma_data_buffer[i+100];
  }
  potenciometer = potenciometer / 100;

  potenciometerInt = (int)potenciometer;

  SendInt2MTLB(23, &potenciometerInt);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  UNUSED(hadc);

  potenciometer = 0;

  for(int i = 0; i < 100; i++){
	  potenciometer = potenciometer + dma_data_buffer[i];
  }
  potenciometer = potenciometer / 100;

  potenciometerInt = (int)potenciometer;

  //SendInt2MTLB(23, &potenciometerInt);
}

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
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM6_Init();
  MX_USB_DEVICE_Init();
  MX_TIM5_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */


  // zapnuti zelene ledky
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);


  // volani casovace
  HAL_TIM_Base_Start_IT(&htim6);

  //char *msg = "Hello world!\n\r";
  //HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), 0xFFFF);
  //HAL_UART_Receive_DMA(&hlpuart1, (uint8_t*)testdata, 10);

  //zjistovani casu potrebneho pro kopirovani mezi poli
  HAL_TIM_Base_Start(&htim5);

  for (int i = 0; i < 1000; i++){
	  dma[i] = i;
  }

  //tic = htim5.Instance->CNT;
  //memcpy(cpy, dma, 500*sizeof(float));
  //toc = htim5.Instance->CNT;

  HAL_StatusTypeDef status = HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel2, HAL_DMA_XFER_CPLT_CB_ID, &myDmaFunction);
  UNUSED(status);

  tic = htim5.Instance->CNT;
  HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel2, dma, cpy, 1000);
  dma_toc = htim5.Instance->CNT;
  toc = htim5.Instance->CNT;

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  HAL_StatusTypeDef adc_status = HAL_ADC_Start_DMA(&hadc1, dma_data_buffer, 200);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  m2s_Process();

	  //tx_process();

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 30;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

#ifdef  USE_FULL_ASSERT
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
