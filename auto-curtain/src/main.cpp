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
namespace clusters = chip::app::Clusters;
namespace em = esp_matter;

// PINを設定してください
const int LED_PIN = D0;
const int TOGGLE_BUTTON_PIN = D9;

// トグルボタンのチャッタリング防止時間と状態を覚えておくための変数
const int DEBOUNCE_DELAY = 500;
int last_toggle;

// Matterライトデバイスで使用されるクラスターと属性ID
// const uint32_t CLUSTER_ID = clusters::OnOff::Id;
// const uint32_t ATTRIBUTE_ID = clusters::OnOff::Attributes::OnOff::Id;
const uint32_t CURTAIN_CLUSTER_ID = clusters::WindowCovering::Id;
const uint32_t CURTAIN_ATTRIBUTE_ID = clusters::WindowCovering::Attributes::OperationalStatus::Id;

// Matterデバイスに割り当てられるエンドポイントと属性参照
// uint16_t light_endpoint_id = 0;
uint16_t curtain_endpoint_id = 0;
em::attribute_t *attribute_ref;


// いろいろなデバイスのイベントを聴取する可能性があるけど、ここでは使わないので空欄にしておく
// セットアッププロセスに関連するさまざまなデバイスイベントをリッスンする可能性があります
// 簡潔にするために空のままにしています
/**
  * @brief デバイスイベントのリスナー。
  * この例では、デバイスイベントをリッスンしていません。
  * @param event デバイスイベント
  * @param arg ユーザー定義の引数
  */
static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(em::identification::callback_type_t type, uint16_t endpoint_id,
                   uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
    return ESP_OK;
}


/**
  * @brief 属性(attribute)が更新される時に呼び出されるリスナー関数
  * この例では、
  * ライトのon/off attributeの更新がリクエストされたとき
  * パス（エンドポイント、クラスター、属性）をチェックして，ライト属性と一致するかどうかを確認
  * 一致する場合、LEDは新しい状態に変更

  * @param type 属性更新のタイプ
  * @param endpoint_id エンドポイントID
  * @param cluster_id クラスターID
  * @param attribute_id 属性ID
  * @param val 属性値
  * @param priv_data プライベートデータ
  * @return esp_err_t 処理結果
  */
// static esp_err_t on_attribute_update(em::attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
//                    uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
//     if (type == em::attribute::PRE_UPDATE && endpoint_id == light_endpoint_id &&
//         cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
//         // ライトのon/off attributeの更新を受け取りました！
//         bool new_state = val->val.b;
//         digitalWrite(LED_PIN, new_state);
//     }
//     return ESP_OK;
// }

