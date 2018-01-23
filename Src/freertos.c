/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * Copyright (c) 2018 STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */     
#include "defines.h"

#ifdef WS2812B_ENABLE
#include "ws2812b.h"
#endif

#ifdef LCD_ENABLE
#include "tm_stm32_hd44780.h"
#endif

#ifdef AUDIO_ENABLE
#include "audio.h"
#include "spx.h"
#endif

#ifdef LED_MATRIX_ENABLE
#include "max7219.h"
#endif

#include "otherFunctions.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/
osThreadId LCDTaskHandle;

/* USER CODE BEGIN Variables */
osThreadId ButtonHandle;
osThreadId LEDmatrixHandle;
osThreadId AudioMessageHandle;
osThreadId RGBws2812bHandle;
osThreadId USARTHandle;

extern UART_HandleTypeDef huart2;
extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim6;

/*----------------------------------------------------------------------------*/
/*--------------------------------STATUS--------------------------------------*/
/*----------------------------------------------------------------------------*/
// Здесь хранится состояние кнопки
extern uint8_t playSound;

extern uint8_t eightbit[766][3];

/*----------------------------------------------------------------------------*/
/*--------------------------------FLASH---------------------------------------*/
/*----------------------------------------------------------------------------*/
// Здесь храниться адрес ячейки памяти с которой необходимо начинать запись/чтение
uint32_t addressDes = START_FLASH_PAGE;
// Размер(в байтах) всех полученных данных
uint16_t allDataSize;

// Режим работы кубка. 0-нормальная работа, 1-демо режим, 2-режим обновления
extern uint8_t systemMode;

// Массив, хранящий принимаемые данные до парсинга
uint8_t dataBuffer[60];
/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void StartLCDTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */
void StartButtonTask(void const * argument);

#ifdef LED_MATRIX_ENABLE
void StartLEDmatrixTask(void const * argument);
#endif

#ifdef AUDIO_ENABLE
void StartAudioMessageTask(void const * argument);
#endif

#ifdef WS2812B_ENABLE
void StartRGBws2812bTask(void const * argument);
#endif

void StartUSARTTask(void const * argument);
/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* Create the thread(s) */
	/* definition and creation of LCDTask */
	osThreadDef(LCDTask, StartLCDTask, osPriorityNormal, 0, 64);
	LCDTaskHandle = osThreadCreate(osThread(LCDTask), NULL);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	if(systemMode == SYSTEM_MODE_UPDATE)
	{
		/* definition and creation of USART */
		osThreadDef(USART, StartUSARTTask, osPriorityIdle, 0, 256);
		USARTHandle = osThreadCreate(osThread(USART), NULL);
	}
	else
	{
		osThreadDef(Button, StartButtonTask, osPriorityIdle, 0, 256);
		ButtonHandle = osThreadCreate(osThread(Button), NULL);

#ifdef	LED_MATRIX_ENABLE
		/* definition and creation of LEDmatrix */
		osThreadDef(LEDmatrix, StartLEDmatrixTask, osPriorityIdle, 0, 256);
		LEDmatrixHandle = osThreadCreate(osThread(LEDmatrix), NULL);
#endif

#ifdef	AUDIO_ENABLE
		/* definition and creation of AudioMessage */
		osThreadDef(AudioMessage, StartAudioMessageTask, osPriorityIdle, 0, 256);
		AudioMessageHandle = osThreadCreate(osThread(AudioMessage), NULL);
#endif
#ifdef	WS2812B_ENABLE
		/* definition and creation of RGBws2812b */
		osThreadDef(RGBws2812b, StartRGBws2812bTask, osPriorityIdle, 0, 512);
		RGBws2812bHandle = osThreadCreate(osThread(RGBws2812b), NULL);
#endif
	}
	/* USER CODE END RTOS_THREADS */

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */
}

