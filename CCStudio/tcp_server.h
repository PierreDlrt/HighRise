/*
 * tcp_server.h
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_


typedef struct {
    unsigned char  ulStatus = 0;
    unsigned long  ulStaIp = 0;
    unsigned long  ulPingPacketsRecv = 0;
    unsigned long  uiGatewayIP = 0;
    unsigned char  acBsdSendBuf[BUF_SIZE];
    unsigned char  acBsdRecvBuf[BUF_SIZE];
}NETWORK_CONTEXT, *PNETWORK_CONTEXT;



void TCPServerTask( void *pvParameters );


#endif /* TCP_SERVER_H_ */
