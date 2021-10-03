/*
 * network_utils.h
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#ifndef NETWORK_UTILS_H_
#define NETWORK_UTILS_H_

#define SSID_DRONE       "High-Rise";
#define PASSWORD        "pierre12";

#define     BUF_SIZE            960


typedef struct {
    unsigned char  ulStatus;
    unsigned long  ulStaIp;
    unsigned long  ulPingPacketsRecv;
    unsigned long  uiGatewayIP;
    unsigned char  acBsdSendBuf[BUF_SIZE];
    unsigned char  acBsdRecvBuf[BUF_SIZE];
}NETWORK_CONTEXT, *PNETWORK_CONTEXT;



// Application specific status/error codes
typedef enum{
    // Choosing this number to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    SOCKET_CREATE_ERROR = LAN_CONNECTION_FAILED - 1,
    BIND_ERROR = SOCKET_CREATE_ERROR - 1,
    LISTEN_ERROR = BIND_ERROR -1,
    SOCKET_OPT_ERROR = LISTEN_ERROR -1,
    CONNECT_ERROR = SOCKET_OPT_ERROR -1,
    ACCEPT_ERROR = CONNECT_ERROR - 1,
    SEND_ERROR = ACCEPT_ERROR -1,
    RECV_ERROR = SEND_ERROR -1,
    SOCKET_CLOSE_ERROR = RECV_ERROR -1,
    CLIENT_CONNECTION_FAILED = SOCKET_CLOSE_ERROR -1,
    DEVICE_NOT_IN_STATION_MODE = CLIENT_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;



long ConfigureSimpleLinkToDefaultState(PNETWORK_CONTEXT pNetCtx);
void InitializeAppVariables(PNETWORK_CONTEXT pNetCtx);
int  ConfigureMode(int iMode, PNETWORK_CONTEXT pNetCtx);


#endif /* NETWORK_UTILS_H_ */
