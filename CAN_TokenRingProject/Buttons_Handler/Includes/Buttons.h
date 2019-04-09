/*
 * Buttons.h
 *
 *  Created on: Mar 18, 2019
 *      Author: AVE-LAP-049
 */

#ifndef BUTTONS_HANDLER_INCLUDES_BUTTONS_H_
#define BUTTONS_HANDLER_INCLUDES_BUTTONS_H_


extern volatile uint8_t ButtonPressedFlag;

enum ButtonState_e
{
    NotPressed = 0U,
    Pressed    = 1U,
};

void Button1Task(void *pvParameters);
void Button2Task(void *pvParameters);
void Init_Buttons(void);


#endif /* BUTTONS_HANDLER_INCLUDES_BUTTONS_H_ */