static esp_err_t on_attribute_update(em::attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                   uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
    if (type == em::attribute::PRE_UPDATE) {
        Serial.print("Update on endpoint: ");
        Serial.print(endpoint_id);
        Serial.print(" cluster: ");
        Serial.print(cluster_id);
        Serial.print(" attribute: ");
        Serial.println(attribute_id);

        if(endpoint_id == curtain_endpoint_id &&
        cluster_id == CURTAIN_CLUSTER_ID && attribute_id == CURTAIN_ATTRIBUTE_ID) {
            // カーテンのattributeの更新を受け取りました！
            bool new_state = val->val.b;
            Serial.print("New state: ");
            Serial.println(new_state);
            // digitalWrite(LED_PIN, new_state);
        }
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
 * - 後で使用するためにon/off attributeの参照を保存します。
 * - ライトエンドポイントの生成されたエンドポイントIDを保存します。
 * - カスタムコミッショニングデータを使用してDAC（デバイス認証証明書）を設定します。
 * - Matterデバイスを起動し、コミッショニングのためのオンボーディングコードを印刷します。
 */
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(TOGGLE_BUTTON_PIN, INPUT);

    // デバッグログを有効にする
    // ESP_LOG_ERRORなどに変更するとメッセージが減る
    // ESP_LOG_INFOもある
    esp_log_level_set("*", ESP_LOG_DEBUG);

    // Matterノードをセットアップする
    em::node::config_t node_config;
    snprintf(node_config.root_node.basic_information.node_label, sizeof(node_config.root_node.basic_information.node_label), "DIY Smart Light");
    em::node_t *node = em::node::create(&node_config, on_attribute_update, on_identification);

    // デフォルト値でライトエンドポイント/クラスター/属性をセットアップする
    // コンストラクタで初期化されているはずだけどね
    em::endpoint::window_covering_device::config_t curtain_config;
    curtain_config.window_covering.type = 0x00;
    curtain_config.window_covering.config_status = 0b000000;
    curtain_config.window_covering.operational_status = 0b000000;
    curtain_config.window_covering.mode = 0x00;
    curtain_config.window_covering.lift.number_of_actuations_lift = 0;
    // em::endpoint::on_off_light::config_t light_config;
    // light_config.on_off.on_off = false;
    // light_config.on_off.lighting.start_up_on_off = false;

    // endpointを作成
    em::endpoint_t *endpoint = em::endpoint::window_covering_device::create(node, &curtain_config, em::ENDPOINT_FLAG_NONE, NULL);
    // em::endpoint_t *endpoint = em::endpoint::on_off_light::create(node, &light_config, em::ENDPOINT_FLAG_NONE, NULL);

    // on/off attribute の参照を保存
    // 後で属性値を読み取るために使用
    attribute_ref = em::attribute::get(em::cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

    // 生成されたエンドポイントIDを保存する
    // light_endpoint_id = em::endpoint::get_id(endpoint);
    curtain_endpoint_id = em::endpoint::get_id(endpoint);
    
    // DACをセットアップする（ここはカスタムのコミッションデータ、パスコードなどを設定するのに適しています）
    em::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

    // Matterデバイスを起動する
    em::start(on_device_event);

    // Matterデバイスをセットアップするために必要なコードを表示（ペアリングコードなど）
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}

/**
  * @brief 現在のMatterデバイスの light on/off attribute 値を読み取る
  * 
  * @return esp_matter_attr_val_t ライトのon/off attribute 値
  */
// esp_matter_attr_val_t get_onoff_attribute_value() {
//     esp_matter_attr_val_t onoff_value = esp_matter_invalid(NULL);
//     em::attribute::get_val(attribute_ref, &onoff_value);
//     return onoff_value;
// }

esp_matter_attr_val_t get_curtain_attribute_value() {
    esp_matter_attr_val_t curtain_value = esp_matter_invalid(NULL);
    em::attribute::get_val(attribute_ref, &curtain_value);
    return curtain_value;
}

/**
  * @brief light on/off attribute 値を設定する
  * @param onoff_value light on/off attribute 値
  */
// void set_onoff_attribute_value(esp_matter_attr_val_t* onoff_value) {
//     em::attribute::update(light_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, onoff_value);
// }

void set_curtain_attribute_value(esp_matter_attr_val_t* curtain_value) {
    em::attribute::update(curtain_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, curtain_value);
}

/**
  * @brief メインループ。
  * トグルライトボタンが押されたとき（デバウンス処理付き），light on/off attribute 値を変更します。
  */
void loop() {
    // チャッタリング防止のために500ms毎に押しボタンスイッチ状態を調べて押されていたらLEDを反転
    if ((millis() - last_toggle) > DEBOUNCE_DELAY) {
        if (!digitalRead(TOGGLE_BUTTON_PIN)) {
            last_toggle = millis();
            // 実際のオン/オフ値を読み取り、反転して設定する
            // esp_matter_attr_val_t onoff_value = get_onoff_attribute_value();
            // onoff_value.val.b = !onoff_value.val.b;
            // set_onoff_attribute_value(&onoff_value);
            esp_matter_attr_val_t curtain_value = get_curtain_attribute_value();
            curtain_value.val.u8 = !curtain_value.val.u8;
            set_curtain_attribute_value(&curtain_value);
        }
    }
}
