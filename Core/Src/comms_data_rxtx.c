#include "usb_device.h"
#include "cmsis_gcc.h"
#include "comms_data_rxtx.h"
#include "usart.h"

// --------------------------NEW COMMS------------------------------

uint8_t comms_tx_buffer1[MAX_TX_BUFFER_SIZE] = { 0 }; // buffer for tx data
uint8_t comms_tx_buffer2[MAX_TX_BUFFER_SIZE] = { 0 };
uint8_t *comms_tx_active_buffer; // pointer to wr ready buffer
uint8_t *comms_tx_active_wr_pointer; // pointer to first empty wr position in active buffer
uint8_t *comms_tx_prepared_buffer; // pointer to tx ready buffer
uint8_t *comms_tx_prepared_wr_pointer; // same but in tx ready buffer - just for counting the buffer size
void *comms_tx_data_id_register[MAX_DATA_ID] = { NULL }; // register of written data id

uint8_t comms_rx_buffer1[MAX_RX_BUFFER_SIZE] = { 0 }; // buffer for rx data
uint8_t comms_rx_buffer2[MAX_RX_BUFFER_SIZE] = { 0 };
uint8_t *comms_rx_active_buffer; // buffer which is being used to load data to
uint8_t *comms_rx_active_rd_pointer;
uint8_t *comms_rx_prepared_buffer; // pointer to buffer of ready to read received data
uint8_t *comms_rx_prepared_rd_pointer;

comms_interface comms_selected_interface = COMMS_UART;

comms_state wr_status = COMMS_READY;
comms_state tx_status = COMMS_READY;
comms_state rx_status = COMMS_READY;

comms_uart_rx_state uart_rx_state = COMMS_UART_HEAD;


extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);


void comms_reset_active_tx_buffer() {
	// write start bytes
	*((uint16_t*) (comms_tx_active_buffer)) = START_HEADER;
	// reset buffer id
	comms_tx_active_buffer[2] = 0;
	//reset number of elements
	*((uint16_t*) (comms_tx_active_buffer + 3)) = 0;
	// set pointer to first position where data elements can be writen to
	comms_tx_active_wr_pointer = comms_tx_active_buffer + 5;
}


void comms_reset_active_rx_buffer() {
	//reset whole head to correctly detect next incoming buffer
	comms_rx_active_buffer[0] = 0;
	comms_rx_active_buffer[1] = 0;
	comms_rx_active_buffer[2] = 0;
	*((uint16_t*) (comms_rx_active_buffer + 3)) = 0; // reset number of data packets inside the buffer
	comms_rx_active_rd_pointer = comms_rx_active_buffer; // reset the active pointer
}


void comms_uart_init() {
	// reset the state
	uart_rx_state = COMMS_UART_HEAD;
	// set callback for incoming data buffer head
	HAL_StatusTypeDef rcode = HAL_UART_Receive_IT(&hlpuart1, comms_rx_active_buffer,
			5);
	UNUSED(rcode);
}


void comms_init() {
	// init TX
	comms_tx_active_buffer = comms_tx_buffer1;
	comms_tx_prepared_buffer = comms_tx_buffer2;

	comms_reset_active_tx_buffer();
	comms_tx_prepared_wr_pointer = comms_tx_prepared_buffer + 5; // set pointer to first position of data elements ready for transfer

	// init RX
	if (comms_selected_interface == COMMS_UART){
		comms_uart_init();
	}

	comms_rx_active_buffer = comms_rx_buffer1;
	comms_rx_prepared_buffer = comms_rx_buffer2;

	comms_reset_active_rx_buffer();
	comms_rx_prepared_rd_pointer = comms_rx_prepared_buffer + 5; // set pointer to first position of readable data elements
}


void comms_purge_id_register() {
	// reset the evidence of existing data packets in tx buffer
	memset(comms_tx_data_id_register, NULL, sizeof(comms_tx_data_id_register));
}


void* comms_find_existing_data(uint8_t data_id) {
	// returns pointer to existing data packet with this data_id, or NULL
	if (comms_tx_data_id_register[data_id] != NULL) {
		return comms_tx_data_id_register[data_id];
	}
	return NULL;
}


