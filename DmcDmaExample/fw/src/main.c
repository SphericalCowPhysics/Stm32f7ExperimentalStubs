
#include "hwConfig.h"
#include "regs.h"
#include "commands.h"
#include "usbd_cdc_if.h"
#include "main.h"

int VCP_read(void *pBuffer, int size);
int UsbVcp_write(const void *pBuffer, int size);
void BlinkLed();

uint16_t FirmWareVersion = 1;
//version 1, VID/PID = 0x0483/0x5740, defined in usbd_desc.c



/*****************************************************DacDma*/
DAC_HandleTypeDef    DacHandle;
static DAC_ChannelConfTypeDef sConfig;
static void MPU_Config(void);
static void DAC_Ch1_TriangleConfig(void);
static void DAC_Ch1_EscalatorConfig(void);
static void TIM6_Config(void);
void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);



int main(void)
{
	/*****************************************************DacDma*/
	MPU_Config();
	CPU_CACHE_Enable();
	
	
	HAL_Init();
	SystemClock_Config();
	
	
	/*****************************************************DacDma*/
	/*##-1- Configure the DAC peripheral #######################################*/
	DacHandle.Instance = DACx;
	/*##-2- Configure the TIM peripheral #######################################*/
	TIM6_Config();
	
	
	MX_GPIO_Init();
	InitUsb();
	/*Initialize all peripherals*/
	
	InitRegs();

	char byte;
	
	
	
	
	
	
	
	
	
	/*****************************************************DacDma*/
	
	/*No signal on scope.Attempting to use DMA with TIM6 as a trigger.*/
	//DAC_Ch1_TriangleConfig();			
	
	/*Attempt to write directly to DAC regs. Regs remain at 0, nothing on scope.*/
	HAL_DAC_Start(&DacHandle, DACx_CHANNEL);
	DACx->DHR12L1 = 0xF0;
	DACx->DOR1 = 0xF0;
	DACx->CR = 1;
	
	uint16_t count = 0;
	while (1)
	{
	
		count++;
		BlinkLed(); //cannot do this while using SPI because of pin conflict
		if (rxLen > 0) {
			Parse();
		}
	}
}

uint32_t lastBlink = 0;
void BlinkLed()
{
	static uint32_t blinkTime = 500; //on/off time
	uint32_t sysTick = HAL_GetTick();
  
	if ((sysTick - lastBlink) > blinkTime) {
		HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_1);
		lastBlink = sysTick;
	}
}
void Error_Handler()
{
	asm("bkpt 255");
}






/*****************************************************DacDma*/

/**
  * @brief  DAC Channel1 Triangle Configuration
  * @param  None
  * @retval None
  */
void DAC_Ch1_TriangleConfig(void)
{
	/*##-1- Initialize the DAC peripheral ######################################*/
	if (HAL_DAC_Init(&DacHandle) != HAL_OK)
	{
		/* DAC initialization Error */
		Error_Handler();
	}

	/*##-2- DAC channel2 Configuration #########################################*/
	sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

	if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DACx_CHANNEL) != HAL_OK)
	{
		/* Channel configuration Error */
		Error_Handler();
	}

	/*##-3- DAC channel2 Triangle Wave generation configuration ################*/
	if (HAL_DACEx_TriangleWaveGenerate(&DacHandle, DACx_CHANNEL, DAC_TRIANGLEAMPLITUDE_1023) != HAL_OK)
	{
		/* Triangle wave generation Error */
		Error_Handler();
	}

	/*##-4- Enable DAC Channel1 ################################################*/
	if (HAL_DAC_Start(&DacHandle, DACx_CHANNEL) != HAL_OK)
	{
		/* Start Error */
		Error_Handler();
	}

	/*##-5- Set DAC channel1 DHR12RD register ################################################*/
	if (HAL_DAC_SetValue(&DacHandle, DACx_CHANNEL, DAC_ALIGN_12B_R, 0x100) != HAL_OK)
	{
		/* Setting value Error */
		Error_Handler();
	}
}
/**
  * @brief  TIM6 Configuration
  * @note   TIM6 configuration is based on APB1 frequency
  * @note   TIM6 Update event occurs each TIM6CLK/256
  * @param  None
  * @retval None
  */
