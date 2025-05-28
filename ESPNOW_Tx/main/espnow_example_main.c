#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "esp_mac.h" // For ESP_ETH_MAC_LEN
#include "./keypad.h"


static const char *TAG = "ESP_NOW_TX";

#define ESP_ETH_MAC_LEN         6   
#define STATUS_LED GPIO_NUM_22
#define LED_DELAY 100
#define KEYPAD_DELAY 80


// REPLACE WITH THE MAC ADDRESS OF THE RECEIVER BOARD
static uint8_t receiver_mac[] = {0x08, 0xd1, 0xf9, 0x3d, 0x78, 0xb8};

// Define a structure to send data
typedef struct {
    char text[2];
} struct_message_t;

// Global message objects
struct_message_t my_data;
struct_message_t incoming_data;

// Flag to indicate if a reply has been received
static volatile bool reply_received = false;

extern uint8_t repeat;

bool keyPressed = false;
uint8_t receiveKey = 0;
QueueHandle_t key_event_queue;


void keypad_Task(void *pvParameters)
{
    while(1)
    {

        if(uxQueueMessagesWaiting(key_event_queue) > 0)
        {
            keyPressed = true;
            xQueuePeek(key_event_queue, &receiveKey, 0);
            if(!repeat)
            {
                ESP_LOGI(TAG, "GPIO PIN was pressed: %c", receiveKey); 
                repeat = 0x01;
            }
        }

        switch(row_state)
        {
            case ROW1:
            // ESP_LOGI(TAG, "Handling Row1");
                gpio_set_level(rows[ROW1], LOW);
                vTaskDelay(KEYPAD_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(rows[ROW1], HIGH);
                row_state = ROW2;
            break;

            case ROW2:
            // ESP_LOGI(TAG, "Handling Row2");

                gpio_set_level(rows[ROW2], LOW);
                vTaskDelay(KEYPAD_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(rows[ROW2], HIGH);
                row_state = ROW3;
            break;

            case ROW3:
            // ESP_LOGI(TAG, "Handling Row3");

                gpio_set_level(rows[ROW3], LOW);
                vTaskDelay(KEYPAD_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(rows[ROW3], HIGH);
                row_state = ROW4;
            break;

            case ROW4:
            // ESP_LOGI(TAG, "Handling Row4");

                gpio_set_level(rows[ROW4], LOW);
                vTaskDelay(KEYPAD_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(rows[ROW4], HIGH);
                row_state = ROW1;
            break;
        default:
        break;
        }

        taskYIELD();
    }
}

// ESP-NOW send callback
static void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send CB MAC_ADDR NULL");
        return;
    }
    ESP_LOGI(TAG, "Last Packet Send Status to " MACSTR ": %s",
             MAC2STR(mac_addr),
             (status == ESP_NOW_SEND_SUCCESS) ? "Delivery Success" : "Delivery Fail");
}

// ESP-NOW receive callback
static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (recv_info == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive CB error");
        return;
    }
    const uint8_t *mac_addr = recv_info->src_addr;
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Receive CB MAC_ADDR NULL");
        return;
    }

    ESP_LOGI(TAG, "Received %d bytes from " MACSTR, len, MAC2STR(mac_addr));
    if (len == sizeof(struct_message_t)) {
        memcpy(&incoming_data, data, sizeof(struct_message_t));
        ESP_LOGI(TAG, "Received message: %s", incoming_data.text);
    } else {
        ESP_LOGW(TAG, "Received unexpected data length: %d", len);
    }
}

// Wi-Fi and ESP-NOW initialization
static void wifi_espnow_init(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Optional: Set a specific Wi-Fi channel (e.g., channel 1)
    // ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));


    // Get and log MAC address
    uint8_t mac[ESP_ETH_MAC_LEN];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "Transmitter MAC Address: " MACSTR, MAC2STR(mac));

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb));

    // Add peer
    esp_now_peer_info_t peer_info = {0}; // Initialize to zeros
    memcpy(peer_info.peer_addr, receiver_mac, ESP_ETH_MAC_LEN);
    peer_info.channel = 0; // 0 means current Wi-Fi channel. If not connected, it's channel 1.
                           // If you set a channel above with esp_wifi_set_channel, use that channel.
    peer_info.ifidx = ESP_IF_WIFI_STA; // Interface to use for sending
    peer_info.encrypt = false;      // No encryption

    if (esp_now_add_peer(&peer_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer");
        return;
    }
    ESP_LOGI(TAG, "Receiver Peer Added: " MACSTR, MAC2STR(receiver_mac));
    reply_received = true; // Allow the first message to be sent
}

// Task to send messages
void esp_now_send_task(void *pvParameter) {
    ESP_LOGI(TAG, "Send task started.");
    while (1) {
        // if (reply_received && keyPressed) {
        if(keyPressed)
        {
            reply_received = false; // Reset flag
            keyPressed = false;
            xQueueReceive(key_event_queue, &receiveKey, 0);
            // Prepare message
            // strcpy(my_data.text, &receiveKey);
            my_data.text[0] = receiveKey;
            ESP_LOGI(TAG, "Sending %c", receiveKey);

            // Send message via ESP-NOW
            esp_err_t result = esp_now_send(receiver_mac, (uint8_t *)&my_data, sizeof(my_data));

            if (result == ESP_OK) {
                ESP_LOGI(TAG, "Sent %c successfully.", receiveKey);
            } else {
                ESP_LOGE(TAG, "Error sending %c: %s", receiveKey, esp_err_to_name(result));
                reply_received = true; // Allow retry if send failed immediately
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before next potential send
    }
}


void statusLED_Task(void *pvParameters)
{
    uint8_t toggle = 0;
    gpio_reset_pin(STATUS_LED);
    gpio_set_direction(STATUS_LED, GPIO_MODE_OUTPUT);

    while(1)
    {
        gpio_set_level(STATUS_LED, toggle);
        toggle ^= 1;
        vTaskDelay(LED_DELAY / portTICK_PERIOD_MS);
        taskYIELD();
    }

}




void app_main(void) {
    init_keypad();
    key_event_queue = xQueueCreate(2, sizeof(uint8_t));
    // if(xTaskCreate(statusLED_Task, "Status LED Task", 2048, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
    // {
    //     ESP_LOGW(TAG, "Warning: Status led task was not created. Not status led blinking.");
    // }

    if(xTaskCreate(keypad_Task, "Keypad Task", 2048, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Keypad task was not created. Waiting within while(1)");
        while(1);
    }
    wifi_espnow_init();
    xTaskCreate(esp_now_send_task, "esp_now_send_task", 2048, NULL, 5, NULL);
}
