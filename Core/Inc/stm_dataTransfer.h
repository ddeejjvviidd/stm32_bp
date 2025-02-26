
#include "main.h"
#include "usb_device.h"
#include "cmsis_gcc.h"

#define MAX_TXRX_DATA 4096 //200

uint32_t buf_M_RX[MAX_TXRX_DATA + 1]; //+4bytes pro hlavicku
uint32_t buf_M_TX[MAX_TXRX_DATA + 1];
uint32_t m2s_buf[MAX_TXRX_DATA]; //


// --------------------------NEW COMMS------------------------------

#define MAX_TX_BUFFER_SIZE 16384
#define MAX_DATA_ID 255

uint8_t comms_tx_buffer1[MAX_TX_BUFFER_SIZE] = {0};
uint8_t comms_tx_buffer2[MAX_TX_BUFFER_SIZE] = {0};
uint8_t * comms_active_buffer; // pointer to wr ready buffer
uint8_t * comms_prepared_buffer; // pointer to tx ready buffer
void * comms_id_register[MAX_DATA_ID] = {NULL}; // register of written data id

uint8_t * comms_active_wr_pointer; // pointer to first empty wr position in active buffer
uint8_t * comms_prepared_wr_pointer; // same but in tx ready buffer - just for counting the buffer size

int empty = 0;

typedef enum {
	COMMS_READY,
	COMMS_INPROGRESS,
} comms_status;

comms_status wr_status = COMMS_READY;
comms_status tx_status = COMMS_READY;

typedef enum {
	COMMS_SUCCESS, // general success
	COMMS_FAILED, // general fail
	COMMS_TX_CDC_FAIL, // CDC for data transfer returned anything but USB_OK
	COMMS_TX_BUFFER_EMPTY, // tx buffer does not contain any data
	COMMS_TX_LOCKED, // tx function is already running elsewhere
	COMMS_WR_LOCKED, // active buffer is already being written to

} comms_return_codes;

extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);
extern void DataReceive_MTLB_Callback(uint16_t iD, uint32_t *xData, uint16_t nData_in_values);

void comms_reset_active_buffer(){
	*((uint16_t *)(comms_active_buffer+1)) = 0;
	//comms_active_buffer[1] = 0;
	comms_active_wr_pointer = comms_active_buffer+3;
}

void comms_init(){
	comms_active_buffer = comms_tx_buffer1;
	comms_prepared_buffer = comms_tx_buffer2;

	comms_reset_active_buffer();
	comms_prepared_wr_pointer = comms_prepared_buffer+3;
}

void comms_purge_id_register(){
	memset(comms_id_register, NULL, sizeof(comms_id_register));
}

void * comms_find_existing_data(uint8_t data_id){
	if (comms_id_register[data_id] != NULL) {
		return comms_id_register[data_id];
	}
	return NULL;
}

void comms_increment_active_buffer_data(){
	*((uint16_t *)(comms_active_buffer+1)) += 1;
}

int comms_append_int32(uint8_t data_id, uint8_t data_count, int * data){
	// dissable interrupts
//	uint32_t primask = __get_PRIMASK();
//	__disable_irq();

	//check tx_register for same data id, return if existing
	if (comms_find_existing_data(data_id) != NULL) {
			return 1;
	}

	if (wr_status) {
		return COMMS_WR_LOCKED;
	}
//	else {
//		wr_status = COMMS_INPROGRESS;
//	}

	// save the pointer to new data to register
	comms_id_register[data_id] = (void *)comms_active_wr_pointer;

	// increment total data in buffer
	comms_increment_active_buffer_data();

	// write id, bytes and count
	*comms_active_wr_pointer = data_id;
	*(comms_active_wr_pointer+1) = (uint8_t)sizeof(*data);
	*(comms_active_wr_pointer+2) = data_count;

	// write integer as 4 uint8_t to tx_buffer
	*((int *)(comms_active_wr_pointer+3)) = *data;

	// move pointer comms_tx_buffer_wr_pointer
	comms_active_wr_pointer = (comms_active_wr_pointer+3+sizeof(*data));

	wr_status = COMMS_READY;

	// restore interrupts
//	__set_PRIMASK(primask);

	return 0;
}

