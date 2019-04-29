#ifndef __PERIPH_LEDS_H__
#define __PERIPH_LEDS_H__

#include <stdbool.h>

#include "discoveryf4utils.h"

#include "FreeRTOS.h"
#include "task.h"

#define DEFAULT_LED_BLINK_RATE 1000 /* ticks */
#define NUM_LEDS 4

typedef enum led_state { LED_ON, LED_BLINK, LED_OFF } led_state_t;

typedef struct led {
  Led_TypeDef colour;
  led_state_t state;
  uint32_t blink_rate;
} led_t;

typedef struct led_set {
  led_t *blue;
  led_t *red;
  led_t *green;
  led_t *orange;
  led_t *active;
  led_t *pouring;
  bool state_changed;
  TaskHandle_t handle;
} led_set_t;

led_set_t *init_leds(void);
void led_blink(led_set_t *led_set, led_t *led, uint32_t rate);
void led_cycle(led_set_t *led_set);
void led_off(led_set_t *);

void vLedToggle(void *pvParameters);

#endif
