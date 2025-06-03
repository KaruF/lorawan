/*
 * loramac.c
 *
 *  Created on: Dec 7, 2024
 *      Author: PC
 */


#include <LoraMac.h>


//STATICS FUNCSTIONS DECLARATIONS
static enum LoraMac_Error_States Compare_joinresponse_mic(struct Payload_Str *payload_str, uint8_t *in_buffer, uint8_t size);


//STATICS VARIABLES
//Assigned before join
static uint8_t DevEUI[8];
static uint8_t AppEUI[8];
static uint8_t AppKey[16];

//will be assigned after joining
static uint32_t DevAddr;
static uint8_t NwksKey[16];
static uint8_t AppSKey[16];

static struct loramac_state lora_state;

/*brief:
 *
 *
 *
 * */
struct Payload_Str *LoraMac_Init(){

	struct Payload_Str *p = calloc(1,sizeof(struct Payload_Str));

	if(p == NULL)
		LoraMac_Error_Handler();

	lora_state.is_join = false;

	srand(1233);

	return p;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_MHDR(struct Payload_Str *payload_str, uint8_t mtype, uint8_t major){

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;

	payload_str->mhdr_str.MType = mtype;
	payload_str->mhdr_str.Major = major;

	return 0;
}



/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_FHDR(struct Payload_Str *payload_str, bool adr, enum Transfer_Type trans_type, bool mac_command){

	static uint16_t uplink_fcnt = 0;

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;

	if(!(lora_state.is_join))
		return LORAMAC_IS_NOT_JOINED;

	payload_str->macpayload_str.FHDR.DevAddr = DevAddr;

	//do a job to donwloik ftcrl or uplink fctrl
	payload_str->macpayload_str.FHDR.FCTRL_UP.ADR = adr;

	if(uplink_fcnt > MAX_FCNT_GAP); //TAKE ACTION, probably there is data loss!

	switch(trans_type){
		case UPLINK:
			uplink_fcnt++;
			payload_str->macpayload_str.FHDR.FCnt = uplink_fcnt;
			break;
		case DOWNLINK:

			break;
		case RETRANSMISSION:
			//do not change FCNT!
			break;
		case JOIN_DONE:
			uplink_fcnt = 0;
			payload_str->macpayload_str.FHDR.FCnt = uplink_fcnt;
			break;
	}

	if(mac_command){

		//FoptsLen can not be
		//and Fopts must include mac command!
		//and Fport can not be 0!
	}

	payload_str->macpayload_str.FHDR.total_FHDR_size = 7;//fill macpayload fhdr size!!!!

	//then call serialized fhdr!!!
	LoraMac_Serializer(payload_str, TASK_FHDR);

	return 0;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_FPort(struct Payload_Str *payload_str, uint8_t port_value){

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;

	if(port_value >= 224)
		return LORAMAC_WRONG_FPORT_VALUE; //Actually it is not wrong value, this values has specific mission that we will cover later!

	payload_str->macpayload_str.FPort = port_value;

	return LORAMAC_FPORT_OK;
}


/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_FRMPayload(struct Payload_Str *payload_str, uint8_t *data, uint8_t data_size){

	if(payload_str == NULL || data == NULL)
		return LORAMAC_NULL_STRUCT;

	//Check if requested data size conforms with lorawan!

	payload_str->macpayload_str.FRMPayload_size = data_size;

	 if(payload_str->macpayload_str.FRMPayload != NULL)
	    	free(payload_str->macpayload_str.FRMPayload);

	 payload_str->macpayload_str.FRMPayload = malloc(payload_str->macpayload_str.FRMPayload_size);

	 memcpy(payload_str->macpayload_str.FRMPayload, data, payload_str->macpayload_str.FRMPayload_size); //save raw data to mic calculation


	//data must be encrypted by end device!
	uint8_t *key;

	if(payload_str->macpayload_str.FPort == 0)
		key = NwksKey;
	else
		key = AppSKey;


	uint8_t Ai_array[16] = {0};
	uint8_t Si_array[16] = {0};
	uint8_t index = 1;
	uint8_t buffer_iterator = 0;


	Ai_array[0] = 0x01;
	Ai_array[5] = 0; // it is for uplink

	Ai_array[6] = DevAddr & 0xFF;
	Ai_array[7] = (DevAddr >> 8) & 0xFF;
	Ai_array[8] = (DevAddr >> 16) & 0xFF;
	Ai_array[9] = (DevAddr >> 24) & 0xFF;

	Ai_array[10] = (payload_str->macpayload_str.FHDR.FCnt) & 0xFF;
	Ai_array[11] = ((payload_str->macpayload_str.FHDR.FCnt) >> 8) & 0xFF;
	Ai_array[12] = ((payload_str->macpayload_str.FHDR.FCnt) >> 16) & 0xFF;
	Ai_array[13] = ((payload_str->macpayload_str.FHDR.FCnt) >> 24) & 0xFF;

    while(data_size > 0)
    {
    	Ai_array[15] = index & 0xFF;
    	index++;

        aes128_encrypt(key, Ai_array, 16, Si_array);

        for(uint8_t i=0; i<((data_size > 16) ? 16 : data_size); i++)
        {
        	data[buffer_iterator + i] = data[buffer_iterator + i] ^ Si_array[i];
        }
        data_size -= 16;
        buffer_iterator += 16;
    }



    if(payload_str->macpayload_str.encrypted_FRMPayload != NULL)
    	free(payload_str->macpayload_str.encrypted_FRMPayload);

    payload_str->macpayload_str.encrypted_FRMPayload = malloc(payload_str->macpayload_str.FRMPayload_size);

    memcpy(payload_str->macpayload_str.encrypted_FRMPayload, data, payload_str->macpayload_str.FRMPayload_size);


	return LORAMAC_OK;
}


enum LoraMac_Error_States Set_MacPayload(struct Payload_Str *payload_str){

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;


	if(LoraMac_Serializer(payload_str, TASK_DATA) != LORAMAC_SERIALIZED_SUCCESS){

		return LORAMAC_SERIALIZED_UNSUCCESS;
	}

	Calculate_Data_Mic(payload_str);

	if(LoraMac_Serializer(payload_str, TASK_ADD_DATA_MIC) != LORAMAC_SERIALIZED_SUCCESS){

		return LORAMAC_SERIALIZED_UNSUCCESS;
	}

	return LORAMAC_OK;
}
/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Calculate_Data_Mic(struct Payload_Str *payload_str){

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;

	//check serialized data macpayload
	uint8_t size_B0 = payload_str->serial_data.serialized_macpayload_size + 16 - MIC_SIZE_BYTE;//payload_str->serial_data.serialized_raw_size + 16;
//	size_B0 += (16 - (size_B0 % 16)); //!!!!!!!!!!!!!!!! is true?

	uint8_t *B0_array = calloc(1,size_B0);
	if(B0_array == NULL)
		return LORAMAC_NULL_STRUCT;

	B0_array[0] = 0x49;

	B0_array[1] = 0x00;
	B0_array[2] = 0x00;
	B0_array[3] = 0x00;
	B0_array[4] = 0x00;
	B0_array[5] = 0x00; //for uplink

	B0_array[6] = DevAddr & 0xFF;
	B0_array[7] = (DevAddr >> 8) & 0xFF;
	B0_array[8] = (DevAddr >> 16) & 0xFF;
	B0_array[9] = (DevAddr >> 24) & 0xFF;

	B0_array[10] = (payload_str->macpayload_str.FHDR.FCnt) & 0xFF;
	B0_array[11] = ((payload_str->macpayload_str.FHDR.FCnt) >> 8) & 0xFF;
	B0_array[12] = ((payload_str->macpayload_str.FHDR.FCnt) >> 16) & 0xFF;
	B0_array[13] = ((payload_str->macpayload_str.FHDR.FCnt) >> 24) & 0xFF;

	B0_array[14] = 0x00;

	B0_array[15] = payload_str->serial_data.serialized_macpayload_size - MIC_SIZE_BYTE;//payload_str->serial_data.serialized_raw_size;


	//memcpy(&B0_array[16], payload_str->serial_data.serialized_raw, payload_str->serial_data.serialized_raw_size);
	memcpy(&B0_array[16], payload_str->serial_data.serialized_macpayload, B0_array[15]);

	payload_str->macpayload_str.macpayload_mic = aes128_cmac(NwksKey, B0_array, size_B0); // - MIC_SIZE_BYTE

	free(B0_array);

	return LORAMAC_OK;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Send_Join_Request(struct Payload_Str *payload_str){

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;

	if(lora_state.is_join)
		return LORAMAC_IS_ALREADY_JOINED;

	Set_MHDR(payload_str, JOIN_REQUEST, LORAWAN_R1);

	//Join request message consists of 8 byte AppEUI, 8 byte DevEUI, 2 byte DevNonce
	memcpy(payload_str->join_req_str.AppEUI, AppEUI, sizeof(AppEUI));
	memcpy(payload_str->join_req_str.DevEUI, DevEUI, sizeof(DevEUI));

	//The DevNonce can be extracted by issuing a sequence of RSSI measurements under the
	//assumption that the quality of randomness fulfills the criteria of true randomness

	payload_str->join_req_str.DevNonce = rand() % 65535; // we assign random number. We might take care of it later releases of this stack!


	//cmac = aes128_cmac(AppKey, MHDR | AppEUI | DevEUI | DevNonce)
	//MIC = cmac[0..3]
	if(LoraMac_Serializer(payload_str, TASK_JOIN_REQUEST) != LORAMAC_SERIALIZED_SUCCESS){

		return LORAMAC_SERIALIZED_UNSUCCESS;
	}

	payload_str->join_req_str.request_mic = aes128_cmac(AppKey, payload_str->serial_data.serialized_join_req, TOTAL_JOIN_REQUEST_SIZE);


	if(LoraMac_Serializer(payload_str, TASK_ADD_REQ_MIC) != LORAMAC_SERIALIZED_SUCCESS){

			return LORAMAC_SERIALIZED_UNSUCCESS;
	}

	return LORAMAC_JOIN_REQUEST_PACKET_READY;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Join_Accept_Handler(struct Payload_Str *payload_str, uint8_t *data, uint8_t size){

	if(payload_str == NULL || data == NULL)
		return LORAMAC_NULL_STRUCT;

	if(lora_state.is_join)
		return LORAMAC_IS_ALREADY_JOINED;

	//	uint8_t in[16] = {0x4F, 0x4D, 0x45, 0x52, 0x46, 0x41, 0x52, 0x55, 0x4B, 0x42, 0x55, 0x4C, 0x55, 0x54, 0x4F, 0x4D};
	//	uint8_t out[16];
	//	aes128_encrypt(AppKey, in,16,out);

	uint8_t *out_data = malloc(size);
	if(out_data == NULL)
		return LORAMAC_NULL_STRUCT;

	memcpy(out_data, data, size);
	aes128_encrypt(AppKey, &data[1], size, &out_data[1]); //data[1] because mhdr is not crypted by network. Just for join_response packet crypted!

	LoraMac_Deserializer(payload_str, out_data, size, TASK_JOIN_ACCEPT);

	DevAddr = (payload_str->join_res_str.DevAddr[0] & 0xff) | (payload_str->join_res_str.DevAddr[1] << 8) |
			(payload_str->join_res_str.DevAddr[2] << 16) | (payload_str->join_res_str.DevAddr[3] << 24);

//	if(Compare_joinresponse_mic(payload_str, data, size) != LORAMAC_OK)
//		return LORAMAC_JOIN_ACCEPT_MIC_ERROR;

	uint8_t key_in_buffer[16] = {0x01, payload_str->join_res_str.AppNonce[0], payload_str->join_res_str.AppNonce[1], payload_str->join_res_str.AppNonce[2],
			payload_str->join_res_str.NetID[0], payload_str->join_res_str.NetID[1], payload_str->join_res_str.NetID[2], payload_str->join_req_str.DevNonce & 0xff,
			(payload_str->join_req_str.DevNonce >> 8) & 0xff};
	aes128_encrypt(AppKey, key_in_buffer, 16, NwksKey); //nwskey


	key_in_buffer[0] = 0x02;
	aes128_encrypt(AppKey, key_in_buffer, 16, AppSKey); //appskey


	lora_state.is_join = true;

	free(out_data);

	return 0;
}


/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States LoraMac_Serializer(struct Payload_Str *payload_str, enum Serializer_Task_Id serializer_task_id){

	if(payload_str == NULL)
		return LORAMAC_NULL_STRUCT;

	switch(serializer_task_id){
	case TASK_JOIN_REQUEST:

		//payload_str->serial_data.seralized_mhdr = (payload_str->mhdr_str.MType << 5) | (payload_str->mhdr_str.Major); //add it to directly serialized join_req!
		if(payload_str->serial_data.serialized_join_req != NULL)
			free(payload_str->serial_data.serialized_join_req); //Let's free up memory that allocated previous join request!

		payload_str->serial_data.serialized_join_req_size = TOTAL_JOIN_REQUEST_SIZE + MIC_SIZE_BYTE;

		payload_str->serial_data.serialized_join_req = malloc(sizeof(payload_str->serial_data.serialized_join_req_size));

		if(payload_str->serial_data.serialized_join_req == NULL)
			return LORAMAC_SERIALIZED_UNSUCCESS;

	    uint8_t buff_iteration = 0;

	    payload_str->serial_data.serialized_join_req[buff_iteration++] = (payload_str->mhdr_str.MType << 5) | (payload_str->mhdr_str.Major);//payload_str->serial_data.seralized_mhdr;

		memcpy(&payload_str->serial_data.serialized_join_req[buff_iteration], AppEUI, APPEUI_SIZE_BYTE);
		buff_iteration += APPEUI_SIZE_BYTE;

		memcpy(&payload_str->serial_data.serialized_join_req[buff_iteration], DevEUI, DEVEUI_SIZE_BYTE);
		buff_iteration += DEVEUI_SIZE_BYTE;

		payload_str->serial_data.serialized_join_req[buff_iteration++] = payload_str->join_req_str.DevNonce & 0xFF;
		payload_str->serial_data.serialized_join_req[buff_iteration++] = (payload_str->join_req_str.DevNonce >> 8) & 0xFF;


		break;
	case TASK_FHDR:

		//serialized_fhdr

		payload_str->serial_data.serialized_fhdr_size = payload_str->macpayload_str.FHDR.total_FHDR_size;

		payload_str->serial_data.serialized_fhdr = malloc(payload_str->serial_data.serialized_fhdr_size);
		if(payload_str->serial_data.serialized_fhdr  == NULL)
			return LORAMAC_SERIALIZED_UNSUCCESS;

		uint8_t fhdr_iteration = 0;

		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = payload_str->macpayload_str.FHDR.DevAddr & 0xff;
		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = (payload_str->macpayload_str.FHDR.DevAddr >> 8) & 0xff;
		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = (payload_str->macpayload_str.FHDR.DevAddr >> 16) & 0xff;
		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = (payload_str->macpayload_str.FHDR.DevAddr >> 24) & 0xff;

		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = 0;//payload_str->macpayload_str.FHDR.FCTRL_UP;

		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = payload_str->macpayload_str.FHDR.FCnt & 0xff;
		payload_str->serial_data.serialized_fhdr[fhdr_iteration++] = (payload_str->macpayload_str.FHDR.FCnt >> 8) & 0xff;

		//AND FOPTS!!!


		break;
	case TASK_DATA:

		if(payload_str->serial_data.serialized_macpayload != NULL)
			free(payload_str->serial_data.serialized_macpayload); //Let's free up memory that allocated previous data serialization request!

		payload_str->serial_data.serialized_macpayload_size =  1 + payload_str->macpayload_str.FHDR.total_FHDR_size + 1 + payload_str->macpayload_str.FRMPayload_size + MIC_SIZE_BYTE;  //MHDR + FHDR + FPORT + FRMPAYLOAD + MIC;

		payload_str->serial_data.serialized_macpayload = malloc(payload_str->serial_data.serialized_macpayload_size);
		if(payload_str->serial_data.serialized_macpayload == NULL)
			return LORAMAC_SERIALIZED_UNSUCCESS;


// 		if(payload_str->serial_data.serialized_raw != NULL)
//					free(payload_str->serial_data.serialized_raw); //Let's free up memory that allocated previous data serialization request!
//
// 		payload_str->serial_data.serialized_raw_size = payload_str->serial_data.serialized_macpayload_size - MIC_SIZE_BYTE;
//
//		payload_str->serial_data.serialized_raw = malloc(payload_str->serial_data.serialized_raw_size);
//		if(payload_str->serial_data.serialized_raw == NULL)
//			return LORAMAC_SERIALIZED_UNSUCCESS;


		uint8_t data_iteration = 0;

		payload_str->serial_data.serialized_macpayload[data_iteration++] = (payload_str->mhdr_str.MType << 5) | (payload_str->mhdr_str.Major);

 		memcpy(&(payload_str->serial_data.serialized_macpayload[data_iteration]), payload_str->serial_data.serialized_fhdr, payload_str->serial_data.serialized_fhdr_size);
 		data_iteration += payload_str->serial_data.serialized_fhdr_size;

 		payload_str->serial_data.serialized_macpayload[data_iteration++] = payload_str->macpayload_str.FPort;


// 		memcpy(payload_str->serial_data.serialized_raw, payload_str->serial_data.serialized_macpayload, data_iteration); //raw data copy for mic
// 		memcpy(&(payload_str->serial_data.serialized_raw[data_iteration]), payload_str->macpayload_str.FRMPayload, payload_str->macpayload_str.FRMPayload_size); //raw data copy for mic


 		memcpy(&(payload_str->serial_data.serialized_macpayload[data_iteration]), payload_str->macpayload_str.encrypted_FRMPayload, payload_str->macpayload_str.FRMPayload_size);
 		data_iteration += payload_str->macpayload_str.FRMPayload_size;


 		//and then mic must be calculated and be added to serialized macpayload

		break;
	case TASK_ADD_REQ_MIC:

		uint8_t req_mic_iteration = TOTAL_JOIN_REQUEST_SIZE;

		payload_str->serial_data.serialized_join_req[req_mic_iteration++] = payload_str->join_req_str.request_mic & 0xFF;
		payload_str->serial_data.serialized_join_req[req_mic_iteration++] = (payload_str->join_req_str.request_mic >> 8) & 0xFF;
		payload_str->serial_data.serialized_join_req[req_mic_iteration++] = (payload_str->join_req_str.request_mic >> 16) & 0xFF;
		payload_str->serial_data.serialized_join_req[req_mic_iteration] = (payload_str->join_req_str.request_mic >> 24) & 0xFF;

		break;
	case TASK_ADD_DATA_MIC:

		uint8_t data_mic_iteration = payload_str->serial_data.serialized_macpayload_size - MIC_SIZE_BYTE;

		payload_str->serial_data.serialized_macpayload[data_mic_iteration++] = payload_str->macpayload_str.macpayload_mic & 0xFF;
		payload_str->serial_data.serialized_macpayload[data_mic_iteration++] = (payload_str->macpayload_str.macpayload_mic >> 8) & 0xFF;
		payload_str->serial_data.serialized_macpayload[data_mic_iteration++] = (payload_str->macpayload_str.macpayload_mic >> 16) & 0xFF;
		payload_str->serial_data.serialized_macpayload[data_mic_iteration] = (payload_str->macpayload_str.macpayload_mic >> 24) & 0xFF;

		break;

	}


	return LORAMAC_SERIALIZED_SUCCESS;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States LoraMac_Deserializer(struct Payload_Str *payload_str, uint8_t *data, uint8_t size, enum Deserializer_Task_Id deserializer_task_id){

	if(payload_str == NULL || data == NULL)
		return LORAMAC_NULL_STRUCT;

	switch(deserializer_task_id){
	case TASK_JOIN_ACCEPT:

		uint8_t data_iteration = 0;

		payload_str->join_res_str.res_mhdr = data[data_iteration++];

		memcpy(payload_str->join_res_str.AppNonce, &data[data_iteration], sizeof(payload_str->join_res_str.AppNonce));
		data_iteration += sizeof(payload_str->join_res_str.AppNonce);

		memcpy(payload_str->join_res_str.NetID, &data[data_iteration], sizeof(payload_str->join_res_str.NetID));
		data_iteration += sizeof(payload_str->join_res_str.NetID);

		memcpy(payload_str->join_res_str.DevAddr, &data[data_iteration], sizeof(payload_str->join_res_str.DevAddr));
		data_iteration += sizeof(payload_str->join_res_str.DevAddr);

		payload_str->join_res_str.DLSettings = data[data_iteration++];

		payload_str->join_res_str.RxDelay = data[data_iteration++];

		memcpy(payload_str->join_res_str.CFlist, &data[data_iteration], size);
		data_iteration += (size - data_iteration);

		payload_str->join_res_str.response_mic = (data[data_iteration++] & 0xFF);
		payload_str->join_res_str.response_mic |= ((data[data_iteration++] >> 8 ) & 0xFF);
		payload_str->join_res_str.response_mic |= ((data[data_iteration++] >> 16 ) & 0xFF);
		payload_str->join_res_str.response_mic |= ((data[data_iteration++] >> 24) & 0xFF);

		break;

	}


	return LORAMAC_SERIALIZED_SUCCESS;
}


static enum LoraMac_Error_States Compare_joinresponse_mic(struct Payload_Str *payload_str, uint8_t *in_buffer, uint8_t size){

	if(payload_str == NULL || in_buffer == NULL)
		return LORAMAC_NULL_STRUCT;

	payload_str->join_res_str.response_mic = (in_buffer[size - MIC_SIZE_BYTE] & 0xff) | (in_buffer[size - MIC_SIZE_BYTE +1] << 8) |
			(in_buffer[size - MIC_SIZE_BYTE +1] << 16)| (in_buffer[size - MIC_SIZE_BYTE +1] << 24) ;


	uint8_t key_in_buffer[16];
	uint8_t key_in_buffer_iteration = 0;

	key_in_buffer[key_in_buffer_iteration++] = payload_str->join_res_str.res_mhdr;

	memcpy(&key_in_buffer[key_in_buffer_iteration], payload_str->join_res_str.AppNonce, sizeof(payload_str->join_res_str.AppNonce));
	key_in_buffer_iteration += sizeof(payload_str->join_res_str.AppNonce);

	memcpy(&key_in_buffer[key_in_buffer_iteration], payload_str->join_res_str.NetID, sizeof(payload_str->join_res_str.NetID));
	key_in_buffer_iteration += sizeof(payload_str->join_res_str.NetID);

	memcpy(&key_in_buffer[key_in_buffer_iteration], payload_str->join_res_str.DevAddr, sizeof(payload_str->join_res_str.DevAddr));
	key_in_buffer_iteration += sizeof(payload_str->join_res_str.DevAddr);

	key_in_buffer[key_in_buffer_iteration++] = payload_str->join_res_str.DLSettings;

	key_in_buffer[key_in_buffer_iteration++] = payload_str->join_res_str.RxDelay;

	memcpy(&key_in_buffer[key_in_buffer_iteration], payload_str->join_res_str.CFlist, size - key_in_buffer_iteration);
	key_in_buffer_iteration += (size - key_in_buffer_iteration);


	uint32_t computed_cmac = aes128_cmac(AppKey, key_in_buffer, size);

	if(computed_cmac != payload_str->join_res_str.response_mic)
		return LORAMAC_JOIN_ACCEPT_MIC_ERROR;

	return 0;
}

/*brief: It is the way of abstraction of static variable. Internal functions can check directly lora_state.is_join!
 *
 *
 *
 * */
uint8_t Is_Join(){

	return lora_state.is_join;
}

/*brief: It is the way of abstraction of static variable. Internal functions can check directly lora_state.is_join!
 *
 *
 *
 * */
void  Set_Join(bool join_state){

	lora_state.is_join = join_state;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_DevAddr(uint32_t devaddr){

	DevAddr = devaddr;

	return 0;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_AppEUI(uint8_t *appeui){

	memcpy(AppEUI, appeui, sizeof(AppEUI));

	return 0;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_DevEUI(uint8_t *deveui){

	memcpy(DevEUI, deveui, sizeof(DevEUI));

	return 0;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_AppKey(uint8_t *appkey){


	memcpy(AppKey, appkey, sizeof(AppKey));

	return 0;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_AppsKey(uint8_t *appskey){

	memcpy(AppSKey, appskey, sizeof(AppSKey));

	return 0;
}

/*brief:
 *
 *
 *
 * */
enum LoraMac_Error_States Set_Nwkskey(uint8_t *nwkskey){

	memcpy(NwksKey, nwkskey, sizeof(NwksKey));

	return 0;
}



//is payload length ok?



/*brief:
 *
 *
 *
 * */
void LoraMac_Error_Handler(){

	while(1){



	}

}












