#ifndef __MQTTECHO_H__
#define __MQTTECHO_H__
#define APP_QOS         QOS1

void prvMQTTEchoTask(void *pvParameters);
void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority);
#endif