/*
 * Buttons.c
 *
 *  Created on: Mar 18, 2019
 *      Author: AVE-LAP-049
 */
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "gpio.h"
#include "OS/Includes/FreeRTOS.h"
#include "OS/Includes/queue.h"
#include "../OS/Includes/event_groups.h"
#include "Includes/Buttons.h"
#include "../CAN_Handler/Includes/CAN_cfg.h"




volatile uint8_t ButtonPressedFlag = 0 ;

extern EventGroupHandle_t xEventGroup;
extern volatile uint8_t Source;
/* Initialize Buttons */
void Init_Buttons(void)
{

    HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE+GPIO_O_CR) |= 0x01;

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);  /* Set pin 4 in PortF to be Input */

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);  /* Set pin 0 in PortF to be Input */

    GPIOPadConfigSet(GPIO_PORTF_BASE ,GPIO_PIN_4,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD_WPU); /* Set Pin as Pull-up */

    GPIOPadConfigSet(GPIO_PORTF_BASE ,GPIO_PIN_0,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD_WPU); /* Set Pin as Pull-up */



}


void Button1Task(void *pvParameters)
{
    uint32_t Button1Send = 0 ;
    volatile static uint8_t Button1State = 1 ;
    volatile static uint8_t Button1StatePrev = 1 ;
    static uint8_t ReverseMode;
    while(1)
    {
        Button1State = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0); /* Check if Button Pressed */

        if( (Button1State == 0) && (Button1StatePrev == 1) )
        {
            xEventGroupSetBits(xEventGroup,BIT_0); /* Set Flag 1 in EventGroup*/
            Button1Send++;

            if( BIT_2 == (xEventGroupGetBits(xEventGroup)&(BIT_2)) )
            {

                if(TARGET == MAX_NODES) /* Check if TARGET is Last Node */
                {
                    if(ReverseMode)
                    {
                        Source = MAX_NODES - 1;
                    }
                    else
                    {
                        Source = NODE_1 ;
                    }

                }
                else if(TARGET == NODE_1) /* Check if TARGET is First Node */
                {
                    if(ReverseMode)
                    {
                        Source = MAX_NODES;
                    }
                    else
                    {
                        Source = TARGET + 1 ;
                    }

                }
                else /* TARGET is Node between last and first */
                {
                    if(ReverseMode)
                    {
                        Source = TARGET-1;
                    }
                    else
                    {
                        Source = TARGET+1;
                    }

                }
                ReverseMode ^=1;
            }
        }
        Button1StatePrev = Button1State;

        vTaskDelay(1); /* RTOS delay for 50ms */

    }
}
