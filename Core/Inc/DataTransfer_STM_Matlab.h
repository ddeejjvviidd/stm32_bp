#include "main.h"
#include "usb_device.h"

#define MAX_TXRX_DATA 4096 //200
uint32_t buf_M_RX[MAX_TXRX_DATA + 1];//+4bytes for head
uint32_t buf_M_TX[MAX_TXRX_DATA + 1];
uint32_t m2s_buf[MAX_TXRX_DATA];//
int s2m_Status;// transmit to Matlab
int m2s_Status;// received status 0...waiting for incomming data, 1...call m2s_Process, -1... init receiving, 2...n data , 3... xData
int m2s_ID;
int m2s_nData_in_bytes;
)
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
extern void DataReceive_MTLB_Callback(uint16_t iD, uint32_t * xData, uint16_t nData_in_values);

// Send float or uint32 or none data
int DataTransmit2MTLB(uint16_t iD, uint8_t * xData, uint16_t nData_in_values)
{
	if(s2m_Status) return -1;
	if((sizeof(buf_M_TX)-4)<(nData_in_values*4)) return -2;

	s2m_Status=1;

	((uint16_t *) buf_M_TX)[0] = iD;
	((uint16_t *) buf_M_TX)[1] = nData_in_values;
	if(nData_in_values>0) memcpy(buf_M_TX+1, xData, nData_in_values*4);

	s2m_Status = CDC_Transmit_FS((uint8_t*) buf_M_TX, nData_in_values*4 + 4);

	if(s2m_Status)
	{
		s2m_Status=0;//if on zero... USB busy
	}
	return 0;
}
int SendData2MTLB(uint16_t iD, uint8_t * xData, uint16_t nData_in_values){
	return DataTransmit2MTLB(iD, xData, nData_in_values);
}

int SendInt2MTLB(uint16_t iD, int * xInt){

	//uint8_t tx_int[sizeof(int)];

	//uint8_t *ptr = xInt;
	//ptr += sizeof(uint8_t);


//	for (int i = 0; i < sizeof(int); ++i) {
//
//
//		//tx_int[i] = (*(uint8_t *)(xInt) >> (i * 8)) & 0xFF;
//        tx_int[sizeof(int) - 1 - i] = (*(uint8_t *)(xInt) >> (i * 8)) & 0xFF;
//		//tx_int[i] = (uint8_t)(tx_int >> (i * 8)); // Posunutí bytů integer hodnoty a přetypování
//	}
//	return DataTransmit2MTLB(iD, tx_int, sizeof(int));
	return DataTransmit2MTLB(iD, (uint8_t *)xInt, 1);
}

//int SendPotenciometer2MTLB(uint16_t iD, )

void m2s_Process(void)//called from inf. loop
{
	if(!m2s_Status) return;//the most often ....
	if(m2s_Status==1)
	{
		DataReceive_MTLB_Callback(m2s_ID, m2s_buf, m2s_nData_in_bytes/4);
		m2s_Status = 0;
		return;
	}
	if(m2s_Status==2)
	{
	}

	if(m2s_Status== -1)//init receiving new message from matlab
	{
		m2s_Status = 0;
		return;
	}

}
void USB_My_Receive(uint8_t* Buf, uint32_t Len)
{
	if(m2s_Status==0)//new message
	{
		  m2s_ID = ((uint16_t *) Buf)[0] ;
		  if(m2s_ID == 0)
			    return;
		  m2s_Status=100;
		  return;
	}
	if(m2s_Status==100)
	{
		  m2s_nData_in_bytes = ((uint16_t *) Buf)[0] *4; //
		  if(m2s_nData_in_bytes == 0)
		  {
			  m2s_Status=1;
			  return;
		  }
		  m2s_Status=3;//wait for xData
		  return;
	}
	if(m2s_Status==3)//xData
	{
		  if(Len<m2s_nData_in_bytes)
			  m2s_nData_in_bytes=Len;
		  memcpy(m2s_buf, Buf, m2s_nData_in_bytes);
		  m2s_Status=1;
		  return;
	}

	return;
}
