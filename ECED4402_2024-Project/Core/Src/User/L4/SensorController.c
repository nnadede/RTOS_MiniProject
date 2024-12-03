/*
 * SensorController.c
 *
 *  Created on: Oct 24, 2022
 *      Author: kadh1
 *      Modified by: Nnaemeka Nnadede & Temitope Onafalujo
 */


#include <stdio.h>

#include "main.h"
#include "User/L2/Comm_Datalink.h"
#include "User/L3/TurbiditySensor.h"
#include "User/L3/MicroplasticSensor.h"
#include "User/L3/DOLevelSensor.h"
#include "User/L4/SensorPlatform.h"
#include "User/L4/SensorController.h"
#include "User/util.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

QueueHandle_t Queue_Sensor_Data;
QueueHandle_t Queue_HostPC_Data;
QueueHandle_t Queue_Scaled_Data;
QueueHandle_t Queue_LED_Data;


static enum ControllerState ControlState = Init_S; // Initialize to the starting state


static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}




/******************************************************************************
This task is created from the main.
******************************************************************************/
void SensorControllerTask(void *params) {
	static ScaledData data_s;
    struct CommMessage receivedRxMessage;       // Message from the Sensor Platform
    enum HostPCCommands HostPCInstruction;     // Command from the Host PC
    uint8_t TurbidityAck = 0, MicroplasticAck = 0, DOLevelAck = 0;     // Acknowledgment flags for sensors

    while (1) {
        switch (ControlState) {
            case Init_S:
                // Wait for a START command from the Host PC
                if (xQueueReceive(Queue_HostPC_Data, &HostPCInstruction, portMAX_DELAY) == pdPASS) {
                    if (HostPCInstruction == PC_Command_START) {
                        // Transition to Start state
                        print_str("Start command received from Host PC.\r\n");
                        ControlState = Start_S;
                    }
                }
                break;

            case Start_S:
                // Send enable commands to sensors
                send_sensorEnable_message(Turbidity, 1000);
                send_sensorEnable_message(Microplastic, 1000);
                send_sensorEnable_message(DOLevel, 1000);

                // Wait for acknowledgments from both sensors
                while (!(TurbidityAck && MicroplasticAck && DOLevelAck)) {
                    if (xQueueReceive(Queue_Sensor_Data, &receivedRxMessage, portMAX_DELAY) == pdPASS) {
                        if (receivedRxMessage.SensorID == Turbidity && receivedRxMessage.messageId == 01) {
                            print_str("Turbidity sensor enabled.\r\n");
                            TurbidityAck = 1;
                        } else if (receivedRxMessage.SensorID == Microplastic && receivedRxMessage.messageId == 01) {
                            print_str("Microplastic sensor enabled.\r\n");
                            MicroplasticAck = 1;
                        } else if (receivedRxMessage.SensorID == DOLevel && receivedRxMessage.messageId == 01) {
                            print_str("DOLevel sensor enabled.\r\n");
                            DOLevelAck = 1;
                        }
                    }
                }

                // Transition to Parsing state once both sensors are acknowledged
                ControlState = Parsing_S;
                break;

            case Parsing_S:
            	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
                // Process sensor data
                if (xQueueReceive(Queue_Sensor_Data, &receivedRxMessage, portMAX_DELAY) == pdPASS) {
                    if (receivedRxMessage.SensorID == Turbidity && receivedRxMessage.messageId == 03) {
//                        char Turbidity_data[50];
//                        sprintf(Turbidity_data, "Turbidity Data: %ld\r\n", receivedRxMessage.params);
//                        print_str(Turbidity_data);
                        data_s.sensorID = Turbidity;
                        data_s.data = receivedRxMessage.params;

                    } else if (receivedRxMessage.SensorID == Microplastic && receivedRxMessage.messageId == 03) {
//                        char Microplastic_data[50];
//                        sprintf(Microplastic_data, "Microplastic Data: %ld\r\n", receivedRxMessage.params);
//                        print_str(Microplastic_data);
                        data_s.sensorID = Microplastic;
                        data_s.data = receivedRxMessage.params;

                    } else if (receivedRxMessage.SensorID == DOLevel && receivedRxMessage.messageId == 03) {
//                        char DOLevel_data[50];
//                        sprintf(DOLevel_data, "DOLevel Data: %ld\r\n", receivedRxMessage.params);
//                        print_str(DOLevel_data);
                        data_s.sensorID = DOLevel;
                        data_s.data = receivedRxMessage.params;

                    }

                    xQueueSendToBack(Queue_Scaled_Data, &data_s, 0);

                }


                // Check for a RESET command from the Host PC
                if (xQueueReceive(Queue_HostPC_Data, &HostPCInstruction, 0) == pdPASS) {
                    if (HostPCInstruction == PC_Command_RESET) {
                        print_str("Reset command received from Host PC.\r\n");
                        ControlState = Reset_S;
                    }
                }
                break;

            case Reset_S:
				disableLED();
                // Send reset command to the Sensor Platform
                send_sensorReset_message();
                print_str("Sending reset command to Sensor Platform.\r\n");

                // Wait for reset acknowledgment
                if (xQueueReceive(Queue_Sensor_Data, &receivedRxMessage, portMAX_DELAY) == pdPASS) {
                    if (receivedRxMessage.SensorID == Controller && receivedRxMessage.messageId == 01) {
                        print_str("Reset acknowledgment received.\r\n");

                        // Reset flags
                        TurbidityAck = 0;
                        MicroplasticAck = 0;
                        DOLevelAck = 0;

                        // Transition back to Init state
                        ControlState = Init_S;
                    }
                }
                break;

            default:
                // Stay in Init state if the state is unknown
                ControlState = Init_S;
                break;
        }

        // Add a small delay to prevent task starvation
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}





/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void SensorPlatform_RX_Task(){
	struct CommMessage currentRxMessage = {0};
	Queue_Sensor_Data = xQueueCreate(80, sizeof(struct CommMessage));

	request_sensor_read();  // requests a usart read (through the callback)

	while(1){
		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

			xQueueSendToBack(Queue_Sensor_Data, &currentRxMessage, 0);
			ResetMessageStruct(&currentRxMessage);
		}
	}
}



