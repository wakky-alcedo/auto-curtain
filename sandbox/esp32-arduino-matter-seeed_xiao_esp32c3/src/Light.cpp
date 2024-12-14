/**
 * @file Light.cpp
 * @brief Matterとトグルボタンで制御するLEDライトデバイスの実装例
 * 
 * このプログラムは、LEDをMatterとトグルボタンで制御することにより、OnOffクラスターを持つMatterライトデバイスの例を示します。
 * 
 * @details
 * - ESPにビルドインLEDがない場合は、LED_PINに接続してください。
 * - ライトをトグルする方法は以下の通りです：
 *   - Matter（CHIPToolや他のMatterコントローラーを介して）
 *   - トグルボタン（デフォルトではGPIO0 - リセットボタンに接
 */

// 作ったやつ
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20241214
#define CHIP_DEVICE_CONFIG_USE_TEST_SPAKE2P_ITERATION_COUNT 1000
#define CHIP_DEVICE_CONFIG_USE_TEST_SPAKE2P_SALT "U1BBS0UyUCBLZXkgU2FsdA==" 
#define CHIP_DEVICE_CONFIG_USE_TEST_SPAKE2P_VERIFIER \
  "vX+/5yAFHWg2KCbFIp+If91O/rTn43pQeaPcLiByviwEEbs0ZPsKtHn5ebBl/ifC+ixMOmVXcOMH8HaJgLPGRUqNbdvdZHWSEcfNBzkXzSx0fGK/uyd8IO6HxTAZSNiuTQ=="
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF01


// Default
// #define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
// #define CHIP_DEVICE_CONFIG_USE_TEST_SPAKE2P_ITERATION_COUNT 1000
// #define CHIP_DEVICE_CONFIG_USE_TEST_SPAKE2P_SALT "U1BBS0UyUCBLZXkgU2FsdA==" 
// #define CHIP_DEVICE_CONFIG_USE_TEST_SPAKE2P_VERIFIER \
//   "uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ65CJzbeUB49s31EH+NEkg0JVI5MGCQGMMT/SRPFNRODm3wH/MBiehuFc6FJ/NH6Rmzw=="
// #define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00

#include <Arduino.h>
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

// PINを設定してください
const int LED_PIN = D0;
const int TOGGLE_BUTTON_PIN = D9;

// トグルボタンのデバウンス
const int DEBOUNCE_DELAY = 500;
int last_toggle;

// Matterライトデバイスで使用されるクラスターと属性ID
const uint32_t CLUSTER_ID = OnOff::Id;
const uint32_t ATTRIBUTE_ID = OnOff::Attributes::OnOff::Id;

// Matterデバイスに割り当てられるエンドポイントと属性参照
uint16_t light_endpoint_id = 0;
attribute_t *attribute_ref;


// セットアッププロセスに関連するさまざまなデバイスイベントをリッスンする可能性があります
// 簡潔にするために空のままにしています
/**
  * @brief デバイスイベントのリスナー。
  * この例では、デバイスイベントをリッスンしていません。
  * @param event デバイスイベント
  * @param arg ユーザー定義の引数
  */
static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}

/**
  * @brief Identificationコールバック関数。
  * この例では、Identificationコールバックをリッスンしていません。
  * @param type Identificationコールバックのタイプ
  * @param endpoint_id エンドポイントID
  * @param effect_id エフェクトID
  * @param effect_variant エフェクトバリアント
  * @param priv_data プライベートデータ
  * @return esp_err_t 処理結果
  */
static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id,
                   uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
    return ESP_OK;
}

/**
  * @brief 属性更新リクエストのリスナー。
  * この例では、
  * ライトのオン/オフ属性の更新がリクエストされたとき
  * パス（エンドポイント、クラスター、属性）がライト属性と一致するかどうかを確認
  * 一致する場合、LEDは新しい状態に変更

  * @param type 属性更新のタイプ
  * @param endpoint_id エンドポイントID
  * @param cluster_id クラスターID
  * @param attribute_id 属性ID
  * @param val 属性値
  * @param priv_data プライベートデータ
  * @return esp_err_t 処理結果
  */
static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                   uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
    if (type == attribute::PRE_UPDATE && endpoint_id == light_endpoint_id &&
        cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
        // ライトのオン/オフ属性の更新を受け取りました！
        bool new_state = val->val.b;
        digitalWrite(LED_PIN, new_state);
    }
    return ESP_OK;
}

#include <setup_payload/SetupPayload.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
// using namespace chip;
// using namespace chip::setuppSetupPayload;

