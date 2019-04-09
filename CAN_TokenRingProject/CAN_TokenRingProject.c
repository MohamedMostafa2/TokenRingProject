//*****************************************************************************
//
// freertos_demo.c - Simple FreeRTOS example.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "gpio.h"
#include "pin_map.h"
#include "rom.h"
#include "sysctl.h"
#include "OS/Includes/FreeRTOS.h"
#include "OS/Includes/task.h"
#include "OS/Includes/queue.h"
#include "OS/Includes/semphr.h"
#include "OS/Includes/event_groups.h"
#include "Buttons_Handler/Includes/Buttons.h"
#include "LEDs_Handler/Includes/LEDs.h"
#include "CAN_Handler/Includes/CAN.h"



EventGroupHandle_t xEventGroup;
QueueHandle_t xQueue;


/*-----------------------------------------------------------------------

 Initializes The Tasks

------------------------------------------------------------------------- */
void TasksInit(void *pvParameters)
{
    volatile uint8_t counter=0;
    while(1)
    {


        Init_LEDS();    /* Initialize LEDs  */
        Init_Buttons(); /* Initialize Buttons */
        CAN_Init();    /* Initialize CAN */
        vTaskSuspend(NULL);
    }


}

int main(void)
{

    /* Set the clocking to run at 50 MHz from the PLL. */
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |SYSCTL_OSC_MAIN);

    xEventGroup = xEventGroupCreate();


    /* Create TasksInit */
    xTaskCreate(TasksInit, "Task_Init", 200, NULL, tskIDLE_PRIORITY + 9, NULL);

    /* Create Button Task */
    xTaskCreate(Button1Task, "Button_1", 200, NULL, tskIDLE_PRIORITY + 3, NULL);

    /* Create LED Task */
    xTaskCreate(LEDTask1, "LED_1", 200, NULL, tskIDLE_PRIORITY + 2, NULL);

    /* Create CAN Receiver Task */
    xTaskCreate(CAN_ReceiverTask, "ReceiverTask", 200, NULL, tskIDLE_PRIORITY + 5, NULL);

    /* Create CAN Sender Task */
    xTaskCreate(CAN_SenderTask, "SenderTask", 200, NULL, tskIDLE_PRIORITY + 7, NULL);


    /* Start the scheduler.  This should not return. */
    vTaskStartScheduler();

    /* In case the scheduler returns for some reason,loop forever.*/


    while(1)
    {

    }
}
