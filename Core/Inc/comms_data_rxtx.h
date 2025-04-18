#ifndef COMMS_DATA_RXTX_H
#define COMMS_DATA_RXTX_H


// --------------------------NEW COMMS------------------------------

#define MAX_TX_BUFFER_SIZE 16384 / 16
#define MAX_RX_BUFFER_SIZE 16384 / 16
#define MAX_DATA_ID 255

#define START_HEADER 0xABCD
#define END_CR 0x0D;
#define END_LF 0x0A;

#define ALLOW_TX_APPEND_DUPLICITE_DATA_ID 0

extern uint8_t comms_tx_buffer1[MAX_TX_BUFFER_SIZE]; // buffer for tx data
extern uint8_t comms_tx_buffer2[MAX_TX_BUFFER_SIZE];
extern uint8_t *comms_tx_active_buffer; // pointer to wr ready buffer
extern uint8_t *comms_tx_active_wr_pointer; // pointer to first empty wr position in active buffer
extern uint8_t *comms_tx_prepared_buffer; // pointer to tx ready buffer
extern uint8_t *comms_tx_prepared_wr_pointer; // same but in tx ready buffer - just for counting the buffer size
extern void *comms_tx_data_id_register[MAX_DATA_ID]; // register of written data id

extern uint8_t comms_rx_buffer1[MAX_RX_BUFFER_SIZE]; // buffer for rx data
extern uint8_t comms_rx_buffer2[MAX_RX_BUFFER_SIZE];
extern uint8_t *comms_rx_active_buffer; // buffer which is being used to load data to
extern uint8_t *comms_rx_active_rd_pointer;
extern uint8_t *comms_rx_prepared_buffer; // pointer to buffer of ready to read received data
extern uint8_t *comms_rx_prepared_rd_pointer;

typedef enum {
	COMMS_UART,
	COMMS_USB_OTG,
} comms_interface;


typedef enum {
	COMMS_READY,
	COMMS_INPROGRESS,
	COMMS_RECEIVED
} comms_state;

typedef enum {
	COMMS_SUCCESS, // general success
	COMMS_FAILED, // general fail
	COMMS_TX_CDC_FAIL, // CDC for data transfer returned anything but USB_OK
	COMMS_TX_BUFFER_EMPTY, // tx buffer does not contain any data
	COMMS_TX_LOCKED, // tx function is already running elsewhere
	COMMS_WR_LOCKED, // active buffer is already being written to
	COMMS_TX_UART_FAIL, // UART func for data transfer returned anything but HAL_OK
	COMMS_DATA_ID_EXISTS, // user tried to append a data_id which already is in the tx buffer
} comms_return_codes;

typedef enum {
	COMMS_UART_HEAD,
	COMMS_UART_PACKET_HEAD,
	COMMS_UART_PACKET_DATA,
	COMMS_UART_COMPLETE
} comms_uart_rx_state;


typedef union {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
} DataValue;

typedef struct {
	uint8_t data_id;
	uint8_t data_size;
	uint8_t data_count;
	DataValue data[255];
} CommsData;


extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);


void comms_reset_active_tx_buffer();


void comms_reset_active_rx_buffer();


void comms_uart_init();


void comms_init();


void comms_purge_id_register();


void* comms_find_existing_data(uint8_t data_id);


void comms_increment_active_buffer_data();


int comms_append_int32(uint8_t data_id, uint8_t data_count, int *data);


void comms_switch_tx_buffers();


int comms_send();


void comms_switch_rx_buffers();


void comms_cdc_rx_callback(uint8_t *buffer, uint32_t length);


__weak void comms_data_handler(CommsData *data);


void comms_rx_process();


void comms_lpuart_rx_callback(UART_HandleTypeDef *huart);


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif
