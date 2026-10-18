// control.cpp, implementation of the lcd+buttons submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 260926 - fix button debouncing
// 260925 - implement saving config to eeprom
// 260924 - implement servo configuration
// 260922 - write the menu+configuration system
// 260921 - write the lcd controlling code

#include <Arduino.h>
#include "control.h"
#include "distance.h"
#include "motion.h"
#include "alert.h"

#include <LiquidCrystal.h>

// pins defined in control.h

#define LCD_CHAR_FULL_THIN  1
#define LCD_CHAR_HALF_THIN  2
#define LCD_CHAR_FULL	    3
#define LCD_CHAR_HALF		4

#define VERSION_MAJOR '0'
#define VERSION_MINOR 'a'

// main.ino import
int save_eeprom(int b);

// private forward decs
int configuration();

#define CFG_MODE 1
#define CFG_SV_EN 2 // servo enable
#define CFG_SV_LS 3 // servo left stop
#define CFG_SV_RS 4 // servo right stop
#define CFG_SV_AS 5 // servo angle step
#define CFG_MT_TL 6 // tilt sensor behaviour
#define CFG_TH_US 7 // ultrasonic threshold
#define CFG_TH_LD 8 // lidar threshold
#define CFG_SAVE  9 // write to eeprom
#define CFG_EXIT  10 // always the last option

// private vars
LiquidCrystal lcd(PIN_LCD_RS, 
                  PIN_LCD_EN, 
                  PIN_LCD_D4, 
                  PIN_LCD_D5, 
                  PIN_LCD_D6, 
                  PIN_LCD_D7);
								
int menu_state, menu_state_last;
unsigned int menu_timer;
bool enter, enter_cfg;
unsigned int config_state;
bool config_sel;
unsigned int config_val;

bool btn_ok, btn_right, btn_left;
bool btn_ok_, btn_right_, btn_left_;
int btn_cnt;

int control_setup() {
	lcd.begin(16, 2); // we are using 16c*2r lcd
    lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(F("SWF-OD-CWS"));
	lcd.setCursor(0,1);
	lcd.print(F("Starting..."));
	
	lcd.setCursor(14,0); lcd.write('V');
	lcd.setCursor(15,0); lcd.write(VERSION_MAJOR);
	lcd.setCursor(15,1); lcd.write(VERSION_MINOR);
	
	const char full_t[8] = {3,3,3,3,3,3,3,3}; 
	const char half_t[8] = {0,0,0,0,3,3,3,3};
	lcd.createChar(LCD_CHAR_FULL_THIN, full_t); // full cell thin
	lcd.createChar(LCD_CHAR_HALF_THIN, half_t); // half cell thin
	
	const char full[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}; 
	const char half[8] = {0,0,0,0,0xff,0xff,0xff,0xff};
	lcd.createChar(LCD_CHAR_FULL, full); // full cell
	lcd.createChar(LCD_CHAR_HALF, half); // half cell
	
	pinMode(PIN_BTN_OK, INPUT);
	pinMode(PIN_BTN_LEFT, INPUT);
	pinMode(PIN_BTN_RIGHT, INPUT);
	
	menu_state = 0;	// Set to 1 in prod
	menu_state_last = 0; // Set to 1 in prod
	menu_timer = 0;
	config_state = 0;
	config_sel = 0;
	config_val = 0;
	enter = true; enter_cfg = false;
	
	btn_ok = false; btn_right = false; btn_left = false;
	btn_ok_ = false; btn_right_ = false; btn_left_ = false;
	btn_cnt = 0;
	
	return 0;
}

int control_config() {
	return 0;
}

void print_battery() {
	// show state of charge of the battery
	if (m_battery > 80) {
		lcd.setCursor(15,0); lcd.write(LCD_CHAR_FULL_THIN);
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_FULL_THIN);
	} else if (m_battery > 60) {
		lcd.setCursor(15,0); lcd.write(LCD_CHAR_HALF_THIN);
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_FULL_THIN);
	} else if (m_battery > 40) {
		lcd.setCursor(15,0); lcd.write(' ');
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_FULL_THIN);
	} else if (m_battery > 30) {
		lcd.setCursor(15,0); lcd.write(' ');
		lcd.setCursor(15,1); lcd.write(LCD_CHAR_HALF_THIN);
	} else { // critical battery
		lcd.setCursor(15,0); lcd.write('x');
		lcd.setCursor(15,1); lcd.write('x');
	} return;
}

