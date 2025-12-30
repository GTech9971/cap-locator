/** INCLUDES *******************************************************/
#include "usb.h"
#include "usb_device_hid.h"

#include <stdint.h>
#include <string.h>

#include <xc.h>
#include "system.h"


#define REPORT_ID 0xFF
// LEDステータスチェック
#define CMD_STATUS 0x01
// LEDステータス変更
#define CMD_SET 0x02

// LEDの数
#define LED_COUNT 5u
// LEDステータス変更時のビットマスク
#define LED_MASK  0x1Fu

// LEDステータス管理用変数
static uint8_t LedState = 0;



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


static void APP_InitializeLeds(void);
static void APP_UpdateLedOutputs(uint8_t mask);

void APP_DeviceCustomHIDInitialize() {
    //initialize the variable holding the handle for the last
    // transmission
    USBInHandle = 0;

    //enable the HID endpoint
    USBEnableEndpoint(CUSTOM_DEVICE_HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);

    //Re-arm the OUT endpoint for the next packet
    USBOutHandle = (volatile USB_HANDLE)HIDRxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*) & ReceivedDataBuffer[0], HID_INT_OUT_EP_SIZE);

    // LED初期化
    APP_InitializeLeds();
}

void APP_DeviceCustomHIDTasks() {
    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop. */
    if (USBGetDeviceState() < CONFIGURED_STATE) {
        return;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop. */
    if (USBIsDeviceSuspended() == true) {
        return;
    }

    //Check if we have received an OUT data packet from the host
    if (HIDRxHandleBusy(USBOutHandle) == false) {
        uint8_t command = ReceivedDataBuffer[0];
        switch (command) {
                // LED Status問い合わせ
            case CMD_STATUS:
            {
                if (!HIDRxHandleBusy(USBInHandle)) {
                    memset(ToSendDataBuffer, 0, 64);
                    ToSendDataBuffer[0] = REPORT_ID;
                    ToSendDataBuffer[1] = LedState;
                    USBInHandle = HIDTxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*) & ToSendDataBuffer[0], 64);
                }
                break;
            }
            case CMD_SET:
            {
                if (!HIDRxHandleBusy(USBInHandle)) {
                    uint8_t led_data = ReceivedDataBuffer[1];
                    APP_UpdateLedOutputs(led_data);

                    memset(ToSendDataBuffer, 0, 64);
                    ToSendDataBuffer[0] = REPORT_ID;
                    ToSendDataBuffer[1] = LedState;
                    USBInHandle = HIDTxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*) & ToSendDataBuffer[0], 64);
                }
                break;
            }
        }

        //Re-arm the OUT endpoint, so we can receive the next OUT data packet
        //that the host may try to send us.
        USBOutHandle = HIDRxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*) & ReceivedDataBuffer[0], HID_INT_OUT_EP_SIZE);
    }
}

static void APP_InitializeLeds(void) {
    // RC2〜RC5とRA4をLED出力に使う。電源投入時はすべて消灯。
    ANSELC = 0x00; // RCポートをデジタルIOに設定
    ANSELA &= 0xCF; // RA4,RA5をデジタルIOに設定（bit4=0, bit5=0）
    TRISCbits.TRISC2 = 0; // LED0
    TRISCbits.TRISC3 = 0; // LED1
    TRISCbits.TRISC4 = 0; // LED2
    TRISCbits.TRISC5 = 0; // LED3
    TRISAbits.TRISA4 = 0; // LED4

    //Status LED
    TRISAbits.TRISA5 = 0; // LED5
    LATAbits.LATA5 = 1;

    APP_UpdateLedOutputs(0x00);
}

static void APP_UpdateLedOutputs(uint8_t mask) {
    LedState = (mask & LED_MASK);

    LATCbits.LATC2 = ((LedState & 0x01u) != 0u); // LED0
    LATCbits.LATC3 = ((LedState & 0x02u) != 0u); // LED1
    LATCbits.LATC4 = ((LedState & 0x04u) != 0u); // LED2
    LATCbits.LATC5 = ((LedState & 0x08u) != 0u); // LED3
    LATAbits.LATA4 = ((LedState & 0x10u) != 0u); // LED4
}