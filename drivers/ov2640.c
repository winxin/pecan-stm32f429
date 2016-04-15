/**
  * This is the OV2640 driver
  *
  */

#include "ch.h"
#include "hal.h"
#include "ov2640.h"
#include "pi2c.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_dcmi.h"
#include "board.h"
#include "debug.h"
#include <string.h>

#define OV2640_BUFFER_SIZE		150*1024
#define OV2640_I2C_ADR			0x30
#define DCMI_BASE_ADR			((uint32_t)0x50050000)
#define DCMI_REG_DR_OFFSET		0x28
#define DCMI_REG_DR_ADDRESS		(DCMI_BASE_ADR | DCMI_REG_DR_OFFSET)

uint8_t ov2640_ram_buffer[OV2640_BUFFER_SIZE];
bool ov2640_samplingFinished;

// I2C camera configuration
static const uint8_t OV2640_CONFIG[] =
{
//     0xFF, 0x01,
//     0xFF, 0x01,
//     0x12, 0x80,

0xff, 0x00,
0xff, 0x00,
0x2c, 0xff,
0x2e, 0xdf,
0xff, 0x01,
0x3c, 0x32,
     //{0x11, 0x30,  // Divide clock by 49
0x11, 0x02,   // Divide clock by 3
0x09, 0x02,
0x04, 0x28,
0x13, 0xe5,
0x14, 0x48,
0x2c, 0x0c,
0x33, 0x78,
0x3a, 0x33,
0x3b, 0xfB,
0x3e, 0x00,
0x43, 0x11,
0x16, 0x10,
0x39, 0x92,
0x35, 0xda,
0x22, 0x1a,
0x37, 0xc3,
0x23, 0x00,
0x34, 0xc0,
0x36, 0x1a,
0x06, 0x88,
0x07, 0xc0,
0x0d, 0x87,
0x0e, 0x41,
0x4c, 0x00,
0x48, 0x00,
0x5B, 0x00,
0x42, 0x03,
0x4a, 0x81,
0x21, 0x99,
0x24, 0x40,
0x25, 0x38,
0x26, 0x82,
0x5c, 0x00,
0x63, 0x00,
0x61, 0x70,
0x62, 0x80,
0x7c, 0x05,
0x20, 0x80,
0x28, 0x30,
0x6c, 0x00,
0x6d, 0x80,
0x6e, 0x00,
0x70, 0x02,
0x71, 0x94,
0x73, 0xc1,
0x12, 0x40,
0x17, 0x11,
0x18, 0x43,
0x19, 0x00,
0x1a, 0x4b,
0x32, 0x09,
0x37, 0xc0,
0x4f, 0x60,
0x50, 0xa8,
0x6d, 0x00,
0x3d, 0x38,
0x46, 0x3f,
0x4f, 0x60,
0x0c, 0x3c,
0xff, 0x00,
0xe5, 0x7f,
0xf9, 0xc0,
0x41, 0x24,
0xe0, 0x14,
0x76, 0xff,
0x33, 0xa0,
0x42, 0x20,
0x43, 0x18,
0x4c, 0x00,
0x87, 0xd5,
0x88, 0x3f,
0xd7, 0x03,
0xd9, 0x10,
0xd3, 0x82,
0xc8, 0x08,
0xc9, 0x80,
0x7c, 0x00,
0x7d, 0x00,
0x7c, 0x03,
0x7d, 0x48,
0x7d, 0x48,
0x7c, 0x08,
0x7d, 0x20,
0x7d, 0x10,
0x7d, 0x0e,
0x90, 0x00,
0x91, 0x0e,
0x91, 0x1a,
0x91, 0x31,
0x91, 0x5a,
0x91, 0x69,
0x91, 0x75,
0x91, 0x7e,
0x91, 0x88,
0x91, 0x8f,
0x91, 0x96,
0x91, 0xa3,
0x91, 0xaf,
0x91, 0xc4,
0x91, 0xd7,
0x91, 0xe8,
0x91, 0x20,
0x92, 0x00,
0x93, 0x06,
0x93, 0xe3,
0x93, 0x05,
0x93, 0x05,
0x93, 0x00,
0x93, 0x04,
0x93, 0x00,
0x93, 0x00,
0x93, 0x00,
0x93, 0x00,
0x93, 0x00,
0x93, 0x00,
0x93, 0x00,
0x96, 0x00,
0x97, 0x08,
0x97, 0x19,
0x97, 0x02,
0x97, 0x0c,
0x97, 0x24,
0x97, 0x30,
0x97, 0x28,
0x97, 0x26,
0x97, 0x02,
0x97, 0x98,
0x97, 0x80,
0x97, 0x00,
0x97, 0x00,
0xc3, 0xed,
0xa4, 0x00,
0xa8, 0x00,
0xc5, 0x11,
0xc6, 0x51,
0xbf, 0x80,
0xc7, 0x10,
0xb6, 0x66,
0xb8, 0xA5,
0xb7, 0x64,
0xb9, 0x7C,
0xb3, 0xaf,
0xb4, 0x97,
0xb5, 0xFF,
0xb0, 0xC5,
0xb1, 0x94,
0xb2, 0x0f,
0xc4, 0x5c,
0xc0, 0x64,
0xc1, 0x4B,
0x8c, 0x00,
0x86, 0x3D,
0x50, 0x00,
0x51, 0xC8,
0x52, 0x96,
0x53, 0x00,
0x54, 0x00,
0x55, 0x00,
0x5a, 0xC8,
0x5b, 0x96,
0x5c, 0x00,
0xd3, 0x7f,
0xc3, 0xed,
0x7f, 0x00,
0xda, 0x00,
0xe5, 0x1f,
0xe1, 0x67,
0xe0, 0x00,
0xdd, 0x7f,
0x05, 0x00,
0x12, 0x40,
0xd3, 0x7f,
0xc0, 0x16,
0xC1, 0x12,
0x8c, 0x00,
0x86, 0x3d,
0x50, 0x00,
0x51, 0x2C,
0x52, 0x24,
0x53, 0x00,
0x54, 0x00,
0x55, 0x00,
0x5A, 0x2c,
0x5b, 0x24,
0x5c, 0x00,
0xff, 0xff,

0xFF, 0x00,
0x05, 0x00,
0xDA, 0x10,
0xD7, 0x03,
0xDF, 0x00,
0x33, 0x80,
0x3C, 0x40,
0xe1, 0x77,
0x00, 0x00,
0xff, 0xff,

0xFF, 0x01,
0x15, 0x00,

0xe0, 0x14,
0xe1, 0x77,
0xe5, 0x1f,
0xd7, 0x03,
0xda, 0x10,
0xe0, 0x00,
0xFF, 0x01,
0x04, 0x08,
0xff, 0xff,






	0xff, 0x01,
	0x11, 0x01,
	0x12, 0x00, // Bit[6:4]: Resolution selection//0x02为彩条
	0x17, 0x11, // HREFST[10:3]
	0x18, 0x75, // HREFEND[10:3]
	0x32, 0x36, // Bit[5:3]: HREFEND[2:0]; Bit[2:0]: HREFST[2:0]
	0x19, 0x01, // VSTRT[9:2]
	0x1a, 0x97, // VEND[9:2]
	0x03, 0x0f, // Bit[3:2]: VEND[1:0]; Bit[1:0]: VSTRT[1:0]
	0x37, 0x40,
	0x4f, 0xbb,
	0x50, 0x9c,
	0x5a, 0x57,
	0x6d, 0x80,
	0x3d, 0x34,
	0x39, 0x02,
	0x35, 0x88,
	0x22, 0x0a,
	0x37, 0x40,
	0x34, 0xa0,
	0x06, 0x02,
	0x0d, 0xb7,
	0x0e, 0x01,
	
	0xff, 0x00,        	                              
	0xe0, 0x04,                                   
	0xc0, 0xc8,                                   
	0xc1, 0x96,                                   
	0x86, 0x3d,                                   
	0x50, 0x00,                                   
	0x51, 0x90,                                   
	0x52, 0x2c,                                   
	0x53, 0x00,                                   
	0x54, 0x00,                                   
	0x55, 0x88,                                   
	0x57, 0x00,                                   
	0x5a, 0x90,                                   
	0x5b, 0x2C,                                   
	0x5c, 0x05,              //bit2->1;bit[1:0]->1
	0xd3, 0x02,                                   
	0xe0, 0x00,                                   
                      
  	0xff, 0xff,


0xe0, 0x14,
0xe1, 0x77,
0xe5, 0x1f,
0xd7, 0x03,
0xda, 0x10,
0xe0, 0x00,
0xFF, 0x01,
0x04, 0x08,
0xff, 0xff
};


