/**
 * @file TwoEndpointsPluginUnit.cpp
 * @brief Matterプラグインユニット（スイッチ）デバイスの例を示すプログラム。
 * 
 * このプログラムは、Matterとトグルボタンを使用してLEDを制御する2つのエンドポイントを持つOnOffクラスターを備えたMatterプラグインユニット（スイッチ）デバイスの例を示します。
 * 
 * @details
 * プラグインユニットは以下の方法でトグルできます:
 *  - Matter（CHIPToolや他のMatterコントローラーを介して）
 *  - トグルボタン（デバウンス付き）
 * 
 * @note PINの設定が必要です。
 * 
 * @section pins ピン設定
 * - LED_PIN_1: D0
 * - LED_PIN_2: D1
 * - TOGGLE_BUTTON_PIN_1: D9
 * - TOGGLE_BUTTON_PIN_2: D8
 * 
 * @section debounce デバウンス設定
 * - DEBOUNCE_DELAY: 500ms
 * 
 * @section clusters クラスターと属性ID
 * - CLUSTER_ID: OnOff::Id
 * - ATTRIBUTE_ID: OnOff::Attributes::OnOff::Id
 * 
 * @section endpoints エンドポイントと属性参照
 * - plugin_unit_endpoint_id_1: エンドポイント1のID
 * - plugin_unit_endpoint_id_2: エンドポイント2のID
 * - attribute_ref_1: エンドポイント1の属性参照
 * - attribute_ref_2: エンドポイント2の属性参照
 * 
 * @section functions 関数
 * - setup(): 初期設定を行います。
 * - loop(): トグルボタンの状態を監視し、デバウンス処理を行います。
 * - on_device_event(): デバイスイベントのリスナー（空の実装）。
 * - on_identification(): デバイス識別のコールバック。
 * - on_attribute_update(): 属性更新リクエストのリスナー。
 * - get_onoff_attribute_value(): プラグインユニットのOn/Off属性値を読み取ります。
 * - set_onoff_attribute_value(): プラグインユニットのOn/Off属性値を設定します。
 * 
 * @section usage 使用方法
 * 1. 必要なPINを設定します。
 * 2. setup()関数を呼び出して初期設定を行います。
 * 3. loop()関数をメインループで呼び出し、トグルボタンの状態を監視します。
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
const int LED_PIN_1 = D0;
const int LED_PIN_2 = D1;
const int TOGGLE_BUTTON_PIN_1 = D9;
const int TOGGLE_BUTTON_PIN_2 = D8;

// トグルボタンのデバウンス
const int DEBOUNCE_DELAY = 500;
int last_toggle;

// Matterプラグインユニットデバイスで使用されるクラスターと属性ID
const uint32_t CLUSTER_ID = OnOff::Id;
const uint32_t ATTRIBUTE_ID = OnOff::Attributes::OnOff::Id;

// Matterデバイスに割り当てられるエンドポイントと属性参照
uint16_t plugin_unit_endpoint_id_1 = 0;
uint16_t plugin_unit_endpoint_id_2 = 0;
attribute_t *attribute_ref_1;
attribute_t *attribute_ref_2;

// セットアッププロセスに関連するさまざまなデバイスイベントをリッスンする可能性があります。簡単のために空のままにしてあります。
static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(identification::callback_type_t type,
                   uint16_t endpoint_id, uint8_t effect_id,
                   uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}

/**
 * @brief 属性更新リクエストのリスナー
 * この例では、更新がリクエストされたとき、パス（エンドポイント、クラスター、属性）がプラグインユニット属性と一致するかどうかを確認します。もし一致する場合、LEDは新しい状態に変更されます。
 * 
 * @param type 属性更新のタイプ
 * @param endpoint_id エンドポイントID
 * @param cluster_id クラスターID
 * @param attribute_id 属性ID
 * @param val 属性値
 * @param priv_data プライベートデータ
 * @return esp_err_t 
 */