void comms_increment_active_buffer_data() {
	// increments total number of data elements in active tx buffer.
	*((uint16_t*) (comms_tx_active_buffer + 3)) += 1;
}


int comms_append_int32(uint8_t data_id, uint8_t data_count, int *data) {
	//check tx_register for same data id, return if existing
	if (comms_find_existing_data(data_id) != NULL && !ALLOW_TX_APPEND_DUPLICITE_DATA_ID) {
		return COMMS_DATA_ID_EXISTS;
	}

	if (wr_status) {
		return COMMS_WR_LOCKED;
	}
	else {
		wr_status = COMMS_INPROGRESS;
	}

	// save the pointer to new data to register
	comms_tx_data_id_register[data_id] = (void*) comms_tx_active_wr_pointer;

	// increment total data in buffer
	comms_increment_active_buffer_data();

	// write id, bytes and count
	*comms_tx_active_wr_pointer = data_id;
	*(comms_tx_active_wr_pointer + 1) = (uint8_t) sizeof(*data);
	*(comms_tx_active_wr_pointer + 2) = data_count;

	// write integer as 4 uint8_t to tx_buffer
	*((int*) (comms_tx_active_wr_pointer + 3)) = *data;

	// move pointer comms_tx_buffer_wr_pointer
	comms_tx_active_wr_pointer = (comms_tx_active_wr_pointer + 3 + sizeof(*data));

	wr_status = COMMS_READY;

	return 0;
}


