#ifndef __SSL_MQTT_H__
#define __SSL_MQTT_H__

mbedtls_ssl_context* SSL_ConnectionInit(const char* serverName, const char* serverPort, const char* pers );

#endif // __SSL_MQTT_H__