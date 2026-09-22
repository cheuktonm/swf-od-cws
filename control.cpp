// control.cpp, implementation of the lcd+buttons submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

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

// private forward decs
int configuration(bool btn_ok, bool btn_right, bool btn_left);
#define CFG_MODE 1
#define CFG_SV_EN 2 // servo enable
#define CFG_SV_LS 3 // servo left stop
#define CFG_SV_RS 4 // servo right stop
#define CFG_SV_AS 5 // servo angle step
#define CFG_EXIT 6 // always the last option

// private vars
LiquidCrystal lcd(PIN_LCD_RS, 
                  PIN_LCD_EN, 
                  PIN_LCD_D4, 
                  PIN_LCD_D5, 
                  PIN_LCD_D6, 
                  PIN_LCD_D7);
				  
int menu_state;
int config_state;
bool config_sel;
unsigned int config_val;

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
	config_state = 0;
	config_sel = 0;
	config_val = 0;
	
	return 0;
}

int control_config() {
	return 0;
}

void control_loop() {
	// read button state now
	bool btn_ok = digitalRead(PIN_BTN_OK);
	bool btn_right = digitalRead(PIN_BTN_RIGHT);
	bool btn_left = digitalRead(PIN_BTN_LEFT);
	
	// print stuffs to lcd
	switch (menu_state) {
		case 0: // disabled
			lcd.clear();
			lcd.print(F("OFF"));
			break;
		case 1: // idle state
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("US "); lcd.print(m_us_cm/100); // print ultrasonic reading (m)
			lcd.print("TL "); lcd.write(m_is_tilted ? ' ' : 'X'); // tilt sensor
			lcd.setCursor(0,1); 
			lcd.print("LD "); lcd.print(m_ld_cm/100); // print lidar reading (m)
			lcd.print("AC "); lcd.write(true ? ' ' : 'X'); // accelerometer status
			// TODO: fill in accelerometer bool
			
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
			}
			
			break;
		
		case 2: // configuration mode
			// moved to seperate fn
			int ret = configuration(btn_ok, btn_right, btn_left); 
			if (ret == 99) menu_state = 1;
			else if (ret == 98) menu_state = 0;
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
				menu_state = 1; // TODO: actually enable the device
			}
			break;
		case 1:
			if (btn_ok) { // idle -> configuration
				menu_state = 2;
				config_state = 0;
			}
			break;
		default: break;
	}
	
	
	
	return 0;
}

void lcd_print_cfghd(bool sel) {
	if (sel) {
		lcd.write(LCD_CHAR_FULL);
		lcd.write(LCD_CHAR_FULL);
		lcd.write(LCD_CHAR_FULL);
		lcd.write(':');
	} else lcd.print(F("CFG:"));
	return;
}

int configuration(bool btn_ok, bool btn_right, bool btn_left) {
	bool e = false; // true if option just selected
	// reread buttons
	btn_ok = (digitalRead(PIN_BTN_OK) && btn_ok) ? true : false;
	btn_right = (digitalRead(PIN_BTN_RIGHT) && btn_right) ? true : false;
	btn_left = (digitalRead(PIN_BTN_LEFT) && btn_left) ? true : false;
	
	// handle button inputs
	if (config_state && btn_right && config_state < 99) {
		if (!config_sel) {
			e = true; config_state++; // navigate right
		} else config_val++; // increment value
	} else if (config_state && btn_left && config_state > 1) {
		if (!config_sel) {
			e = true; config_state--; // navigate left
		} else config_val--; // decrement value
	} else switch (config_state) { // abuse of notation
		case CFG_EXIT: // leave config mode
			if (btn_ok) {
				config_sel = 0; config_val = 0; config_state = 0;
				return 99; // return to idle=99, return to off=98
			} break;
		case CFG_SV_EN:
			if (btn_ok) { // servo enable toggle
				m_sv_enable = m_sv_enable ? false : true;
			} break;
			
		case CFG_SV_LS: // servo left stop
			if (btn_ok && config_sel) { // set value
				config_sel = false;
				m_sv_lefts = config_val;
			} else if (btn_ok && !config_sel) { // pick value
				config_sel = true;
			} break;
		case CFG_SV_RS: // servo right stop
			if (btn_ok && config_sel) { // set value
				config_sel = false;
				m_sv_rights = config_val;
			} else if (btn_ok && !config_sel) { // pick value
				config_sel = true;
			} break;
			
		default: break;
	}
	
	// print stuffs to lcd
	lcd.clear(); lcd.setCursor(0,0);
	switch (config_state) {
		case 0: // just entered config mode
			delay(1); // enforce delay to prevent dbl btn
			config_sel = false; config_val = 0; 
			config_state = 1; // half relying on switch's behaviour
		// -- below state becomes 1st selection upon entering config
		
		case CFG_MODE: // mode selection
			lcd_print_cfghd(config_sel);
			lcd.print(F("Mode"));
			lcd.setCursor(0,1); lcd.print(" 1");
			break;
			
		case CFG_SV_EN: // servo on/off
			lcd_print_cfghd(false);
			lcd.print(F("Servo"));
			lcd.setCursor(0,1); lcd.print(m_sv_enable ? " On" : " Off");
			break;
			
		case CFG_SV_LS: // servo left stop
			if (e) { e = 0; config_val = m_sv_lefts; }
			lcd_print_cfghd(config_sel);
			lcd.print(F("sv left S"));
			lcd.setCursor(0,1); lcd.print(config_val);
			break;
		case CFG_SV_RS: // servo right stop
			if (e) { e = 0; config_val = m_sv_rights; }
			lcd_print_cfghd(config_sel);
			lcd.print(F("sv rght S"));
			lcd.setCursor(0,1); lcd.print(config_val);
			break;
			
		case CFG_EXIT: // leave config mode
			lcd_print_cfghd(false); lcd.print(F("Exit?"));
			lcd.setCursor(0,1); lcd.print(F(" press OK..."));			
			break;
			
		default: config_state = CFG_EXIT; break;
	}

	return 0;
}

void control_errmsg(char* msg1, char* msg2) {
	lcd.clear(); 
	lcd.setCursor(0,0); lcd.print(msg1); 
	lcd.setCursor(0,1); lcd.print(msg2);
	lcd.blink();
	return 0;
}