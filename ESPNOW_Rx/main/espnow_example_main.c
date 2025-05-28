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
#include "./lcd.h"


static const char *TAG = "ESP_NOW_RX";

#define ESP_ETH_MAC_LEN         6   

// Define a structure to receive data
typedef struct {
    char text[2];
} struct_message_t;

// Global message objects
struct_message_t received_data;
struct_message_t reply_data;

uint8_t charsPrinted = 0;
QueueHandle_t key_event_queue;


void lcd_Task(void *pvParameters)
{
    uint8_t button_pressed;

    while(1)
    {

        if(uxQueueMessagesWaiting(key_event_queue) > 0)
        {

            xQueueReceive(key_event_queue, &button_pressed, 0);
            ESP_LOGI(TAG, "Sending button pressed: %c, to lcd", button_pressed);            
            if(charsPrinted == 16)
            {
                send_command(newline); //this is a new line command
            }
            else if(charsPrinted == 32)
            {
                send_command(clearLCD);
                send_command(returnHome);
                charsPrinted = 0;
            }

            send_character(button_pressed);
            charsPrinted++;
        
        }

        vTaskDelay(pdMS_TO_TICKS(10));
        taskYIELD();
    }
}

// ESP-NOW send callback (for the reply)
static void esp_now_send_cb_reply(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Reply Send CB MAC_ADDR NULL");
        return;
    }
    ESP_LOGI(TAG, "Reply Send Status to " MACSTR ": %s",
             MAC2STR(mac_addr),
             (status == ESP_NOW_SEND_SUCCESS) ? "Delivery Success" : "Delivery Fail");
}

// ESP-NOW receive callback
static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (recv_info == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive CB error");
        return;
    }
    const uint8_t *sender_mac = recv_info->src_addr;
    if (sender_mac == NULL) {
        ESP_LOGE(TAG, "Receive CB MAC_ADDR NULL");
        return;
    }

    ESP_LOGI(TAG, "Received %d bytes from " MACSTR, len, MAC2STR(sender_mac));

    if (len == sizeof(struct_message_t)) {
        memcpy(&received_data, data, sizeof(struct_message_t));
        ESP_LOGI(TAG, "Received message: %s", received_data.text);

        xQueueSend(key_event_queue, &received_data, 0);
        
        
        


        // Add sender as a peer if not already added (for sending reply)
        // This is important if the sender's MAC was not known beforehand.
        if (!esp_now_is_peer_exist(sender_mac)) {
            esp_now_peer_info_t peer_info = {0}; // Initialize to zeros
            memcpy(peer_info.peer_addr, sender_mac, ESP_ETH_MAC_LEN);
            peer_info.channel = 0; // Use the current Wi-Fi channel
                                    // If you set a channel with esp_wifi_set_channel, use that.
            peer_info.ifidx = ESP_IF_WIFI_STA;
            peer_info.encrypt = false;

            esp_err_t add_peer_ret = esp_now_add_peer(&peer_info);
            if (add_peer_ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to add sender as peer for reply: %s", esp_err_to_name(add_peer_ret));
                return;
            }
            ESP_LOGI(TAG, "Sender Peer Added for reply: " MACSTR, MAC2STR(sender_mac));
        } else {
            ESP_LOGI(TAG, "Sender peer " MACSTR " already exists.", MAC2STR(sender_mac));
        }

        // Prepare reply message
        strcpy(reply_data.text, "");

        // Send reply message
        esp_err_t result = esp_now_send(sender_mac, (uint8_t *)&reply_data, sizeof(reply_data));
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Sent msg recv confirmation successfully.");
        } else {
                ESP_LOGE(TAG, "Error sending msg recv confirmation: %s", esp_err_to_name(result));
            }
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

    // Get and log MAC address
    uint8_t mac[ESP_ETH_MAC_LEN];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "Receiver MAC Address: " MACSTR, MAC2STR(mac));

    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb_reply)); // For replies
}

void setup(void)
{
    #ifndef DEBUG
    wifi_espnow_init();
    #endif
    init_lcd();
    key_event_queue = xQueueCreate(2, sizeof(uint8_t));
    if(xTaskCreate(lcd_Task, "LCD Task", 2048, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "LCD task was not created. Waiting within while(1)");
        while(1);
    }



}

void app_main(void) {
    setup();
    ESP_LOGI(TAG, "ESP-NOW Receiver initialized and waiting for messages.");
    // All logic is handled by callbacks, main task can sleep or do other things.
    // For this example, we'll let it exit as FreeRTOS will keep tasks running.
    // Or, you can have a loop here:
    // while(1) {
    //    vTaskDelay(pdMS_TO_TICKS(1000));
    // }



    #ifdef DEBUG

    send_character(0x48);
    send_character(0x65);
    send_character(0x6C);
    send_character(0x6C);
    send_character(0x6F);

    send_command(newline);

    send_character(0x57);
    send_character(0x6F);
    send_character(0x72);
    send_character(0x6C);
    send_character(0x64);


    while(1)
    {
        ;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    #endif
}
