#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "can.h"
#include "gpio.h"
#include "interrupt.h"
#include "pin_map.h"
#include "sysctl.h"
#include "uart.h"
#include "utils/uartstdio.h"
#include "../OS/Includes/FreeRTOS.h"
#include "../OS/Includes/task.h"
#include "../OS/Includes/event_groups.h"
#include "Includes/CAN_cfg.h"

/* ------------------------------------------
                Imported
   ------------------------------------------*/
extern EventGroupHandle_t xEventGroup;


/* --------------------------------------------------
                Constant
   --------------------------------------------------*/
#define ENTER_NORMAL_MODE_MESSAGE               0U
#define ENTER_NORMAL_MODE_MESSAGE_ID            0U
#define NORMAL_MODE_FLAG                        BIT_1
#define TOKEN_FLAG                              BIT_2
/* ------------------------------------------
                Global Variables
   ------------------------------------------*/
/* A counter that keeps track of the number of times the TX interrupt has
 * occurred, which should match the number of TX messages that were sent*/
volatile uint32_t g_ui32MsgCount = 0;

/* A flag for the interrupt handler to indicate that a message was received. */
volatile bool g_bRXFlag = 0;

/*A flag to indicate that some transmission error occurred*/
volatile bool g_bErrFlag = 0;

volatile uint8_t Index = TARGET;
volatile uint8_t Source;
volatile uint8_t NextID;
volatile uint8_t Node[4] = {0,NODE_1,NODE_2,NODE_3};

static uint8_t GetID(void);

//*****************************************************************************
//
// This function sets up UART0 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void InitConsole(void)
{
    /* Enable GPIO port A which is used for UART0 pins.*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    /* Configure the pin muxing for UART0 functions on port A0 and A1.
       This step is not necessary if your part does not support pin muxing. */
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    /* Enable UART0 so that we can configure the clock. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    /* Use the internal 16MHz oscillator as the UART clock source. */
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    /* Select the alternate (UART) function for these pins.*/
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Initialize the UART for console I/O. */
    UARTStdioConfig(0, 115200, 16000000);
}

/*--------------------------------------------------------------------------
 This function is the interrupt handler for the CAN peripheral.  It checks
 for the cause of the interrupt, and maintains a count of all messages that
 have been transmitted.
--------------------------------------------------------------------------*/
void CANIntHandler(void)
{
    uint32_t ui32Status;


    /* Read the CAN interrupt status to find the cause of the interrupt */
    ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    /* If the cause is a controller status interrupt, then get the status */
    if(ui32Status == CAN_INT_INTID_STATUS)
    {
        /*--------------------------------------------------------------
         Read the controller status.  This will return a field of status
         error bits that can indicate various errors.  Error processing
         is not done in this example for simplicity.  Refer to the
         API documentation for details about the error status bits.
         The act of reading this status will clear the interrupt.  If the
         CAN peripheral is not connected to a CAN bus with other CAN devices
         present, then errors will occur and will be indicated in the
         controller status.
        ------------------------------------------------------------------*/
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);


        /* Set a flag to indicate some errors may have occurred.*/
        g_bErrFlag = 1;
    }

    /* Check if the cause is message object 1, which what we are using for
       sending messages.*/
    else if(ui32Status == 1)
    {

        /* Getting to this point means that the TX interrupt occurred on
           message object 1, and the message TX is complete.  Clear the
           message object interrupt. */
        CANIntClear(CAN0_BASE, 1);


        /* Since the message was sent, clear any error flags. */
        g_bErrFlag = 0;
    }
    else if(ui32Status == 2)
    {
        /* Getting to this point means that the TX interrupt occurred on
           message object 1, and the message TX is complete.  Clear the
           message object interrupt. */
        CANIntClear(CAN0_BASE, 2);

        /* Set flag to indicate received message is pending. */
        g_bRXFlag = 1;
        /* Since the message was sent, clear any error flags. */
        g_bErrFlag = 0;
    }


    /* Otherwise, something unexpected caused the interrupt.  This should
       never happen. */
    else
    {

        /* Spurious interrupt handling can go here. */

    }
}

