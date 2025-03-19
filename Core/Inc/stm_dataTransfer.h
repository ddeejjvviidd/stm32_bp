#include "main.h"
#include "usb_device.h"
#include "cmsis_gcc.h"

// --------------------------NEW COMMS------------------------------

#define MAX_TX_BUFFER_SIZE 16384 / 16
#define MAX_RX_BUFFER_SIZE 16384 / 16
#define MAX_DATA_ID 255

#define START_HEADER 0xABCD

uint8_t comms_tx_buffer1[MAX_TX_BUFFER_SIZE] = { 0 };
uint8_t comms_tx_buffer2[MAX_TX_BUFFER_SIZE] = { 0 };
uint8_t *comms_active_buffer; // pointer to wr ready buffer
uint8_t *comms_active_wr_pointer; // pointer to first empty wr position in active buffer
uint8_t *comms_prepared_buffer; // pointer to tx ready buffer
uint8_t *comms_prepared_wr_pointer; // same but in tx ready buffer - just for counting the buffer size
void *comms_id_register[MAX_DATA_ID] = { NULL }; // register of written data id

uint8_t comms_rx_buffer1[MAX_RX_BUFFER_SIZE] = { 0 }; // buffer for rx data
uint8_t comms_rx_buffer2[MAX_RX_BUFFER_SIZE] = { 0 };
uint8_t *comms_rx_active_buffer;
uint8_t *comms_rx_read_pointer;

int empty = 0;

typedef enum {
	COMMS_READY,
	COMMS_INPROGRESS,
	COMMS_RECEIVED,
	COMMS_UART_HEAD,
	COMMS_UART_DATA,
} comms_state;

comms_state wr_status = COMMS_READY;
comms_state tx_status = COMMS_READY;
comms_state rx_status = COMMS_READY;

typedef enum {
	COMMS_SUCCESS, // general success
	COMMS_FAILED, // general fail
	COMMS_TX_CDC_FAIL, // CDC for data transfer returned anything but USB_OK
	COMMS_TX_BUFFER_EMPTY, // tx buffer does not contain any data
	COMMS_TX_LOCKED, // tx function is already running elsewhere
	COMMS_WR_LOCKED, // active buffer is already being written to
	COMMS_TX_UART_FAIL, // UART func for data transfer returned anything but HAL_OK
} comms_return_codes;

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

void comms_reset_active_buffer() {
	*((uint16_t*) (comms_active_buffer)) = START_HEADER; // start bits
	comms_active_buffer[2] = 0; // buffer id
	*((uint16_t*) (comms_active_buffer + 3)) = 0; // num of elements
	comms_active_wr_pointer = comms_active_buffer + 5; // first empty position for data
}

void comms_init() {
	comms_active_buffer = comms_tx_buffer1;
	comms_prepared_buffer = comms_tx_buffer2;

	comms_reset_active_buffer();
	comms_prepared_wr_pointer = comms_prepared_buffer + 5;
}

void comms_purge_id_register() {
	memset(comms_id_register, NULL, sizeof(comms_id_register));
}

void* comms_find_existing_data(uint8_t data_id) {
	if (comms_id_register[data_id] != NULL) {
		return comms_id_register[data_id];
	}
	return NULL;
}

void comms_increment_active_buffer_data() {
	*((uint16_t*) (comms_active_buffer + 3)) += 1;
}

int comms_append_int32(uint8_t data_id, uint8_t data_count, int *data) {
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
	comms_id_register[data_id] = (void*) comms_active_wr_pointer;

	// increment total data in buffer
	comms_increment_active_buffer_data();

	// write id, bytes and count
	*comms_active_wr_pointer = data_id;
	*(comms_active_wr_pointer + 1) = (uint8_t) sizeof(*data);
	*(comms_active_wr_pointer + 2) = data_count;

	// write integer as 4 uint8_t to tx_buffer
	*((int*) (comms_active_wr_pointer + 3)) = *data;

	// move pointer comms_tx_buffer_wr_pointer
	comms_active_wr_pointer = (comms_active_wr_pointer + 3 + sizeof(*data));

	wr_status = COMMS_READY;

	// restore interrupts
//	__set_PRIMASK(primask);

	return 0;
}

