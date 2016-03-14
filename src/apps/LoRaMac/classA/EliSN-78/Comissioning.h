/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2015 Semtech

Description: End device comissioning parameters

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __LORA_COMISSIONING_H__
#define __LORA_COMISSIONING_H__

/*!
 * When set to 1 the application uses the Over-the-Air activation procedure
 * When set to 0 the application uses the Personalization activation procedure
 */
#define OVER_THE_AIR_ACTIVATION                     0

/*!
 * Indicates if the end-device is to be connected to a private or public network
 */
#define LORAWAN_PUBLIC_NETWORK                      true

#if( OVER_THE_AIR_ACTIVATION != 0 )

/*!
 * Mote device IEEE EUI (big endian)
 *
 * \remark In this application the value is automatically generated by calling
 *         BoardGetUniqueId function
 */
#define LORAWAN_DEVICE_EUI                          { 0x00, 0x22, 0x44, 0x66, 0x66, 0x44, 0x22, 0x00 }

/*!
 * Application IEEE EUI (big endian)
 */
#define LORAWAN_APPLICATION_EUI                     { 0x01, 0x02, 0x03, 0x04, 0x04, 0x03, 0x02, 0x01 }

/*!
 * AES encryption/decryption cipher application key
 */
#define LORAWAN_APPLICATION_KEY                     { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }

#else

/*!
 * Current network ID
 */
#define LORAWAN_NETWORK_ID                          ( uint32_t )0

/*!
 * Device address on the network (big endian)
 *
 * \remark In this application the value is automatically generated using
 *         a pseudo random generator seeded with a value derived from
 *         BoardUniqueId value
 */
#define LORAWAN_DEVICE_ADDRESS                      ( uint32_t )0x03111103

/*!
 * AES encryption/decryption cipher network session key
 */
#define LORAWAN_NWKSKEY                             { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }

/*!
 * AES encryption/decryption cipher application session key
 */
#define LORAWAN_APPSKEY                             { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }

#endif

#endif // __LORA_COMISSIONING_H__