static esp_err_t on_attribute_update(attribute::callback_type_t type,
                   uint16_t endpoint_id, uint32_t cluster_id,
                   uint32_t attribute_id,
                   esp_matter_attr_val_t *val,
                   void *priv_data) {
  if (type == attribute::PRE_UPDATE && cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
  // プラグインユニットのオン/オフ属性の更新を受け取りました！
  bool new_state = val->val.b;
  if (endpoint_id == plugin_unit_endpoint_id_1) {
    digitalWrite(LED_PIN_1, new_state);
  } else if (endpoint_id == plugin_unit_endpoint_id_2) {
    digitalWrite(LED_PIN_2, new_state);
  }
  }
  return ESP_OK;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(TOGGLE_BUTTON_PIN_1, INPUT);
  pinMode(TOGGLE_BUTTON_PIN_2, INPUT);

  // デバッグログを有効にする
  esp_log_level_set("*", ESP_LOG_DEBUG);

  // Matterノードをセットアップ
  node::config_t node_config;
  node_t *node =
  node::create(&node_config, on_attribute_update, on_identification);

  // デフォルト値でプラグインユニットエンドポイント/クラスター/属性をセットアップ
  on_off_plugin_unit::config_t plugin_unit_config;
  plugin_unit_config.on_off.on_off = false;
  plugin_unit_config.on_off.lighting.start_up_on_off = false;
  endpoint_t *endpoint_1 = on_off_plugin_unit::create(node, &plugin_unit_config,
                            ENDPOINT_FLAG_NONE, NULL);
  endpoint_t *endpoint_2 = on_off_plugin_unit::create(node, &plugin_unit_config,
                            ENDPOINT_FLAG_NONE, NULL);

  // オン/オフ属性参照を保存します。後で属性値を読み取るために使用されます。
  attribute_ref_1 =
  attribute::get(cluster::get(endpoint_1, CLUSTER_ID), ATTRIBUTE_ID);
  attribute_ref_2 =
  attribute::get(cluster::get(endpoint_2, CLUSTER_ID), ATTRIBUTE_ID);

  // 生成されたエンドポイントIDを保存
  plugin_unit_endpoint_id_1 = endpoint::get_id(endpoint_1);
  plugin_unit_endpoint_id_2 = endpoint::get_id(endpoint_2);

  // DACをセットアップ（ここでカスタム委任データ、パスコードなどを設定するのが良い場所です）
  esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

  // Matterデバイスを起動
  esp_matter::start(on_device_event);

  // Matterデバイスのセットアップに必要なコードを印刷
  PrintOnboardingCodes(
  chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}

// プラグインユニットのオン/オフ属性値を読み取ります
esp_matter_attr_val_t get_onoff_attribute_value(esp_matter::attribute_t *attribute_ref) {
  esp_matter_attr_val_t onoff_value = esp_matter_invalid(NULL);
  attribute::get_val(attribute_ref, &onoff_value);
  return onoff_value;
}

// プラグインユニットのオン/オフ属性値を設定します
void set_onoff_attribute_value(esp_matter_attr_val_t *onoff_value, uint16_t plugin_unit_endpoint_id) {
  attribute::update(plugin_unit_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, onoff_value);
}

// トグルプラグインユニットボタンが押されたとき（デバウンス付き）、プラグインユニット属性値が変更されます
void loop() {
  if ((millis() - last_toggle) > DEBOUNCE_DELAY) {
  if (!digitalRead(TOGGLE_BUTTON_PIN_1)) {
    last_toggle = millis();
    // 実際のオン/オフ値を読み取り、反転して設定
    esp_matter_attr_val_t onoff_value = get_onoff_attribute_value(attribute_ref_1);
    onoff_value.val.b = !onoff_value.val.b;
    set_onoff_attribute_value(&onoff_value, plugin_unit_endpoint_id_1);
  }

  if (!digitalRead(TOGGLE_BUTTON_PIN_2)) {
    last_toggle = millis();
    // 実際のオン/オフ値を読み取り、反転して設定
    esp_matter_attr_val_t onoff_value = get_onoff_attribute_value(attribute_ref_2);
    onoff_value.val.b = !onoff_value.val.b;
    set_onoff_attribute_value(&onoff_value, plugin_unit_endpoint_id_2);
  }
  }
}
