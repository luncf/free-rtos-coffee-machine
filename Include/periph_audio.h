#ifndef __PERIPH_AUDIO_H__
#define __PERIPH_AUDIO_H__

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx.h"

#define GRIND_PITCH 64
#define VOLUME 5
#define MAX_NOTE_FNS 3

/* Pins to codec */
#define I2S3_WS_PIN GPIO_Pin_4     /* Port A */
#define I2S3_MCLK_PIN GPIO_Pin_7   /* Port C */
#define I2S3_SCLK_PIN GPIO_Pin_10  /* Port C */
#define I2S3_SD_PIN GPIO_Pin_12    /* Port C */
#define CODEC_RESET_PIN GPIO_Pin_4 /* Port D */
#define I2C_SCL_PIN GPIO_Pin_6     /* Port B */
#define I2C_SDA_PIN GPIO_Pin_9     /* Port B */

#define CODEC_I2C I2C1
#define CODEC_I2S SPI3

#define CORE_I2C_ADDRESS 0x33
#define CODEC_I2C_ADDRESS 0x94

#define CODEC_MAPBYTE_INC 0x80

/* Register map bytes for CS42L22 */
#define CODEC_MAP_PWR_CTRL1 0x02
#define CODEC_MAP_PWR_CTRL2 0x04
#define CODEC_MAP_CLK_CTRL 0x05
#define CODEC_MAP_IF_CTRL1 0x06
#define CODEC_MAP_PLAYBACK_CTRL1 0x0D

typedef uint16_t (*note_fn_t)(double*);

typedef struct audio {
	note_fn_t notes[MAX_NOTE_FNS];
	double duration;
	TaskHandle_t handle;
} audio_t;


audio_t* init_audio(void);
void audio_play_steam(audio_t *audio);
void audio_play_grind(audio_t *audio);
void audio_play_tune(audio_t *audio);

void vPlayAudio(void *pvParameters);

#endif