void control_loop() {
	unsigned int now = millis();
	// lockout buttons for 250ms since last press
	if (now - btn_cnt > 250) { 
		// read button state now
		bool btn_ok_now = digitalRead(PIN_BTN_OK);
		bool btn_right_now = digitalRead(PIN_BTN_RIGHT);
		bool btn_left_now = digitalRead(PIN_BTN_LEFT);
	
		// buttons can be ON if wasnt ON before
		btn_ok = !btn_ok_ && btn_ok_now; 
		btn_right = !btn_right && btn_right_now;
		btn_left = !btn_left && btn_left_now;
		
		// current state becomes previous state
		btn_ok_ = btn_ok_now; 
		btn_right_ = btn_right_now; 
		btn_left_ = btn_left_now;
		
		btn_cnt = now;
	} else {
		btn_ok = false; btn_right = false; btn_left = false;
	}

	
	// print stuffs to lcd
	switch (menu_state) {
		case 0: // disabled
			if (enter) {
				lcd.clear();
				lcd.print(F("OFF"));
				enter = false;
			} 
			if (now - menu_timer < 10000) break;
			else menu_timer = now; // rarely update b status in off mode 
			print_battery();
			break;
		case 1: // normal state
			if (now - menu_timer < 500) break;
			else menu_timer = now; // update lcd every half second

			lcd.clear();
			lcd.setCursor(0,0);
			//lcd.print("US "); lcd.print(min(m_us_cm/100, 99)); // print ultrasonic reading (m)
			lcd.print("US "); m_us_cm ? lcd.print(m_us_cm) : lcd.print('-');
			lcd.setCursor(7,0);
			lcd.print("TL "); lcd.write(m_is_tilted ? LCD_CHAR_FULL : ' '); // tilt sensor
			lcd.setCursor(0,1); 
			//lcd.print("LD "); lcd.print(min(m_ld_cm/100, 99)); // print lidar reading (m)
			lcd.print("LD "); m_ld_cm ? lcd.print(m_ld_cm) : lcd.print('-');
			lcd.setCursor(7,1); 
			lcd.print("AC "); lcd.write(m_mtn_abnormal ? LCD_CHAR_FULL : ' '); // accelerometer status
			
			// print_mute()
			lcd.setCursor(12,0); lcd.write(m_bzz_mode ? 'M' : ' ');
			lcd.setCursor(12,1);
			if (m_bzz_mode == 0) lcd.write(' ');
			else if (m_bzz_mode == 1) lcd.write('/');
			else if (m_bzz_mode == 2) lcd.write('X');

			print_battery();
			break;
		
		case 2: // configuration mode
			// moved to seperate fn
			if (configuration()) {
				enter = true;
				menu_state = menu_state_last;
			} return; // skip below to reset buttons
			
		case 100: // display error message
			break;
			
		default: break;
	}

	switch (menu_state) {
		case 0:
			if (btn_ok) { // disabled -> enabled
				enter = true;
				menu_state_last = 0;
				menu_state = 1;
				
				alert_stop();
				motion_dismiss();
			} break;
		case 1:
			if (btn_ok) { // idle -> configuration
				enter = true;
				menu_state_last = 1;
				menu_state = 2;
				config_state = 0;
				
				alert_stop();
				motion_dismiss();
			} else if (btn_left) { // dismiss any alerts
				alert_stop();
				motion_dismiss();
			} else if (btn_right) { // set  mute mode
				m_bzz_mode = (m_bzz_mode == 2) ? 0 : m_bzz_mode+1;
				alert_stop();
			} break;
		case 100:
			if (btn_ok) { // dismiss error
				lcd.noBlink();
				menu_state = menu_state_last;
				enter = true;
			}
		default: break;
	}
	
	return;
}

void lcd_print_cfghd(bool sel, char* name) {
	lcd.clear(); lcd.setCursor(0,0);
	lcd.print(sel ? F("--> ") : F("CFG:"));
	lcd.print(name ? name : " ");
	return;
}

