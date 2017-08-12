/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"



#define MQTT_TASK 1
#include "MQTTClient.h"
#include "MQTTEcho.h"
#include "lwip/dns.h"


void messageArrived(MessageData* data)
{
	PRINTF("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
		data->message->payloadlen, data->message->payload);
}

void prvMQTTEchoTask(void *pvParameters)
{
	/* connect to m2m.eclipse.org, subscribe to a topic, send and receive messages regularly every 1 sec */
	MQTTClient client;
	Network network;
	unsigned char sendbuf[80], readbuf[80];
	int rc = 0, 
		count = 0;
        uint32_t taskNotifyValue;
        if (xTaskNotifyWait(0x01, 0x01, &taskNotifyValue, pdMS_TO_TICKS(5000)) == pdFALSE)
        {
            vTaskDelete(NULL);
        }
        PRINTF("START MQTT Task\r\n");
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	pvParameters = 0;
	NetworkInit(&network);
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	char* address = "iot.eclipse.org";
        uint16_t port = 1883;
        ip_addr_t primaryDnsServer;
        // set up dns server.
        ip4_addr_set_u32(&primaryDnsServer, ipaddr_addr("8.8.8.8"));    
        dns_setserver(0, &primaryDnsServer);
        dns_init();
        PRINTF("connect to broker @ %s:%d\r\n", address, port);
	if ((rc = NetworkConnect(&network, address, port)) != 0)
        {
		PRINTF("Return code from network connect is %d\n", rc);
                vTaskDelete(NULL);
        }

#if defined(MQTT_TASK)
	if ((rc = MQTTStartTask(&client)) != pdPASS)
        {
		PRINTF("Return code from start tasks is %d\r\n", rc);
                vTaskDelete(NULL);
        }
#endif

	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "FreeRTOS_sample";

	if ((rc = MQTTConnect(&client, &connectData)) != 0)
        {
		PRINTF("Return code from MQTT connect is %d\r\n", rc);
#if defined(MQTT_TASK)
                vTaskDelete(NULL);
#endif
        }
	else
		PRINTF("MQTT Connected\n");

	if ((rc = MQTTSubscribe(&client, "FreeRTOS/sample/#", 2, messageArrived)) != 0)
		PRINTF("Return code from MQTT subscribe is %d\n", rc);

	while (1)
	{
		MQTTMessage message;
		char payload[30];

		message.qos = 1;
		message.retained = 0;
		message.payload = payload;
		sprintf(payload, "message number %d", ++count);
		message.payloadlen = strlen(payload);

		if ((rc = MQTTPublish(&client, "FreeRTOS/sample/a", &message)) != 0)
			PRINTF("Return code from MQTT publish is %d\n", rc);
#if !defined(MQTT_TASK)
		if ((rc = MQTTYield(&client, 1000)) != 0)
			PRINTF("Return code from yield is %d\n", rc);
#endif
	}
}


void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
	BaseType_t x = 0L;

	xTaskCreate(prvMQTTEchoTask,	/* The function that implements the task. */
			"MQTTEcho0",			/* Just a text name for the task to aid debugging. */
			usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
			(void *)x,		/* The task parameter, not used in this case. */
			uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
			NULL);				/* The task handle is not used. */
}
/*-----------------------------------------------------------*/


