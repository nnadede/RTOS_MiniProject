/*
 * main_user.c
 *
 *  Created on: Aug 8, 2022
 *      Author: Andre Hendricks
 *      Modified by: Nnaemeka Nnadede & Temitope Onafalujo
 */

#include <stdio.h>

// STM32 generated header files
#include "main.h"

// User-generated header files
#include "User/main_user.h" // Main user-defined functionalities
#include "User/util.h"      // Utility functions
#include "User/L1/USART_Driver.h" // USART driver for serial communication
#include "User/L2/Comm_Datalink.h" // Communication datalink layer
#include "User/L4/SensorPlatform.h" // Sensor platform management
#include "User/L4/SensorController.h" // Sensor controller logic

// Required FreeRTOS header files for RTOS functionality
#include "FreeRTOS.h" // FreeRTOS main header
#include "task.h"     // FreeRTOS task management functions

// Code modes to define the operating configuration
#define SENSORPLATFORM_MODE 0 // Mode for sensor platform functionality
#define SENSORCONTROLLER_MODE 1 // Mode for sensor controller functionality

// Set the mode in which the code will run: SENSORPLATFORM_MODE or SENSORCONTROLLER_MODE
// Uncomment the desired mode below and comment out the other
//#define CODE_MODE SENSORPLATFORM_MODE
#define CODE_MODE SENSORCONTROLLER_MODE // Currently set to run in SENSORCONTROLLER_MODE

/*
 * Main user function to initialize and configure the system based on the selected mode.
 * This function sets up utilities, datalinks, and creates tasks for either SensorController
 * or SensorPlatform modes.
 */
void main_user() {
    // Initialize utility functions, such as clock setup or basic configurations
    util_init();

    // Initialize the sensor communication datalink layer
    initialize_sensor_datalink();

    // If running in SENSORCONTROLLER_MODE, initialize the Host PC communication datalink
#if CODE_MODE == SENSORCONTROLLER_MODE
    initialize_hostPC_datalink();
#endif

    // Task creation for SENSORCONTROLLER_MODE
#if CODE_MODE == SENSORCONTROLLER_MODE
    // Task for receiving data from the Host PC
    xTaskCreate(HostPC_RX_Task,               // Task function
                "HostPC_RX_Task",             // Task name
                configMINIMAL_STACK_SIZE + 100, // Task stack size
                NULL,                         // Task parameters (none in this case)
                tskIDLE_PRIORITY + 2,         // Task priority
                NULL);                        // Task handle (not used here)

    // Task for receiving data from the Sensor Platform
    xTaskCreate(SensorPlatform_RX_Task,
                "SensorPlatform_RX_Task",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);

    // Task for controlling the sensor controller's main logic
    xTaskCreate(SensorControllerTask,
                "Sensor_Controller_Task",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);

    // Task for compressing data before sending it
    xTaskCreate(CompressionTask,
                "Compression_Task",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);

    // Task for controlling LED indicators based on pollution levels or status
    xTaskCreate(LEDControllerTask,
                "LED_Controller_Task",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);

#elif CODE_MODE == SENSORPLATFORM_MODE
    // Task creation for SENSORPLATFORM_MODE
    // Main task for managing sensor platform operations
    xTaskCreate(SensorPlatformTask,
                "Sensor_Platform_Task",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
#endif

    // Start the FreeRTOS scheduler to begin task execution
    vTaskStartScheduler();

    // Infinite loop to ensure the program does not terminate
    while (1);
}
