//****************************************************************************
//
//! \addtogroup getting_started_ap
//! @{
//
//****************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Simplelink includes
#include "simplelink.h"
#include "wlan.h"

// driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"

// free_rtos/ti-rtos includes
#include "osi.h"
#include "heap_3.c"

// common interface includes
#include "common.h"
#include "udma_if.h"
#include "hw_memmap.h"
#include "rom_map.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "uart.h"
#include "pinmux.h"

#define APP_NAME                "WLAN AP"
#define APPLICATION_VERSION     "1.1.1"
#define OSI_STACK_SIZE          2048
#define UART_BAUD_RATE_FCU      100000


//
// Values for below macros shall be modified for setting the 'Ping' properties
//

#define NUM_MY_CHANNEL          16
#define NUM_FCU_CHANNEL         16
#define SBUS_SIZE               25

#define PORT_NUM            5001
#define BUF_SIZE            1000
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

typedef struct taskParam{
    OsiSyncObj_t *pSyncObjStart;
    unsigned short usPort;
}taskParam;


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
unsigned char  g_ulStatus = 0;
unsigned long  g_ulStaIp = 0;
unsigned long  g_ulPingPacketsRecv = 0;
unsigned long  g_uiGatewayIP = 0;
unsigned char g_cBsdSendBuf[BUF_SIZE];
uint16_t g_cBsdRecvBuf[BUF_SIZE]= {0};


OsiMsgQ_t msgQfb = NULL;

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

const uint32_t _sbusBaud = 100000;
const uint8_t _sbusHeader = 0x0F;
const uint8_t _sbusFooter = 0x00;


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************

static long ConfigureSimpleLinkToDefaultState();
static void InitializeAppVariables();
int connectionManager();



#ifdef USE_FREERTOS
//*****************************************************************************
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//*****************************************************************************

void
vAssertCalled( const char *pcFile, unsigned long ulLine ){
    //Handle Assert here
    while(1) {}
}

void
vApplicationIdleHook( void){
    //Handle Idle Hook for Profiling, Power Management etc
}

void vApplicationMallocFailedHook(){
    //Handle Memory Allocation Errors
    while(1) {}
}

void vApplicationStackOverflowHook(OsiTaskHandle *pxTask, signed char *pcTaskName){
    //Handle FreeRTOS Stack Overflow
    while(1) {}
}
#endif //USE_FREERTOS

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent){

    switch(pSlWlanEvent->Event){
        case SL_WLAN_CONNECT_EVENT:
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pSlWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code){
                UART_PRINT("Device disconnected from the AP on application's "
                            "request \n\r");
            }
            else{
                UART_PRINT("Device disconnected from the AP on an ERROR..!! \n\r");
            }

        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
            // when device is in AP mode and any client connects to device cc3xxx
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
            // when client disconnects from device (AP)
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);
            break;

        default:
            UART_PRINT("[WLAN EVENT] Unexpected event \n\r");
            break;
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent){

    switch(pNetAppEvent->Event){
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        case SL_NETAPP_IPV6_IPACQUIRED_EVENT:
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);
            break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);
            g_ulStaIp = (pNetAppEvent)->EventData.ipLeased.ip_address;
            UART_PRINT("[NETAPP EVENT] IP Leased to Client: IP=%d.%d.%d.%d , ",
                        SL_IPV4_BYTE(g_ulStaIp,3), SL_IPV4_BYTE(g_ulStaIp,2),
                        SL_IPV4_BYTE(g_ulStaIp,1), SL_IPV4_BYTE(g_ulStaIp,0));
        }
        break;

        case SL_NETAPP_IP_RELEASED_EVENT:
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);
            UART_PRINT("[NETAPP EVENT] IP Released for Client: IP=%d.%d.%d.%d , ",
                        SL_IPV4_BYTE(g_ulStaIp,3), SL_IPV4_BYTE(g_ulStaIp,2),
                        SL_IPV4_BYTE(g_ulStaIp,1), SL_IPV4_BYTE(g_ulStaIp,0));
            break;

        default:
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r", pNetAppEvent->Event);
            break;
    }
}


void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse){
    // Unused in this application
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent){
    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


void SimpleLinkSockEventHandler(SlSockEvent_t *pSock){
    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event ){
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status){
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                    break;
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
            break;
    }

}

void SimpleLinkPingReport(SlPingReport_t *pPingReport){

    SET_STATUS_BIT(g_ulStatus, STATUS_BIT_PING_DONE);
    g_ulPingPacketsRecv = pPingReport->PacketsReceived;
}

static void InitializeAppVariables(){

    g_ulStatus = 0;
    g_ulStaIp = 0;
    g_ulPingPacketsRecv = 0;
    g_uiGatewayIP = 0;
}

