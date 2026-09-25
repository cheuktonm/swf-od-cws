// control.cpp, implementation of the lcd+buttons submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 260925 - implement saving config to eeprom
// 260924 - implement servo configuration
// 260922 - write the menu+configuration system
// 260921 - write the lcd controlling code

#include <Arduino.h>
#include "control.h"
#include "distance.h"
#include "motion.h"

#include <LiquidCrystal.h>

// pins defined in control.h

#define LCD_CHAR_FULL 1
#define LCD_CHAR_HALF 2

// main.ino import
int save_eeprom(int b);

// private forward decs
int configuration(bool btn_ok, bool btn_right, bool btn_left);
#define CFG_MODE 1
#define CFG_SV_EN 2 // servo enable
#define CFG_SV_LS 3 // servo left stop
#define CFG_SV_RS 4 // servo right stop
#define CFG_SV_AS 5 // servo angle step
#define CFG_SAVE 6 // write to eeprom
#define CFG_EXIT 7 // always the last option

// private vars
LiquidCrystal lcd(PIN_LCD_RS, 
                  PIN_LCD_EN, 
                  PIN_LCD_D4, 
                  PIN_LCD_D5, 
                  PIN_LCD_D6, 
                  PIN_LCD_D7);
								
int menu_state, menu_state_last;
unsigned int menu_timer;
bool enter;
unsigned int config_state;
bool config_sel;
unsigned int config_val;

bool btn_ok, btn_right, btn_left;
bool btn_ok_, btn_right_, btn_left_;
int btn_ok_cnt, btn_right_cnt, btn_left_cnt;

int control_setup() {
	lcd.begin(16, 2); // we are using 16c*2r lcd
    lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(F("SWF-OD-CWS"));
	lcd.setCursor(0,1);
	lcd.print(F("Starting..."));
	
	const char full[8] = {1,1,1,1,1,1,1,1};
	const char half[8] = {0,0,0,0,1,1,1,1};
	lcd.createChar(LCD_CHAR_FULL, full); // full cell
	lcd.createChar(LCD_CHAR_HALF, half); // half cell
	
	pinMode(PIN_BTN_OK, INPUT);
	pinMode(PIN_BTN_LEFT, INPUT);
	pinMode(PIN_BTN_RIGHT, INPUT);
	
	menu_state = 0;	
	menu_state_last = 0;
	menu_timer = 0;
	config_state = 0;
	config_sel = 0;
	config_val = 0;
	enter = true;
	
	return 0;
}

int control_config() {
	return 0;
}

void print_battery() {
	// show state of charge of the battery
	if (m_battery > 80) {
		lcd.setCursor(15,0); lcd.write(LCD_CHAR_FULL);
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_FULL);
	} else if (m_battery > 60) {
		lcd.setCursor(15,0); lcd.write(LCD_CHAR_HALF);
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_FULL);
	} else if (m_battery > 40) {
		lcd.setCursor(15,0); lcd.write(' ');
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_FULL);
	} else if (m_battery > 30) {
		lcd.setCursor(15,0); lcd.write(' ');
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_HALF);
	} else { // critical battery
		lcd.setCursor(15,0); lcd.write('x');
		lcd.setCursor(15,1); lcd.write('x');
	} return;
}

void control_loop() {
	// read button state now
	bool btn_ok = digitalRead(PIN_BTN_OK);
	bool btn_right = digitalRead(PIN_BTN_RIGHT);
	bool btn_left = digitalRead(PIN_BTN_LEFT);
	
	unsigned int now = millis();
	// print stuffs to lcd
	switch (menu_state) {
		case 0: // disabled
			if (enter) {
				lcd.clear();
				lcd.print(F("OFF"));
				enter = false;
			} 
			if (now - menu_timer < 500) break;
			else menu_timer = now; // update lcd every half second
			print_battery();
			break;
		case 1: // idle state
			if (now - menu_timer < 500) break;
			else menu_timer = now; // update lcd every half second
			lcd.clear(); lcd.print("idle");
						

			print_battery();
			break;
		
		case 2: // configuration mode
			// moved to seperate fn
			int ret = configuration(btn_ok, btn_right, btn_left); 
			if (ret == 1) {
				enter = true;
				menu_state = menu_state_last;
			} else return; // skip below if still in config
			break;
			
		case 100: // display error message
			break;
			
		default: break;
	}
	
	// read button state after printing
	// this serves as a way to debounce
	btn_ok = (digitalRead(PIN_BTN_OK) && btn_ok) ? true : false;
	btn_right = (digitalRead(PIN_BTN_RIGHT) && btn_right) ? true : false;
	btn_left = (digitalRead(PIN_BTN_LEFT) && btn_left) ? true : false;
	
	switch (menu_state) {
		case 0:
			if (btn_ok) { // disabled -> enabled
				enter = true;
				menu_state_last = 0;
				menu_state = 1;
				delay(100); // TODO: debounce
			} break;
		case 1:
			if (btn_ok) { // idle -> configuration
				enter = true;
				menu_state_last = 1;
				menu_state = 2;
				config_state = 0;
				delay(100); // TODO: debounce
			} break;
		case 100:
			if (btn_ok) { // dismiss error
				lcd.noBlink();
				menu_state = menu_state_last;
				delay(100); // TODO: debounce
			}
		default: break;
	}
	
	return;
}

