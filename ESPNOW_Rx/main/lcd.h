#ifndef _LCD_H
#define _LCD_H

#include "stdint.h"
#include "driver/gpio.h"
#include "stdbool.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h" 


#define clearLCD 0x01
#define returnHome 0x02

#define entryModeLR 0b00000110
#define entryModeRL 0b00000100

#define displayOn 0b00000100
#define displayOff 0b00000000

#define cursorOn 0b00000010
#define cursorOff 0b00000000

#define blinkOn 0b00000001
#define blinkOff 0b00000000

#define dataLength8bit 0b00010000
#define dataLength4bit 0b00000000

#define displayLinesTwo 0b00001000
#define displayLinesOne 0x00

#define font10Dots 0b00000100
#define font8Dots 0x00

#define newline 0b11000000



#define displayMode 0b00001000 | displayOn | cursorOn | blinkOn
#define functionSet 0b00100000 | dataLength8bit | displayLinesTwo | font10Dots


void send_command(uint8_t command);
void reset_lcd_pins(void);
void send_msg_over_pins(uint8_t msg);
void process_command(uint8_t command);
void send_character(uint8_t character);
void init_lcd(void);

#endif