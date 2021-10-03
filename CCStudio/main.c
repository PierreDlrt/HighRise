//****************************************************************************
//
//! \addtogroup drone_project
//! @{
//
//****************************************************************************

#include "cc3200_sdk.h"

//#include "heap_3.c"


#define APP_NAME                "WLAN AP"
#define APPLICATION_VERSION     "1.1.1"
#define OSI_STACK_SIZE          2048
#define UART_BAUD_RATE_FCU      100000


//
// Values for below macros shall be modified for setting the 'Ping' properties
//

/*typedef struct {
    OsiSyncObj_t *pSyncObjStart;
    unsigned short usPort;
}taskParam;*/


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************


PNETWORK_CONTEXT pNetCtx = NULL;
//OsiMsgQ_t msgQfb = NULL;

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************


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


uint16_t mapValue(uint8_t val, int out_min, int out_max) {
    return val * (out_max - out_min)/255 + out_min;
}


static void
DisplayBanner() {
    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t       CC3200 "APP_NAME" Application       \n\r");
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



void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent){

    switch(pSlWlanEvent->Event){
        case SL_WLAN_CONNECT_EVENT:
            SET_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_CONNECTION);
            break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_IP_AQUIRED);

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
            SET_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_CONNECTION);
            break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
            // when client disconnects from device (AP)
            CLR_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_IP_LEASED);
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
            SET_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_IP_AQUIRED);
            break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_IP_LEASED);
            pNetCtx->ulStaIp = (pNetAppEvent)->EventData.ipLeased.ip_address;
            UART_PRINT("[NETAPP EVENT] IP Leased to Client: IP=%d.%d.%d.%d , ",
                        SL_IPV4_BYTE(pNetCtx->ulStaIp,3), SL_IPV4_BYTE(pNetCtx->ulStaIp,2),
                        SL_IPV4_BYTE(pNetCtx->ulStaIp,1), SL_IPV4_BYTE(pNetCtx->ulStaIp,0));
        }
        break;

        case SL_NETAPP_IP_RELEASED_EVENT:
            CLR_STATUS_BIT(pNetCtx->ulStatus, STATUS_BIT_IP_LEASED);
            UART_PRINT("[NETAPP EVENT] IP Released for Client: IP=%d.%d.%d.%d , ",
                        SL_IPV4_BYTE(pNetCtx->ulStaIp,3), SL_IPV4_BYTE(pNetCtx->ulStaIp,2),
                        SL_IPV4_BYTE(pNetCtx->ulStaIp,1), SL_IPV4_BYTE(pNetCtx->ulStaIp,0));
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


//*****************************************************************************
//                            MAIN FUNCTION
//*****************************************************************************
void main() {

    long lRetVal = -1;
    OsiTaskHandle pServerHandle, pCommandTaskHandle;

    BoardInit();
    PinMuxConfig();

#ifndef NOTERM
    InitTerm();
#endif

    ClearTerm();
    DisplayBanner();

    /*
     * UART configuration for Matek F411
     */
    MAP_UARTConfigSetExpClk(UARTA1_BASE, MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                          UART_BAUD_RATE_FCU, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_TWO | // UART_CONFIG_STOP_TWO
                          UART_CONFIG_PAR_EVEN)); //UART_BAUD_RATE_FCU // UART_CONFIG_PAR_EVEN

    pNetCtx = (PNETWORK_CONTEXT) mem_Malloc(sizeof(NETWORK_CONTEXT));

    if (pNetCtx == NULL) {
        Report("Error on network network context creation\r\n");
        LOOP_FOREVER();
    }
    //
    // Start the SimpleLink Host
    //

    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the WlanAPMode task
    //

    lRetVal = osi_TaskCreate(TCPServerTask, \
                      (const signed char*)"Wireless LAN in AP mode", \
                      OSI_STACK_SIZE, pNetCtx, tskIDLE_PRIORITY+1, &pServerHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    lRetVal = osi_TaskCreate(CommandManagerTask, \
                      (const signed char*)"Process command from controller", \
                      OSI_STACK_SIZE, pNetCtx, tskIDLE_PRIORITY+2, &pCommandTaskHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the task scheduler
    //

    osi_start();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
