/* Automatically generated file - this comment ensures resources.h file creation */ 
/* Auto-generated header file. Do not edit */
#ifndef INCLUDED_RESOURCES_H_
#define INCLUDED_RESOURCES_H_
//#include "wiced_resource.h"
#include <stddef.h>
/**
 * Memory handle
 */
typedef struct
{
    /*@shared@*/ const char* data;
} memory_resource_handle_t;

typedef struct {
    unsigned long size;
    union {
        memory_resource_handle_t mem;
    } val;
} resource_hnd_t;

extern char rootCa[];
extern char clientCert[];
extern char privateKey[];
extern const size_t rootCaSize;
extern const size_t clientCertSize;
extern const size_t privateKeySize;
#endif /* ifndef INCLUDED_RESOURCES_H_ */
