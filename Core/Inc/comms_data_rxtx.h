#ifndef COMMS_DATA_RXTX_H
#define COMMS_DATA_RXTX_H

/**
 * @file comms_data_rxtx.h
 * @brief Header file defining functions for communication between STM32 and MATLAB using USB OTG and UART.
 */

/**
 * @brief Defines the maximum size allocated for the transmission buffers.
 * @details The buffer size is divided by 16 to reduce RAM usage.
 *          The value is set to 16384 / 16, which equals 1024.
 * 	        Each buffer has 5 bytes for head, each data packet has 3 bytes for head, and 4 bytes for one integer.
 *          This configuration can hold up to 145 data packets.
 * @note The buffer size can be increased if needed, but it will consume more RAM.
 * @warning Setting this value too low may lead to errors and data loss.
 */
#define MAX_TX_BUFFER_SIZE 65536 / 64
#define MAX_RX_BUFFER_SIZE 65536 / 64

/**
 * @brief Size for the data ID register, which keeps track of written data packet IDs.
 *        Maximum size is 255, limited by uint8_t.
 * @warning If you set maximum value to 240, you wont be able to append data packet
 * 			with ID over 240 into the TX buffer.
 */
#define MAX_DATA_ID 255

/**
 * @brief Defines the start and end bytes for the data packets.
 *        The start byte is 0xABCD, and the end bytes are CR (0x0D) and LF (0x0A).
 */
#define START_HEADER 0xABCD
#define END_CR 0x0D
#define END_LF 0x0A

/**
 * @def ALLOW_TX_APPEND_DUPLICITE_DATA_ID
 * @brief This macro controlls whether duplicate data packet IDs are allowed
 *        to be appended into the TX buffer.
 *
 * When set to 0, duplicate data IDs are not allowed to be appended.
 * When set to 1, duplicate data IDs are allowed to be appended.
 * @note User can set this value to 1 if he wants to allow appending packets with duplicate data IDs.
 * @warning Setting this value to anything but 0 or 1 may lead to undefined behavior.
 */
#define ALLOW_TX_APPEND_DUPLICITE_DATA_ID 1


/**
 * @brief Buffers for TX operations and all pointers for their control.
 * @details This library implements double buffering.
 * 		  	Active buffer is the one user is appending data to.
 * 		  	Prepared buffer is the one ready to be sent by comms_send() function.
 * 	  	  
 * - comms_tx_buffer1 and comms_tx_buffer2 are the two buffers used for TX operations.
 * - comms_tx_active_buffer is a pointer to the buffer that is currently being used for appending data.
 * - comms_tx_active_wr_pointer is a pointer to the first empty write position in the active buffer.
 * - comms_tx_prepared_buffer is a pointer to the buffer that is ready to be sent.
 * - comms_tx_prepared_wr_pointer is a pointer to the first empty write position in the prepared buffer.
 * - comms_tx_data_id_register is an array of pointers to the data IDs that have been written to the TX buffer.
 * @warning Do not modify any of these pointers. TX RX functions depend on them and modifying them will
 * 			lead to undefined behavior.
 */
extern uint8_t comms_tx_buffer1[MAX_TX_BUFFER_SIZE];
extern uint8_t comms_tx_buffer2[MAX_TX_BUFFER_SIZE];
extern uint8_t *comms_tx_active_buffer;
extern uint8_t *comms_tx_active_wr_pointer;
extern uint8_t *comms_tx_prepared_buffer;
extern uint8_t *comms_tx_prepared_wr_pointer;
extern void *comms_tx_data_id_register[MAX_DATA_ID + 1];

/**
 * @brief Buffers for RX operations and all pointers for their control.
 * @details This library implements double buffering.
 * 		  	Active buffer is the one user is receiving data to.
 * 		  	Prepared buffer is the one ready to be read by comms_rx_process() function.
 * 
 * - comms_rx_buffer1 and comms_rx_buffer2 are the two buffers used for RX operations.
 * - comms_rx_active_buffer is a pointer to the buffer that is currently being used for receiving data.
 * - comms_rx_active_rd_pointer is a pointer to the first empty read position in the active buffer.
 * - comms_rx_prepared_buffer is a pointer to the buffer that is ready to be processed.
 * - comms_rx_prepared_rd_pointer is a pointer to the first data packet position in the prepared buffer.
 * @warning Do not modify any of these pointers. TX RX functions depend on them and modifying them will
 * 			lead to undefined behavior.
 */
extern uint8_t comms_rx_buffer1[MAX_RX_BUFFER_SIZE]; // buffer for rx data
extern uint8_t comms_rx_buffer2[MAX_RX_BUFFER_SIZE];
extern uint8_t *comms_rx_active_buffer;
extern uint8_t *comms_rx_active_rd_pointer;
extern uint8_t *comms_rx_prepared_buffer;
extern uint8_t *comms_rx_prepared_rd_pointer;