void TIM6_Config(void)
{
	static TIM_HandleTypeDef  htim;
	TIM_MasterConfigTypeDef sMasterConfig;

	/*##-1- Configure the TIM peripheral #######################################*/
	/* Time base configuration */
	htim.Instance = TIM6;

	htim.Init.Period            = 0x7FF;
	htim.Init.Prescaler         = 0;
	htim.Init.ClockDivision     = 0;
	htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim.Init.RepetitionCounter = 0;
	htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim);

	/* TIM6 TRGO selection */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig);

	/*##-2- Enable TIM peripheral counter ######################################*/
	HAL_TIM_Base_Start(&htim);
}
/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
	SCB_EnableDCache();
}
/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	/* Disable the MPU */
	HAL_MPU_Disable();

	/* Configure the MPU as Strongly ordered for not defined regions */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = 0x00;
	MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
	MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x87;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Enable the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/** @defgroup HAL_MSP_Private_Functions
  * @{
  */

/**
  * @brief DAC MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hdac: DAC handle pointer
  * @retval None
  */
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
	GPIO_InitTypeDef          GPIO_InitStruct;
	static DMA_HandleTypeDef  hdma_dac1;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO clock ****************************************/
	DACx_CHANNEL_GPIO_CLK_ENABLE();
	/* DAC Periph clock enable */
	DACx_CLK_ENABLE();
	/* DMA1 clock enable */
	DMAx_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* DAC Channel1 GPIO pin configuration */
	GPIO_InitStruct.Pin = DACx_CHANNEL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DACx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);

	/*##-3- Configure the DMA ##########################################*/
	/* Set the parameters to be configured for DACx_DMA_STREAM */
	hdma_dac1.Instance = DACx_DMA_INSTANCE;

	hdma_dac1.Init.Channel  = DACx_DMA_CHANNEL;

	hdma_dac1.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_dac1.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_dac1.Init.MemInc = DMA_MINC_ENABLE;
	hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_dac1.Init.Mode = DMA_CIRCULAR;
	hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;

	HAL_DMA_Init(&hdma_dac1);

	/* Associate the initialized DMA handle to the DAC handle */
	__HAL_LINKDMA(hdac, DMA_Handle1, hdma_dac1);

	/*##-4- Configure the NVIC for DMA #########################################*/
	/* Enable the DMA1_Stream5 IRQ Channel */
	HAL_NVIC_SetPriority(DACx_DMA_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(DACx_DMA_IRQn);

}

/**
  * @brief  DeInitializes the DAC MSP.
  * @param  hdac: pointer to a DAC_HandleTypeDef structure that contains
  *         the configuration information for the specified DAC.
  * @retval None
  */
void HAL_DAC_MspDeInit(DAC_HandleTypeDef *hdac)
{
	/*##-1- Reset peripherals ##################################################*/
	DACx_FORCE_RESET();
	DACx_RELEASE_RESET();

	/*##-2- Disable peripherals and GPIO Clocks ################################*/
	/* De-initialize the DAC Channel1 GPIO pin */
	HAL_GPIO_DeInit(DACx_CHANNEL_GPIO_PORT, DACx_CHANNEL_PIN);

	/*##-3- Disable the DMA Stream ############################################*/
	/* De-Initialize the DMA Stream associate to DAC_Channel1 */
	HAL_DMA_DeInit(hdac->DMA_Handle1);

	/*##-4- Disable the NVIC for DMA ###########################################*/
	HAL_NVIC_DisableIRQ(DACx_DMA_IRQn);
}

/**
  * @brief TIM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	/* TIM6 Periph clock enable */
	__HAL_RCC_TIM6_CLK_ENABLE();
}

/**
  * @brief TIM MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO to their default state
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{

	/*##-1- Reset peripherals ##################################################*/
	__HAL_RCC_TIM6_FORCE_RESET();
	__HAL_RCC_TIM6_RELEASE_RESET();
}