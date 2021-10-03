/*
 * server.c
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#include "cc3200_sdk.h"

#define     TCP_PACKET_COUNT    50
#define     PORT_NUM            5001



int connectionManager(PNETWORK_CONTEXT pNetCtx) {

    SlSockAddrIn_t  sLocalAddr;
    SlSockAddrIn_t  sAddr;
    int             iSockID;
    int             iAddrSize;
    int             iStatus;
    long            lNonBlocking = 1;
    int             iNewSockID;
    static TickType_t xLastWakeTime;
    static const TickType_t xFrequency = 20;

    while(IS_IP_LEASED(pNetCtx->ulStatus)){
        //waiting for the client to connect
        UART_PRINT("Client is connected to Device\n\r");

        ////////////////////////// Client connected to Device /////////////////////////////

        //filling the TCP server socket address
        sLocalAddr.sin_family = SL_AF_INET;
        sLocalAddr.sin_port = sl_Htons((unsigned short) PORT_NUM);
        sLocalAddr.sin_addr.s_addr = 0;

        // creating a TCP socket
        iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
        if( iSockID < 0 ){
            // error
            ASSERT_ON_ERROR(SOCKET_CREATE_ERROR);
        }
        else {
            iAddrSize = sizeof(SlSockAddrIn_t);

            // binding the TCP socket to the TCP server address
            if(sl_Bind(iSockID, (SlSockAddr_t *)&sLocalAddr, iAddrSize)){
                // error
                sl_Close(iSockID);
                ERR_PRINT(BIND_ERROR);
            }
            else {
                // putting the socket for listening to the incoming TCP connection
                if(sl_Listen(iSockID, 0)){
                    sl_Close(iSockID);
                    ERR_PRINT(LISTEN_ERROR);
                }
                else {
                    // setting socket option to make the socket as non blocking
                    if(sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
                                     &lNonBlocking, sizeof(lNonBlocking))){
                        sl_Close(iSockID);
                        ERR_PRINT(SOCKET_OPT_ERROR);
                    }
                    else {
                        UART_PRINT("Waiting for TCP client connection...\n\r");
                        iNewSockID = SL_EAGAIN;

                        // waiting for an incoming TCP connection
                        while( iNewSockID < 0 ){
                            // accepts a connection form a TCP client, if there is any
                            // otherwise returns SL_EAGAIN
                            iNewSockID = sl_Accept(iSockID, ( struct SlSockAddr_t *)&sAddr,
                                                    (SlSocklen_t*)&iAddrSize);
                            if( iNewSockID == SL_EAGAIN ){
                               MAP_UtilsDelay(10000);
                            }
                            else if( iNewSockID < 0 ){
                                // error
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                ERR_PRINT(ACCEPT_ERROR);
                            }
                        }
                        UART_PRINT("TCP client connected\n\r");

                        if(sl_SetSockOpt(iNewSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
                                                &lNonBlocking, sizeof(lNonBlocking))){
                            ERR_PRINT(SOCKET_OPT_ERROR);
                            sl_Close(iNewSockID);
                            sl_Close(iSockID);
                            break;
                        }

                        // waits for 1000 packets from the connected TCP client
                        UART_PRINT("Waiting for incoming message...\n\r");
                        while (1) {
                            xLastWakeTime = xTaskGetTickCount();
                            //UART_PRINT("1\n\r");
                            if(!IS_IP_LEASED(pNetCtx->ulStatus)) {
                                UART_PRINT("Connect a client to device...\n\r");
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                return -1;
                            }

                            iStatus = sl_Recv(iNewSockID, pNetCtx->acBsdRecvBuf, BUF_SIZE, 0);
;
                            if (iStatus == SL_EAGAIN) {
                                // do nothing
                            }
                            else if (iStatus > 0) {

                            }
                            else {
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                break;
                            }
                            vTaskDelayUntil( &xLastWakeTime, xFrequency);
                        }
                    }
                }
            }
        }
        UART_PRINT("[TCP Server Task] Error occurred\n\r");
    }
    return -1; // Wifi client disconnected
}



void TCPServerTask(void *pvParameters) {

    PNETWORK_CONTEXT pNetCtx = (PNETWORK_CONTEXT) pvParameters;
    unsigned char   ucDHCP;
    long            lRetVal = -1;

    InitializeAppVariables(pNetCtx);

    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState(pNetCtx);
    if(lRetVal < 0){
        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
            UART_PRINT("Failed to configure the device in its default state \n\r");

        LOOP_FOREVER();
    }

    UART_PRINT("Device is configured in default state \n\r");

    //
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //

    lRetVal = sl_Start(NULL,NULL,NULL);

    if (lRetVal < 0){
        UART_PRINT("Failed to start the device \n\r");
        LOOP_FOREVER();
    }

    UART_PRINT("Device started as STATION \n\r");

    //
    // Configure the networking mode and ssid name(for AP mode)
    //
    if(lRetVal != ROLE_AP){
        if(ConfigureMode(lRetVal, pNetCtx) != ROLE_AP){
            UART_PRINT("Unable to set AP mode, exiting Application...\n\r");
            sl_Stop(SL_STOP_TIMEOUT);
            LOOP_FOREVER();
        }
    }

    while(!IS_IP_ACQUIRED(pNetCtx->ulStatus)){
        //looping till ip is acquired
    }

    unsigned char len = sizeof(SlNetCfgIpV4Args_t);
    SlNetCfgIpV4Args_t ipV4 = {0};

    // get network configuration
    lRetVal = sl_NetCfgGet(SL_IPV4_AP_P2P_GO_GET_INFO,&ucDHCP,&len,
                            (unsigned char *)&ipV4);
    if (lRetVal < 0){
        UART_PRINT("Failed to get network configuration \n\r");
        LOOP_FOREVER();
    }

    for(;;){
        connectionManager(pNetCtx);
    }
}




