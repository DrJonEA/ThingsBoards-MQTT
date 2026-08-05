/*
 * MQTTRouterLED.cpp
 *
 *  Created on: 4 Oct 2022
 *      Author: jondurrant
 */

#include "MQTTRouterLED.h"
#include "MQTTTopicHelper.h"

#define LED_TOPIC  "LED"
#define PAYLOAD_ON "on"

MQTTRouterLED::MQTTRouterLED(LEDAgent *agent) {
	pAgent = agent;
}

MQTTRouterLED::~MQTTRouterLED() {
	// Nop
}



/***
 * Use the interface to setup all the subscriptions
 * @param interface
 */
void MQTTRouterLED::subscribe(MQTTInterface *interface){
	interface->subToTopic("v1/devices/me/attributes", 1);
	interface->subToTopic("v1/devices/me/rpc/request/+",1);
}

/***
 * Route the message the appropriate part of the application
 * @param topic
 * @param topicLen
 * @param payload
 * @param payloadLen
 * @param interface
 */
void MQTTRouterLED::route(const char *topic,
		size_t topicLen,
		const void * payload,
		size_t payloadLen,
		MQTTInterface *interface){

	//printf("Route topic %s\n", topic);
	if(strstr(topic, "v1/devices/me/rpc/request/") != NULL) {
		//printf("Handling RPC\n");
		if (pAgent != NULL){
			pAgent->addJSON(payload, payloadLen);
		}
	} else if(strstr(topic, "v1/devices/me/attributes") != NULL) {
		if (pAgent != NULL){
			pAgent->addJSON(payload, payloadLen);
		}
	}

}
