# Add ${CMAKE_CURRENT_LIST_DIR}rary cpp files

if (NOT DEFINED CORE_MQTT_DIR)
    set(CORE_MQTT_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/coreMQTT")
endif()
if (NOT DEFINED CORE_MQTT_PORT_PATH)
    set(CORE_MQTT_PORT_PATH "${CMAKE_CURRENT_LIST_DIR}/port/coreMQTT")
endif()

message("CORE_MQTT_DIR: ${CORE_MQTT_DIR}" )
message("CORE_MQTT_PORT_PATH: ${COREMQTT_PORT_PATH}" )

add_library(coreMQTT STATIC)
include("${CORE_MQTT_DIR}/mqttFilePaths.cmake")

target_sources(coreMQTT PUBLIC
	${MQTT_SOURCES}
	${MQTT_SERIALIZER_SOURCES}
)

target_include_directories(coreMQTT PUBLIC 
	${MQTT_INCLUDE_PUBLIC_DIRS}
	${CORE_MQTT_PORT_PATH}
)


# Add the standard ${CMAKE_CURRENT_LIST_DIR}rary to the build
target_link_libraries(coreMQTT 
	FreeRTOS-Kernel
	pico_stdlib)