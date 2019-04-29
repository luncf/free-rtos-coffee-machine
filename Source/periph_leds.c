#include <stdlib.h>

#include "free_rtos_constants.h"
#include "periph_leds.h"

led_t *led_blue;
led_t *led_red;
led_t *led_green;
led_t *led_orange;

led_t *init_led(Led_TypeDef colour) {
  led_t *led = malloc(sizeof(led_t));
  led->colour = colour;
  led->state = LED_OFF;
  led->blink_rate = DEFAULT_LED_BLINK_RATE;

  /* Initialize LED */
  STM_EVAL_LEDInit(colour);

  return led;
}

led_set_t *init_leds(void) {
  led_set_t *led_set = malloc(sizeof(led_set_t));
  led_set->blue = init_led(LED_BLUE);
  led_set->red = init_led(LED_RED);
  led_set->green = init_led(LED_GREEN);
  led_set->orange = init_led(LED_ORANGE);
  led_set->active = led_set->green;
  led_set->state_changed = false;
  led_set->handle = (TaskHandle_t) "Toggle LED handle";

  xTaskCreate(vLedToggle, (const char *)"Toggle LED", MIN_STACK_SIZE, led_set,
              configMAX_PRIORITIES - 3, &led_set->handle);

  return led_set;
}

void led_change_active_state(led_set_t *led_set, led_t *active, led_state_t state) {
  /* Change state of active LED, turn off all other LEDs */
  active->state = state;
  if (active != led_set->blue) {
    led_set->blue->state = LED_OFF;
  }
  if (active != led_set->red) {
    led_set->red->state = LED_OFF;
  }
  if (active != led_set->green) {
    led_set->green->state = LED_OFF;
  }
  if (active != led_set->orange) {
    led_set->orange->state = LED_OFF;
  }

  /* Set active LED, turn ON changed flag */
  led_set->active = active;
  led_set->state_changed = true;

  /* Resume task to toggle LEDs */
  vTaskResume(led_set->handle);
}

void led_blink(led_set_t *led_set, led_t *led, uint32_t rate) {
  /* Set LED state to blink */
  if (led->state != LED_BLINK) {
    led_change_active_state(led_set, led, LED_BLINK);
  }
  led->blink_rate = rate;
}

void led_cycle(led_set_t *led_set) {
	if (led_set->active == led_set->blue) {
		/* Switch from Lattle/Milk to Mocha/Chocolate Milk */
		led_change_active_state(led_set, led_set->orange, LED_ON);
	} else if (led_set->active == led_set->red) {
		/* Switch from Espresso to Latte/Milk */
		led_change_active_state(led_set, led_set->blue, LED_ON);
	} else {
		/* Switch from Mocha/Chocolate Milk/IDLE to Espresso */
		led_change_active_state(led_set, led_set->red, LED_ON);
	}
}

void led_toggle(led_t *led, bool changed) {
  /* Send command to change LED state */
  if (changed || led->state == LED_BLINK) {
    switch (led->state) {
    case LED_ON:
      STM_EVAL_LEDOn(led->colour);
      break;
    case LED_BLINK:
      STM_EVAL_LEDToggle(led->colour);
      break;
    case LED_OFF:
      STM_EVAL_LEDOff(led->colour);
      break;
    }
  }
}

void led_off(led_set_t *led_set) {
  STM_EVAL_LEDOff(led_set->active->colour);
}

void vLedToggle(void *pvParameters) {
  led_set_t *led_set = (led_set_t *)pvParameters;

  while (true) {
    /* Switch LED light mode based on led_t state */
    led_toggle(led_set->red, led_set->state_changed);
    led_toggle(led_set->blue, led_set->state_changed);
    led_toggle(led_set->orange, led_set->state_changed);
    led_toggle(led_set->green, led_set->state_changed);

    led_set->state_changed = false;

    switch (led_set->active->state) {
    case LED_BLINK:
      vTaskDelay(led_set->active->blink_rate / portTICK_RATE_MS);
      break;
    default:
      vTaskSuspend(led_set->handle);
      break;
    }
  }
}