/**
 * @brief Enum for the selected communication interface.
 */
typedef enum {
	COMMS_UART,
	COMMS_USB_OTG,
} comms_interface;

/**
 * @brief Enum for the selected communication interface.
 * @details This enum is used to select between UART and USB OTG interfaces.
 * 
 * - COMMS_UART: UART interface.
 * - COMMS_USB_OTG: USB OTG interface.
 * 
 * @note User can select the interface by setting the comms_selected_interface variable inside the .c file.
 * @warning Only one interface can be used at a time.
 */
extern comms_interface comms_selected_interface;

/**
 * @brief Enums for inner state control of the TX and RX functions.
 */
typedef enum {
	COMMS_READY,
	COMMS_INPROGRESS,
	COMMS_RECEIVED
} comms_state;

extern comms_state tx_wr_status;
extern comms_state tx_status;
extern comms_state rx_status;

/**
 * @brief Enum for the return codes of the communication functions.
 * @details These codes indicate the success or failure of the function calls.
 *          - COMMS_SUCCESS: General success.
 *          - COMMS_FAILED: General failure.
 *          - COMMS_TX_CDC_FAIL: CDC for data transfer returned anything but USB_OK.
 *          - COMMS_TX_BUFFER_EMPTY: TX buffer does not contain any data.
 * 			- COMMS_TX_LOCKED: TX function is already running elsewhere.
 * 			- COMMS_WR_LOCKED: Active buffer is already being written to.
 * 			- COMMS_TX_UART_FAIL: UART function for data transfer returned anything but HAL_OK.
 * 			- COMMS_DATA_ID_EXISTS: User tried to append a data_id which already exists in the TX buffer.
 * 			- COMMS_RX_DATA_ID_NOT_IMPLEMENTED: Data id received was not implemented in the user code.
 */
typedef enum {
	COMMS_SUCCESS,
	COMMS_FAILED,
	COMMS_TX_CDC_FAIL,
	COMMS_TX_BUFFER_EMPTY,
	COMMS_TX_LOCKED, 
	COMMS_WR_LOCKED,
	COMMS_TX_UART_FAIL,
	COMMS_DATA_ID_EXISTS,
	COMMS_RX_DATA_ID_NOT_IMPLEMENTED,
	COMMS_TX_BUFFER_FULL,
} comms_return_codes;

/**
 * @brief Enum for the inner state automat in the UART RX function.
 */
typedef enum {
	COMMS_UART_HEAD,
	COMMS_UART_PACKET_HEAD,
	COMMS_UART_PACKET_DATA,
	COMMS_UART_COMPLETE
} comms_uart_rx_state;

extern comms_uart_rx_state uart_rx_state;

/**
 * @brief Union for the data values given to the user in the comms_data_handler() function.
 * @details This union allows the user to access the data as uint8_t, uint16_t, or uint32_t.
 */
typedef union {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
} DataValue;

/**
 * @brief Struct for the data packets received from the MATLAB.
 * @details This struct contains the data ID, size, count, and the data itself.
 *          The data is stored in an array of DataValue unions, so user can access them simply.
 * 			comms_rx_process() functions parses data packets to the CommsData structure
 * 			and handles them to the user in the comms_data_handler() function.
 * @todo Find more optimal way to hand data to the user. DataValue data is 
 * 		 a static array of 255 elements, which is not optimal, but the 
 *       dynamic allocation is more CPU demanding. 
 */
typedef struct {
	uint8_t data_id;
	uint8_t data_size;
	uint8_t data_count;
	DataValue data[255];
} CommsData;

/**
 * @brief Function to transmit data over USB CDC.
 * @details This function can be located inside usbd_cdc_if.c file.
 * @warning This function is not defined in this library.
 */
extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);


/**
 * @brief Function to initialize the communication interface.
 * @details This function initializes the communication interface,
 * 			depending on the selected interface (UART or USB OTG).
 * @note This function must be called before using any other communication functions.
 * @warning Communication wont work if this function is not called.
 */
void comms_init();

/**
 * @brief Function to append data to the TX buffer.
 * @details This function appends data to the TX buffer.
 * 			It checks if the data ID already exists in the TX buffer,
 * @param data_id The ID of the data to be appended.
 * @param data_count The number of data elements to be appended.
 * @param data Pointer to the data or data array to be appended.
 * 
 * @return COMMS_SUCCESS if the data was appended successfully,
 * 		   COMMS_TX_LOCKED if the TX function is already running elsewhere,
 * 		   COMMS_WR_LOCKED if the active buffer is already being written to,
 * 		   COMMS_DATA_ID_EXISTS if the data ID already exists in the TX buffer.
 * 		   COMMS_FAILED if the data was not appended successfully.
 * @note This function can be used to append data to the TX buffer.
 */
