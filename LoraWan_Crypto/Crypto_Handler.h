/*
 * LoraWan_aes128.h
 *
 *  Created on: Dec 22, 2024
 *      Author: PC
 */

#ifndef LORAWAN_LORAWAN_CRYPTO_CRYPTO_HANDLER_H_
#define LORAWAN_LORAWAN_CRYPTO_CRYPTO_HANDLER_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cmac.h"
#include "lorawan_aes.h"
#include "utilities.h"

//MACRO DEFINITIONS

#define MHDR_SIZE_BYTE			1
#define APPEUI_SIZE_BYTE		8
#define DEVEUI_SIZE_BYTE		8
#define DEVNONCE_SIZE_BYTE		2

#define MIC_SIZE_BYTE			4

#define TOTAL_JOIN_REQUEST_SIZE	 MHDR_SIZE_BYTE + APPEUI_SIZE_BYTE + DEVEUI_SIZE_BYTE + DEVNONCE_SIZE_BYTE



//ENUM DEFINITIONS

enum Crypto_Error_States{

	CRYPTO_OK,

	CYRYPTO_NULL_BUFFER_ERROR,
	CRYPTO_ERROR_BUF_SIZE,

};



//FUNCTIONS DECLARATIONS
uint32_t aes128_cmac(uint8_t *key, uint8_t *buffer, uint8_t buffer_size);

uint32_t aes128_encrypt(uint8_t *key, uint8_t *in_buffer, uint8_t buffer_size, uint8_t *out_buffer);

#endif /* LORAWAN_LORAWAN_CRYPTO_CRYPTO_HANDLER_H_ */
