/*
 * DOLevelSensor.h
 *
 *  Created on: Nov 22, 2024
 *      Author: Nnaemeka Nnadede & Temitope Onafalujo
 */

#ifndef INC_USER_L3_DOLEVELSENSOR_H_ // Include guard to prevent multiple inclusions
#define INC_USER_L3_DOLEVELSENSOR_H_

#include "FreeRTOS.h" // Include FreeRTOS for RTOS functionalities
#include "timers.h"   // Include FreeRTOS timer functionalities

/**
 * @brief Callback function executed by a FreeRTOS timer to simulate
 *        dissolved oxygen (DO) level sensor readings.
 *
 * This function simulates a dissolved oxygen level value with added noise,
 * updates the value periodically, and sends it via the communication
 * datalink layer.
 *
 * @param xTimer: The FreeRTOS timer handle triggering this function.
 */
void RunDOLevelSensor(TimerHandle_t xTimer);

#endif /* INC_USER_L3_DOLEVELSENSOR_H_ */
