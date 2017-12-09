/*
 *  SSL client demonstration program
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time       time 
#define mbedtls_time_t     time_t
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#endif

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||  \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C_RTOS) || !defined(MBEDTLS_RSA_C) ||         \
    !defined(MBEDTLS_CERTS_C) || !defined(MBEDTLS_PEM_PARSE_C) || \
    !defined(MBEDTLS_CTR_DRBG_C) || !defined(MBEDTLS_X509_CRT_PARSE_C)
int debug( void )
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
           "MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or "
           "MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
           "MBEDTLS_CTR_DRBG_C and/or MBEDTLS_X509_CRT_PARSE_C "
           "not defined.\n");
    return( 0 );
}
#else

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/threading.h"
#include "certs/resources.h"
#include "SSL/ssl_mqtt.h"
#include <string.h>

#define DEBUG_LEVEL 1
#endif

mbedtls_net_context server_fd;// = (mbedtls_net_context*)malloc(sizeof(mbedtls_net_context));
mbedtls_entropy_context entropy; // = (mbedtls_entropy_context*)malloc(sizeof(mbedtls_entropy_context));
mbedtls_ctr_drbg_context ctr_drbg;// = (mbedtls_ctr_drbg_context*)malloc(sizeof(mbedtls_ctr_drbg_context));
mbedtls_ssl_context ssl;// = (mbedtls_ssl_context *)malloc(sizeof(mbedtls_ssl_context));
mbedtls_ssl_config conf;// = (mbedtls_ssl_config*)malloc(sizeof(mbedtls_ssl_config));
mbedtls_x509_crt cacert;// = (mbedtls_x509_crt*)malloc(sizeof(mbedtls_x509_crt));
mbedtls_x509_crt clientCrt;// = (mbedtls_x509_crt*)malloc(sizeof(mbedtls_x509_crt));
mbedtls_pk_context clientPk;// = (mbedtls_pk_context*)malloc(sizeof(mbedtls_pk_context));

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    mbedtls_printf(":%04d: %s", line, str );
//    fflush(  (FILE *) ctx  );
}

mbedtls_ssl_context* SSL_ConnectionInit(const char* serverName, const char* serverPort, const char* pers )
{
    int ret;
// init mbed tls mutex function
    mbedtls_threading_set_alt( mutex_init_alt, mutex_free_alt,
                                   mutex_lock_alt, mutex_unlock_alt );
#if defined(MBEDTLS_DEBUG_C_RTOS)
    mbedtls_debug_set_threshold( DEBUG_LEVEL );
#endif

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_x509_crt_init( &cacert );
    mbedtls_x509_crt_init( &clientCrt);
    mbedtls_pk_init( &clientPk );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    mbedtls_printf( "\n  . Seeding the random number generator..." );
    mbedtls_entropy_init(&entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 0. Initialize certificates
     */
    mbedtls_printf( "  . Loading the CA root certificate ..." );
    ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) rootCa,
                          rootCaSize );
    if( ret < 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
        goto exit;
    }

    mbedtls_printf( " ok (%d skipped)\n", ret );

    /*
     * 1. Start the connection
     */
    mbedtls_printf( "  . Connecting to tcp/%s/%s...", serverName, serverPort );
//    fflush( stdout );

    if( ( ret = mbedtls_net_connect( &server_fd, serverName,
                                         serverPort, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 2. Setup stuff
     */
    mbedtls_printf( "  . Setting up the SSL/TLS structure..." );
//    fflush( stdout );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
//    if (ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char*) rootCa, rootCaSize) != 0)
//    {
//        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse root ca returned -0x%x\n\n", ret);
//        goto exit;
//    }
    
    if (ret = mbedtls_x509_crt_parse(&clientCrt, (const unsigned char*)clientCert, clientCertSize) != 0)
    {
        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse root ca returned -0x%x\n\n", ret);
        goto exit;
    }
    
    if (ret = mbedtls_pk_parse_key(&clientPk, (const unsigned char *)privateKey, privateKeySize, NULL, 0 ) != 0)
    {
        PRINTF( " failed\n  !  mbedtls_pk_parse_key private key returned -0x%x\n\n", -ret );
        goto exit;
    }
    
    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
    mbedtls_ssl_conf_own_cert( &conf, &clientCrt, &clientPk );
    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_set_hostname( &ssl, serverName ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

    /*
     * 4. Handshake
     */
    mbedtls_printf( "  . Performing the SSL/TLS handshake..." );
//    fflush( stdout );

    while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
            goto exit;
        }
    }

    mbedtls_printf( " ok\n" );
    return (&ssl);

    /*
     * 5. Verify the server certificate
     */
//    mbedtls_printf( "  . Verifying peer X.509 certificate..." );

    /* In real life, we probably want to bail out when ret != 0 */
    
//    if( ( flags = mbedtls_ssl_get_verify_result( ssl ) ) != 0 )
//    {
//        char vrfy_buf[512];
//
//        mbedtls_printf( " failed\n" );
//
//        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );
//
//        mbedtls_printf( "%s\n", vrfy_buf );
//    }
//    else
//        mbedtls_printf( " ok\n" );
//    

exit:

#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    mbedtls_net_free( &server_fd );
    mbedtls_x509_crt_free( &cacert );
    mbedtls_x509_crt_free( &clientCrt);
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
    mbedtls_pk_free( &clientPk );
//    free( server_fd);
//    free( cacert );
//    free( clientCrt );
//    free( ssl );
//    free( conf );
//    free( &ctr_drbg );
//    free( entropy );
//    free( clientPk);
    return NULL;
}
//#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_SSL_TLS_C &&
//          MBEDTLS_SSL_CLI_C && MBEDTLS_NET_C_RTOS && MBEDTLS_RSA_C &&
//          MBEDTLS_CERTS_C && MBEDTLS_PEM_PARSE_C && MBEDTLS_CTR_DRBG_C &&
//          MBEDTLS_X509_CRT_PARSE_C */
