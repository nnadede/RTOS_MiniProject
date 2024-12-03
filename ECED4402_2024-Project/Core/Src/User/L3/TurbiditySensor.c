/*
 * TurbiditySensorController.c
 *
 *  Created on: Nov. 22, 2024
 *      Author: Nnaemeka Nnadede & Temitope Onafalujo
 */

#include <stdbool.h> // Required for using boolean data types

#include "User/L2/Comm_Datalink.h"  // Communication layer header file
#include "User/L3/TurbiditySensor.h" // Turbidity sensor-specific functionalities

// Required FreeRTOS header files
#include "FreeRTOS.h" // FreeRTOS main header
#include "Timers.h"   // Timer functions for periodic sensor execution

/******************************************************************************
 * RunTurbiditySensor
 * Software callback function executed periodically by a FreeRTOS timer.
 * Simulates turbidity sensor data and sends it via the communication layer.
 *
 * @param xTimer: Handle to the FreeRTOS timer that triggers this callback.
 ******************************************************************************/
void RunTurbiditySensor(TimerHandle_t xTimer) {
    static float turbidity = 5.0;      // Initial turbidity value in NTU (Nephelometric Turbidity Units)
    static uint8_t turbidity_up = true; // Boolean flag to determine whether to increase or decrease the value
    const float noise = ((rand() % 5) + 1) / 10.0; // Small random noise between 0.1 and 0.5 NTU

    // Simulate turbidity variation
    if (turbidity_up)
        turbidity += 0.5; // Increment the turbidity by 0.5 NTU
    else
        turbidity -= 0.5; // Decrement the turbidity by 0.5 NTU

    // Reverse the direction when turbidity reaches the boundaries
    if (turbidity >= 55) turbidity_up = false; // Reverse at the upper limit of 55 NTU
    if (turbidity <= 5) turbidity_up = true;  // Reverse at the lower limit of 5 NTU

    // Add simulated noise to the turbidity value for a more realistic reading
    float simulated_turbidity = turbidity + noise;

    // Transmit the simulated turbidity value scaled to an integer
    // Turbidity values are multiplied by 100 to maintain precision during transmission
    send_sensorData_message(Turbidity, simulated_turbidity * 100);
}
