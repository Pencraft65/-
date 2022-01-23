/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
//==========================================================================
// Управление кухонной вытяжкой Krona Steel Jasmin 600 Smart Inox Glass 5P
// С автоматическим режимом
//
// Индикатор 7-сегментный
// a  - A0
// b  - A1
// c  - A2
// d  - A3
// e  - A4
// f  - A5
// g  - A6
// dp - A7
//
// Кнопки управления
// ON/OFF - A4
// +      - A0
// -      - A3
// AUTO   - A1
// LAMP   - A2
//
// Реле двигателя
// 1 скорость - A8
// 2 скорость - A9
// 3 скорость - A10
// 4 скорость - A11
// 5 скорость - A12
//
// Реле ламп подсветки
// B3
//
// Датчик воздуха
// АЦП B0
//==========================================================================

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
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

// Определения символов для семисегментного индикатора
uint8_t LIT_BLANK[8]    = {1,1,1,1,1,1,1,1};
uint8_t LIT_1[8]        = {1,0,0,1,1,1,1,1};
uint8_t LIT_2[8]        = {0,0,1,0,0,1,0,1};
uint8_t LIT_3[8]        = {0,0,0,0,1,1,0,1};
uint8_t LIT_4[8]        = {1,0,0,1,1,0,0,1};
uint8_t LIT_5[8]        = {0,1,0,0,1,0,0,1};
uint8_t LIT_A[8]        = {0,0,0,1,0,0,0,1};

GPIO_InitTypeDef GPIO_InitStruct = {0};

// Переменные состояния кнопок
uint8_t VALCODE1 = 1;
uint8_t VALCODEOLD1 = 1;
uint8_t VALCODE2 = 1;
uint8_t VALCODEOLD2 = 1;
uint8_t VALCODE3 = 1;
uint8_t VALCODEOLD3 = 1;
uint8_t VALCODE4 = 1;
uint8_t VALCODEOLD4 = 1;
uint8_t VALCODE5= 1;
uint8_t VALCODEOLD5= 1;

uint8_t FlagPrintSpeed = 0;
uint8_t LAMP=0; // Состояние ламп подсветки
uint8_t SPEED = 0; // Скорость двигателя
uint8_t AUTO_ON = 0; // Состояние автоматического режима

int mq4_data = 0; //Данные АЦП

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void PrintLit(uint8_t* LIT);    // Вывод символа на индикатор
void KeyScan(void);             // Опрос кнопок
void PrintSpeed(uint8_t Speed); // Вывод скорости двигателя на индикатор
int mq4_get_adc (void);         // Опрос АЦП

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
  MX_TIM3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
HAL_TIM_Base_Start_IT(&htim3); //Запуск таймера, раз в 50мс (опрос кнопок)
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET); //Начальное состояние подсветки - выключено
HAL_ADC_Start(&hadc1); // Запуск АЦП
HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    
  // Обработка флага прерывания таймера (каждые 50 мс)
  if (FlagPrintSpeed)
  {
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET);
    FlagPrintSpeed = 0;
    KeyScan();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
  }
  
//==========================================================================
// Обработка управления двигателем
// В ручном режиме скорость двигателя зависит от переменной SPEED (от 0 до 5)
// 0 - все реле выключены, двигатель остановлен
// 1 - включено реле 1 скорости  
// 2 - включено реле 2 скорости
// 3 - включено реле 3 скорости
// 4 - включено реле 4 скорости
// 5 - включено реле 5 скорости  
// В автоматическом режиме значение переменной SPEED и соответствующее 
// поведение двигателя определяется уровнем, получаемым с АЦП  
// Выбор режима - переменная AUTO_ON  
//==========================================================================
  if (AUTO_ON == 0)
  {
  switch (SPEED)
  {
  case 0: // мотор выключен
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);
    break;
  case 1: // 1-я скорость
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);
    break;
  case 2: // 2-я скорость
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);
    break;
  case 3: // 3-я скорость
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);
    break;    
  case 4: // 4-я скорость
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_12, GPIO_PIN_SET);
    break;    
  case 5: // 5-я скорость
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_SET);
    break;
  }
  }else{
mq4_data = mq4_get_adc();
if (mq4_data>100 & mq4_data<400)
{
  SPEED=1;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);

}else
if (mq4_data>500 & mq4_data<900)
{
  SPEED=2;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);

}else
if (mq4_data>1000 & mq4_data<1400)
{
  SPEED=3;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12, GPIO_PIN_SET);

}else
if (mq4_data>1500 & mq4_data<1900)
{
  SPEED=4;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_12, GPIO_PIN_SET);

}else
if (mq4_data>2000)
{
  SPEED=5;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_SET);

}  

  } // Завершение обработки управления двигателем
  
} // Завершение while 
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

    int mq4_get_adc (void)
 {
 int mq4_adc_bits; 
 HAL_ADC_Start(&hadc1);
 HAL_ADC_PollForConversion(&hadc1,100);
 mq4_adc_bits = HAL_ADC_GetValue(&hadc1);
 HAL_ADC_Stop(&hadc1);
    return mq4_adc_bits;
 }

