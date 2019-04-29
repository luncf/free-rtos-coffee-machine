#include <math.h>
#include <stdlib.h>

#include "free_rtos_constants.h"
#include "periph_audio.h"

void codec_ctrl_init(void);

audio_t* init_audio(void) {
  GPIO_InitTypeDef gpio;
  I2S_InitTypeDef i2s;
  I2C_InitTypeDef i2c;

	audio_t* audio = malloc(sizeof(audio_t));
	audio->notes[0] = NULL;
	audio->notes[1] = NULL;
	audio->notes[2] = NULL;
	audio->duration = 0;
	audio->handle = (TaskHandle_t) "Play audio handle";
	
  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
                             RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD,
                         ENABLE);

  /* Audio reset pin */
  gpio.GPIO_Pin = CODEC_RESET_PIN;
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &gpio);

  /* Keep codec off for now */
  GPIO_ResetBits(GPIOD, CODEC_RESET_PIN);

  /* I2C pin */
  gpio.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_OType = GPIO_OType_OD;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &gpio);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);

  /* Enable I2C clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

  /* Configure I2C port */
  I2C_DeInit(CODEC_I2C);
  i2c.I2C_ClockSpeed = 100000;
  i2c.I2C_Mode = I2C_Mode_I2C;
  i2c.I2C_OwnAddress1 = CORE_I2C_ADDRESS;
  i2c.I2C_Ack = I2C_Ack_Enable;
  i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  i2c.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_Init(CODEC_I2C, &i2c);
  I2C_Cmd(CODEC_I2C, ENABLE);

  /* Enable I2S clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  RCC_PLLI2SCmd(ENABLE);

  /* I2S pins */
  gpio.GPIO_Pin = I2S3_MCLK_PIN | I2S3_SCLK_PIN | I2S3_SD_PIN;
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &gpio);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

  gpio.GPIO_Pin = I2S3_WS_PIN;
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &gpio);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);

  /* Configure I2S port */
  SPI_I2S_DeInit(SPI3);
  i2s.I2S_AudioFreq = I2S_AudioFreq_48k;
  i2s.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
  i2s.I2S_DataFormat = I2S_DataFormat_16b;
  i2s.I2S_Mode = I2S_Mode_MasterTx;
  i2s.I2S_Standard = I2S_Standard_Phillips;
  i2s.I2S_CPOL = I2S_CPOL_Low;
  I2S_Init(SPI3, &i2s);
  I2S_Cmd(CODEC_I2S, ENABLE);
	
	/* Create audio playing task */
	xTaskCreate(vPlayAudio, (const char *)"Play audio", MIN_STACK_SIZE, audio,
						configMAX_PRIORITIES-1, &audio->handle);
	vTaskSuspend(audio->handle);
	
	return audio;
}

void send_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes) {
  uint8_t bytesSent = 0;

  /* Wait until no longer busy */
  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
    ;

  I2C_GenerateSTART(CODEC_I2C, ENABLE);
  /* Wait for generation of start condition */
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
    ;

  I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
  /* Wait for end of address transmission */
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ;

  while (bytesSent < numBytes) {
    I2C_SendData(CODEC_I2C, controlBytes[bytesSent]);
    bytesSent++;

    /* Wait for transmission of byte */
    while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
      ;
  }

  /* Wait until it's finished sending before creating STOP */
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BTF))
    ;
  I2C_GenerateSTOP(CODEC_I2C, ENABLE);
}

uint8_t read_codec_register(uint8_t mapbyte) {
  uint8_t receivedByte = 0;

  /* Just wait until no longer busy */
  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
    ;

  I2C_GenerateSTART(CODEC_I2C, ENABLE);
  /* Wait for generation of start condition */
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
    ;

  I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
  /* Wait for end of address transmission */
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ;

  /* Set the transmitter address */
  I2C_SendData(CODEC_I2C, mapbyte);
  /* Wait for transmission of byte */
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
    ;

  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

  /* Just wait until no longer busy */
  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
    ;

  I2C_AcknowledgeConfig(CODEC_I2C, DISABLE);

  I2C_GenerateSTART(CODEC_I2C, ENABLE);
  /* Wait for generation of start condition */
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
    ;

  I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Receiver);
  /* Wait for end of address transmission */
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    ;

  /* Wait until byte arrived */
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED))
    ;
  receivedByte = I2C_ReceiveData(CODEC_I2C);

  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

  return receivedByte;
}

