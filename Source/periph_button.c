#include <stdlib.h>

#include "free_rtos_constants.h"
#include "periph_button.h"

button_t *init_button(void) {
  GPIO_InitTypeDef gpio;
  EXTI_InitTypeDef exti;
  NVIC_InitTypeDef nvic;

  button_t *button = malloc(sizeof(button_t));
  button->is_reading = false;
  button->previous_down_tick = 0;
  button->previous_up_tick = 0;
  button->click_count = 0;

  /* Initialize USER button */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  gpio.GPIO_Pin = GPIO_Pin_0;
  gpio.GPIO_Mode = GPIO_Mode_IN;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
  gpio.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init(GPIOA, &gpio);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
  exti.EXTI_Line = EXTI_Line0;
  exti.EXTI_LineCmd = ENABLE;
  exti.EXTI_Mode = EXTI_Mode_Interrupt;
  exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  EXTI_Init(&exti);

  nvic.NVIC_IRQChannel = EXTI0_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  return button;
}

void button_debounce(button_t *button, TickType_t current_tick) {
  /* Check to see if we should register change */
  if ((current_tick - button->previous_up_tick) > REGISTER_THRESHOLD) {
    uint32_t is_button_down = STM_EVAL_PBGetState(BUTTON_USER);

    if (is_button_down && !button->is_reading) {
      /* Button is pressed down */
      button->is_reading = true;
      button->previous_down_tick = current_tick;

    } else if (!is_button_down && button->is_reading) {
      /* Button is released */
      button->is_reading = false;
      button->previous_up_tick = current_tick;
      button->click_count++;
    }
  }
}

click_t button_read_click(button_t *button, TickType_t current_tick) {
  /* Long presses */
  if (!button->click_count && button->is_reading &&
      (current_tick - button->previous_down_tick) > LONG_PRESS_THRESHOLD) {
    button->click_count = -1;
    return LONG_PRESS;
  }

  /* Check to see if change is committed */
  if ((current_tick - button->previous_up_tick) > READ_THRESHOLD && !button->is_reading) {
    if (button->click_count == 1) {
      /* Single click */
      button->click_count = 0;
      return SINGLE_CLICK;
    } else if (button->click_count == 2) {
      /* Double click */
      button->click_count = 0;
      return DOUBLE_CLICK;
    } else {
      /* Multiple clicks, check to see if RESET_THRESHOLD is up */
      if (current_tick - button->previous_up_tick > RESET_THRESHOLD) {
        button->click_count = 0;
      }
    }
  }

  return NO_CLICK;
}

TickType_t button_tick_since_last_action(button_t *button, TickType_t current_tick) {
  TickType_t last_action_tick = (button->previous_down_tick > button->previous_up_tick)
                                    ? button->previous_down_tick
                                    : button->previous_up_tick;
  return (TickType_t)current_tick - last_action_tick;
}