/*
 * This task reads the queue of characters from the Host PC when available
 * It then sends the processed data to the Sensor Controller Task
 */
void HostPC_RX_Task(){

	enum HostPCCommands HostPCCommand = PC_Command_NONE;

	Queue_HostPC_Data = xQueueCreate(80, sizeof(enum HostPCCommands));

	request_hostPC_read();

	while(1){
		HostPCCommand = parse_hostPC_message();

		if (HostPCCommand == PC_Command_START){
			print_str("Start Instruction received!\r\n");
		}

		if (HostPCCommand != PC_Command_NONE){
			xQueueSendToBack(Queue_HostPC_Data, &HostPCCommand, 0);
		}

	}
}


enum LEDState get_LEDstatus(enum SensorId_t id, float val){

	enum LEDState led_status = Init;
	switch(id){
		case Turbidity:
			if (val >= 0 && val <= 20){
				led_status = Green;
			}else if (val > 20 && val <= 50){
				led_status = Yellow;
			}else if (val > 50 && val <= 100){
				led_status = Red;
			}
			break;
		case Microplastic:
			if (val >= 0 && val <= 500){
				led_status = Green;
			}else if (val > 500 && val <= 2000){
				led_status = Yellow;
			}else if (val > 2000 && val <= 3000){
				led_status = Red;
			}
			break;
		case DOLevel:
			if (val > 7 && val <= 10){
				led_status = Green;
			}else if (val >= 4 && val <= 7){
				led_status = Yellow;
			}else if (val >= 0 && val < 4){
				led_status = Red;
			}
			break;
		default:
			break;
	}
	return led_status;
}


void WhiteLEDTask(void *params)
{

	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_3);    // 100ms OFF 100ms ON -> 200ms Period
	return;

}


/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void LEDControllerTask(void *params) {
    LEDData received_LEDData;
    Queue_LED_Data = xQueueCreate(80, sizeof(LEDData));



    while (1) {
        switch (ControlState) {
            case Parsing_S:
                // Check the queue for new LED data
                if (xQueueReceive(Queue_LED_Data, &received_LEDData, portMAX_DELAY) == pdPASS) {
                    updateLEDStatus(received_LEDData.turbidity.sensorID, received_LEDData.turbidity.status);
                    updateLEDStatus(received_LEDData.microplastics.sensorID, received_LEDData.microplastics.status);
                    updateLEDStatus(received_LEDData.do_levels.sensorID, received_LEDData.do_levels.status);
                }
                break;
            default:
                // Handle unexpected states
                vTaskDelay(100 / portTICK_PERIOD_MS);  // Allow other tasks to run
                break;
        }
    }
}




