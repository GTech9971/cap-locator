# Cap Locator USB HID ファームウェア

Cap Locator で使用する PIC16F1455 向けのカスタム HID ファームウェアです。Microchip MLA の HID スタックを用い、エンドポイント1を IN/OUT（64 バイト）で動作させています。

## プロジェクト構成

- MPLAB X プロジェクト: `firmware/CapLocator-HID.X`
- 対象デバイス: `PIC16F1455`（HFINTOSC + アクティブクロックチューニング）
- USB スタック: Microchip MLA カスタム HID（フルスピード、エンドポイント 1 IN/OUT、64 バイトパケット）

## HID インターフェース


  - ステータス取得: `0x01` を送信し、その後`[0xFF, LEDマスク, ...]` を受信します。
  - LED 制御: `[0x02, LEDマスク, ...]` を送信し、`[0xFF, LEDマスク, ...]`を受信します。

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
- 構成ディスクリプタはバスパワー設定（`_DEFAULT` のみ）です。セルフパワーにする場合は `usb_descriptors.c` の属性を `_DEFAULT | _SELF` に変更してください。
- `.gitignore` によりビルド成果物はリポジトリに含めません。
