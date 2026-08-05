/*
 * LEDAgent.h
 *
 * Manage LED and latched switch software behaviour
 *
 *  Created on: 23 Oct 2022
 *      Author: jondurrant
 */

#ifndef _LEDAGENT_H_
#define _LEDAGENT_H_


#include "Agent.h"
#include "SwitchObserver.h"
#include "SwitchMgr.h"
#include "tiny-json.h"

#include "pico/stdlib.h"
#include "queue.h"
#include "message_buffer.h"
#include "MQTTConfig.h"
#include "MQTTInterface.h"
#include "MQTTAgentObserver.h"
#include "PicoLed.hpp"

#define LED_QUEUE_LEN 	5
#define MQTT_TOPIC_LED_STATE "LED/state"
#define LED_BUFFER_LEN 	256
#define LED_JSON_LEN 	80
#define LED_JSON_POOL 	5
#define GP_WS2812B				9
#define LED_BAR_LEN 			11


class LEDAgent : public Agent, public SwitchObserver, public MQTTAgentObserver {
public:
	/***
	 * Constructor
	 * @param spstGP - GPIO Pad of SPST non latched switch
	 * @param interface - MQTT Interface that state will be notified to
	 */
	LEDAgent(uint8_t spstGP, MQTTInterface *interface);

	/***
	 * Destructor
	 */
	virtual ~LEDAgent();

	/***
	 * Set the states of the LED to - on
	 * @param on - boolean if the LED should be on or off
	 */
	void setOn(bool on);

	/***
	 * Toggle the state of the LED. so On becomes Off, etc.
	 */
	void toggle();


	/***
	 * Add a JSON string action
	 * @param jsonStr
	 */
	void addJSON(const void  *jsonStr, size_t len);

	/***
	 * Handle a short press from the switch
	 * @param gp - GPIO number of the switch
	 */
	virtual void handleShortPress(uint8_t gp);

	/***
	 * Handle a short press from the switch
	 * @param gp - GPIO number of the switch
	 */
	virtual void handleLongPress(uint8_t gp);

	virtual void MQTTOffline();

	virtual void MQTTOnline();

	virtual void MQTTSend();

	virtual void MQTTRecv();

protected:
	/***
	 * Task main run loop
	 */
	virtual void run();

	/***
	 * Get the static depth required in words
	 * @return - words
	 */
	virtual configSTACK_DEPTH_TYPE getMaxStackSize();

private:
	/***
	 * Toggle LED state from within an intrupt
	 */
	void intToggle();

	/***
	 * Execute the state on the LED and notify MQTT interface
	 * @param state
	 */
	void execLed(bool state);

	/***
	 * Parse a JSON string and add request to queue
	 * @param str - JSON Strging
	 */
	void parseJSON(char *str);

	/***
	 * Publish some telemetry
	 */
	void sendTelem();

	//Interface to publish state to MQTT
	MQTTInterface *pInterface = NULL;

	//State of the LED
	bool xState = false;

	//Switch pads
	uint8_t xSpstGP;

	// Switch manage to manage the SPST switch
	SwitchMgr *pSwitchMgr = NULL;


	//Queue of commands
	QueueHandle_t xCmdQ;

	// Message buffer handle
	MessageBufferHandle_t xBuffer = NULL;

	// Json decoding buffer
	json_t pJsonPool[ LED_JSON_POOL ];

	PicoLed::PicoLedController xNeopixels = PicoLed::addLeds<PicoLed::WS2812B>(
						pio1, 0,
						GP_WS2812B,
						LED_BAR_LEN,
						PicoLed::FORMAT_GRB);


	uint8_t xTxPixel = 0;
	uint8_t xRxPixel = 0;

};


#endif /* _LEDAGENT_H_ */
