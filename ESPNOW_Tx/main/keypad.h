#ifndef _KEYPAD_H
#define _KEYPAD_H

#include "stdint.h"
#include "driver/gpio.h"
#include "stdbool.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
/**
 * The keypad is broken into 4 columns and rows. Each cell in this 4x4 matrix is mapped to two output pins, meaning when 
 * a button is pressed two pins will be bridged. We will probably want to see how to handle the voltages wtih these pins
 * We need a keypad object that contains these rows and cells. 
 * 
 * They keypad works by shorting the cell connecting the row and col together. so cell 1 shorts together row 1 and col 1. 
 * If we hold one set of GPIO pins high and the others low, while having half of them as inputs, we then can monitor which 
 * pins become shorted. Or we see which voltage which was held high is not low. 
 * 
 * We will also tie the row pins high and the col pins low. Then if a short happens, the corresponding button press will
 * short the corresponding row. From there we will need to include a ISR for each GPIO row pin, that way when a button press is 
 * activated, itll populate a structure handling the current state of the gpio pin. 
 * The ISR will be called when a change in the logic level at all is detected, but only update the LCD screen if the change 
 * toggles the state of that cell LOW or shorts it. That tells us that the user pressed the button and intended on 
 * wanting to have that value show up on the screen. 
 */


extern QueueHandle_t key_event_queue;



typedef enum
{
    LOW = 0x00,
    HIGH = 0x01
}keypad_state_t;

typedef enum
{
    OUTPUT=0x00,
    INPUT=0x01
}keypad_gpio_dir_t;

typedef struct 
{
    uint8_t C1;
    uint8_t C2;
    uint8_t C3;
    uint8_t C4;

    uint8_t R1;
    uint8_t R2;
    uint8_t R3;
    uint8_t R4; 
}keypad_gpio_t;

typedef enum
{
    ROW1 = 0x00,
    ROW2, 
    ROW3,
    ROW4
} row_state_t;

extern uint8_t rows[4];
extern uint8_t cols[4];
extern row_state_t row_state;


void init_keypad_struct(keypad_gpio_t *keypad);
static void IRAM_ATTR keypad_isr_handler(void *args);
void init_keypad(void);
void init_keypad_gpio(uint8_t gpioPin, gpio_mode_t DIR, bool isrCond);
void turn_on_pins(void);

#endif