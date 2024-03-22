#include "uart.h"
#include <stdio.h>

UART_HandleTypeDef *myHuart;

int rxBufferGp;									//get pointer (read)
int rxBufferPp;									//put pointer (write)
uint8_t rxBuffer[rxBufferMax];
uint8_t rxChar;

// init device
void initUart(UART_HandleTypeDef *inHuart){
	myHuart = inHuart;
	HAL_UART_Receive_IT(myHuart, &rxChar, 1);	//h = handle arrive 3 --> Start interrupt
}

//ASCII Method//
//process received character
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	rxBuffer[rxBufferPp++] = rxChar;
	rxBufferPp %= rxBufferMax;
	HAL_UART_Receive_IT(myHuart, &rxChar, 1);
}

//get character from buffer
int16_t getChar(){
	int16_t result;
	if(rxBufferGp == rxBufferPp) return -1; // not use -1 anywhere, so uint8_t --> int16_t
	result = rxBuffer[rxBufferGp++];
	rxBufferGp %= rxBufferMax;
	return result;
}

// binary data transmit , Little Endian method
void binaryTransmit(protocol_t inData){
	uint8_t txBuffer[] = {STX, 0, 0, 0, 0, 0, 0, 0, ETX};
	// data copy
	//memcpy(&txBuffer[1], &inData, 6); // copy six data
	txBuffer[1] = inData.id | 0x80;
	txBuffer[2] = inData.command | 0x80;
	txBuffer[3] = inData.data | 0x80;
	txBuffer[4] = (inData.data >> 7) | 0x80;
	txBuffer[5] = (inData.data >> 14) | 0x80;
	txBuffer[6] = (inData.data >> 21) | 0x80;
	// CRC calculation
	for(int i = 0; i < 7 ; i++){
		txBuffer[7] += txBuffer[i];
	}
	// transmit
	HAL_UART_Transmit(myHuart, txBuffer, sizeof(txBuffer), 10);
}

int _write(int file, char *p, int len) {
   HAL_UART_Transmit(myHuart, p, len, 10);
   return len;
}


//packet transmit
void transmitPacket(radarprotocol_t data){
	//prepare
	uint8_t txBuffer[] = {STX, 0, 0, 0, 0, ETX};
	txBuffer[1] = data.command;
	txBuffer[2] = (data.data >> 7) | 0x80;
	txBuffer[3] = (data.data & 0x7f) | 0x80;
	//Calculation CRC
	txBuffer[4] = txBuffer[0] + txBuffer[1] + txBuffer[2] + txBuffer[3];
	//transmit Data
	HAL_UART_Transmit(myHuart, txBuffer, sizeof(txBuffer), 1);
	//Standby data transmit finish
	while((HAL_UART_GetState(myHuart) == HAL_UART_STATE_BUSY_TX)
			|| (HAL_UART_GetState(myHuart) == HAL_UART_STATE_BUSY_TX_RX));
}

//packet receive
radarprotocol_t receivePacket(){
	radarprotocol_t result;
	uint8_t buffer[6];
	uint8_t count = 0;
	uint32_t timeout;

	int16_t ch = getChar();
	memset(&result, 0, sizeof(result));
	if(ch == STX){
		buffer[count++] = ch;
		timeout = HAL_GetTick(); //start to time
		while(ch != ETX){
			ch = getChar();
			if(ch != -1){
				buffer[count++] = ch;
			}
			//calculate to timeout
			if(HAL_GetTick() - timeout >= 2) return result; // 6byte data speed : 0.5ms very good
		}
		// CRC check
		uint8_t crc = 0;
		for(int i = 0; i < 4; i++){
			crc += buffer[i];
		}
		if(crc != buffer[4]) return result;
		//after finish transmit, parsing
		result.command = buffer[1];
		result.data = buffer[3] & 0x7f;
		result.data |= (buffer[2] & 0x7f) << 7;
	}
	return result;
}
