/*
 * loramac.h
 *
 *  Created on: Dec 7, 2024
 *      Author: PC
 */

#ifndef LORAWAN_LORAMAC_LORAMAC_H_
#define LORAWAN_LORAMAC_LORAMAC_H_

//INCLUDES

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "LoraMac_Commands.h"
#include <Crypto_Handler.h>
#include "us915_region.h"


//MACRO DEFINITIONS
#define APPNONCE_SIZE_BYTE		3
#define NETID_SIZE_BYTE			3
#define DEVADDR_SIZE_BYTE		4
#define CFLIST_MAX_SIZE_BYTE 	16



#define FOPTS_SIZE_BYTE			15

#define BZERO_SIZE_BYTE			16




//ENUM DECLARATIONS

enum Transfer_Type{

	UPLINK,
	DOWNLINK,
	RETRANSMISSION,
	JOIN_DONE,

};

enum Serializer_Task_Id{

	TASK_DATA,
	TASK_JOIN_REQUEST,
	TASK_FHDR,
	TASK_ADD_DATA_MIC,
	TASK_ADD_REQ_MIC,

};

enum Deserializer_Task_Id{

	TASK_JOIN_ACCEPT,

};


enum LoraMac_Error_States{

	LORAMAC_OK,
	LORAMAC_GENERIC_ERR,

	LORAMAC_NULL_STRUCT,
	LORAMAC_HEAP_ERROR,

	LORAMAC_IS_NOT_JOINED,
	LORAMAC_IS_ALREADY_JOINED,

	LORAMAC_JOIN_REQUEST_PACKET_READY,
	LORAMAC_JOIN_ACCEPT_MIC_ERROR,

	LORAMAC_SERIALIZED_SUCCESS,
	LORAMAC_SERIALIZED_UNSUCCESS,

	LORAMAC_WRONG_FPORT_VALUE,
	LORAMAC_FPORT_OK,

};

enum MHDR_mtype_field{

	JOIN_REQUEST,
	JOIN_ACCEPT,
	UNCONFIRMED_DATA_UP,
	UNCONFIRMED_DATA_DOWN,
	CONFIRMED_DATA_UP,
	CONFIRMED_DATA_DOWN,
	RFU_MTYPE,
	PROPRIETARY,

};

enum MHDR_major_field{

	LORAWAN_R1,
	RFU_MAJOR,

};


//STRUCT DECLARATIONS

struct FCtrl_downlink_str{

	uint8_t ADR;
	uint8_t ADRACKReq;
	uint8_t ACK;
	uint8_t RFU;
	uint8_t FOptsLen;

};

struct FCtrl_uplink_str{

	uint8_t ADR;
	uint8_t RFU;
	uint8_t ACK;
	uint8_t FPending;
	uint8_t FOptsLen;

};

struct FHDR_str{

	uint32_t DevAddr;
	struct FCtrl_downlink_str FCTRL_DOWN;
	struct FCtrl_uplink_str FCTRL_UP;
	uint32_t FCnt;
	uint8_t FOpts[FOPTS_SIZE_BYTE];

	uint8_t total_FHDR_size;
};

struct MHDR_str{

	uint8_t MType;
	uint8_t Major; //It must be 0

};

struct MACPayload_str{

	struct FHDR_str FHDR;
	uint8_t FPort;

	uint8_t *FRMPayload; //adjust payload lenght
	uint8_t *encrypted_FRMPayload;
	uint8_t FRMPayload_size;

	uint32_t macpayload_mic;
};

struct Join_Req_str{

	uint8_t req_mhdr;
	uint8_t AppEUI[APPEUI_SIZE_BYTE];
	uint8_t DevEUI[DEVEUI_SIZE_BYTE];
	uint16_t DevNonce; //should be random and unique for every join request

	uint32_t request_mic;

};

struct Join_Res_str{

	uint8_t res_mhdr;

	uint8_t AppNonce[APPNONCE_SIZE_BYTE];
	uint8_t NetID[NETID_SIZE_BYTE];
	uint8_t DevAddr[DEVADDR_SIZE_BYTE];
	uint8_t DLSettings;
	uint8_t RxDelay;
	uint8_t CFlist[CFLIST_MAX_SIZE_BYTE];

	uint32_t response_mic;

};

//struct MIC_str{
//
//	uint32_t total_msg; //msg = MHDR | FHDR | FPort | FRMPayload
//	uint8_t B_zero[BZERO_SIZE_BYTE]; //cmac = aes128_cmac(NwkSKey, B0 | msg), then MIC = cmac[0..3]
//
//	uint32_t calculated_mic;
//	uint32_t received_mic;
//};

struct serialized_datas{

	uint8_t seralized_mhdr;

	uint8_t* serialized_join_req;
	uint8_t serialized_join_req_size;

	uint8_t *serialized_raw; //raw payload
	uint8_t serialized_raw_size;

	uint8_t *serialized_macpayload; //encrypted payload
	uint8_t serialized_macpayload_size;

	uint8_t *serialized_fhdr;
	uint8_t serialized_fhdr_size;

//	uint32_t serialized_mic;
//	uint8_t serialized_mic_size;

};

struct Payload_Str{

	struct MHDR_str mhdr_str;
	struct MACPayload_str macpayload_str;
	struct Join_Req_str join_req_str;
	struct Join_Res_str join_res_str;
//	struct MIC_str mic_str;

	struct serialized_datas serial_data;
};


struct loramac_state{

	bool is_join;

};



//FUNCTIONS DECLARATIONS

struct Payload_Str *LoraMac_Init();

enum LoraMac_Error_States Set_MHDR(struct Payload_Str *payload_str, uint8_t mtype, uint8_t major);
enum LoraMac_Error_States Set_FHDR(struct Payload_Str *payload_str, bool adr, enum Transfer_Type trans_type, bool mac_command);
enum LoraMac_Error_States Set_FPort(struct Payload_Str *payload_str, uint8_t port_value);
enum LoraMac_Error_States Set_FRMPayload(struct Payload_Str *payload_str, uint8_t *data, uint8_t data_size);
enum LoraMac_Error_States Set_MacPayload(struct Payload_Str *payload_str);
enum LoraMac_Error_States Calculate_Data_Mic(struct Payload_Str *payload_str);

enum LoraMac_Error_States Send_Join_Request(struct Payload_Str *payload_str);
enum LoraMac_Error_States Join_Accept_Handler(struct Payload_Str *payload_str, uint8_t *data, uint8_t size);


enum LoraMac_Error_States LoraMac_Serializer(struct Payload_Str *payload_str, enum Serializer_Task_Id serializer_task_id);
enum LoraMac_Error_States LoraMac_Deserializer(struct Payload_Str *payload_str, uint8_t *data, uint8_t size, enum Deserializer_Task_Id deserializer_task_id);

uint8_t Is_Join();
void  Set_Join(bool join_state);

enum LoraMac_Error_States Set_DevAddr(uint32_t devaddr);
enum LoraMac_Error_States Set_AppEUI(uint8_t *appeui);
enum LoraMac_Error_States Set_DevEUI(uint8_t *deveui);
enum LoraMac_Error_States Set_AppKey(uint8_t *appkey);
enum LoraMac_Error_States Set_AppsKey(uint8_t *appskey);
enum LoraMac_Error_States Set_Nwkskey(uint8_t *nwkskey);

void LoraMac_Error_Handler();

#endif /* LORAWAN_LORAMAC_LORAMAC_H_ */
