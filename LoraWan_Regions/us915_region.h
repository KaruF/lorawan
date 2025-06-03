/*
 * us902_928_region.h
 *
 *  Created on: Dec 7, 2024
 *      Author: PC
 */

#ifndef LORAWAN_LORAWAN_REGIONS_US915_REGION_H_
#define LORAWAN_LORAWAN_REGIONS_US915_REGION_H_

#include <stdbool.h>
#include <stdint.h>

//DEFINITIONS
#define US915_SYNC_WORD			0x34
#define US915_PREAMBLE_LENGTH	8


//TIMINGS
#define RECEIVE_DELAY1			1
#define RECEIVE_DELAY2			2 //must be RECEIVE_DELAY1 + 1s
#define JOIN_ACCEPT_DELAY1		5
#define JOIN_ACCEPT_DELAY2		6
#define MAX_FCNT_GAP			16384
#define ADR_ACK_LIMIT			64
#define ADR_ACK_DELAY			32
#define ACK_TIMEOUT				2 //(random delay between 1 and 3 seconds)

//DEFAULT TX POWER
#define DEFAULT_TX_POWER	20 //(dBm)

//maximum payload length
struct us915{

	uint8_t data_rate;
	uint8_t tx_power;

	uint8_t ChMaskCntl;

};





#endif /* LORAWAN_LORAWAN_REGIONS_US915_REGION_H_ */
