#include <stdbool.h>
#include <stdlib.h>

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#include "coffee_machine.h"
#include "free_rtos_constants.h"

coffee_machine_t *coffee_machine;

void die() {
  led_blink(coffee_machine->led_set, coffee_machine->led_set->red, 333);
  while (1) {
  }
}

void EXTI0_IRQHandler() {
  if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
    /* Record button click */
    button_debounce(coffee_machine->button, coffee_machine->current_tick);

    /* Set flag to reusme the suspended coffee machine task */
    coffee_machine->resume_button_control = true;

    EXTI_ClearITPendingBit(EXTI_Line0);
  }
}

void vControlCoffeeMachine(void *pvParameters) {
  click_t click = NO_CLICK;
  job_t *job;
  int send;

  while (true) {
    /* Read button click and decide on action */
    click = button_read_click(coffee_machine->button, coffee_machine->current_tick);

    if (coffee_machine->state == IDLE) {
      if (click == SINGLE_CLICK) {
        /* Enter coffee selection menu */
        coffee_machine->state = SELECT_COFFEE;
				coffee_machine->led_set->active = coffee_machine->led_set->green;
        led_cycle(coffee_machine->led_set);
      }
    } else if (coffee_machine->state == SELECT_COFFEE) {
      if (click == SINGLE_CLICK) {
        /* Switch to next coffee */
        led_cycle(coffee_machine->led_set);
      } else if (click == DOUBLE_CLICK) {
        /* Select coffee */
        job = malloc(sizeof(job_t));
        send = 1;

        switch (coffee_machine->led_set->active->colour) {
        case LED_RED:
          *job = ESPRESSO;
          break;
        case LED_BLUE:
          *job = LATTE;
          break;
        case LED_ORANGE:
          *job = MOCHA;
          break;
        case LED_GREEN:
          /* this should never happen */
          send = 0;
          break;
        }
        if (send) {
          if (xQueueSendToBack(coffee_machine->job_queue, (void *)&job, 0) != pdPASS) {
            /* the queue was full, but we can't really do anything about it. */
            /* we could perhaps retry, but right now there's no point. */
          }
        }

        /* back to the start */
        coffee_machine->state = IDLE;
      } else if (click == LONG_PRESS) {
        /* enter programming mode */
        coffee_machine->state = PROGRAM_TIMING;
        coffee_machine->programming_count = 0;
        led_blink(coffee_machine->led_set, coffee_machine->led_set->active, 50);
      }
    } else if (coffee_machine->state == PROGRAM_TIMING) {
      if (click == SINGLE_CLICK) {
        /* increment */
        coffee_machine->programming_count++;
      } else if (click == DOUBLE_CLICK) {
        /* cancel */
        coffee_machine->state = IDLE;
      } else if (click == LONG_PRESS) {
        /* save */
        if (coffee_machine->programming_count) {
          switch (coffee_machine->led_set->active->colour) {
          case LED_RED:
            coffee_machine->timing_espresso = coffee_machine->programming_count;
            break;
          case LED_BLUE:
            coffee_machine->timing_milk = coffee_machine->programming_count;
            break;
          case LED_ORANGE:
            coffee_machine->timing_chocolate_milk = coffee_machine->programming_count;
            break;
          case LED_GREEN:
            /* this should never happen */
            break;
          }
        }
        /* back to start */
        coffee_machine->state = IDLE;
      }
    }

    /* Suspend or delay task */
    if (click == NO_CLICK &&
        button_tick_since_last_action(coffee_machine->button,
                                      coffee_machine->current_tick) > IDLE_TIMEOUT) {
      /* Timed out since there were no more clicks */
      coffee_machine->state = IDLE;
      vTaskSuspend(coffee_machine->handle);
    } else {
      vTaskDelay(100 / portTICK_RATE_MS);
    }
  }
}

void vJobSchedule(void *pvParameters) {
  while (true) {
    /* Update coffee machine tick time */
    coffee_machine->current_tick = xTaskGetTickCount();

    /* Resume button control task */
    if (coffee_machine->resume_button_control) {
      coffee_machine->resume_button_control = false;
      vTaskResume(coffee_machine->handle);
    }

    /* Idle Task: blink an led. */
    /* if something is pouring, blink that one. otherwise, green. */
    if (coffee_machine->state == IDLE) {
      if (coffee_machine->is_pouring) {
        /* blink once per second */
        led_blink(coffee_machine->led_set, coffee_machine->led_set->pouring, 1000);
      } else {
        led_blink(coffee_machine->led_set, coffee_machine->led_set->green,
                  DEFAULT_LED_BLINK_RATE);
      }
    }
    vTaskDelay(10 / portTICK_RATE_MS);
  }
}