void codec_ctrl_init(void) {
  uint32_t delaycount;
  uint8_t CodecCommandBuffer[5];

  uint8_t regValue = 0xFF;

  GPIO_SetBits(GPIOD, CODEC_RESET_PIN);
  delaycount = 1000000;
  while (delaycount > 0) {
    delaycount--;
  }
  /* Keep codec off */
  CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1;
  CodecCommandBuffer[1] = 0x01;
  send_codec_ctrl(CodecCommandBuffer, 2);

  /* Begin initialization sequence */
  CodecCommandBuffer[0] = 0x00;
  CodecCommandBuffer[1] = 0x99;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x47;
  CodecCommandBuffer[1] = 0x80;
  send_codec_ctrl(CodecCommandBuffer, 2);

  regValue = read_codec_register(0x32);

  CodecCommandBuffer[0] = 0x32;
  CodecCommandBuffer[1] = regValue | 0x80;
  send_codec_ctrl(CodecCommandBuffer, 2);

  regValue = read_codec_register(0x32);

  CodecCommandBuffer[0] = 0x32;
  CodecCommandBuffer[1] = regValue & (~0x80);
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x00;
  CodecCommandBuffer[1] = 0x00;
  send_codec_ctrl(CodecCommandBuffer, 2);
  /* End of initialization sequence */

  CodecCommandBuffer[0] = CODEC_MAP_PWR_CTRL2;
  CodecCommandBuffer[1] = 0xAF;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1;
  CodecCommandBuffer[1] = 0x70;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_CLK_CTRL;
  CodecCommandBuffer[1] = 0x81; /* auto detect clock */
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_IF_CTRL1;
  CodecCommandBuffer[1] = 0x07;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x0A;
  CodecCommandBuffer[1] = 0x00;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x27;
  CodecCommandBuffer[1] = 0x00;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x1A | CODEC_MAPBYTE_INC;
  CodecCommandBuffer[1] = 0x0A;
  CodecCommandBuffer[2] = 0x0A;
  send_codec_ctrl(CodecCommandBuffer, 3);

  CodecCommandBuffer[0] = 0x1F;
  CodecCommandBuffer[1] = 0x0F;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_PWR_CTRL1;
  CodecCommandBuffer[1] = 0x9E;
  send_codec_ctrl(CodecCommandBuffer, 2);
}

uint16_t note_steam(double *note) {
  return ((rand() % 4) + 1) * VOLUME;
}

uint16_t note_grind(double *note) {
  *note += (0.001);
  return (uint16_t)*note * VOLUME * 5;
}

uint16_t note_1(double *note) {
  *note += (0.005);
  return (uint16_t)*note * VOLUME * 5;
}

uint16_t note_2(double *note) {
  *note += (0.008);
  return (uint16_t)*note * VOLUME * 5;
}

uint16_t note_3(double *note) {
  *note += (0.010);
  return (uint16_t)*note * VOLUME * 5;
}

void audio_play(uint16_t (*tune)(double *), double duration) {
  double note = 0;
  int count = 0;

  /* Play audio for a set count */
  while (count < 48000 * duration) {
    if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE)) {
      SPI_I2S_SendData(CODEC_I2S, (*tune)(&note));
      count++;
    }
  }
}

void audio_play_steam(audio_t *audio) {
	audio->notes[0] = &note_steam;
	audio->notes[1] = NULL;
	audio->notes[2] = NULL;
	audio->duration = 2;
  vTaskResume(audio->handle);
}

void audio_play_grind(audio_t *audio) {
	audio->notes[0] = &note_grind;
	audio->notes[1] = NULL;
	audio->notes[2] = NULL;
	audio->duration = 2;
	vTaskResume(audio->handle);
}

void audio_play_tune(audio_t *audio) {
	audio->notes[0] = &note_1;
	audio->notes[1] = &note_2;
	audio->notes[2] = &note_3;
	audio->duration = 0.5;
  vTaskResume(audio->handle);
}

void vPlayAudio(void *pvParameters) {
	audio_t *audio = (audio_t *) pvParameters;
	int idx = 0;
	
	while (true) {
		/* Initialize CS43L22 codec*/
		codec_ctrl_init();
		
		/* Play all notes */
		for (idx = 0; idx < MAX_NOTE_FNS; idx++) {
			if (audio->notes[idx] != NULL) {
				audio_play(audio->notes[idx], audio->duration);
			}
		}
		
		/* Reset CS43L22 codec and suspend task */
		GPIO_ResetBits(GPIOD, CODEC_RESET_PIN);
		vTaskSuspend(audio->handle);
	}
}