int comms_append_int32(uint8_t data_id, uint8_t data_count, int *data);

/**
 * @brief Function to send data over the selected communication interface.
 * @details This function sends data over the selected communication interface (UART or USB OTG).
 * 			Function uses comms_switch_tx_buffers() to switch the active and prepared buffers, so the
 * 			user can append data to the active buffer while the prepared buffer is being sent.
 * @return COMMS_SUCCESS if the data was sent successfully,
 * 		   COMMS_TX_CDC_FAIL if the CDC for data transfer returned anything but USB_OK (USB OTG),
 * 		   COMMS_TX_UART_FAIL if the UART function for data transfer returned anything but HAL_OK (UART),
 * 		   COMMS_TX_BUFFER_EMPTY if the TX buffer does not contain any data.
 * @note This function can be used to send data over the selected communication interface.
 * @warning This function should be called after appending data to the TX buffer.
 */
int comms_send();

/**
 * @brief Function to process the received data.
 * @details This function checks, if there are any received data, parses the received data
 * 			and calls the comms_data_handler() function.
 * @note This function should be called in the main loop, to constantly check for new data.
 * @warning User must implement the comms_data_handler() function to handle the received data.
 * @warning Without calling this function, comms_data_handler() will not be ever called.
 */
void comms_rx_process();

/**
 * @brief Function to handle the received data.
 * @details This function is called by the comms_rx_process() function to handle the received data.
 * @param data Pointer to the received data.
 * @note User must implement this function to handle the received data.
 * @warning This function has an __weak attribute, so the user must implement it in his code. 
 * 			Example implementation is provided in the comms_data_rxtx.c file.
 * @warning This function is called by the comms_rx_process() function, it should not be called directly.
 */
__weak void comms_data_handler(CommsData *data);

/**
 * @brief Function to reset the active TX buffer.
 * @warning This function should not be called directly. Only inner logic uses it.
 */
void comms_reset_active_tx_buffer();

/**
 * @brief Function to reset the active RX buffer.
 * @warning This function should not be called directly. Only inner logic uses it.
 */
void comms_reset_active_rx_buffer();

/**
 * @brief Function to initialize the UART.
 * @note This function can be called to reset the UART interface, in case it gets stuck. 
 */
void comms_uart_init();

/**
 * @brief Function to purge the ID register.
 * @details This function resets the evidence of existing data packets in the TX buffer.
 * @warning This function should not be called directly. Only inner logic uses it.
 */
void comms_purge_id_register();

/**
 * @brief Function to find existing data in the TX buffer.
 * @param data_id The ID of the data to be found.
 * @return Pointer to the existing data packet with this data ID, or NULL if not found.
 * @warning This function should not be called directly. Only inner logic uses it.
 */
void* comms_find_existing_data(uint8_t data_id);

/**
 * @brief Function to increment the total number of data elements in the active TX buffer.
 * @warning This function should not be called directly. Only inner logic uses it.
 */
void comms_increment_active_buffer_data();

/**
 * @brief Function to switch the TX buffers.
 * @details This function switches the active and prepared TX buffers.
 *          Other interrupts are disabled while this function is running.
 * @warning This function is called by the comms_send() function, it should not be called directly.
 */
void comms_switch_tx_buffers();

/**
 * @brief Function to switch the RX buffers.
 * @details This function switches the active and prepared RX buffers.
 *          Other interrupts are disabled while this function is running.
 * @warning This function is called by RX functions, it should not be called directly.
 */
void comms_switch_rx_buffers();

/**
 * @brief Function to handle the USB CDC RX callback.
 * @param buffer Pointer to the received data buffer.
 * @param length Length of the received data.
 * @note Call this function inside a function CDC_Receive_FS() located inside usbd_cdc_if.c
 * @warning Without calling this function inside CDC_Receive_FS(), the USB RX will not work.
 */
void comms_cdc_rx_callback(uint8_t *buffer, uint32_t length);

/**
 * @brief Function to handle the UART RX callback and data loading.
 * @details This function is called by the HAL_UART_RxCpltCallback() function.
 *          It handles the incoming data and continuously loads it into the RX buffer.
 * @param huart Pointer to the UART handle.
 * @warning This function is called by the HAL_UART_RxCpltCallback() function, it should not be called directly.
 */
void comms_lpuart_rx_callback(UART_HandleTypeDef *huart);

/**
 * @brief Definition of the __weak function from HAL library to handle the UART RX complete callback.
 * @details This function is called by the HAL library when the UART RX transfer is complete.
 *          Originally defined in stm32l4xx_hal_uart.c
 * 		    It calls the comms_lpuart_rx_callback() function to handle the incoming data.
 * @param huart Pointer to the UART handle.
 * @warning This function is called by the HAL library, it should not be called directly.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif
