#!/usr/bin/env python3
# Testing out a Thingboard.io device using the Thingboard Python MQTT Client library
# Services tested:
# Telematry
# Attributes
# Server Side RPC


from tb_device_mqtt import TBDeviceMqttClient, TBPublishInfo
import os
import time
import sys

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

MQTT_BROKER = host
MQTT_PORT = port

LED = False

def on_attributes_change(result, *args):
  print(result)

def on_server_side_rpc_request(request_id, request_body):
  print(request_id, request_body)
  if request_body["method"] == "reboot":
     print("Rebooting device...")
     client.disconnect()
     os._exit(0)
      


telemetry = {"temperature": 41.9, "enabled": False, "currentFirmwareVersion": "v1.2.2"}
client = TBDeviceMqttClient(
   MQTT_BROKER, 
   port=MQTT_PORT, 
   username=user)
client.set_server_side_rpc_request_handler(on_server_side_rpc_request)
# Connect to ThingsBoard
client.connect()

client.subscribe_to_attribute("led", on_attributes_change)
client.request_attributes(["led"], callback=on_attributes_change)

while [not client.is_connected()]:
    # Sending telemetry without checking the delivery status
    client.send_telemetry(telemetry)
    # Sending telemetry and checking the delivery status (QoS = 1 by default)
    result = client.send_telemetry(telemetry)
    # get is a blocking call that awaits delivery status
    success = result.get() == TBPublishInfo.TB_ERR_SUCCESS
    time.sleep(1)
# Disconnect from ThingsBoard
client.disconnect()
