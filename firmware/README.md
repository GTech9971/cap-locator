# Cap Locator USB HID Firmware

Firmware project based on `/Users/george/Documents/GitHub/USB-HID-Template` to bring up a simple custom HID device on the PIC16F1455 used by Cap Locator.

## Project layout
- MPLAB X project: `firmware/CapLocator-HID.X`
- Target device: `PIC16F1455` (HFINTOSC with active clock tuning enabled)
- USB stack: Microchip MLA custom HID (full speed, endpoint 1 IN/OUT, 64-byte packets)

## HID interface
- Report descriptor: vendor-defined, one interface with IN/OUT interrupt endpoints (64 bytes each).
- Interrupt pipe: send up to 64 bytes on OUT endpoint 1; device replies on IN endpoint 1 with a 64-byte echo packet where `byte0=0xAC`, `byte1=payload length`, `byte2..` = echoed data.
- Feature report (for cap-locator-cli): vendor-defined report ID 0, 64 bytes. Control transfers:
  - Status: host sends `SET_REPORT` with `[0x00, 0x01, 0x00...]`, device responds to `GET_REPORT` with `[0x00, 0x01, <state>, ...]` where `<state>` is `1` when LED is ON.
  - Set LED: host sends `SET_REPORT` with `[0x00, 0x02, <value>, ...]`; `value!=0` turns LED state ON, `0` turns it OFF. `GET_REPORT` afterwards returns the state in byte 2.

## 通信仕様（日本語）
- 割り込み転送（エンドポイント1 IN/OUT, 64バイト）  
  ホストがOUTで最大64バイト送信すると、デバイスはINで64バイトを返す。フォーマットは `[0]=0xAC`, `[1]=受信長`, `[2..]=受信データをそのままエコー`。
- Feature Report（Report ID 0, 64バイト, コントロール転送）  
  - ステータス取得: SET_REPORTで `[0x00, 0x01, 0x00...]` を送信し、その後GET_REPORTで `[0x00, 0x01, LED状態(1/0), ...]` を受信。`LED状態` が1なら点灯。
  - LED制御: SET_REPORTで `[0x00, 0x02, 値, ...]` を送信。`値` が0以外ならLED ON、0ならOFF。GET_REPORTで直近の状態を確認可能。
  CLIデフォルトは `report_id=0`, `command_status=0x01`, `command_set=0x02`, `status_index=2`, `report_len=64` を想定。

## Build and flash
1) Install MPLAB X with XC8 (tested with 2.36 in the project config).
2) Open `firmware/CapLocator-HID.X` in MPLAB X and confirm the device is set to PIC16F1455.
3) Build the `default` configuration; output HEX will be under `firmware/CapLocator-HID.X/dist/default/production/CapLocator-HID.X.production.hex`.
4) Program the PIC16F1455 via your preferred tool (e.g., PICkit/IPE).

## Notes
- USB VID/PID are placeholders (`VID 0x04D8`, `PID 0x1455`); replace with your assigned IDs before production.
- The configuration descriptor currently flags the device as self powered. Switch `_DEFAULT | _SELF` to `_DEFAULT` in `usb_descriptors.c` if the board is purely bus powered.
- Build artifacts are ignored via `.gitignore`; only source is tracked in Git.