static long ConfigureSimpleLinkToDefaultState(){

    //UART_PRINT("Enter configslDefState\n\r");
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    //UART_PRINT("sl_start\n\r");
    lMode = sl_Start(0, 0, 0);
    //UART_PRINT("end sl_start\n\r");
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode
    if (ROLE_STA != lMode){
        if (ROLE_AP == lMode){
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(g_ulStatus)){
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
            }
        }

        // Switch to STA role and restart
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again
        if (ROLE_STA != lRetVal){
            // We don't want to proceed if the device is not coming up in STA-mode
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }

    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);


    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal){
        // Wait
        while(IS_CONNECTED(g_ulStatus)){
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();

    return lRetVal; // Success
}


static int ConfigureMode(int iMode){

    char    pcSsidName[] = "High-Rise";
    char    password[] = "pierre12";
    long    lRetVal = -1;
    _u8  val = SL_SEC_TYPE_WPA_WPA2;

    //UART_PRINT("Enter the AP SSID name: ");
    //GetSsidName(pcSsidName,33);
    UART_PRINT("AP SSID name: %s\n\r", pcSsidName);

    lRetVal = sl_WlanSetMode(ROLE_AP);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(pcSsidName),
                         (unsigned char*)pcSsidName);
    ASSERT_ON_ERROR(lRetVal);

    //Set no password
    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1,
                         (_u8*)&val);

    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD, strlen(password),
                         (unsigned char *)password);
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT("Device is configured in AP mode\n\r");

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
}


void printBinary(unsigned char byte) {
    UART_PRINT("%d%d%d%d%d%d%d%d = %d\n\r", \
        (byte & 0x80 ? 1 : 0), \
        (byte & 0x40 ? 1 : 0), \
        (byte & 0x20 ? 1 : 0), \
        (byte & 0x10 ? 1 : 0), \
        (byte & 0x08 ? 1 : 0), \
        (byte & 0x04 ? 1 : 0), \
        (byte & 0x02 ? 1 : 0), \
        (byte & 0x01 ? 1 : 0), \
        byte);
}

void printdBinary(uint16_t dbyte) {
    UART_PRINT("%d%d%d%d%d%d%d%d %d%d%d%d%d%d%d%d = %d\n\r", \
            (dbyte & 0x8000 ? 1 : 0), \
            (dbyte & 0x4000 ? 1 : 0), \
            (dbyte & 0x2000 ? 1 : 0), \
            (dbyte & 0x1000 ? 1 : 0), \
            (dbyte & 0x0800 ? 1 : 0), \
            (dbyte & 0x0400 ? 1 : 0), \
            (dbyte & 0x0200 ? 1 : 0), \
            (dbyte & 0x0100 ? 1 : 0), \
            (dbyte & 0x0080 ? 1 : 0), \
            (dbyte & 0x0040 ? 1 : 0), \
            (dbyte & 0x0020 ? 1 : 0), \
            (dbyte & 0x0010 ? 1 : 0), \
            (dbyte & 0x0008 ? 1 : 0), \
            (dbyte & 0x0004 ? 1 : 0), \
            (dbyte & 0x0002 ? 1 : 0), \
            (dbyte & 0x0001 ? 1 : 0), \
            dbyte);
}


void TCPServerTask( void *pvParameters ){

    unsigned char   ucDHCP;
    long            lRetVal = -1;
    int             iCounter;
    //long            lLoopCount = 0;
    //size_t          lenfdp;

    taskParam *pxParameters;
    pxParameters = (taskParam*) pvParameters;

    InitializeAppVariables();


    // filling the buffer
    for (iCounter=0 ; iCounter<TCP_PACKET_COUNT ; iCounter++){
        g_cBsdSendBuf[iCounter] = (char)(iCounter+65);
        //UART_PRINT("icounter=%d", iCounter);
    }

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
    lRetVal = ConfigureSimpleLinkToDefaultState();
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
        if(ConfigureMode(lRetVal) != ROLE_AP){
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

    while(1){
        //UART_PRINT("Task 1 start\n\r");
        connectionManager();
    }
}


int connectionManager() {

    SlSockAddrIn_t  sLocalAddr;
    SlSockAddrIn_t  sAddr;
    int             iSockID;
    int             iAddrSize;
    int             iStatus;
    long            lNonBlocking = 1;
    int             iNewSockID;
    int             iTestBufLen;
    int             i;
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
                            //UART_PRINT("Task 1 execute\n\r");
                            if(!IS_IP_LEASED(g_ulStatus)) {
                                UART_PRINT("Connect a client to device...\n\r");
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                return -1;
                            }
                            iStatus = sl_Recv(iNewSockID, g_cBsdRecvBuf, iTestBufLen, 0);
                            if (iStatus == SL_EAGAIN) {
                                //MAP_UtilsDelay(10000);
                            }
                            else if (iStatus > 0) {
                                /*iStatus = sl_Send(iNewSockID, g_cBsdSendBuf, iTestBufLen, 0);*/
                            }
                            else {
                                sl_Close(iNewSockID);
                                sl_Close(iSockID);
                                break;
                            }
                        }
                    }
                }
            }
        }
        UART_PRINT("[TCP Server Task] Error occurred\n\r");
    }
    return -1; // Wifi client disconnected
}

