# Cap Locator USB HID ファームウェア

Cap Locator で使用する PIC16F1455 向けのカスタム HID ファームウェアです。Microchip MLA の HID スタックを用い、エンドポイント1を IN/OUT（64 バイト）で動作させています。

## プロジェクト構成
- MPLAB X プロジェクト: `firmware/CapLocator-HID.X`
- 対象デバイス: `PIC16F1455`（HFINTOSC + アクティブクロックチューニング）
- USB スタック: Microchip MLA カスタム HID（フルスピード、エンドポイント 1 IN/OUT、64 バイトパケット）

## HID インターフェース
- レポートディスクリプタはベンダ定義。インターフェース 1 つ、64 バイトの IN/OUT 割り込みエンドポイントを使用します。
- 割り込み転送（OUT → IN エコー）
  - ホストが OUT で最大 64 バイト送信すると、デバイスは IN で 64 バイトを返します。
  - 返信フォーマット: `byte0=0xAC`, `byte1=受信長`, `byte2..=受信データのエコー`。
- Feature Report（Report ID 0, 64 バイト, コントロール転送）
  - ステータス取得: SET_REPORT で `[0x00, 0x01, 0x00...]` を送信し、その後 GET_REPORT で `[0x00, 0x01, LEDマスク, ...]` を受信します。
  - LED 制御: SET_REPORT で `[0x00, 0x02, LEDマスク, ...]` を送信します。各ビットが対応する LED の ON/OFF（1 で点灯）を示し、GET_REPORT の応答にも同じマスクが返ります。
  - CLI デフォルト想定: `report_id=0`, `command_status=0x01`, `command_set=0x02`, `status_index=2`, `report_len=64`。

### LED ピン割り当て
- 最大 5 個の LED を制御。`LEDマスク` の bit0〜bit4 を以下に対応させています（アクティブ High）。
  - bit0: RC2
  - bit1: RC3
  - bit2: RC4
  - bit3: RC5
  - bit4: RA4
- 電源投入時はすべて消灯（LEDマスク=0x00）。

## ビルドと書き込み
1. MPLAB X と XC8（プロジェクト設定は 2.36 で確認）をインストールします。
2. `firmware/CapLocator-HID.X` を MPLAB X で開き、デバイスが PIC16F1455 に設定されていることを確認します。
3. `default` 構成をビルドすると、生成された HEX は `firmware/CapLocator-HID.X/dist/default/production/CapLocator-HID.X.production.hex` に出力されます。
4. PICkit や IPE などで PIC16F1455 に書き込みます。

## 備考
- USB VID/PID はプレースホルダです（`VID 0x04D8`, `PID 0x1455`）。量産前に割り当てられた値に置き換えてください。
- 構成ディスクリプタはセルフパワーを示す設定になっています。バスパワーのみの場合は `usb_descriptors.c` の `_DEFAULT | _SELF` を `_DEFAULT` に変更してください。
- `.gitignore` によりビルド成果物はリポジトリに含めません。
