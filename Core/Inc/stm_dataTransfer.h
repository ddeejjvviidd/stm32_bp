
#include "main.h"
#include "usb_device.h"

#define MAX_TXRX_DATA 4096 //200

uint32_t buf_M_RX[MAX_TXRX_DATA + 1]; //+4bytes pro hlavicku
uint32_t buf_M_TX[MAX_TXRX_DATA + 1];
uint32_t m2s_buf[MAX_TXRX_DATA]; //


// --------------------------NEW COMMS------------------------------

#define MAX_TX_BUFFER_SIZE 16384
#define MAX_DATA_ID 255
uint8_t tx_buffer[MAX_TX_BUFFER_SIZE] = {0};
void * tx_register[MAX_DATA_ID] = {NULL};

uint8_t tx_data_count = 0;
uint8_t * txArrayWritePosition; // 0 je id celeho ramce, 1 je pocet dat obsazenych v ramci


void comms_init(){
	txArrayWritePosition = *(tx_buffer+2);
}

void purge_tx_register(){
	memset(tx_register, MULL, sizeof(tx_register));
}

int tx_buffer_append_int32(uint8_t data_id, uint8_t data_bytes, uint8_t data_count, int data){
	//check tx_register for same data id, return if existing


	// increment total data in buffer tx_data_count
	tx_buffer[1] += 1;

	// write id, bytes and count


	// write integer as 4 uint8_t to tx_buffer

	// move pointer txArrayWritePosition
}







int s2m_Status; // send to Matlab
int m2s_Status; // 0...ceka na prijem dat, 1...data prisla, -1...inicializace, 100...nData, 3...xData
int m2s_ID;
int m2s_nData_in_bytes;


typedef struct __attribute__((packed)){
	uint8_t data_id;
	uint8_t data_size;
	uint8_t data[4];
	//uint32_t * data_start = (uint8_t *) data;
} item4B;

//*((uint32_t *)(buffer+4)) = neco;

extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);
extern void DataReceive_MTLB_Callback(uint16_t iD, uint32_t *xData, uint16_t nData_in_values);

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

int SendData2MTLB(uint16_t iD, uint8_t *xData, uint16_t nData_in_values) {
	return DataTransmit2MTLB(iD, xData, nData_in_values);
}

int SendInt2MTLB(uint16_t iD, int *xInt) {

	return DataTransmit2MTLB(iD, (uint8_t*) xInt, 1);
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