/*void DroneStatusFeedBack( void *pvParameters ) {

}*/

uint16_t mapValue(uint8_t val, int out_min, int out_max) {
    return val * (out_max - out_min)/255 + out_min;
}

void CommandManagerTask(void *pvParameters) {

    static uint16_t fcuChannels[NUM_FCU_CHANNEL]={0}, myChannels[NUM_MY_CHANNEL]={0};
    uint8_t sbusBytes[SBUS_SIZE]={0};
    int i;
    static TickType_t xLastWakeTime; //, first, second;
    static const TickType_t xFrequency = 500; // SBUS protocol sends every 14ms (analog mode) or 7ms (highspeed mode)

    /*MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1), \
                    UART_BAUD_RATE_FCU, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_TWO | \
                    UART_CONFIG_PAR_EVEN));*/

    while(1){
        xLastWakeTime = xTaskGetTickCount();
        if(IS_IP_LEASED(g_ulStatus)){

            /*
            * g_cBsdRecvBuf = [25 SBUS formatted packets, 15 channels containing key events]
            */
            for (i=0; i<NUM_FCU_CHANNEL; i++){
                fcuChannels[i] = g_cBsdRecvBuf[i];
            }
            //first = xTaskGetTickCount()-xLastWakeTime;

            for (i=0; i<NUM_MY_CHANNEL; i++){
                myChannels[i] = g_cBsdRecvBuf[i+NUM_FCU_CHANNEL];
            }

            sbusBytes[0]  = _sbusHeader;
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
            sbusBytes[24] = _sbusFooter;

            ClearTerm();
            UART_PRINT("\33[%dA", 100); // Move terminal cursor up %d lines

            for (i=0; i<SBUS_SIZE; i++) {
                //MAP_UARTCharPut(UARTA1_BASE, packet[i]);
                UART_PRINT("sbus[%d]=", i);
                printBinary(sbusBytes[i]);
                //UART_PRINT("fcuChannels[%d]: %02X\n\r", i, fcuChannels[i]);
            }
        }
        UART_PRINT("Execution time : %d\n\r", xTaskGetTickCount()-xLastWakeTime);
        vTaskDelayUntil( &xLastWakeTime, xFrequency);
    }
}

static void
DisplayBanner(char * AppName) {
    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t       CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}



static void
BoardInit(void) {
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


//*****************************************************************************
//                            MAIN FUNCTION
//*****************************************************************************
void main() {

    long lRetVal = -1;

    BoardInit();
    PinMuxConfig();

#ifndef NOTERM
    InitTerm();
#endif

    //UART_PRINT("Enter main\n\r");

    ClearTerm();
    DisplayBanner(APP_NAME);

    //
    // Start the SimpleLink Host
    //
    OsiSyncObj_t pSyncObjStart;
    taskParam *pvParameters;
    OsiTaskHandle pServerHandle, pCommandTaskHandle;
    pvParameters = (taskParam *) mem_Malloc(sizeof(taskParam));

    lRetVal = osi_SyncObjCreate(&pSyncObjStart);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //lRetVal = osi_MsgQCreate(&msgQfb,"FeedBack",);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    pvParameters->pSyncObjStart = &pSyncObjStart;
    pvParameters->usPort = PORT_NUM;


    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the WlanAPMode task
    //

    lRetVal = osi_TaskCreate(TCPServerTask, \
                      (const signed char*)"wireless LAN in AP mode", \
                      OSI_STACK_SIZE, (void*) pvParameters, tskIDLE_PRIORITY+1, &pServerHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    lRetVal = osi_TaskCreate(CommandManagerTask, \
                      (const signed char*)"Process command from controller", \
                      OSI_STACK_SIZE, (void*) pvParameters, tskIDLE_PRIORITY+2, &pCommandTaskHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    /*lRetVal = osi_TaskCreate(DroneStatusFeedBack, \
                          (const signed char*)"Send drone state", \
                          OSI_STACK_SIZE, (void*) pvParameters, tskIDLE_PRIORITY+2, &pCommandTaskHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }*/

    //
    // Start the task scheduler
    //
    UART_PRINT("Start scheduler\n\r");
    osi_start();
    //LOOP_FOREVER();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