void CAN_Init(void)
{

     /* Set up the serial console to use for displaying messages.  This is
     just for this example program and is not needed for CAN operation. */
    InitConsole();

    /*For this example CAN0 is used with RX and TX pins on port B4 and B5.
      The actual port and pins used may be different on your part, consult
      the data sheet for more information.
      GPIO port B needs to be enabled so these pins can be used.*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);


    /* Configure the GPIO pin muxing to select CAN0 functions for these pins.
       This step selects which alternate function is available for these pins.
       This is necessary if your part supports GPIO pin function muxing.
       Consult the data sheet to see which functions are allocated per pin. */
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);


    /* Enable the alternate function on the GPIO pins.  The above step selects
       which alternate function is available.  This step actually enables the
       alternate function instead of GPIO for these pins. */
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);


    /* The GPIO port and pins have been set up for CAN.  The CAN peripheral
       must be enabled. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);


    /* Initialize the CAN controller */
    CANInit(CAN0_BASE);


    /* Set up the bit rate for the CAN bus.  This function sets up the CAN
       bus timing for a nominal configuration.  You can achieve more control
       over the CAN bus timing by using the function CANBitTimingSet() instead
       of this one, if needed.
       In this example, the CAN bus is set to 500 kHz.  In the function below,
       the call to SysCtlClockGet() or ui32SysClock is used to determine the
       clock rate that is used for clocking the CAN peripheral.  This can be
       replaced with a  fixed value if you know the value of the system clock,
       saving the extra function call.  For some parts, the CAN peripheral is
       clocked by a fixed 8 MHz regardless of the system clock in which case
       the call to SysCtlClockGet() or ui32SysClock should be replaced with
       8000000.  Consult the data sheet for more information about CAN
       peripheral clocking. */
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);


    /* Enable interrupts on the CAN peripheral.  This example uses static
       allocation of interrupt handlers which means the name of the handler
       is in the vector table of startup code.  If you want to use dynamic
       allocation of the vector table, then you must also call CANIntRegister()
       here. */
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    /* Enable the CAN interrupt on the processor (NVIC). */
    IntEnable(INT_CAN0);


    /* Enable the CAN for operation. */
    CANEnable(CAN0_BASE);

}

void CAN_SenderTask(void *pvParameters)
{
    tCANMsgObject sCANMessage;
    uint8_t ui32MsgData;
    uint8_t *pui8MsgData;
    pui8MsgData = &ui32MsgData;

#if (TARGET == NODE_1)
    static uint8_t RepeatNum = 0;
#endif

    while(1)
       {

        #if (TARGET == NODE_1)
            if(RepeatNum <1 )
            {
                ui32MsgData = ENTER_NORMAL_MODE_MESSAGE;
                sCANMessage.ui32MsgID = ENTER_NORMAL_MODE_MESSAGE_ID;
                sCANMessage.ui32MsgIDMask = 0;
                sCANMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
                sCANMessage.ui32MsgLen = 1;
                sCANMessage.pui8MsgData = pui8MsgData;

                CANMessageSet(CAN0_BASE, 1, &sCANMessage, MSG_OBJ_TYPE_TX);

                if(g_bErrFlag)
                {

                }
                else
                {
                    xEventGroupSetBits(xEventGroup,NORMAL_MODE_FLAG); /* Set NORMAL_MODE_FLAG in EventGroup*/
                    xEventGroupSetBits(xEventGroup,TOKEN_FLAG); /* Set TOKEN_FLAG in EventGroup*/
                }
                RepeatNum++;
            }
        #endif

        /* Check Normal Mode and Token */
        if((TOKEN_FLAG|NORMAL_MODE_FLAG) == ( xEventGroupGetBits(xEventGroup) & (TOKEN_FLAG|NORMAL_MODE_FLAG)))
        {
            /*  Wait 2 Second*/
            vTaskDelay(2000);

            ui32MsgData = TARGET;
            sCANMessage.ui32MsgID = GetID();
            sCANMessage.ui32MsgIDMask = 0;
            sCANMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
            sCANMessage.ui32MsgLen = sizeof(pui8MsgData);
            sCANMessage.pui8MsgData = pui8MsgData;


            /* Send the CAN message using object number 1 (not the same thing as
               CAN ID, which is also 1 in this example).  This function will cause
               the message to be transmitted right away. */
            CANMessageSet(CAN0_BASE, 1, &sCANMessage, MSG_OBJ_TYPE_TX);


            /* Check the error flag to see if errors occurred */
            if(g_bErrFlag)
            {
                UARTprintf(" error - cable connected?\n");

            }
            else
            {
                xEventGroupClearBits(xEventGroup,TOKEN_FLAG); /* Set TOKEN FLAG in EventGroup*/
            }

        }
        else
        {
            /* Do NoThing*/
        }
        vTaskDelay(50);
       }
}




