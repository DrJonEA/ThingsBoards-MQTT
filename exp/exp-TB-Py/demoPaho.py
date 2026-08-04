#!/usr/bin/env python3
# Testing out a Thingboard.io device using the Paho Python MQTT Client library
# Services tested:
# Telematry
# Attributes
# Server Side RPC

import paho.mqtt.client as mqtt
import json
import time
import sys
import os


led = False

# Grab environment variables
clientId=os.environ.get("MQTT_CLIENT")
user=os.environ.get("MQTT_USER")
passwd=os.environ.get("MQTT_PASSWD")
host= os.environ.get("MQTT_HOST")
port=int(os.environ.get("MQTT_PORT"))
print("MQTT %s:%d"%(host,port))
if (len(clientId) > 6):
   print("Client: %s..."%clientId[0:4])
else: 
   print("Client: %s..."%clientId)   
if (len(user) > 6):
   print("User: %s..."%user[0:4])
else:
    print("User: %s"%user)    

#Set up topic name
devTopics = "v1/devices/me/telemetry"
devAttributes = "v1/devices/me/attributes"
devRpc = "v1/devices/me/rpc/request/+"

# The callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected with result code "+str(reason_code))

    
# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print("Rcv topic=" +msg.topic+" msg="+str(msg.payload))
    global led
    if msg.topic == devAttributes:
        data = json.loads(msg.payload)
        if "led" in data:
            led = data["led"]
            print("LED is now %s"%led)
    if msg.topic.startswith("v1/devices/me/rpc/request/"):
        data = json.loads(msg.payload)
        if "method" in data:
            if data["method"] == "reboot":
                print("Rebooting device...")
                client.disconnect()
                os._exit(0)

# Connect to the broker
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,client_id = clientId)
client.username_pw_set(username=user, password=passwd)
client.on_connect = on_connect
client.on_message = on_message
client.connect(host, port, 60)

#Maintain connection loop in thread
client.loop_start()

#Subscribe to the LED Topic so we can see what was sent
client.subscribe( devTopics )
client.subscribe( devAttributes )
client.subscribe( devRpc )

telemetry = {"temperature": 25.9, "enabled": True, "currentFirmwareVersion": "v0.0.1"}

client.publish(devAttributes, json.dumps({"led": led}))
#Stay running so we can see message arrive
while [True]:
    client.publish(devTopics, json.dumps(telemetry))
    time.sleep(30)