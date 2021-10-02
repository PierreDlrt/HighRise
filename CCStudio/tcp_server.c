/*
 * server.c
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#include "cc3200_sdk.h"

#define TCP_PACKET_COUNT    50


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


int connectionManager(uint16_t* acBsdRecvBuf) {

    SlSockAddrIn_t  sLocalAddr;
    SlSockAddrIn_t  sAddr;
    int             iSockID;
    int             iAddrSize;
    int             iStatus;
    long            lNonBlocking = 1;
    int             iNewSockID;
    int             iTestBufLen;
    int             i;
    TickType_t      recv1 = xTaskGetTickCount();//, recv2 = xTaskGetTickCount();
    static TickType_t xLastWakeTime;
    static const TickType_t xFrequency = 20;
    //int             cpt=0;
    //unsigned long   num;

    iTestBufLen  = BUF_SIZE;

    while(IS_IP_LEASED(g_ulStatus)){
        //waiting for the client to connect
        UART_PRINT("Client is connected to Device\n\r");
        //UART_PRINT("g_cBsdRecvBuf = %s\n\r", g_cBsdRecvBuf);

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
                            if(!IS_IP_LEASED(g_ulStatus)) {
                                UART_PRINT("Connect a client to device...\n\r");
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                return -1;
                            }

                            //recv1 = xTaskGetTickCount();
                            //UART_PRINT("Recv: %d\n\r", recv1 - recv2);
                            //recv2 = xTaskGetTickCount();
                            //UART_PRINT("recv\n\r");
                            //UART_PRINT("recv\n\r");
                            //UART_PRINT("2\n\r");
                            iStatus = sl_Recv(iNewSockID, acBsdRecvBuf, iTestBufLen, 0);
                            //UART_PRINT("[0]=%d\n\r", g_cBsdRecvBuf[0]);
                            //UART_PRINT("3\n\r");
                            if (iStatus == SL_EAGAIN) {
                                //UART_PRINT("4\n\r");
                                //recvCall = 0;
                                //pccount = 0;
                                //vTaskDelayUntil( &xLastWakeTime, xFrequency);
                            }
                            else if (iStatus > 0) {

                                //recvCall = xTaskGetTickCount() - recv1;
                                //recv1 = xTaskGetTickCount();
                                //UART_PRINT("5\n\r");
                                //UART_PRINT("PC:%d\n\r", iStatus);
                                //pccount = iStatus;
                            }
                            else {
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                break;
                            }
                            //UART_PRINT("end T1\n\r");
                            //UART_PRINT("6\n\r");
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
    int             iCounter;
    //long            lLoopCount = 0;
    //size_t          lenfdp;

    //taskParam *pxParameters;
    //pxParameters = (taskParam*) pvParameters;

    InitializeAppVariables(pNetCtx);


    // filling the buffer
    /*for (iCounter=0 ; iCounter<TCP_PACKET_COUNT ; iCounter++){
        acBsdSendBuf[iCounter] = (char)(iCounter+65);
        //UART_PRINT("icounter=%d", iCounter);
    }*/

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
    //UART_PRINT("WlanAPEntry\n\r");
    lRetVal = ConfigureSimpleLinkToDefaultState(pNetCtx);
    if(lRetVal < 0){
        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
            UART_PRINT("Failed to configure the device in its default state \n\r");

        LOOP_FOREVER();
    }

    UART_PRINT("Device is configured in default state \n\r");

    //
    // Asumption is that the device is configured in station mode already
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

    while(!IS_IP_ACQUIRED(g_ulStatus)){
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
        //UART_PRINT("Task 1 start\n\r");
        connectionManager(pNetCtx->acBsdRecvBuf);
    }
}




