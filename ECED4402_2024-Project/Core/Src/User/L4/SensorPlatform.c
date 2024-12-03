/*
 * remoteSensingPlatform.c
 *
 *  Created on: Oct. 21, 2022
 *      Author: Andre Hendricks / Dr. JF Bousquet
 *      Modified by: Nnaemeka Nnadede & Temitope Onafalujo
 */
#include <stdio.h>

#include "User/L2/Comm_Datalink.h"
#include "User/L3/TurbiditySensor.h"
#include "User/L3/MicroplasticSensor.h"
#include "User/L3/DOLevelSensor.h"
#include "User/L4/SensorPlatform.h"
#include "User/util.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

/******************************************************************************
This task is created from the main.
It is responsible for managing the messages from the datalink.
It is also responsible for starting the timers for each sensor
******************************************************************************/
void SensorPlatformTask(void *params)
{
	const TickType_t TimerDefaultPeriod = 1000;
	TimerHandle_t TimerID_TurbiditySensor, TimerID_MicroplasticSensor, TimerID_DOLevelSensor;


	TimerID_TurbiditySensor = xTimerCreate(
		"Turbidity Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)0,
		RunTurbiditySensor
		);

	TimerID_MicroplasticSensor = xTimerCreate(
		"Microplastic Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)1,
		RunMicroplasticSensor
		);

	TimerID_DOLevelSensor = xTimerCreate(
		"DOLevel Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)2,
		RunDOLevelSensor
		);


	print_str("Start Instruction received!\r\n");


	request_sensor_read();  // requests a usart read (through the callback)

	struct CommMessage currentRxMessage = {0};

	do {

		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

			switch(currentRxMessage.SensorID){
				case Controller:
					print_str("Reached Here CONTROLLER!\r\n");
					switch(currentRxMessage.messageId){
						case 0:
							xTimerStop(TimerID_TurbiditySensor, portMAX_DELAY);
							xTimerStop(TimerID_MicroplasticSensor, portMAX_DELAY);
							xTimerStop(TimerID_DOLevelSensor, portMAX_DELAY);
							send_ack_message(RemoteSensingPlatformReset);
							break;
						case 1: //Do Nothing
							break;
						case 3: //Do Nothing
							break;
						}
					break;
				case Turbidity:
					print_str("Reached Here TURBIDITY!\r\n");
					switch(currentRxMessage.messageId){
						case 0:
							xTimerChangePeriod(TimerID_TurbiditySensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_TurbiditySensor, portMAX_DELAY);
							send_ack_message(TurbiditySensorEnable);
							break;
						case 1: //Do Nothing
							break;
						case 3: //Do Nothing
							break;
					}
					break;
				case Microplastic:
					print_str("Reached Here MICROPLASTIC!\r\n");
					switch(currentRxMessage.messageId){
						case 0:
							xTimerChangePeriod(TimerID_MicroplasticSensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_MicroplasticSensor, portMAX_DELAY);
							send_ack_message(MicroplasticSensorEnable);
							break;
						case 1: //Do Nothing
							break;
						case 3: //Do Nothing
							break;
					}
					break;
				case DOLevel:
					print_str("Reached Here DOLevel!\r\n");
					switch(currentRxMessage.messageId){
						case 0:
							xTimerChangePeriod(TimerID_DOLevelSensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_DOLevelSensor, portMAX_DELAY);
							send_ack_message(DOLevelSensorEnable);
							break;
						case 1: //Do Nothing
							break;
						case 3: //Do Nothing
							break;
					}
					break;
					default://Should not get here
						break;
			}
			ResetMessageStruct(&currentRxMessage);
		}
	} while(1);
}