void lcd_print_cfghd(bool sel, char* name) {
	lcd.clear(); lcd.setCursor(0,0);
	if (sel) {
		lcd.write(LCD_CHAR_FULL);
		lcd.write(LCD_CHAR_FULL);
		lcd.write(LCD_CHAR_FULL);
		lcd.write(':');
	} else lcd.print(F("CFG:"));
	lcd.print(name ? name : " ");
	return;
}

int configuration(bool btn_ok, bool btn_right, bool btn_left) {
	// reread buttons
	btn_ok = (digitalRead(PIN_BTN_OK) && btn_ok) ? true : false;
	btn_right = (digitalRead(PIN_BTN_RIGHT) && btn_right) ? true : false;
	btn_left = (digitalRead(PIN_BTN_LEFT) && btn_left) ? true : false;
	
	// handle button inputs
	if (btn_right && !config_sel && config_state && config_state < 99) {
		enter = true; config_state++;
		delay(100); // TODO: debounce
	} else (btn_left && !config_sel && config_state && config_state > 1) {
		enter = true; config_state--;
		delay(100); // TODO: debounce
	} else switch(config_state) { // abuse of notation
		case CFG_SAVE:
			if (btn_ok) {
				config_sel = 0; config_val = -1; config_state = 0;
				if (save_eeprom(64+3) == 0) { 
					lcd.setCursor(0,1); lcd.print("       OK!  "); 
				} delay(100); // TODO: debounce		
			} break;
		case CFG_EXIT:
			if (btn_ok) {
				config_sel = 0; config_val = -1; config_state = 0;
				delay(100); // TODO: debounce
				return 1; // leave configuration mode				
			} break;
		
		case CFG_SV_EN:
			if (btn_ok) { 
				if (config_sel) m_sv_enable = m_sv_enable ? false : true;
				config_sel = config_sel ? false : true; enter = true;
			} break;
		case CFG_SV_LS:
			if (btn_ok) {
				if (config_sel) m_sv_lefts = config_val;
				config_sel = config_sel ? false : true; enter = true;
			} else if (btn_left) {
				config_val = max(config_val - 5, 0); enter = true;
			} else if (btn_right) {
				config_val = min(config_val + 5, 180); enter = true;
			} break;
		case CFG_SV_RS:
			if (btn_ok) {
				if (config_sel) m_sv_rights = config_val;
				config_sel = config_sel ? false : true; enter = true;
			} else if (btn_left) {
				config_val = max(config_val - 5, 0); enter = true;
			} else if (btn_right) {
				config_val = min(config_val + 5, 180); enter = true;
			} break;
			
			
		default: break;
	}
	
	// print stuffs to lcd
	switch (config_state) {
		case 0: // just entered config mode
			delay(100); // enforce delay to prevent dbl btn
			config_sel = false; config_val = 0; 
			config_state = 1; // half relying on switch's behaviour
		// -- below state becomes 1st selection upon entering config
		
		case CFG_MODE: // mode selection
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, F("Mode"));
				lcd.setCursor(0,1); lcd.print(" 1");
			} break;
			
		case CFG_SV_EN: // servo on/off
			if (enter) { enter = false;
				lcd_print_cfghd(false, F("Servo"));
				lcd.setCursor(0,1); lcd.print(m_sv_enable ? " On" : " Off");
			} break;
			
		case CFG_SV_LS: // servo left stop
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, F("sv left S"));
				config_val = m_sv_lefts;
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;
		case CFG_SV_RS: // servo right stop
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, F("sv rght S"));
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;
			
		case CFG_SAVE: // write config to eeprom
			if (enter) { enter = false;
				lcd_print_cfghd(false, F("Save?"));
				lcd.setCursor(0,1); lcd.print(F(" press OK..."));
			} break;
			
		case CFG_EXIT: // leave config mode
			if (enter) { enter = false;
				lcd_print_cfghd(false, F("Exit?"));
				lcd.setCursor(0,1); lcd.print(F(" press OK..."));
			} break;
			
		default: config_state = CFG_EXIT; break;
	}


	return 0;
}

void control_errmsg(char* msg1, char* msg2, bool temp) {
#ifdef DEBUG
	Serial.print(msg1); Serial.print(' '); 
	Serial.print(msg2); Serial.println(temp ? '*' : '.');
#endif
	lcd.clear(); 
	lcd.setCursor(0,0); lcd.print(msg1); 
	lcd.setCursor(0,1); lcd.print(msg2);
	lcd.blink();
	if (!temp) {
		menu_state_last = menu_state;
		menu_state = 100;
		delay(500);
	} else delay(1000);
	return 0;
}

#define DEBUG
#ifdef DEBUG
int control_debug(char* cmd) {
	if (!cmd) return 0;
	switch (cmd[0]) {
		case 'E':
			control_errmsg("Device error.", "test error");
			break;
		case 'S':
			menu_state = atoi(&cmd[1]);
			enter = true;
			Serial.print(menu_state);
			break;
		default: Serial.print("<?>"); break;
	} return 0;
}

#endif
