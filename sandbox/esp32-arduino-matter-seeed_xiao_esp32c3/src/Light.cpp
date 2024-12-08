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
    pinMode(LED_PIN, OUTPUT);
    pinMode(TOGGLE_BUTTON_PIN, INPUT);

    // デバッグログを有効にする
    esp_log_level_set("*", ESP_LOG_DEBUG);

    // Matterノードをセットアップする
    node::config_t node_config;
    node_t *node = node::create(&node_config, on_attribute_update, on_identification);

    // デフォルト値でライトエンドポイント/クラスター/属性をセットアップする
    on_off_light::config_t light_config;
    light_config.on_off.on_off = false;
    light_config.on_off.lighting.start_up_on_off = false;
    endpoint_t *endpoint = on_off_light::create(node, &light_config, ENDPOINT_FLAG_NONE, NULL);

    // オン/オフ属性の参照を保存します。後で属性値を読み取るために使用されます。
    attribute_ref = attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

    // 生成されたエンドポイントIDを保存する
    light_endpoint_id = endpoint::get_id(endpoint);
    
    // DACをセットアップする（ここはカスタムのコミッションデータ、パスコードなどを設定するのに適しています）
    esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

    // Matterデバイスを起動する
    esp_matter::start(on_device_event);

    // Matterデバイスをセットアップするために必要なコードを印刷する
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
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
