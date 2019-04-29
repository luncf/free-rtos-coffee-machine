# FreeRTOS Coffee Machine

## Introduction

The goal of project is to implement an automatic coffee machine using FreeRTOS on the STM32F407-Disco1 board. This coffee machine should be capable of executing multiple tasks based on FreeRTOS. All four LEDs, button, audio output and PWM (for servo motor control) of the STM32F407-Disco1 will be utilized.

### Features

1. Programmable coffee brewing
2. Customizable brewing timing


## System Requirements

1. STM32F407-Disco1
2. Servo motor
3. MDK-ARM (5.26) with MDK Legacy Support
4. STLinkV2 Driver


## Functionalities

### Brewing a coffee

The user of this coffee machine has the ability to program add coffee brewing jobs to the FreeRTOS queue with the user button.

First, single click to cycle through the coffee selection menu. Each LED represents a different coffee type, which is shown in the table below. Double click on any coffee type to add the coffee brewing task to the queue.

LED representations in the coffee selection menu:

|  LED   | Coffee type |
| :----: | :---------: |
|  red   |  espresso   |
|  blue  |    latte    |
| orange |    mocha    |

If no actions are captured after entering the coffee selection menu within the idle timeout, the coffee machine will return to the appropriate state.

While the coffee brews, the peripheral devices will perform several actions. First, the LED that represents the coffee will blink. Before the machine dispenses an ingredient, a sound will play. Next, the servo motor will change positions based on the current dispensing ingredient. Lastly, once all ingredients are dispensed, meaning the coffee is brewed, a short tune will play to indicate that the job is completed.

### Customizing brewing times

To customize the brewing times of the ingredients, enter the coffee selection menu with a single click. Once in the selection menu, each LED represents a different ingredient used in the coffee types. The table below shows the corresponding LED and ingredient list.

LED representations in the coffee selection menu:

|  LED   |   Ingredient   |
|:------:|:--------------:|
|  red   |    espresso    |
|  blue  |      milk      |
| orange | chocolate milk |

If no actions are captured after entering the coffee selection menu within the idle timeout, the coffee machine will return to the appropriate state.

Once in the selection menu, long press to enter timing programming mode. This mode will accept three types of inputs. A single click will increment the brewing time by 1 second. Double clicks will exit the programming mode. Lastly, a long press will confirm and set the current timing of an ingredient to the new timing.