void comms_switch_tx_buffers() {
	// dissable interrupts
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	// switch buffers
	uint8_t *_temp = comms_tx_prepared_buffer;
	comms_tx_prepared_buffer = comms_tx_active_buffer;
	comms_tx_active_buffer = _temp;

	// set pointer to the end of prepared buffer data
	comms_tx_prepared_wr_pointer = comms_tx_active_wr_pointer;

	// prepare the new active buffer and pointers
	comms_reset_active_tx_buffer();
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
	comms_switch_tx_buffers();

	// terminator at the end
	*comms_tx_prepared_wr_pointer = END_CR;
	*(comms_tx_prepared_wr_pointer + 1) = END_LF;
	comms_tx_prepared_wr_pointer += 2;

	// buffer is empty
	if (comms_tx_prepared_buffer[3] == 0) {
		tx_status = COMMS_READY;
		return COMMS_TX_BUFFER_EMPTY;
	}

	// send data
	USBD_StatusTypeDef cdc_return = 0;
	HAL_StatusTypeDef uart_return = 0;

	if(comms_selected_interface == COMMS_USB_OTG){
		cdc_return = CDC_Transmit_FS(comms_tx_prepared_buffer, comms_tx_prepared_wr_pointer - comms_tx_prepared_buffer);
	} else {
		uart_return = HAL_UART_Transmit(&hlpuart1, comms_tx_prepared_buffer, comms_tx_prepared_wr_pointer - comms_tx_prepared_buffer, 100);
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


void comms_switch_rx_buffers(){
	// dissable interrupts
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	// set read pointer to first data packet head
	comms_rx_active_rd_pointer = comms_rx_active_buffer + 5;

	// switch rx buffers
	uint8_t *_temp = comms_rx_prepared_buffer;
	comms_rx_prepared_buffer = comms_rx_active_buffer;
	comms_rx_active_buffer = _temp;

	// switch pointers
	comms_rx_prepared_rd_pointer = comms_rx_active_rd_pointer;

	// prepare the new active buffer and pointers
	comms_reset_active_rx_buffer();

	// restore interrupts
	__set_PRIMASK(primask);
}


void comms_cdc_rx_callback(uint8_t *buffer, uint32_t length) {
	// call this func inside of usbd_cdc_if.c in CDC_Receive_FS()

	if (comms_selected_interface != COMMS_USB_OTG){
		// quit if UART is in use
		return;
	}

	if (rx_status) {
		// not ready yet
		return;
	}

	if (length < 3) {
		// invalid buffer
		return;
	}

	rx_status = COMMS_INPROGRESS;

	//copy to the active buffer
	memcpy(comms_rx_active_buffer, buffer, length);
	comms_switch_rx_buffers();

	rx_status = COMMS_RECEIVED;
}


__weak void comms_data_handler(CommsData *data) {

	if (data == NULL) {
		return;
	}

	switch (data->data_id) {
	case 5:
		GPIO_PinState currentState = HAL_GPIO_ReadPin(LD3_GPIO_Port, LD3_Pin);
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,
				(currentState == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
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

	if (comms_rx_prepared_buffer[2] == 0) {
		uint16_t elements = *((uint16_t*) (comms_rx_prepared_buffer + 3));

		for (; elements > 0; --elements) {
			CommsData data;
			data.data_id = *comms_rx_prepared_rd_pointer;
			data.data_size = *(comms_rx_prepared_rd_pointer + 1);
			data.data_count = *(comms_rx_prepared_rd_pointer + 2);

			for (uint8_t x = 0; x < data.data_count; ++x) {
				switch (data.data_size) {
				case 1:
					data.data[x].u8 = *(comms_rx_prepared_rd_pointer + 3);
					break;
				case 2:
					data.data[x].u16 =
							*((uint16_t*) (comms_rx_prepared_rd_pointer + 3));
					break;
				case 4:
					data.data[x].u32 =
							*((uint32_t*) (comms_rx_prepared_rd_pointer + 3));
					break;
				default:
					return;
				}
			}

			comms_data_handler(&data);

			comms_rx_prepared_rd_pointer = (comms_rx_prepared_rd_pointer + 3
					+ (data.data_size * data.data_count));
		}
	}

	rx_status = COMMS_READY;
}


void comms_lpuart_rx_callback(UART_HandleTypeDef *huart) {

	static int uart_elements;
	static int data_total_length;
	static HAL_StatusTypeDef rcode;
	UNUSED(rcode);

	if (comms_selected_interface != COMMS_UART){
		// quit if USB OTG is in use
		return;
	}

	switch (uart_rx_state) {
		case COMMS_UART_HEAD:
			//check start bytes
			if (*((uint16_t*) (comms_rx_active_buffer)) != START_HEADER) {
				//corrupted buffer
				comms_uart_init();
				break;
			}

			//load num of elements
			uart_elements = *((uint16_t*) (comms_rx_active_buffer + 3));

			//increment pointer
			comms_rx_active_rd_pointer = comms_rx_active_buffer + 5;

			//set callback for loading packet head
			rcode = HAL_UART_Receive_IT(&hlpuart1, comms_rx_active_rd_pointer, 3);
			uart_rx_state = COMMS_UART_PACKET_HEAD;

			break;

		case COMMS_UART_PACKET_HEAD:
			//check data size and data count
			data_total_length = (*(comms_rx_active_rd_pointer + 1)) * (*(comms_rx_active_rd_pointer + 2));

			//increment pointer
			comms_rx_active_rd_pointer = comms_rx_active_rd_pointer + 3;

			//set callback
			rcode = HAL_UART_Receive_IT(&hlpuart1, comms_rx_active_rd_pointer, data_total_length);
			uart_rx_state = COMMS_UART_PACKET_DATA;

			break;

		case COMMS_UART_PACKET_DATA:
			//increment pointer
			comms_rx_active_rd_pointer = comms_rx_active_rd_pointer + data_total_length;

			//repeat packet_head if not all
			if(uart_elements > 0) {
				//load next packet head
				UNUSED(rcode);
				uart_rx_state = COMMS_UART_PACKET_HEAD;
				--uart_elements;
			}

			if (uart_elements == 0) {
				//complete
				comms_switch_rx_buffers();
				rx_status = COMMS_RECEIVED;

				//reset uart
				uart_rx_state = COMMS_UART_HEAD;
				uart_elements = 0;
				data_total_length = 0;
				comms_uart_init();
				break;
			}

			HAL_StatusTypeDef rcode = HAL_UART_Receive_IT(&hlpuart1, comms_rx_active_rd_pointer, 3);
			UNUSED(rcode);

			break;

		default:
			break;
	}

}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	// user defined __weak callback from stm32l4xx_hal_uart.c
	// uart loaded data
	if (huart == &hlpuart1) {
		comms_lpuart_rx_callback(huart);
	}
}
