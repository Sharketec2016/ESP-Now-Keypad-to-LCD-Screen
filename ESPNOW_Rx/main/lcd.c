/**
 * @author Matthew Buchkowski
 * 
 * @details File contains driver functions for handling the 16x2 lcd screen
 * 
 * PORTD, ported over from my arduino drivers, is really 8 gpio pins D0->D7. 
 * Instead I need to pick 8 gpio pins for sending data over, and then i need to set
 * the values manually. Creating a sort of mask for each of the bits. 
 * 
 * 
 * 
 * 
 * 
 */
#include "lcd.h"

#define LCD_PIN_NUM         0x08
//                              D0              D1          D2          D3          D4          D5              D6          D7
uint8_t portD[LCD_PIN_NUM] = {GPIO_NUM_15, GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33, GPIO_NUM_32};

uint8_t RS = GPIO_NUM_21; //is Register Select. This allows you to select either the screen to print to or commands to configure where the cursor is.
uint8_t EN = GPIO_NUM_23; //Enables the ability to write to the screen. You need to toggle this when you load a command into the buffer
uint8_t RW = GPIO_NUM_34; //Sets the direction of commands. 

void reset_lcd_pins(void)
{
    gpio_reset_pin(RW);
    gpio_reset_pin(EN);
    gpio_reset_pin(RS);

    for(int i=0; i<LCD_PIN_NUM; i++)
    {
        gpio_reset_pin(portD[i]);
    }

}

void lcd_gpio_init(void) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0; // No pull-ups/downs for output

    uint64_t output_pin_sel = 0;
    for(int i=0; i < LCD_PIN_NUM; i++) {
        output_pin_sel |= (1ULL << portD[i]);
    }
    output_pin_sel |= (1ULL << RS);
    output_pin_sel |= (1ULL << EN);
    // output_pin_sel |= (1ULL << RW); // If you control RW via MCU

    io_conf.pin_bit_mask = output_pin_sel;
    ESP_ERROR_CHECK(gpio_config(&io_conf)); // Use ESP_ERROR_CHECK for debugging

    // Set initial states
    // gpio_set_level(RW, 0); // LCD's RW pin to LOW for write mode
    gpio_set_level(EN, 0); // EN should be initially low
}

void send_msg_over_pins(uint8_t msg){ //this function will put the register set pin into a state where we can send command to configure the LCD screen. 

    for(int i=0; i < LCD_PIN_NUM; i++)
    {
        if( (msg >> i) & 0x01)
        {
            gpio_set_level(portD[i], 1);
        }
        else
        {
            gpio_set_level(portD[i], 0);
        }
    }
}

void pulse_en(void)
{
    ets_delay_us(1);       
    gpio_set_level(EN, 1);
    ets_delay_us(2);       
    gpio_set_level(EN, 0);
    ets_delay_us(1);       
}

void send_command(uint8_t command)
{
    gpio_set_level(RS, 0);

    send_msg_over_pins(command);
  
    pulse_en();

    if(command == clearLCD || command == returnHome)
    {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    else
    {
        ets_delay_us(40);
    }


}

void send_character(uint8_t character){

    gpio_set_level(RS, 1); // Select data register
    // gpio_set_level(RW, 0); // RW should be low
    send_msg_over_pins(character);
    pulse_en();
    ets_delay_us(40); // Data write execution time also ~37µs
    

}

void init_lcd(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_gpio_init();


    gpio_set_level(RS, 0);

    send_msg_over_pins(0x30);
    pulse_en();
    vTaskDelay(pdMS_TO_TICKS(5));


    send_msg_over_pins(0x30);
    pulse_en();
    ets_delay_us(100);

    
    send_msg_over_pins(0x30);
    pulse_en();
    ets_delay_us(40);

    send_command(functionSet);

    send_command(0x08);

    send_command(entryModeLR);
    send_command(displayMode);

    send_command(clearLCD);
    send_command(returnHome);
    send_command(functionSet); 
    send_command(displayMode);
    send_command(entryModeLR);

}