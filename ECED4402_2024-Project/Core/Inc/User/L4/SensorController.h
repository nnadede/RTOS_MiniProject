/*
 * SensorController.h
 *
 *  Created on: Oct 24, 2022
 *      Author: kadh1
 *      Modified by: Nnaemeka Nnadede & Temitope Onafalujo
 */

#ifndef INC_USER_L4_SENSORCONTROLLER_H_ // Include guard to prevent multiple inclusions
#define INC_USER_L4_SENSORCONTROLLER_H_

// Task declarations for the Sensor Controller system

/**
 * @brief Task to process data received from the Host PC.
 */
void HostPC_RX_Task();

/**
 * @brief Task to process data received from the Sensor Platform.
 */
void SensorPlatform_RX_Task();

/**
 * @brief Main task to manage the state machine for the Sensor Controller.
 *
 * @param params: Task parameters (not used in this implementation).
 */
void SensorControllerTask(void *params);

/**
 * @brief Determines the LED status based on sensor ID and its data value.
 *
 * @param id: Sensor ID to evaluate.
 * @param val: The sensor's value used for determining the LED state.
 * @return enum LEDState: The calculated LED state (Green, Yellow, or Red).
 */
enum LEDState get_LEDstatus(enum SensorId_t id, float val);

/**
 * @brief Task to control LED indicators based on received sensor data.
 *
 * @param params: Task parameters (not used in this implementation).
 */
void LEDControllerTask(void *params);

/**
 * @brief Updates the LED status for a specific sensor.
 *
 * @param id: Sensor ID whose LED status is being updated.
 * @param status: The new LED state to set.
 */
void updateLEDStatus(enum SensorId_t id, enum LEDState status);

/**
 * @brief Disables all LEDs by turning them off.
 */
void disableLED();

/**
 * @brief Task to control a white LED for signaling purposes.
 *
 * @param params: Task parameters (not used in this implementation).
 */
void WhiteLEDTask(void *params);

/**
 * @brief Task to compress sensor data, process LED statuses, and manage LED indicators.
 *
 * @param params: Task parameters (not used in this implementation).
 */
void CompressionTask(void *params);

// Enumeration for defining controller states
enum ControllerState {
    Init_S,    // Initialization state
    Start_S,   // Start state for enabling sensors
    Parsing_S, // State for parsing sensor data
    Reset_S    // Reset state for handling system resets
};

// Enumeration for defining LED states
enum LEDState {
    Init,   // Initial state
    Red,    // Red LED state (critical)
    Yellow, // Yellow LED state (warning)
    Green   // Green LED state (normal)
};

// Structure to represent individual sensor LED data
typedef struct {
    enum SensorId_t sensorID; // ID of the sensor
    enum LEDState status;     // LED status for the sensor
} LEDSensorData;

// Structure to represent data for all LEDs
typedef struct {
    LEDSensorData turbidity;       // Turbidity sensor data
    LEDSensorData microplastics;   // Microplastics sensor data
    LEDSensorData do_levels;       // Dissolved oxygen sensor data
} LEDData;

// Structure to represent scaled sensor data
typedef struct {
    enum SensorId_t sensorID; // ID of the sensor
    uint16_t data;            // Scaled sensor data value
} ScaledData;

#endif /* INC_USER_L4_SENSORCONTROLLER_H_ */
