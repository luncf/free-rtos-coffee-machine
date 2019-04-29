#ifndef __PERIPH_BUTTON_H__
#define __PERIPH_BUTTON_H__

#include <stdbool.h>

#include "discoveryf4utils.h"

#include "FreeRTOS.h"
#include "task.h"

#define REGISTER_THRESHOLD 50 /* ticks */
#define READ_THRESHOLD 100    /* ticks */
#define RESET_THRESHOLD 500   /* ticks */

#define LONG_PRESS_THRESHOLD 3000 /* ticks */

typedef enum click { SINGLE_CLICK, DOUBLE_CLICK, LONG_PRESS, NO_CLICK } click_t;

typedef struct button {
  bool is_reading;
  TickType_t previous_down_tick;
  TickType_t previous_up_tick;
  int32_t click_count;
} button_t;

button_t *init_button(void);
void button_debounce(button_t *button, TickType_t current_tick);
click_t button_read_click(button_t *button, TickType_t current_tick);
TickType_t button_tick_since_last_action(button_t *button, TickType_t current_tick);

void vButtonRegisterClicks(void *pvParameters);

#endif
