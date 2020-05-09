#include "bh1750.h"
#include "main.h"

I2C_HandleTypeDef hi2c1;

void BH1750_Init()
{
	uint8_t power_on=0x01;
	uint8_t mode=0x10;

	HAL_I2C_Master_Transmit(&hi2c1, 0x23<<1, &power_on, 1, 0xffff);
	HAL_I2C_Master_Transmit(&hi2c1, 0x23<<1, &mode, 1, 0xffff);
}

float BH1750_Receive(void)
{
	uint8_t Data[2];
	float rslt;

	HAL_I2C_Master_Receive(&hi2c1, 0x23<<1, Data, 2, 0xffff);
	rslt=((Data[0]<<8)| Data[1])/1.2;

	return rslt;
}