void vRunJobQueue(void *pvParameters) {
  int espresso, milk, chocolate_milk;
  job_t *job = malloc(sizeof(job_t));
  servo_set(SERVO_IDLE);

  while (1) {
    if (xQueueReceive(coffee_machine->job_queue, (void *)&job, RECEIVE_TIMEOUT) != pdPASS) {
      /* if the queue is empty, there's nothing to do */
      continue;
    }

    /* save the timing in case they get changed mid-job */
    espresso = (1000 * coffee_machine->timing_espresso) / portTICK_RATE_MS;
    milk = (1000 * coffee_machine->timing_milk) / portTICK_RATE_MS;
    chocolate_milk = (1000 * coffee_machine->timing_chocolate_milk) / portTICK_RATE_MS;

    switch (*job) {
    case ESPRESSO:
			if (coffee_machine->state == IDLE) {
				led_blink(coffee_machine->led_set, coffee_machine->led_set->red, 1000);
			}
      coffee_machine->led_set->pouring = coffee_machine->led_set->red;
      coffee_machine->is_pouring = 1;
			vTaskDelay(1000 / portTICK_RATE_MS);
      servo_set(SERVO_ESPRESSO);
      audio_play_grind(coffee_machine->audio);
      vTaskDelay(espresso);
      break;
    case LATTE:
			if (coffee_machine->state == IDLE) {
				led_blink(coffee_machine->led_set, coffee_machine->led_set->blue, 1000);
			}
      coffee_machine->led_set->pouring = coffee_machine->led_set->blue;
      coffee_machine->is_pouring = 1;
			vTaskDelay(1000 / portTICK_RATE_MS);
      servo_set(SERVO_ESPRESSO);
      audio_play_grind(coffee_machine->audio);
      vTaskDelay(espresso);
      servo_set(SERVO_MILK);
      audio_play_steam(coffee_machine->audio);
      vTaskDelay(milk);
      break;
    case MOCHA:
			if (coffee_machine->state == IDLE) {
				led_blink(coffee_machine->led_set, coffee_machine->led_set->orange, 1000);
			}
      coffee_machine->led_set->pouring = coffee_machine->led_set->orange;
      coffee_machine->is_pouring = 1;
			vTaskDelay(1000 / portTICK_RATE_MS);
      servo_set(SERVO_ESPRESSO);
      audio_play_grind(coffee_machine->audio);
      vTaskDelay(espresso);
      servo_set(SERVO_CHOCOLATE_MILK);
      audio_play_steam(coffee_machine->audio);
      vTaskDelay(chocolate_milk);
      break;
    }

    audio_play_tune(coffee_machine->audio);
    coffee_machine->is_pouring = 0;
    free(job);

    /* move back to idle position, wait a little bit for the servo to move */
    servo_set(SERVO_IDLE);
    vTaskDelay(2000 / portTICK_RATE_MS);
  }
}

int main(void) {
  /*!< Most systems default to the wanted configuration, with the noticeable
          exception of the STM32 driver library. If you are using an STM32 with
          the STM32 driver library then ensure all the priority bits are assigned
          to be preempt priority bits by calling
          NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); before the RTOS is started.
  */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  /* Set up servos */
  InitServos();
  InitPWMTimer4();
  SetupPWM();

  /* Create coffee machine */
  coffee_machine = malloc(sizeof(coffee_machine_t));

  /* Initialize coffee machine hardware */
  coffee_machine->led_set = init_leds();
  coffee_machine->button = init_button();
  coffee_machine->audio = init_audio();

  /* Setup coffee machine */
  coffee_machine->state = IDLE;
  coffee_machine->current_tick = 0;
  coffee_machine->handle = (TaskHandle_t) "Control coffee machine handle";
  coffee_machine->resume_button_control = false;
  coffee_machine->timing_espresso = 5;
  coffee_machine->timing_milk = 5;
  coffee_machine->timing_chocolate_milk = 6;
  coffee_machine->is_pouring = 0;

  coffee_machine->job_queue = xQueueCreate(32, sizeof(job_t *));
  if (coffee_machine->job_queue == NULL) {
    die();
  }

  xTaskCreate(vControlCoffeeMachine, (const char *)"Control coffee machine", MIN_STACK_SIZE,
              NULL, configMAX_PRIORITIES - 2, &coffee_machine->handle);
  xTaskCreate(vJobSchedule, (const char *)"Run job scheduler", MIN_STACK_SIZE, NULL,
              configMAX_PRIORITIES - 3, NULL);
  xTaskCreate(vRunJobQueue, (const char *)"Run job queue", MIN_STACK_SIZE, NULL,
              configMAX_PRIORITIES - 4, NULL);
	
  vTaskSuspend(coffee_machine->handle);
	
  vTaskStartScheduler();
}