/**
  * Captures an image from the camera.
  */
bool OV2640_Snapshot2RAM(void)
{
	palClearPad(PORT(LED_YELLOW), PIN(LED_YELLOW)); // Yellow LED shows when image is captured

	// DCMI init
	OV2640_InitDCMI();

	// Receive JPEG data
	ov2640_samplingFinished = false;
	systime_t timeout = chVTGetSystemTimeX() + MS2ST(3000); // Timeout 1sec
	while(!ov2640_samplingFinished && chVTGetSystemTimeX() < timeout)
		chThdSleepMilliseconds(1);

	palSetPad(PORT(LED_YELLOW), PIN(LED_YELLOW));

	return true;
}

uint32_t OV2640_getBuffer(uint8_t** buffer) {
	*buffer = ov2640_ram_buffer;

	// Detect size
	uint32_t size = sizeof(ov2640_ram_buffer);
	while(!ov2640_ram_buffer[size-1])
		size--;
	
	TRACE_DEBUG("CAM > Image size: %d bytes", size);
	TRACE_BIN(ov2640_ram_buffer, 32*8);

	return size;
}

void OV2640_dma_avail(uint32_t flags)
{
	(void)flags;
	ov2640_samplingFinished = true;
	dmaStreamDisable(STM32_DMA2_STREAM1);
}

