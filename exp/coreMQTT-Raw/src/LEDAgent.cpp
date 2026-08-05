/*
 * LEDAgent.cpp
 *
 * Manage LED and latched switch software behaviour
 *
 *  Created on: 23 Oct 2022
 *      Author: jondurrant
 */

#include "LEDAgent.h"
#include "MQTTTopicHelper.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"

//Local enumerator of the actions to be queued
enum LEDAction {LEDOff, LEDOn, LEDToggle};

/***
 * Constructor
 * @param spstGP - GPIO Pad of SPST non latched switch
 * @param interface - MQTT Interface that state will be notified to
 */
LEDAgent::LEDAgent(uint8_t spstGP, MQTTInterface *interface) {
	xSpstGP = spstGP;
	pInterface = interface;

	//Construct switch observer and listen
	pSwitchMgr = new SwitchMgr(xSpstGP);
	pSwitchMgr->setObserver(this);

	// Queue for actions commands for the class
	xCmdQ = xQueueCreate( LED_QUEUE_LEN, sizeof(LEDAction));
	if (xCmdQ == NULL){
		LogError(("Unable to create Queue\n"));
	}

	//Construct a message buffer
	xBuffer = xMessageBufferCreate(LED_BUFFER_LEN);
	if (xBuffer == NULL){
		LogError(("Buffer could not be allocated\n"));
	}

	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

}

/***
 * Destructor
 */
LEDAgent::~LEDAgent() {
	if (pSwitchMgr != NULL){
		delete pSwitchMgr;
	}
	if (xCmdQ != NULL){
		vQueueDelete(xCmdQ);
	}
	if (xBuffer != NULL){
		vMessageBufferDelete(xBuffer);
	}
}


/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void LEDAgent::handleShortPress(uint8_t gp){
	intToggle();
}

/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void LEDAgent::handleLongPress(uint8_t gp){
	intToggle();
}


/***
 * Set the states of the LED to - on
 * @param on - boolean if the LED should be on or off
 */
void LEDAgent::setOn(bool on){
	LEDAction action = LEDOff;

	if (on){
		action = LEDOn;
	}

	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}


/***
 * Toggle the state of the LED. so On becomes Off, etc.
 */
void LEDAgent::toggle(){
	LEDAction action = LEDToggle;
	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}

/***
 * Toggle LED state from within an intrupt
 */
void LEDAgent::intToggle(){
	LEDAction action = LEDToggle;
	BaseType_t res = xQueueSendToFrontFromISR(xCmdQ, (void *)&action, NULL);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}


/***
  * Main Run Task for agent
  */
void LEDAgent::run(){
	BaseType_t res;
	LEDAction action = LEDOff;
	char jsonStr[LED_JSON_LEN];
	size_t readLen;
	uint32_t next = 0;

	if (xCmdQ == NULL){
		return;
	}

	xNeopixels.setBrightness( 30);
	xNeopixels.fill(PicoLed::RGB(0xff, 0x00, 0x00));
	xNeopixels.show();
	vTaskDelay(3000);
	xNeopixels.clear();
	xNeopixels.show();

	while (true) { // Loop forever
		readLen = xMessageBufferReceive(
						 xBuffer,
				         jsonStr,
						 LED_JSON_LEN,
						 0
				    );
		if (readLen > 0){
			jsonStr[readLen] = 0;
	        parseJSON(jsonStr);
		}

		res = xQueueReceive(xCmdQ, (void *)&action, 0);
		if (res == pdTRUE){
			switch(action){
				case LEDOff:{
					execLed(false);
					break;
				}
				case LEDOn:{
					execLed(true);
					break;
				}
				case LEDToggle:{
					execLed(!xState);
					break;
				}
			}
			taskYIELD();
		}



		uint32_t now = to_ms_since_boot (get_absolute_time());
		if (now > next){
			sendTelem();
			next = now + 5000;
		}

		xNeopixels.show();
		vTaskDelay(250);

		//Decay Tx and Rx Pixels
		xTxPixel = xTxPixel >> 1;
		xNeopixels.setPixelColor(1, PicoLed::RGB(0x00, 0x00, xTxPixel));
		xRxPixel = xRxPixel >> 1;
		xNeopixels.setPixelColor(2, PicoLed::RGB(0x00, 0x00, xRxPixel));
	}
}


