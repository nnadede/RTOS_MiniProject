/*
 * Comm_Datalink.c
 *
 *  Created on: Oct. 21, 2022
 *      Author: Andre Hendricks / Dr. JF Bousquet
 *      Modified by: Nnaemeka Nnadede & Temitope Onafalujo
 */

#include <string.h>  // For string operations
#include <stdio.h>   // For formatted string functions
#include <stdlib.h>  // For standard utility functions
#include <math.h>    // For mathematical functions

#include "User/L1/USART_Driver.h" // USART driver for serial communication
#include "User/L2/Comm_Datalink.h" // Header for communication functionalities
#include "User/util.h" // Utility functions

// Enumeration for message parsing states
enum ParseMessageState_t {Waiting_S, SensorID_S, MessageID_S, ParamsID_S, Star_S, CS_S};

// Static function prototype for sending strings with checksum
static void sendStringSensor(char* tx_string);

/******************************************************************************
 * @brief Initializes the sensor communication datalink.
 * Configures the external USART interface for sensor communication.
 ******************************************************************************/
void initialize_sensor_datalink(void) {
    configure_usart_extern(); // Set up external USART for sensor communication
}

/******************************************************************************
 * @brief Initializes the Host PC communication datalink.
 * Configures the Host PC USART interface for communication.
 ******************************************************************************/
void initialize_hostPC_datalink(void) {
    configure_usart_hostPC(); // Set up USART for Host PC communication
}

/******************************************************************************
 * @brief Calculates the checksum and sends the input string via UART.
 *
 * @param tx_string: The input string to send.
 ******************************************************************************/
static void sendStringSensor(char* tx_string) {
    uint8_t checksum;       // Variable to store checksum
    uint16_t str_length;    // Length of the string

    // Calculate the string length excluding the checksum placeholder
    str_length = strlen((char *)tx_string) - 1;

    // Compute the checksum using XOR over all characters
    checksum = tx_string[0];
    for (int idx = 1; idx < str_length - 2; idx++) {
        checksum ^= tx_string[idx];
    }

    // Append the checksum to the string
    sprintf(&tx_string[str_length - 2], "%02x\n", checksum);

    // Send the string via the external USART
    printStr_extern(tx_string);
}

/******************************************************************************
 * @brief Parses incoming messages from sensors.
 *
 * @param currentRxMessage: Pointer to the structure that will hold the parsed message.
 ******************************************************************************/
void parse_sensor_message(struct CommMessage* currentRxMessage) {
    static enum ParseMessageState_t currentState = Waiting_S; // State machine's current state
    uint8_t CurrentChar; // Current character being processed
    static uint16_t sensorIdIdx = 0, MessageIdIdx = 0, ParamIdx = 0, checksumIdx = 0;
    static char sensorId[6], CSStr[3];
    static uint8_t checksum_val;
    static const struct CommMessage EmptyMessage = {0}; // Empty message template

    // Process each character in the UART queue
    while (xQueueReceive(Queue_extern_UART, &CurrentChar, portMAX_DELAY) == pdPASS &&
           currentRxMessage->IsMessageReady == false) {

        if (CurrentChar == '$') { // Reset state machine when '$' is received
            checksum_val = CurrentChar;
            sensorIdIdx = MessageIdIdx = ParamIdx = checksumIdx = 0;
            currentState = SensorID_S;
            *currentRxMessage = EmptyMessage; // Reset the current message
            continue;
        }

        // State machine for parsing the message
        switch (currentState) {
            case Waiting_S:
                // Do nothing in Waiting state
                break;

            case SensorID_S:
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = MessageID_S;
                } else if (sensorIdIdx < 5) {
                    sensorId[sensorIdIdx++] = CurrentChar;
                }
                if (sensorIdIdx == 5) {
                    sensorId[sensorIdIdx] = '\0'; // Null-terminate the sensor ID string

                    // Map the sensor ID to enum
                    if (strcmp(sensorId, "CNTRL") == 0)
                        currentRxMessage->SensorID = Controller;
                    else if (strcmp(sensorId, "TURBD") == 0)
                        currentRxMessage->SensorID = Turbidity;
                    else if (strcmp(sensorId, "MCRPL") == 0)
                        currentRxMessage->SensorID = Microplastic;
                    else if (strcmp(sensorId, "DOLEV") == 0)
                        currentRxMessage->SensorID = DOLevel;
                    else {
                        currentRxMessage->SensorID = None;
                        currentState = Waiting_S; // Invalid sensor ID
                    }
                }
                break;

            case MessageID_S:
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = ParamsID_S;
                } else {
                    if (MessageIdIdx < 2) {
                        currentRxMessage->messageId = currentRxMessage->messageId * 10 + (CurrentChar - '0');
                    }
                    MessageIdIdx++;
                }
                break;

            case ParamsID_S:
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = Star_S;
                } else if (ParamIdx < 8) {
                    currentRxMessage->params = currentRxMessage->params * 10 + (CurrentChar - '0');
                }
                break;

            case Star_S:
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = CS_S;
                }
                break;

            case CS_S:
                if (checksumIdx < 2) {
                    CSStr[checksumIdx++] = CurrentChar;
                }
                if (checksumIdx == 2) {
                    currentState = Waiting_S;
                    CSStr[checksumIdx] = '\0';
                    currentRxMessage->checksum = strtol(CSStr, NULL, 16);
                    if (currentRxMessage->checksum == checksum_val) {
                        currentRxMessage->IsMessageReady = true;
                        currentRxMessage->IsCheckSumValid = true;
                    } else {
                        currentRxMessage->IsCheckSumValid = false;
                    }
                }
                break;
        }
    }
}