/**
  * Initializes DMA
  */
void OV2640_InitDMA(void)
{
	const stm32_dma_stream_t *stream = STM32_DMA2_STREAM1;
	dmaStreamAllocate(stream, 2, (stm32_dmaisr_t)OV2640_dma_avail, NULL);
	dmaStreamSetPeripheral(stream, ((uint32_t*)DCMI_REG_DR_ADDRESS));
	dmaStreamSetMemory0(stream, (uint32_t)ov2640_ram_buffer);
	dmaStreamSetTransactionSize(stream, OV2640_BUFFER_SIZE);
	dmaStreamSetMode(stream, STM32_DMA_CR_CHSEL(1) | STM32_DMA_CR_DIR_P2M |
							 STM32_DMA_CR_MINC | STM32_DMA_CR_PSIZE_WORD |
							 STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_MBURST_SINGLE |
							 STM32_DMA_CR_PBURST_SINGLE | STM32_DMA_CR_TCIE);
	dmaStreamSetFIFO(stream, STM32_DMA_FCR_FTH_HALF | STM32_DMA_FCR_DMDIS);
	dmaStreamEnable(stream);
}

void OV2640_DeinitDMA(void)
{
    const stm32_dma_stream_t *stream = STM32_DMA2_STREAM1;
    dmaStreamDisable(stream);
}

/**
  * Initializes DCMI
  */
void OV2640_InitDCMI(void)
{
	// Clock enable
	RCC->AHB2ENR |= RCC_AHB2Periph_DCMI;

	// Configure DCMI
	DCMI->CR = DCMI_CaptureMode_SnapShot | DCMI_CR_JPEG | DCMI_CR_PCKPOL;

	// DCMI enable
	DCMI->CR |= (uint32_t)DCMI_CR_ENABLE;
	// Capture enable
	DCMI->CR |= (uint32_t)DCMI_CR_CAPTURE;
}

void OV2640_DeinitDCMI(void)
{
	// Clock enable
	RCC->AHB2ENR &= ~RCC_AHB2Periph_DCMI;
}

/**
  * Initializes GPIO (for DCMI)
  * The high speed clock supports communication by I2C (XCLK = 16MHz)
  */