/***
 * Execute the state on the LED and notify MQTT interface
 * @param state
 */
void LEDAgent::execLed(bool state){
	xState = state;

	char payload[16];
	if (xState){
		sprintf(payload, "{\"led\":True}");
		xNeopixels.setPixelColor(LED_BAR_LEN-1, PicoLed::RGB(0xff, 0xff, 0xff));
	} else {
		sprintf(payload, "{\"led\":False}");
		xNeopixels.setPixelColor(LED_BAR_LEN-1, PicoLed::RGB(0x00, 0x00, 0x00));
	}
	if (pInterface != NULL){
		pInterface->pubToTopic(
			"v1/devices/me/attributes",
			payload,
			strlen(payload),
			1,
			false
			);
	}
}

void LEDAgent::sendTelem(){
	char payload[80];
	uint32_t now = to_ms_since_boot (get_absolute_time());
	const float conversionFactor = 3.3f / (1 << 12);
	float adc = (float)adc_read() * conversionFactor;
	float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

	sprintf(payload, "{\"uptime\":%d, \"temperature\": %.2f, \"enabled\": True}", now, tempC);

	if (pInterface != NULL){
		pInterface->pubToTopic(
		   "v1/devices/me/telemetry",
			payload,
			strlen(payload),
			1,
			false
			);
	}
}



/***
 * Get the static depth required in words
 * @return - words
 */
 configSTACK_DEPTH_TYPE LEDAgent::getMaxStackSize(){
	 return 250;
 }


/***
* Parse a JSON string and add request to queue
* @param str - JSON Strging
*/
void LEDAgent::parseJSON(char *str){
	//printf("JSON is %s\n", str);
	 json_t const* json = json_create( str, pJsonPool, LED_JSON_POOL);
	 if ( !json ) {
		 LogError(("Error json create."));
		 return ;
	 }

	 json_t const* method = json_getProperty( json, "method" );
	 if ( method ) {
		 if (strcmp("reboot", json_getValue(method)) == 0){
			 printf("REBOOT.......\n");
			 //Use watchdog timer to trigger reboot
			 watchdog_enable(100, 1);
		 }
	 }

	 json_t const* on = json_getProperty( json, "led" );
	 if ( !on || JSON_BOOLEAN != json_getType( on ) ) {
		 LogWarn(("Warning, the led property is not found."));
		 return ;
	 }
	 if (on){
		 bool b = (int)json_getBoolean( on );
		 setOn(b);
	 }


}


/***
 * Add a JSON string action
 * @param jsonStr
 */
void LEDAgent::addJSON(const void  *jsonStr, size_t len){
	if (xBuffer != NULL){
		size_t res = xMessageBufferSend(
			xBuffer,
			jsonStr,
			len,
			0);

		if (res != len){
			LogError(("Failed to write"));
		}

		//printf("AddJSON(%u)=%u, %s\n", len, res, jsonStr);
	}
}



void LEDAgent::MQTTOffline(){
	printf("MQTT OFFLINE\n");
	xNeopixels.setPixelColor(0, PicoLed::RGB(0xff, 0x00, 0x00));
}

void LEDAgent::MQTTOnline(){
	printf("MQTT ONLINE\n");
	xNeopixels.setPixelColor(0, PicoLed::RGB(0x00, 0xff, 0x00));
}

void LEDAgent::MQTTSend(){
	xTxPixel = 0xff;
	xNeopixels.setPixelColor(1, PicoLed::RGB(0x00, 0x00, xTxPixel));
}

void LEDAgent::MQTTRecv(){
	xRxPixel = 0xff;
	xNeopixels.setPixelColor(2, PicoLed::RGB(0x00, 0x00, xRxPixel));
}