/******************************************************************************
 * @brief Parses messages received from the Host PC.
 *
 * @return enum HostPCCommands: Command parsed from the Host PC.
 ******************************************************************************/
enum HostPCCommands parse_hostPC_message() {
    uint8_t CurrentChar;
    static char HostPCMessage[10];
    static uint16_t HostPCMessage_IDX = 0;

    while (xQueueReceive(Queue_hostPC_UART, &CurrentChar, portMAX_DELAY) == pdPASS) {
        if (CurrentChar == '\n' || CurrentChar == '\r' || HostPCMessage_IDX >= 6) {
            HostPCMessage[HostPCMessage_IDX++] = '\0';
            HostPCMessage_IDX = 0;
            if (strcmp(HostPCMessage, "START") == 0)
                return PC_Command_START;
            else if (strcmp(HostPCMessage, "RESET") == 0)
                return PC_Command_RESET;
        } else {
            HostPCMessage[HostPCMessage_IDX++] = CurrentChar;
        }
    }
    return PC_Command_NONE;
}

/******************************************************************************
 * @brief Sends sensor data messages.
 *
 * @param sensorType: The type of sensor sending the data.
 * @param data: The sensor data value.
 ******************************************************************************/
void send_sensorData_message(enum SensorId_t sensorType, uint16_t data) {
    char tx_sensor_buffer[50];

    switch (sensorType) {
        case Turbidity:
            sprintf(tx_sensor_buffer, "$TURBD,03,%08u,*,00\n", data);
            break;
        case Microplastic:
            sprintf(tx_sensor_buffer, "$MCRPL,03,%08u,*,00\n", data);
            break;
        case DOLevel:
            sprintf(tx_sensor_buffer, "$DOLEV,03,%08u,*,00\n", data);
            break;
        default:
            return; // Invalid sensor type
    }
    sendStringSensor(tx_sensor_buffer);
}

/******************************************************************************
 * @brief Sends enable messages to sensors.
 *
 * @param sensorType: The sensor type to enable.
 * @param TimePeriod_ms: The time period for the sensor in milliseconds.
 ******************************************************************************/
void send_sensorEnable_message(enum SensorId_t sensorType, uint16_t TimePeriod_ms) {
    char tx_sensor_buffer[50];

    switch (sensorType) {
        case Turbidity:
            sprintf(tx_sensor_buffer, "$TURBD,00,%08u,*,00\n", TimePeriod_ms);
            break;
        case Microplastic:
            sprintf(tx_sensor_buffer, "$MCRPL,00,%08u,*,00\n", TimePeriod_ms);
            break;
        case DOLevel:
            sprintf(tx_sensor_buffer, "$DOLEV,00,%08u,*,00\n", TimePeriod_ms);
            break;
        default:
            return; // Invalid sensor type
    }
    sendStringSensor(tx_sensor_buffer);
}

/******************************************************************************
 * @brief Sends a reset message to all sensors.
 ******************************************************************************/
void send_sensorReset_message(void) {
    char tx_sensor_buffer[50];
    sprintf(tx_sensor_buffer, "$CNTRL,00,,*,00\n");
    sendStringSensor(tx_sensor_buffer);
}

/******************************************************************************
 * @brief Sends acknowledgment messages for specific events.
 *
 * @param AckType: The acknowledgment type.
 ******************************************************************************/
void send_ack_message(enum AckTypes AckType) {
    char tx_sensor_buffer[50];

    switch (AckType) {
        case RemoteSensingPlatformReset:
            sprintf(tx_sensor_buffer, "$CNTRL,01,,*,00\n");
            break;
        case TurbiditySensorEnable:
            sprintf(tx_sensor_buffer, "$TURBD,01,,*,00\n");
            break;
        case MicroplasticSensorEnable:
            sprintf(tx_sensor_buffer, "$MCRPL,01,,*,00\n");
            break;
        case DOLevelSensorEnable:
            sprintf(tx_sensor_buffer, "$DOLEV,01,,*,00\n");
            break;
        default:
            return; // Invalid acknowledgment type
    }
    sendStringSensor(tx_sensor_buffer);
}