void PrintLit(uint8_t* LIT)
{
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,LIT[0]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,LIT[1]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,LIT[2]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,LIT[3]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,LIT[4]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,LIT[5]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,LIT[6]);
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,LIT[7]);
 }

void PrintSpeed(uint8_t Speed)
{
  PrintLit(LIT_BLANK);
if(AUTO_ON == 0)
{
switch (Speed)
{
case 1:
  PrintLit(LIT_1);
  break;
case 2:
  PrintLit(LIT_2);
  break;
case 3:
  PrintLit(LIT_3);
  break;
case 4:
  PrintLit(LIT_4);
  break;
case 5:
  PrintLit(LIT_5);
  break;
}
}else{
  PrintLit(LIT_A);
}
}

void KeyScan(void)
{
    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
//Переключить пины кнопок на вход с подтяжкой к 1
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                            |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
   
// Обработка кнопок

// Кнопка +    
    VALCODE1 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
    if(VALCODE1!=VALCODEOLD1 & VALCODE1 == 0 & SPEED != 0)
    {
      HAL_Delay(30);
      VALCODE1 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
      if (VALCODE1 == 0){SPEED++;}
      if(SPEED>5){SPEED=5;}
    }
    VALCODEOLD1 = VALCODE1;

// Кнопка - 
    VALCODE2 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_3);
    if(VALCODE2!=VALCODEOLD2 & VALCODE2 == 0 & SPEED != 0)
    {
      HAL_Delay(30);
      VALCODE2 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_3);
      if (VALCODE2 == 0){SPEED--;}
      if(SPEED<1){SPEED=1;}
    }        
    VALCODEOLD2 = VALCODE2;        

// Кнопка LAMP    
VALCODE3 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2);
    if(VALCODE3!=VALCODEOLD3 & VALCODE3 == 0)
    {
      HAL_Delay(20);
      VALCODE3 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2);
      if (LAMP == 0)
      {
        if (VALCODE3 == 0){HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); LAMP = 1;}
      }else
      {
        if (VALCODE3 == 0){HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET); LAMP = 0;}      
      }
    }
    VALCODEOLD3 = VALCODE3; 
// Кнопка ON/OFF
    VALCODE4 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4);
    if(VALCODE4!=VALCODEOLD4 & VALCODE4 == 0)
    {
      HAL_Delay(30);
      VALCODE4 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4);
      if (SPEED == 0)
      {
        if (VALCODE4 == 0){SPEED = 2;}
      }else
      {
        if (VALCODE4 == 0){SPEED = 0;}      
      }
    }        
    VALCODEOLD4 = VALCODE4;  

    // Кнопка AUTO
    VALCODE5 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1);
    if(VALCODE5!=VALCODEOLD5 & VALCODE5 == 0 & SPEED != 0)
    {
      HAL_Delay(30);
      VALCODE5 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1);
      if (AUTO_ON == 0)
      {
        if (VALCODE5 == 0){AUTO_ON = 1;}
        
      }else
      {
        if (VALCODE5 == 0){AUTO_ON = 0;}      
      }
    }        
    VALCODEOLD5 = VALCODE5; 

// Вернуть пины кнопок в режим выхода с ОК   
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

// Обновить индикатор
    PrintSpeed(SPEED);
}

void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM3)
  {
    FlagPrintSpeed = 1;
  }
}
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

