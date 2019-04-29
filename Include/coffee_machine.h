#ifndef __COFFEE_MACHINE_H__
#define __COFFEE_MACHINE_H__

#include "discoveryf4utils.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "queue.h"
#include "task.h"

#include "periph_audio.h"
#include "periph_button.h"
#include "periph_leds.h"
#include "periph_servo.h"

#define RECEIVE_TIMEOUT (100 / portTICK_RATE_MS)
#define IDLE_TIMEOUT 7000

typedef enum machine_state {
  IDLE,
  SELECT_COFFEE,
  PROGRAM_TIMING,
} machine_state_t;

typedef enum job {
  ESPRESSO,
  LATTE,
  MOCHA,
} job_t;

typedef struct coffee_machine {
  led_set_t *led_set;
  button_t *button;
	audio_t* audio;

  machine_state_t state;
  TickType_t current_tick;

  TaskHandle_t handle;
  bool resume_button_control;
  QueueHandle_t job_queue;
  int timing_espresso;
  int timing_milk;
  int timing_chocolate_milk;
  bool is_pouring;

  int programming_count;
} coffee_machine_t;

#endif
