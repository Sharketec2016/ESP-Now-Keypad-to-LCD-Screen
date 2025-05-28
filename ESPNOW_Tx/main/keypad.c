#include "keypad.h"

/**
 * 
 * Following the pinout pattern from this image: https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fi.postimg.cc%2FWbx863TZ%2F4x4-pinout.png&f=1&nofb=1&ipt=acd12cb4254890f5374a9536305b6cd11217693f2ef619f95a6822fa3d1ac3be
 * 
 * R1 = GPIO13 (D13)
 * R2 = GPIO12 (D12)
 * R3 = GPIO14 (D14)
 * R4 = GPIO27 (D27)
 * 
 * C1 = GPIO26 (D26)
 * C2 = GPIO25 (D25)
 * C3 = GPIO33 (D33)
 * C4 = GPIO32 (D32)
 * 
 * 
 */

// typedef enum
// {
//     ROW1 = 0x00,
//     ROW2, 
//     ROW3,
//     ROW4
// } row_state_t;


static const char *TAG = "Keypad";

uint8_t rows[4] = {GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14, GPIO_NUM_27};
uint8_t cols[4] = {GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32};
row_state_t row_state;
uint8_t repeat;

static void IRAM_ATTR keypad_isr_handler(void *args)
{
    // uint8_t val = (uint8_t) args;
    uint8_t interCOL = (uint8_t) args;
    uint8_t val;
    switch (row_state)
    {
        case ROW1:
            if(interCOL == 0x1A) //0x1A == 26
            {
                val = 0x31;
            }else if (interCOL == 0x19) //0x19 == 25
            {
                val = 0x32;
            }else if(interCOL == 0x21) //0x21 == 33
            {
                val = 0x33;
            }
            else
            {
                val = 0x41; //this is for pin 32
            }

            break;
        case ROW2:
            if(interCOL == 0x1A) //0x1A == 26
            {
                val = 0x34;
            }else if (interCOL == 0x19) //0x19 == 25
            {
                val = 0x35;
            }else if(interCOL == 0x21) //0x21 == 33
            {
                val = 0x36;
            }
            else
            {
                val = 0x42; //this is for pin 32
            }
            break;
        case ROW3:
            if(interCOL == 0x1A) //0x1A == 26
            {
                val = 0x37;
            }else if (interCOL == 0x19) //0x19 == 25
            {
                val = 0x38;
            }else if(interCOL == 0x21) //0x21 == 33
            {
                val = 0x39;
            }
            else
            {
                val = 0x43; //this is for pin 32
            }
            break;
        case ROW4:
            if(interCOL == 0x1A) //0x1A == 26
            {
                val = 0x2A;
            }else if (interCOL == 0x19) //0x19 == 25
            {
                val = 0x30;
            }else if(interCOL == 0x21) //0x21 == 33
            {
                val = 0x23;
            }
            else
            {
                val = 0x44; //this is for pin 32
            }
            break;
        
        default:
            break;
    }
    repeat = 0x00;
    xQueueSendFromISR(key_event_queue, &val, NULL);
    

    
}


void init_keypad_struct(keypad_gpio_t *keypad)
{
    keypad->R1 = GPIO_NUM_13;
    keypad->R2 = GPIO_NUM_12;
    keypad->R3 = GPIO_NUM_14;
    keypad->R4 = GPIO_NUM_27;
    

    keypad->C1 = GPIO_NUM_26;
    keypad->C2 = GPIO_NUM_25;
    keypad->C3 = GPIO_NUM_33;
    keypad->C4 = GPIO_NUM_32;
}



void init_keypad_gpio(uint8_t gpioPin, gpio_mode_t DIR, bool isrCond)
{
    gpio_config_t io_config = {0};
    io_config.pin_bit_mask = (1ULL << gpioPin);
    io_config.mode = DIR;
    if(DIR == GPIO_MODE_OUTPUT)
    {
        io_config.pull_down_en = 0;
        io_config.pull_up_en = 1;
        io_config.intr_type = GPIO_INTR_DISABLE;
    }
    else
    {
        io_config.pull_down_en = 0;
        io_config.pull_up_en = 1;
        io_config.intr_type = GPIO_INTR_NEGEDGE;
        io_config.mode = GPIO_MODE_INPUT;
    }

    esp_err_t conf_err = gpio_config(&io_config);
    if(conf_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure GPIO %d", gpioPin);
        return;
    }

    if(isrCond)
    {
        esp_err_t keypad_isr_handle_err = gpio_isr_handler_add(gpioPin, keypad_isr_handler, (void *)gpioPin);
        if(keypad_isr_handle_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initalize the isr for gpio pin: %d", gpioPin);
        }
    }

}


void turn_on_pins(void)
{
    for(int i=0; i<4; i++)
    {
        gpio_set_level(rows[i], HIGH);
        gpio_set_level(cols[i], HIGH);
    }
}


void init_keypad(void)
{
    /**
     * All rows will be held high, and all columns will be held low. Same thing as saying all rows will output a high voltage and all cols will input a voltage.
     */

    /*Reset all of the gpio pins to a default state*/
    

    esp_err_t isr_service_err = gpio_install_isr_service(0);
    if(isr_service_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install ISR services.");
    }
    ESP_LOGI(TAG, "Starting initalization of keypad gpio pins");
    
    for(int i=0; i<4; i++)
    {
        init_keypad_gpio(rows[i], GPIO_MODE_OUTPUT, false);
        gpio_set_level(rows[i], HIGH);
    }
    

    for(int i=0; i<4; i++)
    {

        init_keypad_gpio(cols[i], GPIO_MODE_INPUT, true);
    } 

}