void OV2640_InitGPIO(void)
{
	palSetPadMode(PORT(CAM_HREF), PIN(CAM_HREF), PAL_MODE_ALTERNATE(13));	// HSYNC -> PA4
	palSetPadMode(PORT(CAM_PCLK), PIN(CAM_PCLK), PAL_MODE_ALTERNATE(13));	// PCLK  -> PA6
	palSetPadMode(PORT(CAM_VSYNC), PIN(CAM_VSYNC), PAL_MODE_ALTERNATE(13));	// VSYNC -> PB7
	palSetPadMode(PORT(CAM_D2), PIN(CAM_D2), PAL_MODE_ALTERNATE(13));		// D0    -> PC6
	palSetPadMode(PORT(CAM_D3), PIN(CAM_D3), PAL_MODE_ALTERNATE(13));		// D1    -> PC7
	palSetPadMode(PORT(CAM_D4), PIN(CAM_D4), PAL_MODE_ALTERNATE(13));		// D2    -> PC8
	palSetPadMode(PORT(CAM_D5), PIN(CAM_D5), PAL_MODE_ALTERNATE(13));		// D3    -> PC9
	palSetPadMode(PORT(CAM_D6), PIN(CAM_D6), PAL_MODE_ALTERNATE(13));		// D4    -> PE4
	palSetPadMode(PORT(CAM_D7), PIN(CAM_D7), PAL_MODE_ALTERNATE(13));		// D5    -> PB6
	palSetPadMode(PORT(CAM_D8), PIN(CAM_D8), PAL_MODE_ALTERNATE(13));		// D6    -> PE5
	palSetPadMode(PORT(CAM_D9), PIN(CAM_D9), PAL_MODE_ALTERNATE(13));		// D7    -> PE6

	palSetPadMode(PORT(CAM_OFF), PIN(CAM_OFF), PAL_MODE_OUTPUT_PUSHPULL);	// CAM_OFF

	i2cCamInit();
}

/**
  * Setup a CLOCKOUT pin (PA8) which is needed by the camera (XCLK pin)
  */
void OV2640_InitClockout(void)
{
	palSetPadMode(PORT(CAM_XCLK), PIN(CAM_XCLK), PAL_MODE_ALTERNATE(0));	// PA8    -> XCLK
}

void OV2640_TransmitConfig(void)
{
	for(uint32_t i=0; i<sizeof(OV2640_CONFIG); i+=2) {
		TRACE_DEBUG("0x%02x 0x%02x", OV2640_CONFIG[i], OV2640_CONFIG[i+1]);
		i2cCamSend(OV2640_I2C_ADR, &OV2640_CONFIG[i], 2, NULL, 0, MS2ST(100));
		chThdSleepMilliseconds(100);
	}
}

void OV2640_init(void) {
	// Clearing buffer
	uint32_t i;
	for(i=0; i<OV2640_BUFFER_SIZE; i++)
		ov2640_ram_buffer[i] = 0;

	TRACE_INFO("CAM  > Init pins");
	OV2640_InitClockout();
	OV2640_InitGPIO();

	// Power on OV2640
	TRACE_INFO("CAM  > Switch on");
	palClearPad(PORT(CAM_OFF), PIN(CAM_OFF));	// Switch on camera

	chThdSleepMilliseconds(1000);

	// Send settings to OV2640
	TRACE_INFO("CAM  > Transmit config to camera");
	OV2640_TransmitConfig();

	// DCMI DMA
	TRACE_INFO("CAM  > Init DMA");
	OV2640_InitDMA();

	// DCMI Init
	TRACE_INFO("CAM  > Init DCMI");
	OV2640_InitDCMI();
}

void OV2640_deinit(void) {
	// DCMI Init
	TRACE_INFO("CAM  > Deinit DCMI");
	OV2640_DeinitDCMI();

	// DCMI DMA
	TRACE_INFO("CAM  > Deinit DMA");
	OV2640_DeinitDMA();

	// Power off OV2640
	//TRACE_INFO("CAM  > Switch off");
	//palSetPad(PORT(CAM_OFF), PIN(CAM_OFF));	// Switch off camera
}
