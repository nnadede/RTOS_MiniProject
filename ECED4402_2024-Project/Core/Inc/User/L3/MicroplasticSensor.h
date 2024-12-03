/*
 * MicroplasticSensor.h
 *
 *  Created on: Nov 22, 2024
 *      Author: Nnaemeka Nnadede & Temitope Onafalujo
 */

#ifndef INC_USER_L3_MICROPLASTICSENSOR_H_ // Include guard to prevent multiple inclusions
#define INC_USER_L3_MICROPLASTICSENSOR_H_

#include "FreeRTOS.h" // Include FreeRTOS for RTOS functionalities
#include "timers.h"   // Include FreeRTOS timer functionalities

/**
 * @brief Callback function executed by a FreeRTOS timer to simulate
 *        microplastic concentration sensor readings.
 *
 * This function simulates microplastic concentration values with added
 * noise, updates the values periodically, and sends them via the
 * communication datalink layer.
 *
 * @param xTimer: The FreeRTOS timer handle triggering this function.
 */
void RunMicroplasticSensor(TimerHandle_t xTimer);

#endif /* INC_USER_L3_MICROPLASTICSENSOR_H_ */