void GenerateCustomQRCode() {
    // chip::SetupPayload payload;
    chip::PayloadContents payload;

    // ベンダーIDと製品IDを設定
    payload.vendorID = 1217;  // あなたのベンダーID default: 1217
    payload.productID = 5678; // あなたの製品ID
    payload.setUpPINCode = CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE;
    payload.discriminator.SetLongValue(CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR);
    payload.rendezvousInformation.SetValue(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

    Serial.print("isValidManualCode: ");
    Serial.println(payload.isValidManualCode());
    Serial.print("isValidQRCodePayload: ");
    Serial.println(payload.isValidQRCodePayload());
    Serial.print("IsValidSetupPIN: ");
    Serial.println(SetupPayload::IsValidSetupPIN(payload.setUpPINCode));
    Serial.println(payload.IsValidSetupPIN(CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE));

    // chip::Ble::ChipBLEDeviceIdentificationInfo aaaaabb;
    // info.SetVendorId(1234);

    // QRコード文字列を生成
    // std::string qrCode;
    // chip::QRCodeSetupPayloadGenerator generator(payload);
    // if (generator.payloadBase38Representation(qrCode) == CHIP_NO_ERROR) {
    //     ChipLogProgress(DeviceLayer, "Generated QR Code: %s", qrCode.c_str());
    // } else {
    //     ChipLogError(DeviceLayer, "Failed to generate QR Code.");
    // }

    // CHIP_ERROR err = generator.payloadBase38Representation(qrCode);
    // if (err == CHIP_NO_ERROR)
    // {
    //     Serial.println("QR Code:");
    //     Serial.println(qrCode.c_str());
    // }
    // else
    // {
    //     Serial.print("Failed to generate QR Code. Error: ");
    //     Serial.println(ErrorStr(err));
    // }

    PrintOnboardingCodes(payload);
}

#include <app/server/Server.h>
void ResetProvisioning() {
    chip::Server::GetInstance().ScheduleFactoryReset();
      // esp_matter::factory_reset();
  // esp_matter::setup_providers();
  // chip::DeviceLayer::ConnectivityMgr().ClearBLE();
    // chip::DeviceLayer::ChipDeviceEvent::ServiceProvisioningChange provisioningChange;
  // chip::DeviceLayer::DeviceEventType::kServiceProvisioningChange;
}

#include <app/server/Server.h>
void ResetNodeId() {
    auto & server = chip::Server::GetInstance();
    server.GetFabricTable().DeleteAllFabrics();
    ChipLogProgress(DeviceLayer, "Node ID reset successfully.");
}

/**
 * @brief Matterノードを初期化し、ライトエンドポイントを設定するためのセットアップ関数。
 * 
 * この関数は以下のタスクを実行します：
 * - シリアル通信を115200ボーで初期化します。
 * - LEDピンを出力として、トグルボタンピンを入力として設定します。
 * - すべてのコンポーネントのデバッグログを有効にします。
 * - 指定された設定とコールバック関数を使用してMatterノードをセットアップします。
 * - オン/オフクラスターと属性のデフォルト値でライトエンドポイントを設定します。
 * - 後で使用するためにオン/オフ属性の参照を保存します。
 * - ライトエンドポイントの生成されたエンドポイントIDを保存します。
 * - カスタムコミッショニングデータを使用してDAC（デバイス認証証明書）を設定します。
 * - Matterデバイスを起動し、コミッショニングのためのオンボーディングコードを印刷します。
 */
void setup() {
    Serial.begin(115200);

    Serial.println("--- Start Settings ---");

    pinMode(LED_PIN, OUTPUT);
    pinMode(TOGGLE_BUTTON_PIN, INPUT);

    Serial.println("Start Matter Settings"); 

    // デバッグログを有効にする
    esp_log_level_set("*", ESP_LOG_DEBUG);

    // ResetProvisioning();
    // ResetNodeId();

    // ファブリックインデックスのすべてのエントリを削除
    // 危険なので，ちゃんとindexはesp_matter::start()のログから確認すること
    // Serial.println("Delete Access Control Settings");
    // chip::Access::AccessControl accessControl;
    // accessControl.DeleteAllEntriesForFabric(0x1);
    // accessControl.DeleteAllEntriesForFabric(0x2);
    // accessControl.DeleteAllEntriesForFabric(0x3);

    // Matterノードをセットアップする
    Serial.println("Setup Node");
    node::config_t node_config;
    node_t *node = node::create(&node_config, on_attribute_update, on_identification);

    // デフォルト値でライトエンドポイント/クラスター/属性をセットアップする
    Serial.println("Setup Light Endpoint");
    on_off_light::config_t light_config;
    light_config.on_off.on_off = false;
    light_config.on_off.lighting.start_up_on_off = false;
    endpoint_t *endpoint = on_off_light::create(node, &light_config, ENDPOINT_FLAG_NONE, NULL);

    // オン/オフ属性の参照を保存します。後で属性値を読み取るために使用されます。
    attribute_ref = attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

    // 生成されたエンドポイントIDを保存する
    light_endpoint_id = endpoint::get_id(endpoint);
    
    // DACをセットアップする（ここはカスタムのコミッションデータ、パスコードなどを設定するのに適しています）
    Serial.println("Setup DAC");
    esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

    // Matterデバイスを起動する
    Serial.println("Start Matter Device");
    esp_matter::start(on_device_event);

    // Matterデバイスをセットアップするために必要なコードを印刷する
    Serial.println("Print Onboarding Codes");
    // PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
    GenerateCustomQRCode();

    Serial.println("--- Settings Complete ---");
}

/**
  * @brief ライトのオン/オフ属性値を取得する。
  * 
  * @return esp_matter_attr_val_t ライトのオン/オフ属性値
  */
esp_matter_attr_val_t get_onoff_attribute_value() {
    esp_matter_attr_val_t onoff_value = esp_matter_invalid(NULL);
    attribute::get_val(attribute_ref, &onoff_value);
    return onoff_value;
}

/**
  * @brief ライトのオン/オフ属性値を設定する。
  * 
  * @param onoff_value ライトのオン/オフ属性値
  */
void set_onoff_attribute_value(esp_matter_attr_val_t* onoff_value) {
    attribute::update(light_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, onoff_value);
}

/**
  * @brief メインループ。
  * トグルライトボタンが押されたとき（デバウンス処理付き），ライトのオン/オフ属性値を変更します。
  */
void loop() {
    if ((millis() - last_toggle) > DEBOUNCE_DELAY) {
        if (!digitalRead(TOGGLE_BUTTON_PIN)) {
            last_toggle = millis();
            // 実際のオン/オフ値を読み取り、反転して設定する
            esp_matter_attr_val_t onoff_value = get_onoff_attribute_value();
            onoff_value.val.b = !onoff_value.val.b;
            set_onoff_attribute_value(&onoff_value);
        }
    }
}
