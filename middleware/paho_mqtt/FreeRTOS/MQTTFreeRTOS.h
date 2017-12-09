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
 *******************************************************************************/

#if !defined(MQTTFreeRTOS_H)
#define MQTTFreeRTOS_H
#include "FreeRTOS.h"
/* This version of free rtos has no socket libary - use lwIP library instead */
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/def.h"
//#include "FreeRTOS_Sockets.h"
//#include "FreeRTOS_IP.h"
#include "semphr.h"
#include "task.h"
#ifdef MQTT_USE_TLS
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#endif
     
typedef int xSocket_t;
typedef struct Timer 
{
	TickType_t xTicksToWait;
	TimeOut_t xTimeOut;
} Timer;

typedef struct Network Network;

struct Network
{
#ifdef MQTT_USE_TLS
    mbedtls_ssl_context* ssl;
#else
    xSocket_t my_socket;  //socket by freeRTOS   
#endif // MQTT_USE_TLS     
    int (*mqttread) (Network*, unsigned char*, int, int);
    int (*mqttwrite) (Network*, unsigned char*, int, int);
    void (*disconnect) (Network*);
};

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

typedef struct Mutex
{
	SemaphoreHandle_t sem;
} Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);

typedef struct Thread
{
	TaskHandle_t task;
} Thread;

int ThreadStart(Thread*, void (*fn)(void*), void* arg);

int FreeRTOS_read(Network*, unsigned char*, int, int);
int FreeRTOS_write(Network*, unsigned char*, int, int);
void FreeRTOS_disconnect(Network*);

void NetworkInit(Network*);
#ifndef MQTT_USE_TLS
int NetworkConnect(Network*, char*, uint16_t);
#else
int NetworkConnect(Network*, char*, char*, char*);
#endif
/*int NetworkConnectTLS(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);*/

#endif
