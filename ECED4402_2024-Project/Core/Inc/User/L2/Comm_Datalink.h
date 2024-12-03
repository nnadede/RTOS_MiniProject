/*
 * Comm_Datalink.h
 *
 *  Created on: Oct 22, 2022
 *      Author: kadh1
 */

#ifndef INC_USER_L2_COMM_DATALINK_H_
#define INC_USER_L2_COMM_DATALINK_H_

#include <stdbool.h>       // Include standard library for boolean data types
#include "User/L1/USART_Driver.h" // Include USART driver for serial communication
#include "FreeRTOS.h"      // Include FreeRTOS main header
#include "semphr.h"        // Include FreeRTOS semaphore functionalities

// Enumeration for identifying different sensor types
enum SensorId_t {
    None,           // No sensor
    Controller,     // Controller module
    Turbidity,      // Turbidity sensor
    Microplastic,   // Microplastic sensor
    DOLevel         // Dissolved Oxygen (DO) level sensor
};

// Enumeration for types of acknowledgment messages
enum AckTypes {
    RemoteSensingPlatformReset,  // Acknowledge platform reset
    TurbiditySensorEnable,       // Acknowledge Turbidity sensor enable command
    MicroplasticSensorEnable,    // Acknowledge Microplastic sensor enable command
    DOLevelSensorEnable          // Acknowledge DOLevel sensor enable command
};

// Enumeration for commands from the Host PC
enum HostPCCommands {
    PC_Command_NONE,  // No command received
    PC_Command_START, // Command to start operations
    PC_Command_RESET  // Command to reset operations
};

// Structure to represent a communication message
struct CommMessage {
    enum SensorId_t SensorID;     // ID of the sensor sending the message
    uint8_t messageId;            // Message identifier
    uint16_t params;              // Additional parameters for the message
    uint8_t checksum;             // Checksum for message integrity
    bool IsCheckSumValid;         // Flag indicating if the checksum is valid
    bool IsMessageReady;          // Flag indicating if the message is fully decoded
};

// Function prototypes for communication datalink functionalities

/**
 * @brief Send data message for a specific sensor type.
 * @param sensorType The type of sensor sending the data.
 * @param data The data value to send (scaled or raw).
 */
void send_sensorData_message(enum SensorId_t sensorType, uint16_t data);

/**
 * @brief Send acknowledgment message to confirm a command was received.
 * @param AckType The type of acknowledgment message.
 */
void send_ack_message(enum AckTypes AckType);

/**
 * @brief Send a command to enable a specific sensor with a specified time period.
 * @param sensorType The type of sensor to enable.
 * @param TimePeriod The operational time period for the sensor.
 */
void send_sensorEnable_message(enum SensorId_t sensorType, uint16_t TimePeriod);

/**
 * @brief Send a reset command to reset the sensor platform.
 */
void send_sensorReset_message(void);

/**
 * @brief Initialize the communication datalink for sensor messages.
 */
void initialize_sensor_datalink(void);

/**
 * @brief Initialize the communication datalink for Host PC messages.
 */
void initialize_hostPC_datalink(void);

/**
 * @brief Parse and decode an incoming sensor message.
 * @param currentRxMessage Pointer to the message structure to populate.
 */
void parse_sensor_message(struct CommMessage* currentRxMessage);

/**
 * @brief Parse and decode an incoming command message from the Host PC.
 * @return The command parsed from the Host PC.
 */
enum HostPCCommands parse_hostPC_message();

#endif /* INC_USER_L2_COMM_DATALINK_H_ */
