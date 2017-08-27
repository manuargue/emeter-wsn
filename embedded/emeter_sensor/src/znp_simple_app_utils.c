/**
* @file     znp_simple_app_utils.c
* @brief    Funciones utiles de ZNP, basado ejemplo del kit.
* @see      http://processors.wiki.ti.com/index.php/Tutorial_on_the_Examples y
*           http://e2e.ti.com/support/low_power_rf/default.aspx
*/

#include "../lib/HAL/hal.h"
#include "../lib/ZNP/znp_interface.h"
#include "../lib/Common/utilities.h"
#include "../lib/Common/printf.h"
#include "../lib/ZNP/af_zdo.h"              // para pollingAndFindDevice()
#include "../lib/ZNP/znp_commands.h"        // para pollingAndFindDevice()
#include "znp_simple_app_utils.h"
#include "../lib/ZNP/znp_interface_spi.h"
#include "../lib/ZNP/application_configuration.h"

extern unsigned char znpBuf[100];
extern signed int znpResult;

/** Canal de ZigBee por defecto */
#define DEFAULT_CHANNEL     11

/** Configuración de ZigBee para la aplicación */
struct applicationConfiguration zigBeeCfg = {
    .endPoint = 0xD7,
    .profileId = 0xD7D7,
    .deviceId = 1,
    .deviceVersion = 1,
    .latencyRequested = LATENCY_NORMAL,
    .numberOfBindingInputClusters = 0,
    .numberOfBindingOutputClusters = 0    
};

/** Poll the ZNP for any messages and display them to the console.
@pre SRDY went LOW indicating data is ready
*/
void pollAndDisplay()
{
    spiPoll();
    if (znpBuf[SRSP_LENGTH_FIELD] > 0)
    {
        printf("Rx: ");
        printHexBytes(znpBuf, (znpBuf[SRSP_LENGTH_FIELD] + SRSP_HEADER_SIZE));
        znpBuf[SRSP_LENGTH_FIELD] = 0;
    } 
}

#define WAIT_FOR_DEVICE_STATE_TIMEOUT 4000000l
/** Wait until a message is received. Exits if received message is a ZDO_STATE_CHANGE_IND
and the state matches what we want. Else loops. 
@param expectedState the deviceState we are expecting - DEV_ZB_COORD etc.
@return 0 if success, -1 if timeout
*/
signed int waitForDeviceState(unsigned char expectedState)
{
    printf("Esperando a la red... ");
    unsigned char state = 0xFF;
    unsigned long timeout = WAIT_FOR_DEVICE_STATE_TIMEOUT;
    while (state != expectedState)
    {
        while ((SRDY_IS_HIGH()) && (timeout > 0))
            timeout--;
        if (timeout == 0)
            return -1;  //error
        
        spiPoll();
        if (CONVERT_TO_INT(znpBuf[2], znpBuf[1]) == ZDO_STATE_CHANGE_IND)
        {
            state = znpBuf[SRSP_PAYLOAD_START];
            printf("%s ", getDeviceStateName(state));   
        }
    }
    printf("\r\n");
    return 0;
}

/** Attempts to start the ZNP.
@param the type of device we're starting, e.g. ROUTER
@post znpResult contains the ZNP library error code, if any.
@return ZNP_SUCCESS if successful, else an error code indicating where it failed.
@see Communications Examples for more information about each of these steps.
*/
signed int startZnp(unsigned char deviceType)
{
    printf("Iniciando ZNP ");
    znpInit(); 
    if (znpResult != ZNP_SUCCESS) 
        return -1; 
    
    setStartupOptions(STARTOPT_CLEAR_CONFIG + STARTOPT_CLEAR_STATE);
    if (znpResult != ZNP_SUCCESS) 
        return -2; 
    
    znpReset();
    if (znpResult != ZNP_SUCCESS) 
        return -3; 
    
    setZigbeeDeviceType(deviceType);
    if (znpResult != ZNP_SUCCESS) 
        return -4; 
    
    setChannel(DEFAULT_CHANNEL);
    if (znpResult != ZNP_SUCCESS) 
        return -5; 
    
    setCallbacks(CALLBACKS_ENABLED);
    if (znpResult != ZNP_SUCCESS) 
        return -6; 
    
    afRegisterApplication(zigBeeCfg);
    if (znpResult != ZNP_SUCCESS) 
        return -8; 
    
    zdoStartApplication();
    if (znpResult != ZNP_SUCCESS) 
        return -9; 
    
    /* Wait until this device has joined a network.
     * Device State will change to DEV_ROUTER or DEV_COORD to indicate that the
     * device has correctly joined a network. */
    unsigned char deviceState = 0;
    switch (deviceType)
    {
    case ROUTER: deviceState = DEV_ROUTER; break;
    case END_DEVICE: deviceState = DEV_END_DEVICE; break;
    case COORDINATOR: deviceState = DEV_ZB_COORD; break;
    default: printf("ERROR - UNKNOWN DEVICE TYPE\r\n"); return -10;
    }
    
    znpResult = waitForDeviceState(deviceState);
    if (znpResult != ZNP_SUCCESS) 
        return -10; 
    
    return ZNP_SUCCESS;
}
