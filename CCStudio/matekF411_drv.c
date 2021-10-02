/*
 * matekF411_drv.c
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#include "cc3200_sdk.h"


#define NUM_MY_CHANNEL          16
#define NUM_FCU_CHANNEL         16
#define SBUS_SIZE               25

#define SBUS_HEADER             0x0F
#define SBUS_FOOTER             0x00



void CommandManagerTask(void *pvParameters) {

    uint16_t* acBsdRecvBuf = (uint16_t*) pvParameters;
    static uint8_t sbusBytes[SBUS_SIZE]={0};
    int i;
    //static TickType_t xLastWakeTime, endWhile; //, first, second;
    static const TickType_t xFrequency = 10; // SBUS protocol sends every 14ms (analog mode) or 7ms (high speed mode)
    static uint16_t fcuChannels[NUM_FCU_CHANNEL]={0}, myChannels[NUM_MY_CHANNEL]={0};

    for(;;){
        //UART_PRINT("T2\n\r");
        xLastWakeTime = xTaskGetTickCount();
        //UART_PRINT("4:%d\n\r",xTaskGetTickCount());
        //UART_PRINT("A\n\r");
        if(IS_IP_LEASED(g_ulStatus)){

            /*
            * g_cBsdRecvBuf = [25 SBUS formatted packets, 15 channels containing key events]
            */

            for (i=0; i<NUM_FCU_CHANNEL; i++){
                fcuChannels[i] = acBsdRecvBuf[i];
            }

            for (i=0; i<NUM_MY_CHANNEL; i++){
                myChannels[i] = acBsdRecvBuf[i+NUM_FCU_CHANNEL];
            }

            sbusBytes[0]  = SBUS_HEADER;
            sbusBytes[1]  = (uint8_t) ((fcuChannels[0] & 0x07FF));
            sbusBytes[2]  = (uint8_t) ((fcuChannels[0] & 0x07FF)>>8 | (fcuChannels[1] & 0x07FF)<<3);
            sbusBytes[3]  = (uint8_t) ((fcuChannels[1] & 0x07FF)>>5 | (fcuChannels[2] & 0x07FF)<<6);
            sbusBytes[4]  = (uint8_t) ((fcuChannels[2] & 0x07FF)>>2);
            sbusBytes[5]  = (uint8_t) ((fcuChannels[2] & 0x07FF)>>10 | (fcuChannels[3] & 0x07FF)<<1);
            sbusBytes[6]  = (uint8_t) ((fcuChannels[3] & 0x07FF)>>7 | (fcuChannels[4] & 0x07FF)<<4);
            sbusBytes[7]  = (uint8_t) ((fcuChannels[4] & 0x07FF)>>4 | (fcuChannels[5] & 0x07FF)<<7);
            sbusBytes[8]  = (uint8_t) ((fcuChannels[5] & 0x07FF)>>1);
            sbusBytes[9]  = (uint8_t) ((fcuChannels[5] & 0x07FF)>>9 | (fcuChannels[6] & 0x07FF)<<2);
            sbusBytes[10] = (uint8_t) ((fcuChannels[6] & 0x07FF)>>6 | (fcuChannels[7] & 0x07FF)<<5);
            sbusBytes[11] = (uint8_t) ((fcuChannels[7] & 0x07FF)>>3);
            sbusBytes[12] = (uint8_t) ((fcuChannels[8] & 0x07FF));
            sbusBytes[13] = (uint8_t) ((fcuChannels[8] & 0x07FF)>>8 | (fcuChannels[9] & 0x07FF)<<3);
            sbusBytes[14] = (uint8_t) ((fcuChannels[9] & 0x07FF)>>5 | (fcuChannels[10] & 0x07FF)<<6);
            sbusBytes[15] = (uint8_t) ((fcuChannels[10] & 0x07FF)>>2);
            sbusBytes[16] = (uint8_t) ((fcuChannels[10] & 0x07FF)>>10 | (fcuChannels[11] & 0x07FF)<<1);
            sbusBytes[17] = (uint8_t) ((fcuChannels[11] & 0x07FF)>>7 | (fcuChannels[12] & 0x07FF)<<4);
            sbusBytes[18] = (uint8_t) ((fcuChannels[12] & 0x07FF)>>4 | (fcuChannels[13] & 0x07FF)<<7);
            sbusBytes[19] = (uint8_t) ((fcuChannels[13] & 0x07FF)>>1);
            sbusBytes[20] = (uint8_t) ((fcuChannels[13] & 0x07FF)>>9 | (fcuChannels[14] & 0x07FF)<<2);
            sbusBytes[21] = (uint8_t) ((fcuChannels[14] & 0x07FF)>>6 | (fcuChannels[15] & 0x07FF)<<5);
            sbusBytes[22] = (uint8_t) ((fcuChannels[15] & 0x07FF)>>3);
            sbusBytes[23] = 0x00;
            sbusBytes[24] = SBUS_FOOTER;

            //ClearTerm();
            //UART_PRINT("\33[%dA", 30); // Move terminal cursor up %d lines
            //UART_PRINT("5:%d\n\r",xTaskGetTickCount());
            for (i=0; i<SBUS_SIZE; i++) {
                MAP_UARTCharPut(UARTA1_BASE, sbusBytes[i]);//sbusBytes[i]);
                //Message("test\n\r");
                //UART_PRINT("%02X ", sbusBytes[i]);
                //printBinary(sbusBytes[i]);
            }
            //endWhile = xTaskGetTickCount()-xLastWakeTime;
            //UART_PRINT("%d | %d\n\r", pccount, recvCall);
        }
        //UART_PRINT("B\n\r");
        //UART_PRINT("6:%d\n\r",xTaskGetTickCount());
        //UART_PRINT("%d\n\r", endWhile);
        //UART_PRINT("end T2\n\r");
        vTaskDelayUntil( &xLastWakeTime, xFrequency);
    }
}



