#ifndef __PERIPH_SERVO_H__
#define __PERIPH_SERVO_H__

#include "FreeRTOS.h"
#include "discoveryf4utils.h"
#include "queue.h"
#include "task.h"

#include "periph_leds.h"

#define SERVO_IDLE 100
#define SERVO_ESPRESSO 1000
#define SERVO_MILK 1400
#define SERVO_CHOCOLATE_MILK 1800

void servo_set(int);
void InitServos(void);
void InitPWMTimer4(void);
void SetupPWM(void);
void vServoTask(void *);

#endif