int configuration() {
	// handle button inputs
	if (btn_right && !config_sel && config_state && config_state < CFG_EXIT) {
		enter = true; config_state++; enter_cfg = true;
	} else if (btn_left && !config_sel && config_state > 1) {
		enter = true; config_state--; enter_cfg = true;
	} else switch(config_state) { // abuse of notation
		case CFG_SAVE:
			if (btn_ok) {
				lcd.setCursor(0,1); lcd.print('\\'); // give indication it is trying
				config_sel = 0; config_val = -1;
				if (save_eeprom(64+3) == 0) { 
					lcd.setCursor(0,1); lcd.print("       OK!"); 
				} else { lcd.setCursor(0,1); lcd.print(" save error!"); }
			} break;
		case CFG_EXIT:
			if (btn_ok) {
				config_sel = 0; config_val = -1; config_state = 0;
				return 1; // leave configuration mode				
			} break;
			
		case CFG_MODE:
			if (btn_ok) {
				menu_state_last = (menu_state_last >=1) ? 0 : menu_state_last+1;
				enter = true;
			} break;
		
		case CFG_SV_EN:
			if (btn_ok) { 
				m_sv_enable = m_sv_enable ? 0 : 1;
				enter = true;
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
		case CFG_SV_AS:
			if (btn_ok) {
				if (config_sel) m_sv_steps = config_val;
				config_sel = config_sel ? false : true; enter = true;
			} else if (btn_left) {
				config_val = max(config_val-1, 5); enter = true;
			} else if (btn_right) {
				config_val = min(config_val+1, 180); enter = true;
			} break;
			
		case CFG_TH_US:
			if (btn_ok) {
				if (config_sel) m_us_thres = config_val;
				config_sel = config_sel ? false : true; enter = true;
			} else if (btn_left) {
				config_val = max(config_val - 10, 0); enter = true;
			} else if (btn_right) {
				config_val = config_val + 10; enter = true;
			} break;
		case CFG_TH_LD:
			if (btn_ok) {
				if (config_sel) m_ld_thres = config_val;
				config_sel = config_sel ? false : true; enter = true;
			} else if (btn_left) {
				config_val = max(config_val-10, 0); enter = true;
			} else if (btn_right) {
				config_val = config_val+10; enter = true;
			} break;
			
		case CFG_MT_TL: 
			if (btn_ok) {
				switch (c_tilt_neg) {
					case 'A': c_tilt_neg = 'H'; break;
					case 'H': c_tilt_neg = 'V'; break;
					case 'V': c_tilt_neg = 'D'; break;
					case 'D': c_tilt_neg = 'A'; break;
				} enter = true;
			} break;
			
		default: break;
	}
	
	// print stuffs to lcd
	switch (config_state) {
		case 0: // just entered config mode
			config_sel = false; config_val = 0;
			config_state = 1; // falls through switch
		// -- below state becomes 1st selection upon entering config
		
		case CFG_MODE: // mode selection
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "Mode");
				lcd.setCursor(0,1); lcd.print(' '); 
				switch (menu_state_last) {
					case 0: lcd.print("Off"); break;
					case 1: lcd.print("On"); break;
					default: break;
				}
			} break;
			
		case CFG_SV_EN: // servo on/off
			if (enter) { enter = false;
				lcd_print_cfghd(false, "Servo");
				lcd.setCursor(0,1); lcd.print(m_sv_enable ? " On" : " Off");
			} break;
			
		case CFG_SV_LS: // servo left stop
			if (enter_cfg) { config_val = m_sv_lefts; enter_cfg = false; }
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "SV Left Stop");
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;
		case CFG_SV_RS: // servo right stop
			if (enter_cfg) { config_val = m_sv_rights; enter_cfg = false; }
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "SV Rght Stop");
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;
		case CFG_SV_AS: // servo angle step
			if (enter_cfg) { config_val = m_sv_steps; enter_cfg = false; }
      		if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "SV Angl Step");
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;
			
		case CFG_MT_TL: 
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "Motion mode");
				lcd.setCursor(0,1);
				switch (c_tilt_neg) {
					case 'A': lcd.print(" Auto"); break;
					case 'H': lcd.print(" Horizontal"); break;
					case 'V': lcd.print(" Vertical"); break;
					case 'D': lcd.print(" Disabled"); break;
				}
			} break;
			
		case CFG_TH_US: // ultrasonic threshold
			if (enter_cfg) { config_val = m_us_thres; enter_cfg = false; }
			if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "US Threshold");
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;
		case CFG_TH_LD: // lidar threshold
			if (enter_cfg) { config_val = m_ld_thres; enter_cfg = false; }
      		if (enter) { enter = false;
				lcd_print_cfghd(config_sel, "LD Threshold");
				lcd.setCursor(0,1); lcd.print(config_val);
			} break;			
			
		case CFG_SAVE: // write config to eeprom
			if (enter) { enter = false;
				lcd_print_cfghd(false, "Save?");
				lcd.setCursor(0,1); lcd.print(F(" press OK..."));
			} break;
			
		case CFG_EXIT: // leave config mode
			if (enter) { enter = false;
				lcd_print_cfghd(false, "Exit?");
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
		delay(1000);
	} else delay(5000);
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
