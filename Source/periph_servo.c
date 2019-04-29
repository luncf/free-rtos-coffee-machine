#include "periph_servo.h"

void servo_set(int x) {
  TIM4->CCR2 = x;
}

void InitServos(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  // Initalize PB7 (TIM4 Ch1)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // GPIO_High_Speed
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  // Assign Alternate Functions to pins
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_TIM4);
}

void InitPWMTimer4(void) {
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  // TIM4 Clock Enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  // Time Base Configuration for 50Hz
  TIM_TimeBaseStructure.TIM_Period = 20000 - 1;
  TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
  TIM_Cmd(TIM4, ENABLE);
}

void SetupPWM(void) {
  TIM_OCInitTypeDef TIM_OCInitStructure;
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // Set output capture as PWM mode
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // Enable output compare
  TIM_OCInitStructure.TIM_Pulse = 0;                            // Initial duty cycle at 0%
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // HIGH output compare active
  // Set the output capture channel 2 (upto 4)
  TIM_OC2Init(TIM4, &TIM_OCInitStructure); // Channel 2
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_ARRPreloadConfig(TIM4, ENABLE);
}
