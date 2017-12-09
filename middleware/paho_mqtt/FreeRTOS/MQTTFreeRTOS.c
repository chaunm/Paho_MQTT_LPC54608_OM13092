/*******************************************************************************
* Copyright (c) 2014, 2015 IBM Corp.
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* and Eclipse Distribution License v1.0 which accompany this distribution.
*
* The Eclipse Public License is available at
*    http://www.eclipse.org/legal/epl-v10.html
* and the Eclipse Distribution License is available at
*   http://www.eclipse.org/org/documents/edl-v10.php.
*
* Contributors:
*    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
*    Ian Craggs - convert to FreeRTOS
*******************************************************************************/

#include "MQTTFreeRTOS.h"
#ifdef MQTT_USE_TLS
#include "SSL/ssl_mqtt.h"
#endif


int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
    int rc = 0;
    uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
    UBaseType_t uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/
    if (uxTaskPriority > 0) // chaunm add
        uxTaskPriority--;
    
    rc = xTaskCreate(fn,	/* The function that implements the task. */
                     "MQTTTask",			/* Just a text name for the task to aid debugging. */
                     usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
                     arg,				/* The task parameter, not used in this case. */
                     uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
                     &thread->task);		/* The task handle is not used. */
    
    return rc;
}


void MutexInit(Mutex* mutex)
{
    mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
    return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int MutexUnlock(Mutex* mutex)
{
    return xSemaphoreGive(mutex->sem);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
    timer->xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
    vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}


void TimerCountdown(Timer* timer, unsigned int timeout) 
{
    TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(Timer* timer) 
{
    xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
    return (timer->xTicksToWait <= 0) ? 0 : (timer->xTicksToWait * portTICK_PERIOD_MS);
}


char TimerIsExpired(Timer* timer)
{
    return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}


void TimerInit(Timer* timer)
{
    timer->xTicksToWait = 0;
    memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}


int FreeRTOS_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
    TimeOut_t xTimeOut;
    int recvLen = 0;
    
    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
    do
    {
        int rc = 0;
#ifndef MQTT_USE_TLS
        lwip_setsockopt(n->my_socket, 0, SO_RCVTIMEO, &xTicksToWait, sizeof(xTicksToWait));;
        rc = lwip_recv(n->my_socket, buffer + recvLen, len - recvLen, 0);
#else
        setsockopt(((mbedtls_net_context*)n->ssl->p_bio)->fd, 0, SO_RCVTIMEO, &xTicksToWait, sizeof(xTicksToWait));
        rc = mbedtls_ssl_read(n->ssl, buffer + recvLen, len - recvLen);
#endif
        if (rc > 0)
            recvLen += rc;
        else if (rc < 0)
        {
            recvLen = rc;
            break;
        }
    } while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
    return recvLen;
}


int FreeRTOS_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
    TimeOut_t xTimeOut;
    int sentLen = 0;
    
    vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
    do
    {
        int rc = 0;
#ifndef MQTT_USE_TLS
        lwip_setsockopt(n->my_socket, 0, SO_SNDTIMEO, &xTicksToWait, sizeof(xTicksToWait));
        rc = lwip_send(n->my_socket, buffer + sentLen, len - sentLen, 0);
#else
        setsockopt(((mbedtls_net_context*)n->ssl->p_bio)->fd, 0, SO_SNDTIMEO, &xTicksToWait, sizeof(xTicksToWait));
        rc = mbedtls_ssl_write(n->ssl, buffer + sentLen, len - sentLen);
#endif
        if (rc > 0)
            sentLen += rc;
        else if (rc < 0)
        {
            sentLen = rc;
            break;
        }
    } while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
    
    return sentLen;
}


void FreeRTOS_disconnect(Network* n)
{
//	FreeRTOS_closesocket(n->my_socket);
#ifndef MQTT_USE_TLS
    lwip_close(n->my_socket);
#endif
}


void NetworkInit(Network* n)
{
#ifndef MQTT_USE_TLS
    n->my_socket = 0;
#else
    n->ssl = NULL;
#endif
    n->mqttread = FreeRTOS_read;
    n->mqttwrite = FreeRTOS_write;
    n->disconnect = FreeRTOS_disconnect;
}

#ifdef MQTT_USE_TLS
int NetworkConnect(Network* n, char* addr, char* port, char* pers)
#else
int NetworkConnect(Network* n, char* addr, uint16_t port)
#endif
{
#ifndef MQTT_USE_TLS
    struct hostent *hostAddr;
    struct sockaddr_in sAddr;
    int retVal = -1;
    struct in_addr inAddr;
    uint32_t serverIP;
    
    if ((hostAddr = lwip_gethostbyname(addr)) == 0)
        goto exit;
    inAddr.s_addr = *((uint32_t*)hostAddr->h_addr);
    serverIP = inAddr.s_addr;
    PRINTF("host IP address %d.%d.%d.%d\r\n", serverIP & 0x000000FF, (serverIP & 0x0000FF00) >> 8,
           (serverIP & 0x00FF0000) >> 16, (serverIP & 0xFF000000) >> 24);
    sAddr.sin_len = sizeof(sAddr);
    sAddr.sin_port = lwip_htons(port); 
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = (serverIP);
    if ((n->my_socket = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        goto exit;
    
    if ((retVal = lwip_connect(n->my_socket, (struct sockaddr*)&sAddr, sizeof(sAddr))) < 0)
    {
        lwip_close(n->my_socket);
        goto exit;
    }
exit:
    return retVal;
#else
    n->ssl = SSL_ConnectionInit(addr, port, pers);
    if (n->ssl != NULL)
    {
        return 0;
    }
    else
        return -1;
#endif // !defined(MQTT_USE_TLS)
}