/*
 * network_utils.c
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#include "cc3200_sdk.h"




static void InitializeAppVariables(PNETWORK_CONTEXT pNetCtx){

    pNetCtx->ulStatus = 0;
    pNetCtx->ulStaIp = 0;
    pNetCtx->ulPingPacketsRecv = 0;
    pNetCtx->uiGatewayIP = 0;
}

/*void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent){

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
*/

static long ConfigureSimpleLinkToDefaultState(PNETWORK_CONTEXT pNetCtx){

    //UART_PRINT("Enter configslDefState\n\r");
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    UART_PRINT("sl_start\n\r");
    lMode = sl_Start(0, 0, 0);
    UART_PRINT("end sl_start\n\r");
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode
    if (ROLE_STA != lMode){
        if (ROLE_AP == lMode){
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(pNetCtx->ulStatus)){
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
        while(IS_CONNECTED(pNetCtx->ulStatus)){
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

    InitializeAppVariables(pNetCtx);

    return lRetVal; // Success
}


static int ConfigureMode(int iMode, PNETWORK_CONTEXT pNetCtx){

    long    lRetVal = -1;
    _u8  val = SL_SEC_TYPE_WPA_WPA2;

    //UART_PRINT("Enter the AP SSID name: ");
    //GetSsidName(pcSsidName,33);
    UART_PRINT("AP SSID name: %s\n\r", SSID_NAME);

    lRetVal = sl_WlanSetMode(ROLE_AP);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(SSID_NAME),
                         SSID_NAME);
    ASSERT_ON_ERROR(lRetVal);

    //Set no password
    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1,
                         (_u8*)&val);

    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD, strlen(PASSWORD),
                         PASSWORD);
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT("Device is configured in AP mode\n\r");

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(pNetCtx->ulStatus);

    return sl_Start(NULL,NULL,NULL);
}
