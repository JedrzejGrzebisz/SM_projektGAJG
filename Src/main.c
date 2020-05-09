/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bh1750.h"
#include "i2c-lcd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

int BH1750_int;                   //Aktualna wartość natężenia światła
int lx_zadana_int;                //Zadana wartość natężenia światła
int duty;                         //Wypełnienie PWM regulowane w zakresie 0-1000
int uchyb;                        //Uchyb regulacji
int kontrola_znaku;				  //Zmienna kontrolująca błąd w zadawnej wartości
int transmit_size_lx;             //Rozmiar wysyłanego komunikatu o natężeniu
int transmit_size_error;          //Rozmiar wysyłanego komunikatu o błędzie
const int receive_size = 4;       //Oczekiwany rozmiar odbieranej wiadmości
char bufor_lx[200];               //Bufor na wysyłany komunikat o natężeniu
char bufor_error[200];            //Bufor na wysyłany komunikat o błędzie
char lx_zadana_char[4];           //Bufor na odebraną wartość zadaną
char lx_lcd[20];                  //Bufor na wartość dla wyświetlacza
HAL_StatusTypeDef port_status;    //Status odbioru

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Obsługa transmisji komunikatu o błędzie
 * W przypadku niepoprawnego znaku w wartości zadanej
 * wyświetlany jest odpowiedni komunikat  */
void transmisja_error()
{
	transmit_size_error = sprintf(bufor_error, "Blad wartosci zadanej!!\n\r");
	HAL_UART_Transmit(&huart3, (uint8_t*)bufor_error, transmit_size_error, 100);
}

/* Obsługa przerwania w przypadku odbioru przez UART
 * Funkcja odczytuje wartość zadaną wpisując do bufora(tablica znaków)
 * oraz konwertuje na wartość całkowitoliczbową(int)
 * Dodatkowo zaimplementowana została odporność na błędy
 * w postaci niepoprawnego znaku w wartości zadanej  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	for (int i=0; i<4; i++)
	{
		if(lx_zadana_char[i] > 47 && lx_zadana_char[i] < 58)
		{
			kontrola_znaku = 1;
		}
		else
		{
			kontrola_znaku = 0;
			break;
		}
	}
	if(kontrola_znaku)
	{
		lx_zadana_int = atoi(lx_zadana_char);
	}
	else
	{
		transmisja_error();
		memset(lx_zadana_char, 0, sizeof(lx_zadana_char));
	}
	port_status = HAL_UART_Receive_IT(&huart3, (uint8_t*)lx_zadana_char, receive_size);
}

/* Obsługa transmisji danych przez interfejs UART
 * Funkcja zapisuje zadany tekst do bufora
 * oraz wysyła wiadomość przez interfejs UART  */
void transmisja_danych()
{
	transmit_size_lx = sprintf(bufor_lx, "Natezenie swiatla [lux]: %d\n\r", BH1750_int);
	HAL_UART_Transmit(&huart3, (uint8_t*)bufor_lx, transmit_size_lx, 100);
}

/* Regulacja histerezowa
 * Funkcja porównuje wartość zadaną oraz odczytaną z czujnika
 * oraz reguluje wypełnienie PWM, w zależności od uchybu  */
void regulacja()
{
	if(uchyb > 70)
	{
		if (BH1750_int <= lx_zadana_int && duty <= 1000)
		{
			duty +=10;
			TIM3->CCR3=duty;
		}
		else if (BH1750_int > lx_zadana_int && duty > 0)
		{
			duty -=10;
			TIM3->CCR3=duty;
		}
	}

	if(uchyb <= 70)
	{
		if (BH1750_int <= lx_zadana_int && duty <= 1000)
		{
			 duty +=1;
			 TIM3->CCR3=duty;
		}
		else if (BH1750_int > lx_zadana_int && duty > 0)
		{
			 duty -=1;
			 TIM3->CCR3=duty;
		}
	}
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
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  BH1750_Init();                            //inicjalizacja czujnika
  lcd_init();								//inicjalizacja wyświetlacza
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); //wystartowanie PWM
  TIM3->CCR3 = 0;                           //ustalenie startowej wartości wypełnienia
  lcd_put_cur(0, 5);                        //umieszczenie kursora wyświetlacza w zadanej pozycji
  lcd_send_string("lx");                    //wysłanie symbolu jednostki na wyświetlacz

  //Jednorazowe wywołanie odbioru danych przez UART
  port_status = HAL_UART_Receive_IT(&huart3, (uint8_t*)lx_zadana_char, receive_size);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  BH1750_int=(int)BH1750_Receive();            //Odczyt wartości natężenia światła z czujnika
	  transmisja_danych();                         //Transmisja komunikatu w terminalu
	  itoa(BH1750_int, lx_lcd, 10);                //Konwersja odczytanej wartości int do string
	  lcd_put_cur(0, 0);                           //Umieszczenie kursora na wyświetlaczu w zadanym punkcie
	  lcd_send_string(lx_lcd);                     //Wysyłanie wartości natężenia na wyświetlacz, jako ciąg znaków
	  HAL_Delay(500);                              //Odczekanie 0.5s(regulowana częstotliwość wysyłania)
	  uchyb = abs(lx_zadana_int - BH1750_int);     //Obliczenie uchybu
	  regulacja();                                 //Regulacja natężenia światła

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_I2C2|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