/* StartLCDTask function */
void StartLCDTask(void const * argument)
{

	/* USER CODE BEGIN StartLCDTask */
	/* Infinite loop */
	for(;;)
	{
#ifndef LCD_ENABLE
		vTaskSusped(NULL);
#endif
#ifdef LCD_ENABLE
		//	// Чистим экран
		//	TM_HD44780_Clear();
		//	osDelay(100);
		//	// Если включен демо-режим, то показываем демо-сообщение,
		//	// если нет, то выводим текст, записанный в память
		//	if(systemMode)
		//	{
		//		TM_HD44780_Puts(0, 0, (uint8_t*)DEMO_TEXT_1, LEN_DEMO_TEXT_1);
		//		TM_HD44780_Puts(0, 1, (uint8_t*)DEMO_TEXT_2, LEN_DEMO_TEXT_2);
		//	}
		//	else
		//	{
		//		TM_HD44780_Puts(0, 0, (void *)TEXT2_address, *(uint32_t*)SIZE_TEXT2_address);
		//		TM_HD44780_Puts(0, 1, (void *)TEXT1_address, *(uint32_t*)SIZE_TEXT1_address);
		//	}
		TM_HD44780_Clear();
		TM_HD44780_Puts(0, 0, (uint8_t*)"Команда N", strlen("Команда N"));
		TM_HD44780_Puts(0, 1, (uint8_t*)"Участник n1", strlen("Участник n1"));
		osDelay(5000);
		TM_HD44780_Clear();
		TM_HD44780_Puts(0, 0, (uint8_t*)"Команда N", strlen("Команда N"));
		TM_HD44780_Puts(0, 1, (uint8_t*)"Участник n2", strlen("Участник n2"));
		osDelay(5000);
		TM_HD44780_Clear();
		TM_HD44780_Puts(0, 0, (uint8_t*)"Команда N", strlen("Команда N"));
		TM_HD44780_Puts(0, 1, (uint8_t*)"Участник n3", strlen("Участник n3"));
		osDelay(5000);
		TM_HD44780_Clear();
		TM_HD44780_Puts(0, 0, (uint8_t*)"Команда N", strlen("Команда N"));
		TM_HD44780_Puts(0, 1, (uint8_t*)"Участник n4", strlen("Участник n4"));
		osDelay(5000);
		TM_HD44780_Clear();
		TM_HD44780_Puts(0, 0, (uint8_t*)"Команда N", strlen("Команда N"));
		TM_HD44780_Puts(0, 1, (uint8_t*)"Участник n5", strlen("Участник n5"));
		osDelay(5000);
		TM_HD44780_Clear();
		TM_HD44780_Puts(0, 0, (uint8_t*)"Команда N", strlen("Команда N"));
		TM_HD44780_Puts(0, 1, (uint8_t*)"Участник n6", strlen("Участник n6"));
#endif
	}
	/* USER CODE END StartLCDTask */
}

/* USER CODE BEGIN Application */

/* StartButtonTask function */
void StartButtonTask(void const * argument)
{
	// Создаем счётчик для фиксации времени нажатия кнопки
	uint8_t pressTime;

	for(;;)
	{
		if(AntiContactBounce(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))
		{
			pressTime = 1;
			// Крутим счётчик до тех пор пока зажата кнопка или
			// значение счётчика меньше 255
			while((!HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))&&(pressTime != 255))
			{
				osDelay(100);
				pressTime++;
			}
			// Если было одно нажатие, то включай запись
			// и зависай в цикле
			if((pressTime <= 30)&&(pressTime >= 1))
			{
#ifdef AUDIO_ENABLE
				playSound = 1;
				osDelay(1000);
				vTaskResume(AudioMessageHandle);
#endif
				// Обнуляем счётчик нажатия кнопки
				pressTime = 0;
			}
		}
		osDelay(10);
	}
	/* USER CODE END StartButtonTask */
}

#ifdef LED_MATRIX_ENABLE
/* StartLEDmatrixTask function */
void StartLEDmatrixTask(void const * argument)
{
	/* USER CODE BEGIN StartLEDmatrixTask */

	/* Infinite loop */
	for(;;)
	{
		// Чистка LED матрицы от изображений
		MAX729_Clean();
		// Если включен демо режим, то крутим демо-анимацию
		// Если демо-режим выключен, то показываем место+анимация
		if(systemMode == SYSTEM_MODE_DEMO)
		{
			while(1)
			{
				// Выводим анимацию на LED matrix
				for(uint8_t i = 1; i < 4; i++)
				{
					DrawAll(i);
				}
				osDelay(1);
			}
		}
		else
		{
			// Выводим место пока не будет нажата кнопка
			switch (*(uint32_t*)START_FLASH_PAGE)
			{
			case 1:
				while(DrawAll(1))
					osDelay(1);
				break;
			case 2:
				while(DrawAll(2))
					osDelay(1);
				break;
			case 3:
				while(DrawAll(3))
					osDelay(1);
				break;
			default:
				Draw((uint8_t*)symbols, 0);  // смайлик
				break;
				// Пока сотояние кнопки не изменится на "выкл"
				// крутим особую анимацию
				while(playSound) Draw((uint8_t*)symbols, 0);
			}
		}
		osDelay(1);
	}
	/* USER CODE END StartLEDmatrixTask */
}
#endif

#ifdef AUDIO_ENABLE
/* StartAudioMessageTask function */
void StartAudioMessageTask(void const * argument)
{
	/* USER CODE BEGIN StartAudioMessageTask */
	/* Infinite loop */
	for(;;)
	{
		if(!playSound)
			vTaskSuspend(NULL);
		// Включаем прерывания по таймеру
		HAL_TIM_Base_Start_IT(&htim6);
		// Включаем ЦАП
		HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

		// Если включен демо-режим, то включаем демо-запись
		if(systemMode == SYSTEM_MODE_DEMO)
			PlayMessage(&spx_voice2[0], spx_frames2);
		// В противном случае проигрываем то, что мы записали в память
		else
			PlayMessage((void*)SPEEX_address, *(uint32_t*)BLOCK_address);
		// Выключаем прерывания по таймеру
		HAL_TIM_Base_Stop_IT(&htim6);
		// Выключаем ЦАП
		HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
		//vTaskResume(RGBws2812bHandle);
		playSound = 0;

		osDelay(1);
	}
	/* USER CODE END StartAudioMessageTask */
}
#endif

