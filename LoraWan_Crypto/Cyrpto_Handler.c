/*
 * LoraWan_aes128.c
 *
 *  Created on: Dec 22, 2024
 *      Author: PC
 */


#include <Crypto_Handler.h>




uint32_t aes128_cmac(uint8_t *key, uint8_t *buffer, uint8_t buffer_size){


	if(key == NULL || buffer == NULL)
		return CYRYPTO_NULL_BUFFER_ERROR;

	AES_CMAC_CTX ctx;
	AES_CMAC_Init(&ctx);

	AES_CMAC_SetKey(&ctx, key);

	AES_CMAC_Update(&ctx, buffer, buffer_size);

	uint8_t Cmac[16];
	AES_CMAC_Final(Cmac, &ctx);


	return  (uint32_t)((uint32_t)Cmac[3]<<24 | (uint32_t)Cmac[2]<<16 | (uint32_t)Cmac[1]<<8 | (uint32_t)Cmac[0]);
}


uint32_t aes128_encrypt(uint8_t *key, uint8_t *in_buffer, uint8_t buffer_size, uint8_t *out_buffer){


	if(key == NULL || in_buffer == NULL || out_buffer == NULL)
		return CYRYPTO_NULL_BUFFER_ERROR;

    if((buffer_size % 16) != 0)
    {
        return CRYPTO_ERROR_BUF_SIZE;
    }

    lorawan_aes_context aesContext;
    memset1( aesContext.ksch, '\0', 240 ); //'\0'

	lorawan_aes_set_key(key, 16, &aesContext);

	uint8_t block = 0;
    while(buffer_size != 0)
    {
        lorawan_aes_encrypt(&in_buffer[block], &out_buffer[block], &aesContext);
        block = block + 16;
        buffer_size  = buffer_size - 16;
    }


	return 0;
}










