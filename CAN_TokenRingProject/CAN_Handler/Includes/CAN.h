/*
 * CAN.h
 *
 *  Created on: Apr 3, 2019
 *      Author: AVE-LAP-049
 */

#ifndef CAN_HANDLER_INCLUDES_CAN_H_
#define CAN_HANDLER_INCLUDES_CAN_H_

extern void CAN_ReceiverTask(void *pvParameters);
extern void CAN_SenderTask(void *pvParameters);
extern void CAN_Init(void);
#endif /* CAN_HANDLER_INCLUDES_CAN_H_ */