#ifdef WS2812B_ENABLE
/* StartRGBws2812bTask function */
void StartRGBws2812bTask(void const * argument)
{
	/* USER CODE BEGIN StartRGBws2812bTask */

	/* Infinite loop */
	for(;;)
	{
		for(uint8_t i = 0; i<=255; i++)
		{
			WS2812_combination_2(i);
			osDelay(1000);
		}
		for (uint16_t i = 0; i+50 < 765; i++)
		{
			if(playSound)
			{
				WS2812_send_group(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				while(playSound)
					osDelay(100);
			}
			else
			{
				WS2812_send_group(eightbit[i][0], eightbit[i][1], eightbit[i][2],
						eightbit[i+50][0], eightbit[i+50][1], eightbit[i+50][2],
						eightbit[i][0], eightbit[i][1], eightbit[i][2],
						eightbit[i+50][0], eightbit[i+50][1], eightbit[i+50][2]);
				osDelay(1000);
			}
		}
		osDelay(100);
	}
	/* USER CODE END StartRGBws2812bTask */
}
#endif

/* StartUSARTTask function */
void StartUSARTTask(void const * argument)
{
	/* USER CODE BEGIN StartUSARTTask */
	/* Infinite loop */
	// Счетчик, показывающий сколько мы уже приняли байт в "data_buffer"
	uint8_t numBuff;

	// Ответ о состоянии принятого сообщения
	uint8_t answer;

	// Здесь будут храниться данные после парсинга
	RecData dataBufferPars;

	uint8_t *ptrBuff;

#ifdef LCD_ENABLE
	uint8_t updateLine;
	// Чистим экран
	TM_HD44780_Clear();
	updateLine = 0;
#endif

	for(;;)
	{
		// Крутимся в цикле пока не закончится обновление
		// Обнуляем счётчик полученных данных
		allDataSize = 0;
		// Сообщаем, что мы готовы к получению первого блока данных
		answer = 's';
		HAL_UART_Transmit(&huart2, (uint8_t*)&answer, 1, 0xFFFF);
		while(1)
		{
			// Крутимся в цикле пока не встретим символ окончания сообщения
			// или пока не будет переполнения буфера данных "data_buffer",
			// или пока не перестанут приходить данные

			// Присваеваем указателю адрес начала буфера
			ptrBuff = dataBuffer;
			// Обнуляем счётчик байтов в буфере "data_buffer"
			numBuff = 0;
			// По умолчанию ответное сообщение означает успешную передачу
			answer = 'g';
			// Принимаем данные по байту и забрасываем в наш буфер
			while(numBuff < SIZE_BUFF)
			{
				// Приём
				HAL_UART_Receive(&huart2, ptrBuff, 1, 0xFFFF);
				// Увеличиваем счётчик принятых байтов
				numBuff++;
				// Если встретили символ окончания сообщения, то выходим из цикла
				if(*ptrBuff == '>')
					break;
				// Двигаем указатель на следующий элемент массива
				ptrBuff++;
			}
			// Если переданный пакет байтов состоит лишь из <0>, то это означает конец
			// передачи. Выходим из цикла приёма пакетов
			if((*ptrBuff == '>')&&(*(ptrBuff-1) == '0')&&(*(ptrBuff-2) == '<'))
			{
				break;
			}
			// Парсим полученные данные
			dataBufferPars = parsing((char*)dataBuffer, numBuff);

#ifdef LCD_ENABLE
			// Полоска закрузки на LCD
			// Если полоска достигла окончания строки дисплея, то...
			if(!((updateLine + 1) % 17))
			{
				// Чистим экран
				TM_HD44780_Clear();
				// Обнуляем счётчик полоски
				updateLine = 0;
				// Выводим сообщение об обновлении, так как оно стёрлось
				TM_HD44780_Puts(0, 0, (uint8_t*)UPDATE_TEXT, LEN_UPDATE_TEXT);
			}
			// Выводим элемент полоски загрузки на LCD
			TM_HD44780_PutCustom(updateLine, 1, 255);
			// Увеличиваем счётчик полоски загрузки
			updateLine++;
#endif

			// Если не возникло ошибок при парсинге, то сохраняем
			// полученные данные в памяти МК
			if(dataBufferPars.command != 6)
			{
				// Сохраняем данные в памяти
				answer = SU_FLASH_Save_User_Data(dataBufferPars, numBuff);
			}
			else
			{
				// Если возникли проблемы при записи, то переменной,
				// в которой хранится ответ приложению, присваеваем
				// символ ошибки
				answer = 'f';
			}
			// Отправляем результат работы
			HAL_UART_Transmit(&huart2, (uint8_t*)&answer, 1, 0xFFFF);
		}
		osDelay(100);
		vTaskDelete(NULL);
	}
	/* USER CODE END StartUSARTTask */
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
