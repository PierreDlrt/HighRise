/*
 * cc3200_sdk.h
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#ifndef CC3200_SDK_H_
#define CC3200_SDK_H_


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

// pinmux include
#include "hw_gpio.h"
#include "pin.h"
#include "gpio.h"

#include "network_utils.h"
#include "tcp_server.h"
#include "matekF411_drv.h"


#endif /* CC3200_SDK_H_ */
