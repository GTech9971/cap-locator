/** INCLUDES *******************************************************/
#include "usb.h"
#include "usb_device_hid.h"

#include <stdint.h>
#include <string.h>

#include "system.h"



/** VARIABLES ******************************************************/
/* Some processors have a limited range of RAM addresses where the USB module
 * is able to access.  The following section is for those devices.  This section
 * assigns the buffers that need to be used by the USB module into those
 * specific areas.
 */
#if defined(FIXED_ADDRESS_MEMORY)
    #if defined(COMPILER_MPLAB_C18)
        #pragma udata HID_CUSTOM_OUT_DATA_BUFFER = HID_CUSTOM_OUT_DATA_BUFFER_ADDRESS
        unsigned char ReceivedDataBuffer[64];
        #pragma udata HID_CUSTOM_IN_DATA_BUFFER = HID_CUSTOM_IN_DATA_BUFFER_ADDRESS
        unsigned char ToSendDataBuffer[64];
        #pragma udata

    #elif defined(__XC8)
        unsigned char ReceivedDataBuffer[64] HID_CUSTOM_OUT_DATA_BUFFER_ADDRESS;
        unsigned char ToSendDataBuffer[64] HID_CUSTOM_IN_DATA_BUFFER_ADDRESS;
    #endif
#else
    unsigned char ReceivedDataBuffer[64];
    unsigned char ToSendDataBuffer[64];
#endif

volatile USB_HANDLE USBOutHandle;    
volatile USB_HANDLE USBInHandle;

#define FEATURE_REPORT_SIZE HID_INT_IN_EP_SIZE
#define REPORT_TYPE_FEATURE 0x03
#define CMD_STATUS 0x01   // ステータス問い合わせ (LED状態を読む)
#define CMD_SET    0x02   // LED状態を書き換える

static uint8_t FeatureRxBuffer[FEATURE_REPORT_SIZE];
static uint8_t FeatureTxBuffer[FEATURE_REPORT_SIZE];
static uint8_t FeatureRxLen = 0;
static uint8_t LedState = 0;

static void APP_BuildFeatureResponse(uint8_t reportId, uint8_t command);
static void APP_FeatureReportReceived(void);


void APP_DeviceCustomHIDInitialize()
{
    //initialize the variable holding the handle for the last
    // transmission
    USBInHandle = 0;

    //enable the HID endpoint
    USBEnableEndpoint(CUSTOM_DEVICE_HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //Re-arm the OUT endpoint for the next packet
    USBOutHandle = (volatile USB_HANDLE)HIDRxPacket(CUSTOM_DEVICE_HID_EP,(uint8_t*)&ReceivedDataBuffer[0],HID_INT_OUT_EP_SIZE);

    APP_BuildFeatureResponse(0x00, 0x00);
}


void APP_DeviceCustomHIDTasks()
{   
    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if( USBGetDeviceState() < CONFIGURED_STATE )
    {
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if( USBIsDeviceSuspended()== true )
    {
        return;
    }
    
    //Check if we have received an OUT data packet from the host
    if(HIDRxHandleBusy(USBOutHandle) == false)
    {   
        uint8_t rxLength = USBHandleGetLength(USBOutHandle);
        if(rxLength > HID_INT_OUT_EP_SIZE)
        {
            rxLength = HID_INT_OUT_EP_SIZE;
        }

        if(HIDTxHandleBusy(USBInHandle) == false)
        {
            // 割り込み転送: [0]=0xAC, [1]=受信長, [2..]=受信データをそのまま返す簡易エコー
            memset(ToSendDataBuffer, 0, HID_INT_IN_EP_SIZE);
            ToSendDataBuffer[0] = 0xAC;
            ToSendDataBuffer[1] = rxLength;

            uint8_t payloadLength = rxLength;
            if(payloadLength > (HID_INT_IN_EP_SIZE - 2))
            {
                payloadLength = (HID_INT_IN_EP_SIZE - 2);
            }

            memcpy(&ToSendDataBuffer[2], ReceivedDataBuffer, payloadLength);
            USBInHandle = HIDTxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*)&ToSendDataBuffer[0], HID_INT_IN_EP_SIZE);
        }
        
        //Re-arm the OUT endpoint, so we can receive the next OUT data packet 
        //that the host may try to send us.
        USBOutHandle = HIDRxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*)&ReceivedDataBuffer[0], HID_INT_OUT_EP_SIZE);
    }
}

static void APP_BuildFeatureResponse(uint8_t reportId, uint8_t command)
{
    // Feature Report応答バッファを組み立てる (LED状態はbyte2)
    memset(FeatureTxBuffer, 0x00, FEATURE_REPORT_SIZE);
    FeatureTxBuffer[0] = reportId;
    FeatureTxBuffer[1] = command;
    FeatureTxBuffer[2] = (LedState ? 1u : 0u);
}

static void APP_FeatureReportReceived(void)
{
    // SET_REPORTで受信した内容を反映し、次のGET_REPORT応答を準備
    uint8_t len = FeatureRxLen;
    if(len > FEATURE_REPORT_SIZE)
    {
        len = FEATURE_REPORT_SIZE;
    }

    uint8_t reportId = 0x00;
    uint8_t command = 0x00;
    if(len > 0)
    {
        reportId = FeatureRxBuffer[0];
    }
    if(len > 1)
    {
        command = FeatureRxBuffer[1];
    }

    switch(command)
    {
        case CMD_SET:
            if(len > 2)
            {
                LedState = (FeatureRxBuffer[2] != 0u);
            }
            break;
        case CMD_STATUS:
        default:
            // No state change; just respond with current status
            break;
    }

    APP_BuildFeatureResponse(reportId, command);
}

void APP_UserSetReportHandler(void)
{
    // Host -> Device (SET_REPORT, Feature)
    if(SetupPkt.W_Value.byte.HB != REPORT_TYPE_FEATURE)
    {
        return;
    }

    uint16_t expected = SetupPkt.wLength;
    if(expected > FEATURE_REPORT_SIZE)
    {
        expected = FEATURE_REPORT_SIZE;
    }
    FeatureRxLen = (uint8_t)expected;

    USBEP0Receive((uint8_t*)&FeatureRxBuffer[0], expected, APP_FeatureReportReceived);
    USBCtrlEPAllowDataStage();
}

void APP_UserGetReportHandler(void)
{
    // Device -> Host (GET_REPORT, Feature)
    if(SetupPkt.W_Value.byte.HB != REPORT_TYPE_FEATURE)
    {
        return;
    }

    uint16_t requested = SetupPkt.wLength;
    if(requested > FEATURE_REPORT_SIZE)
    {
        requested = FEATURE_REPORT_SIZE;
    }

    uint8_t sendLen = (uint8_t)requested;
    if(sendLen < 3u)
    {
        sendLen = 3u;
    }

    USBEP0SendRAMPtr((uint8_t*)&FeatureTxBuffer[0], sendLen, USB_EP0_INCLUDE_ZERO);
}