void updateLEDStatus(enum SensorId_t id, enum LEDState status){
	switch(id){
		case Turbidity:
			switch(status){
				case Green:
					// Turn on the green light
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);   // Green ON
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // Orange OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); // Red OFF

					// Fill: Turn on crossing light (white)
					//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);   // White ON
					break;
				case Yellow:
					// Turn on the orange light
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); // Green OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);   // Orange ON
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); // Red OFF

					// Fill: Blink crossing light (white) by calling a 2nd software timer
					break;
				case Red:
					// Turn on the red light
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); // Green OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // Orange OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);   // Red ON

					// Fill: Turn off crossing light (white) and stop the 2nd software timer
					//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // White OFF
					break;
				default:
					break;
			}
			break;
		case Microplastic:
			switch(status){
				case Green:
					// Turn on the green light
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);   // Green ON
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // Orange OFF
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // Red OFF

					// Fill: Turn on crossing light (white)
					//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);   // White ON
					break;
				case Yellow:
					// Turn on the orange light
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // Green OFF
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);   // Orange ON
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // Red OFF

					// Fill: Blink crossing light (white) by calling a 2nd software timer
					break;
				case Red:
					// Turn on the red light
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // Green OFF
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // Orange OFF
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);   // Red ON

					// Fill: Turn off crossing light (white) and stop the 2nd software timer
					//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // White OFF
					break;
				default:
					break;
			}
			break;
		case DOLevel:
			switch(status){
				case Green:
					// Turn on the green light
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);   // Green ON
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET); // Orange OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); // Red OFF

					// Fill: Turn on crossing light (white)
					//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);   // White ON
					break;
				case Yellow:
					// Turn on the orange light
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET); // Green OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);   // Orange ON
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); // Red OFF

					// Fill: Blink crossing light (white) by calling a 2nd software timer
					break;
				case Red:
					// Turn on the red light
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET); // Green OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET); // Orange OFF
					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);   // Red ON

					// Fill: Turn off crossing light (white) and stop the 2nd software timer
					//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // White OFF
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

}



void disableLED(){

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);   // Green ON
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // Orange OFF
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); // Red OFF

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);   // Green ON
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // Orange OFF
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // Red OFF

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);   // Green ON
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET); // Orange OFF
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); // Red OFF

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // White OFF

}



void CompressionTask(void *params){
	static LEDData send_LEDData;
	static uint8_t count = 0;
	static float val = 0.0;
	Queue_Scaled_Data = xQueueCreate(80, sizeof(LEDData));
	ScaledData data_s;
	do {
		if (xQueueReceive(Queue_Scaled_Data, &data_s, portMAX_DELAY) == pdPASS) {
			switch (data_s.sensorID){
			case Turbidity:
				char Turbidity_data_string[20];
				val = (float)data_s.data/100.0;
				sprintf(Turbidity_data_string, "%04.01f\r\n", val);
				print_str(Turbidity_data_string);
				send_LEDData.turbidity.sensorID = Turbidity;
				send_LEDData.turbidity.status = get_LEDstatus(Turbidity, val);
				break;
			case Microplastic:
				char Microplastic_data_string[20];
				val = (float)data_s.data/1.0;
				sprintf(Microplastic_data_string, "%-4.0f\r\n", val);
				print_str(Microplastic_data_string);
				send_LEDData.microplastics.sensorID = Microplastic;
				send_LEDData.microplastics.status = get_LEDstatus(Microplastic, val);
				break;
			case DOLevel:
				char DOLevel_data_string[20];
				val = (float)data_s.data/100.0;
				sprintf(DOLevel_data_string, "%-4.02f\r\n", val);
				print_str(DOLevel_data_string);
				send_LEDData.do_levels.sensorID = DOLevel;
				send_LEDData.do_levels.status = get_LEDstatus(DOLevel, val);
				break;
			default:
				break;
			}

			count++;
			if (count == 3){
				xQueueSendToBack(Queue_LED_Data, &send_LEDData, 0);
				count = 0;
			}

		}
	} while (1);
}

