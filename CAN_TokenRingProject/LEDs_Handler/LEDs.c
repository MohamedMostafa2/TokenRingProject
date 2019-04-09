/*
 * LEDs.c
 *
 *  Created on: Mar 18, 2019
 *      Author: AVE-LAP-049
 */

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "gpio.h"
#include "sysctl.h"
#include "Includes/LEDs.h"
#include "../OS/Includes/FreeRTOS.h"
#include "../OS/Includes/event_groups.h"
#include "../Buttons_Handler/Includes/Buttons.h"

extern EventGroupHandle_t xEventGroup;


/* Initialize Leds */
void Init_LEDS(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); /* Enable the GPIOF port that is used for the on-board Leds . */

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)); /* Wait until Peripheral is Stable to use */

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2); /* Set Pin 1,2 in PortF to Output */



}

void LEDTask1(void *pvParameters)
{


    while(1)
    {

        if(BIT_2 == ( xEventGroupGetBits(xEventGroup) & BIT_2 ))
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
        }
        else
        {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
        }

        vTaskDelay(1); /* RTOS delay for 1ms */


    }
}