void CAN_ReceiverTask(void *pvParameters)
{

    tCANMsgObject MessageReceived;
    uint8_t pui8MsgData;


    /* Initialize a message object to be used for receiving CAN messages with
       any CAN ID.  In order to receive any CAN ID, the ID and mask must both
       be set to 0, and the ID filter enabled. */
    MessageReceived.ui32MsgID = 0;
    MessageReceived.ui32MsgIDMask = 0;
    MessageReceived.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    MessageReceived.ui32MsgLen = 1;


    /* Now load the message object into the CAN peripheral.  Once loaded the
       CAN will receive any message on the bus, and an interrupt will occur.
       Use message object 1 for receiving messages (this is not the same as
       the CAN ID which can be any value in this example).*/
    CANMessageSet(CAN0_BASE, 2, &MessageReceived, MSG_OBJ_TYPE_RX);


     /* Enter loop to process received messages.  This loop just checks a flag
     that is set by the interrupt handler, and if set it reads out the
     message and displays the contents.  This is not a robust method for
     processing incoming CAN data and can only handle one messages at a time.
     If many messages are being received close together, then some messages
     may be dropped.  In a real application, some other method should be used
     for queuing received messages in a way to ensure they are not lost.  You
     can also make use of CAN FIFO mode which will allow messages to be
     buffered before they are processed.*/
    while(1)
    {
        /* If the flag is set, that means that the RX interrupt occurred and
        there is a message ready to be read from the CAN */
        if(g_bRXFlag)
        {

            /* Reuse the same message object that was used earlier to configure
            the CAN for receiving messages.  A buffer for storing the
            received data must also be provided, so set the buffer pointer
            within the message object. */
            MessageReceived.pui8MsgData = &pui8MsgData;


            /* Read the message from the CAN.  Message object number 1 is used
            (which is not the same thing as CAN ID).  The interrupt clearing
            flag is not set because this interrupt was already cleared in
            the interrupt handler. */
            CANMessageGet(CAN0_BASE, 2, &MessageReceived, 0);

        #if (TARGET != NODE_1)
            if( ENTER_NORMAL_MODE_MESSAGE == pui8MsgData )
            {
                  xEventGroupSetBits(xEventGroup,NORMAL_MODE_FLAG); /* Set Flag 1 in EventGroup*/
                  /* Clear the pending message flag so that the interrupt handler can
                     set it again when the next message arrives. */
                  g_bRXFlag = 0;

            }
            else
            {

            }
        #endif

            if(TARGET == MessageReceived.ui32MsgID)
            {
                xEventGroupSetBits(xEventGroup,TOKEN_FLAG); /* Set TOKEN FLAG in EventGroup*/
                Source = pui8MsgData;
                g_bRXFlag = 0;
            }
            else
            {

            }


        }

        vTaskDelay(10);
    }

}

static uint8_t GetID(void)
{
    uint8_t ID;
#if (TARGET != MAX_NODES)
    if(Node[TARGET+1] == Source )
#else
    if(Node[NODE_1] == Source)
#endif
    {
        /* Reverse Rotate */
        xEventGroupSetBits(xEventGroup,BIT_3); /* Set Flag 4 in EventGroup*/
        if( TARGET == 1 )
        {
            ID = MAX_NODES ; /*  Destination  = Last Node */
        }
        else
        {
            ID = TARGET - 1 ; /*  Destination  = Prev Node */
        }
    }
    else
    {
        /* Normal Rotate */
        xEventGroupClearBits(xEventGroup,BIT_3); /* Clear Flag 4 in EventGroup*/
        if(TARGET == MAX_NODES)
        {
            ID = NODE_1 ; /*  Destination  = First Node */
        }
        else
        {
            ID = TARGET + 1 ; /*  Destination  = Next Node */
        }
    }
    return ID;
}