void comms_switch_buffers() {
	// dissable interrupts
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	// switch buffers
	uint8_t *_temp = comms_prepared_buffer;
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

int comms_send() {

	if (tx_status > 0) {
		return COMMS_TX_LOCKED;
	}

	tx_status = COMMS_INPROGRESS;

	// need to switch buffers
	comms_switch_buffers();

	// buffer is empty
	if (comms_prepared_buffer[3] == 0) {
		++empty; //DEBUG
		tx_status = COMMS_READY;
		return COMMS_TX_BUFFER_EMPTY;
	}

	// send data
	USBD_StatusTypeDef cdc_return = 0;
	HAL_StatusTypeDef uart_return = 0;

	if(1){
		cdc_return = CDC_Transmit_FS(comms_prepared_buffer, comms_prepared_wr_pointer - comms_prepared_buffer);
	} else {
		uart_return = HAL_UART_Transmit(&hlpuart1, comms_prepared_buffer, comms_prepared_wr_pointer - comms_prepared_buffer, 100);
	}

	tx_status = COMMS_READY;

	if (cdc_return) {
		return COMMS_TX_CDC_FAIL;
	}

	if (uart_return) {
		return COMMS_TX_UART_FAIL;
	}

	return COMMS_SUCCESS;
}

void comms_cdc_rx_callback(uint8_t *buffer, uint32_t length) {
	// call this func inside of usbd_cdc_if.c in CDC_Receive_FS()

	if (rx_status) {
		// not ready yet
		return;
	}

	if (length < 3) {
		// invalid
		return;
	}

	rx_status = COMMS_INPROGRESS;

	//comms_rx_buffer = buffer;
	memcpy(comms_rx_buffer1, buffer, length);
	comms_rx_read_pointer = comms_rx_buffer1 + 3;

	rx_status = COMMS_RECEIVED;
}

__weak void comms_data_handler(CommsData *data) {

	if (data == NULL) {
		return;
	}

	switch (data->data_id) {
	case 5:
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,
				(GPIO_PinState) (data->data[0].u8));
		break;
	default:
		break;
	}

}

void comms_rx_process() {
	if (!rx_status) {
		// no data yet
		return;
	}

	if (comms_rx_buffer1[0] == 0) {
		uint16_t elements = *((uint16_t*) (comms_rx_buffer1 + 1));

		for (; elements > 0; --elements) {
			CommsData data;
			data.data_id = *comms_rx_read_pointer;
			data.data_size = *(comms_rx_read_pointer + 1);
			data.data_count = *(comms_rx_read_pointer + 2);

			for (uint8_t x = 0; x < data.data_count; ++x) {
				switch (data.data_size) {
				case 1:
					data.data[x].u8 = *(comms_rx_read_pointer + 3);
					break;
				case 2:
					data.data[x].u16 =
							*((uint16_t*) (comms_rx_read_pointer + 3));
					break;
				case 4:
					data.data[x].u32 =
							*((uint32_t*) (comms_rx_read_pointer + 3));
					break;
				default:
					return;
				}
			}

			comms_data_handler(&data);

			comms_rx_read_pointer = (comms_rx_read_pointer + 3
					+ (data.data_size * data.data_count));
		}
	}

	rx_status = COMMS_READY;
}

void comms_uart_init() {
	HAL_StatusTypeDef rcode = HAL_UART_Receive_IT(&hlpuart1, comms_rx_buffer1,
			3);
}

void comms_lpuart_rx_callback(UART_HandleTypeDef *huart) {
	// load head 3 bytes
	// get number of elements
	// FOR ELEMENT LOOP:
	//		load packet head
	//		load data
	static uint16_t elements = 0;

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	// user defined __weak callback from stm32l4xx_hal_uart.c
	// uart loaded data
	if (huart == &hlpuart1) {
		comms_lpuart_rx_callback(huart);
	}
}
