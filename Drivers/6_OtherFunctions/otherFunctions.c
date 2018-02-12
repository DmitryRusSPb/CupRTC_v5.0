/*
 * otherFunctions.c
 *
 *  Created on: 21 дек. 2017 г.
 *      Author: root
 */
#include "otherFunctions.h"


// Размер(в байтах) всех полученных данных
uint16_t allDataSize;
// Число страниц флеш памяти
uint8_t pageNum = 0;
// Сообщает нам о том, была ли уже очищена первая страница
uint8_t firstErase = 0;

uint8_t textRecCount;

GPIO_PinState AntiContactBounce(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	uint8_t numOfPolls;
	numOfPolls = 1;
	while(!HAL_GPIO_ReadPin(GPIOx, GPIO_Pin))
	{
		numOfPolls++;
		osDelay(1);
		if(numOfPolls >= NUMBER_OF_POLLS)
			return GPIO_PIN_SET;
	}
	return GPIO_PIN_RESET;
}

/*----------------------------------------------------------------------------*/
/*--------------------------------FLASH---------------------------------------*/
/*----------------------------------------------------------------------------*/
// Функция записи в флеш-память
AnswerStatus SU_FLASH_Save_User_Data(RecData parsData, uint8_t numReceivedBytes)
{
	// Размер принятого сообщения
	uint8_t messageSize = 0;

	uint32_t convert = 0;

	uint8_t multipleMessageSize = 0;

	switch (parsData.command)
	{
	// Занятое место
	case STATE:
		// Место будет занимать в памяти 4 байта
		messageSize = 4;
		allDataSize += messageSize;
		break;
		// Тексты 1 - 16
	case TEXT1:
	case TEXT2:
	case TEXT3:
	case TEXT4:
	case TEXT5:
	case TEXT6:
	case TEXT7:
	case TEXT8:
	case TEXT9:
	case TEXT10:
	case TEXT11:
	case TEXT12:
	case TEXT13:
	case TEXT14:
	case TEXT15:
	case TEXT16:
		// Вычитаем 15 из общего числа полученных байт, так как это не сами данные
		// ,а лишь информация о роде данных.
		messageSize = numReceivedBytes - 15;
		multipleMessageSize = messageSize;
		allDataSize += MAX_TEXT_SIZE;
		// Прибавляем 4, так как нужно учесть запись о размере текста
		// , которая занимает в памяти 4 байта
		allDataSize += 4;
		break;
		// Количество фреймов (блоки по 20 байт)
	case BLOCK:
		// Количество фреймов будет занимать в памяти 4 байта
		messageSize = 4;
		allDataSize += messageSize;
		break;
		// Аудиоданные
	case SPEEX:
		// Аудиоданные будут занимать в памяти 20 байт
		messageSize = 20;
		allDataSize += messageSize;
		break;
	default:
		//		while(1);
		break;
	}
	osDelay(1);

	if(HAL_FLASH_Unlock() != HAL_OK)  // Открываем флеш для чистки
	{
		return ERROR9;
	}

	// Если это первая страница и мы её ещё не чистили, то чистим её
	if(!firstErase)
	{
		firstErase = 1;
		FLASH_PageErase(START_FLASH_PAGE);
		// Очищаем бит PER в регистре FLASH->CR,
		// который мы записали туда в ходе чистки страницы памяти
		CLEAR_BIT(FLASH->CR, (FLASH_CR_PER));
	}
	else
		// Если размер уже принятых данных больше размера страницы флеша,
		// то чистим следующую страницу
		if(allDataSize >= MEMORY_PAGE_SIZE)
		{
			// Вычисляем, сколько байт у нас не поместилось на предыдущей странице
			allDataSize -= MEMORY_PAGE_SIZE;
			// Увеличиваем счётчик страниц флеша на 1
			pageNum++;
			FLASH_PageErase(START_FLASH_PAGE + 0x00000800*pageNum);
			// Очищаем бит PER в регистре FLASH->CR,
			// который мы записали туда в ходе чистки страницы памяти
			CLEAR_BIT(FLASH->CR, (FLASH_CR_PER));
		}
	switch (parsData.command)
	{
	case STATE:
		convert |= parsData.data[0];
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, START_FLASH_PAGE, convert);
		break;
	case TEXT1:
	case TEXT2:
	case TEXT3:
	case TEXT4:
	case TEXT5:
	case TEXT6:
	case TEXT7:
	case TEXT8:
	case TEXT9:
	case TEXT10:
	case TEXT11:
	case TEXT12:
	case TEXT13:
	case TEXT14:
	case TEXT15:
	case TEXT16:
		WriteToFlash((uint32_t*)(SIZE_TEXT1_address + (parsData.command - 1) * (SIZE_TEXT2_address-SIZE_TEXT1_address)),
				(uint32_t)SIZE_RECORD_OF_TEXT_SIZE, (uint8_t*)&multipleMessageSize);
		WriteToFlash((uint32_t*)(TEXT1_address + (parsData.command - 1) * (TEXT2_address - TEXT1_address)),
				(uint32_t)multipleMessageSize, parsData.data);
		textRecCount++;
		break;
	case BLOCK:
		convert |= textRecCount;
		//	Делаем запись о том, сколько строчек текста мы приняли
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, TEXT_REC_COUNT, convert);

		convert |= parsData.blockNumber;
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, BLOCK_address, convert);
		break;
	case SPEEX:
		WriteToFlash((uint32_t*)(SPEEX_address + parsData.blockNumber*20), 20, parsData.data);
		break;
	default:
		//		HAL_UART_Transmit(&huart2, (uint8_t*) "Memory write error", (uint16_t)strlen("Memory write error"), 0);
		break;
	}
	HAL_FLASH_Lock();
	return Flash_OK;
}

void WriteToFlash(uint32_t* writeAddress, uint32_t sizeData, uint8_t *data)
{
	// Записываем по 4 байта данные из буфера
	for(uint8_t i = 0; i < sizeData; i+=4)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, writeAddress, *(uint32_t*)(data)) == HAL_OK)
		{
			writeAddress ++;
			data += 4;
		}
		// Закрываем флеш в случае неудачной записи
		else
		{
			HAL_FLASH_Lock();
		}
	}
}