void comms_switch_buffers(){
	// dissable interrupts
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	// switch buffers
	uint8_t * _temp = comms_prepared_buffer;
	comms_prepared_buffer = comms_active_buffer;
	comms_active_buffer = _temp;

	// set pointer to the end of prepared buffer data
	comms_prepared_wr_pointer = comms_active_wr_pointer;

	// prepare the new active buffer and pointers
	comms_reset_active_buffer();
	comms_purge_id_register();

	// restore interrupts
	__set_PRIMASK(primask);
}

int comms_send(){

	if(tx_status > 0){
		return COMMS_TX_LOCKED;
	}

	tx_status = COMMS_INPROGRESS;

	// need to switch buffers
	comms_switch_buffers();


	// buffer is empty
	if (comms_prepared_buffer[1] == 0) {
		++empty;
		tx_status = COMMS_READY;
		return COMMS_TX_BUFFER_EMPTY;
	}

	// send data
	USBD_StatusTypeDef cdc_return = CDC_Transmit_FS(comms_prepared_buffer, comms_prepared_wr_pointer - comms_prepared_buffer);

	tx_status = COMMS_READY;

	if (cdc_return) {
		return COMMS_TX_CDC_FAIL;
	}

	return COMMS_SUCCESS;
}






int s2m_Status; // send to Matlab
int m2s_Status; // 0...ceka na prijem dat, 1...data prisla, -1...inicializace, 100...nData, 3...xData
int m2s_ID;
int m2s_nData_in_bytes;



// Send float or uint32 or none data
int DataTransmit2MTLB(uint16_t iD, uint8_t *xData, uint16_t nData_in_values) {
	// USB busy
	if (s2m_Status)
		return -1;

	if ((sizeof(buf_M_TX) - 4) < (nData_in_values * 4)) {
		// vystupni buffer je mensi nez odesilana data
		return -2;
	}

	s2m_Status = 1; // manualni nastaveni na USB busy, predpoklada se cinnost

	// prekopirovani dat do bufferu

	((uint16_t*) buf_M_TX)[0] = iD;

	((uint16_t*) buf_M_TX)[1] = nData_in_values;
	if (nData_in_values > 0)
			memcpy(buf_M_TX + 1, xData, nData_in_values * 4);

	// volani funkce na transmit dat
	s2m_Status = CDC_Transmit_FS((uint8_t*) buf_M_TX, nData_in_values * 4 + 4);

	if (s2m_Status) {
		// USB busy, vyresetuje na USB_OK
		s2m_Status = 0;
	}

	return 0;
}

void m2s_Process() {
	// funkce volana z nekonecne smycky

	if (!m2s_Status)
		return; // pokud neprisla data, ukonci se

	if (m2s_Status == 1) {
		// zpracuje data
		DataReceive_MTLB_Callback(m2s_ID, m2s_buf, m2s_nData_in_bytes / 4);
		m2s_Status = 0;
		return;
	}

	if (m2s_Status == -1) {
		// inicializace
		m2s_Status = 0;
		return;
	}

}

void USB_My_Receive(uint8_t *Buf, uint32_t Len) {
	// call this in usbd_cdc_if.c to CDC_Receive_FS

	// callback na prijem dat
	if (m2s_Status == 0) {
		// iD
		m2s_ID = ((uint16_t*) Buf)[0];
		if (m2s_ID == 0)
			return;
		m2s_Status = 100;
		return;
	}

	if (m2s_Status == 100) {
		// nData
		m2s_nData_in_bytes = ((uint16_t*) Buf)[0] * 4;
		if (m2s_nData_in_bytes == 0) {
			m2s_Status = 1;
			return;
		}
		m2s_Status = 3; //wait for xData
		return;
	}

	if (m2s_Status == 3) {
		// xData
		if (Len < m2s_nData_in_bytes)
			m2s_nData_in_bytes = Len;
		memcpy(m2s_buf, Buf, m2s_nData_in_bytes);
		m2s_Status = 1;
		return;
	}

	return;
}
