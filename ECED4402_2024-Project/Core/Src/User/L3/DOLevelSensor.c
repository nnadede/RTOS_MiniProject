/*
 * DOLevelSensorController.c
 *
 *  Created on: Nov. 22, 2024
 *      Author: Nnaemeka Nnadede & Temitope Onafalujo
 */

#include <stdbool.h> // Required for boolean data types

#include "User/L2/Comm_Datalink.h"   // Communication layer header file
#include "User/L3/DOLevelSensor.h"   // DO level sensor-specific functionalities

// Required FreeRTOS header files
#include "FreeRTOS.h"  // FreeRTOS main header
#include "Timers.h"    // Timer functions for periodic sensor execution

/******************************************************************************
 * RunDOLevelSensor
 * Software callback function executed periodically by a FreeRTOS timer.
 * Simulates dissolved oxygen (DO) level data and sends it via the communication layer.
 *
 * @param xTimer: Handle to the FreeRTOS timer that triggers this callback.
 ******************************************************************************/
void RunDOLevelSensor(TimerHandle_t xTimer) {
    static float do_level = 6.0;      // Initial DO level value in mg/L (milligrams per liter)
    static uint8_t do_up = true;     // Boolean flag to determine whether to increase or decrease the value
    const float noise = ((rand() % 20) + 1) / 100.0; // Small random noise between 0.01 and 0.20 mg/L

    // Simulate DO level variation
    if (do_up)
        do_level += 0.1; // Increment the DO level by 0.1 mg/L
    else
        do_level -= 0.1; // Decrement the DO level by 0.1 mg/L

    // Reverse the direction when DO level reaches the boundaries
    if (do_level >= 8.0) do_up = false; // Reverse at the upper limit of 8.0 mg/L
    if (do_level <= 3.5) do_up = true;  // Reverse at the lower limit of 3.5 mg/L

    // Add simulated noise to the DO level value for a more realistic reading
    float simulated_do_level = do_level + noise;

    // Transmit the simulated DO level value scaled to an integer
    // DO levels are multiplied by 100 to maintain precision during transmission
    send_sensorData_message(DOLevel, simulated_do_level * 100);
}
