//****************************************************************************
//
//! \addtogroup getting_started_ap
//! @{
//
//****************************************************************************
#include "cc3200_sdk.h"


#define APP_NAME                "WLAN AP"
#define APPLICATION_VERSION     "1.1.1"
#define OSI_STACK_SIZE          2048
#define UART_BAUD_RATE_FCU      100000


//
// Values for below macros shall be modified for setting the 'Ping' properties
//


#define PORT_NUM            5001
#define BUF_SIZE            960



typedef struct {
    OsiSyncObj_t *pSyncObjStart;
    unsigned short usPort;
}taskParam;


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************


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



/*void DroneStatusFeedBack( void *pvParameters ) {

}*/

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

    MAP_UARTConfigSetExpClk(UARTA1_BASE, MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                          UART_BAUD_RATE_FCU, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_TWO | // UART_CONFIG_STOP_TWO
                          UART_CONFIG_PAR_EVEN)); //UART_BAUD_RATE_FCU // UART_CONFIG_PAR_EVEN

    ClearTerm();
    DisplayBanner();

    //
    // Start the SimpleLink Host
    //
    OsiSyncObj_t pSyncObjStart;
    taskParam *pvParameters;
    OsiTaskHandle pServerHandle, pCommandTaskHandle;

    PNETWORK_CONTEXT pNetCtx = (PNETWORK_CONTEXT) mem_Malloc(sizeof(NETWORK_CONTEXT));

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
                      (const signed char*)"Wireless LAN in AP mode", \
                      OSI_STACK_SIZE, pNetCtx, tskIDLE_PRIORITY+1, &pServerHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    lRetVal = osi_TaskCreate(CommandManagerTask, \
                      (const signed char*)"Process command from controller", \
                      OSI_STACK_SIZE, pNetCtx->acBsdRecvBuf, tskIDLE_PRIORITY+2, &pCommandTaskHandle );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    /*lRetVal = osi_TaskCreate(DroneStatusFeedBack, \
                          (const signed char*)"Send drone state summary", \
                          OSI_STACK_SIZE, (void*) pvParameters, tskIDLE_PRIORITY+2, &pDroneStatusFB );
    if(lRetVal < 0){
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }*/

    //
    // Start the task scheduler
    //

    osi_start();
    //LOOP_FOREVER();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
