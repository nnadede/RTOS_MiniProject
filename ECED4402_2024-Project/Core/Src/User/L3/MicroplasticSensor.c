/*
 * MicroplasticSensorController.c
 *
 *  Created on: Nov. 22, 2024
 *      Author: Nnaemeka Nnadede & Temitope Onafalujo
 */

#include <stdbool.h> // Required for boolean data types

#include "User/L2/Comm_Datalink.h"   // Communication layer header file
#include "User/L3/MicroplasticSensor.h" // Microplastic sensor-specific functionalities

// Required FreeRTOS header files
#include "FreeRTOS.h"  // FreeRTOS main header
#include "Timers.h"    // Timer functions for periodic sensor execution

/******************************************************************************
 * RunMicroplasticSensor
 * Software callback function executed periodically by a FreeRTOS timer.
 * Simulates microplastic concentration data and sends it via the communication layer.
 *
 * @param xTimer: Handle to the FreeRTOS timer that triggers this callback.
 ******************************************************************************/
void RunMicroplasticSensor(TimerHandle_t xTimer) {
    static int microplastic = 300;      // Initial microplastic value in particles per liter (particles/L)
    static uint8_t microplastic_up = true; // Boolean flag to determine whether to increase or decrease the value
    const int noise = rand() % 50;      // Small random noise between 0 and 49 particles/L

    // Simulate microplastic variation
    if (microplastic_up)
        microplastic += 20; // Increment the microplastic concentration by 20 particles/L
    else
        microplastic -= 20; // Decrement the microplastic concentration by 20 particles/L

    // Reverse the direction when microplastic concentration reaches the boundaries
    if (microplastic >= 2100) microplastic_up = false; // Reverse at the upper limit of 2100 particles/L
    if (microplastic <= 100) microplastic_up = true;  // Reverse at the lower limit of 100 particles/L

    // Add simulated noise to the microplastic value for a more realistic reading
    int simulated_microplastic = microplastic + noise;

    // Transmit the simulated microplastic concentration directly
    send_sensorData_message(Microplastic, simulated_microplastic);
}
