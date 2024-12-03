/*
 * TurbiditySensor.h
 *
 *  Created on: Nov 22, 2024
 *      Author: Nnaemeka Nnadede & Temitope Onafalujo
 */

#ifndef INC_USER_L3_TURBIDITYSENSOR_H_ // Include guard to prevent multiple inclusions
#define INC_USER_L3_TURBIDITYSENSOR_H_

#include "FreeRTOS.h" // Include FreeRTOS for RTOS functionalities
#include "timers.h"   // Include FreeRTOS timer functionalities

/**
 * @brief Callback function executed by a FreeRTOS timer to simulate
 *        turbidity sensor readings.
 *
 * This function simulates turbidity sensor data, including noise for
 * realistic variability, and periodically updates the values. The
 * simulated data is transmitted via the communication datalink.
 *
 * @param xTimer: The FreeRTOS timer handle triggering this function.
 */
void RunTurbiditySensor(TimerHandle_t xTimer);

#endif /* INC_USER_L3_TURBIDITYSENSOR_H_ */
