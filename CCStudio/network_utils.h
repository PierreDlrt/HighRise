/*
 * network_utils.h
 *
 *  Created on: 2 oct. 2021
 *      Author: pierr
 */

#ifndef NETWORK_UTILS_H_
#define NETWORK_UTILS_H_

#define SSID_NAME       "High-Rise";
#define PASSWORD        "pierre12";


static long ConfigureSimpleLinkToDefaultState(PNETWORK_CONTEXT pNetCtx);
static void InitializeAppVariables(PNETWORK_CONTEXT pNetCtx);
static int ConfigureMode(int iMode, PNETWORK_CONTEXT pNetCtx);


#endif /* NETWORK_UTILS_H_ */
