// main.ino, implementation of the SWF device, central controlling hub
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 260925 - implement eeprom function
// 260916 - implement debug module
// 260915 - laid out execution flow

#define DEBUG

#include <EEPROM.h>
#include <Wire.h>

#include "control.h"
#include "alert.h"
#include "motion.h"
#include "distance.h"

#ifdef DEBUG
#define DBG_BUF_LEN 50
char dbg_buf[DBG_BUF_LEN];
int dbg_pos;
#endif

unsigned int m_battery;

int load_eeprom(int b) {
	int i = 0;
	
	// servo config
	EEPROM.get(b+i, m_sv_enable); 	i = i+sizeof(m_sv_enable);
	EEPROM.get(b+i, m_sv_lefts); 	i = i+sizeof(m_sv_lefts);
	EEPROM.get(b+i, m_sv_rights); 	i = i+sizeof(m_sv_rights);
	EEPROM.get(b+i, m_sv_steps); 	i = i+sizeof(m_sv_steps);
	
	// motion module mode
	EEPROM.get(b+i, c_tilt_neg); 	i = i+sizeof(c_tilt_neg);

	// process config
	motion_config();
	distance_config();
	return 0;
}

int save_eeprom(int b) {
	if (EEPROM.length() - b < 256) {
		control_errmsg("Device error.", "Save fail");
		return -1;
	} else { // save header
		EEPROM.put(b-3, (char)'s');
		EEPROM.put(b-2, (char)'w');
		EEPROM.put(b-1, (char)'f');
	} int i = 0;
	
	// servo config
	EEPROM.put(b+i, m_sv_enable); 	i = i+sizeof(m_sv_enable);
	EEPROM.put(b+i, m_sv_lefts); 	i = i+sizeof(m_sv_lefts);
	EEPROM.put(b+i, m_sv_rights); 	i = i+sizeof(m_sv_rights);
	EEPROM.put(b+i, m_sv_steps); 	i = i+sizeof(m_sv_steps);
	
	// motion module mode
	EEPROM.put(b+i, c_tilt_neg); 	i = i+sizeof(c_tilt_neg);
	
	// process config
	motion_config();
	distance_config();
	return 0;
}

void setup() {
	m_battery = 0;
	Wire.begin(); // start the i2c bus
	Wire.setClock(400000); // 400khz i2c might be too vuln to noise?
	
#ifdef DEBUG
	memset(dbg_buf, 0x00, DBG_BUF_LEN); dbg_pos = 0;
	Serial.begin(9600);
	Serial.println("====================");
	Serial.println("\nSWF-OD-CWS is starting...");

	if (control_setup()) Serial.println("control_setup NG");
	if (alert_setup()) Serial.println("alert_setup NG");
	if (motion_setup()) Serial.println("motion_setup NG");
	if (distance_setup()) Serial.println("distance_setup NG");
	
	Serial.println("setup OK. now read EEPROM  ...");	
#else // non debug behaviour
	int e = 0;
	if (control_setup()) 	ok = min(e, 1);
	if (alert_setup()) 		ok = min(e, 2);
	if (motion_setup())		ok = min(e, 3);
	if (distance_setup())	ok = min(e, 4);

#define HANGUP // disable in prod
#ifdef HANGUP 
	if (!ok) {
		char x[3]; x[0] = 0x00; x[1] = 0x00; x[2] = 0x00;
		control_errmsg(F("Device error."), itoa(e, &x[0], 10));
		alert_start(ALERT_ERROR);
		delay(3600000); // hang up the device
	}
#endif
#endif

	bool eeprom_ok = false;
	// load configuration data from EEPROM
	if (0 && EEPROM.length() > 256) {
		// first, verify the eeprom has config data
		char ep_head[3]; 
		for (int i = 0; i < 3; i++) {
			ep_head[i] = EEPROM.read(64+i);
		} // i hope that gets unrolled
		if (ep_head[0] != 's' || ep_head[1] != 'w' && ep_head[2] != 'f') {
			control_errmsg("Device error.", "No config", true);
		} else eeprom_ok = !load_eeprom(64+3);
	}
	
#ifdef DEBUG
	if (eeprom_ok) {
		Serial.print("m_sv_enable:"); Serial.print(m_sv_enable ? 'o':'x'); Serial.print(' ');
		Serial.print("m_sv_lefts:"); Serial.print(m_sv_lefts); Serial.print(' ');
		Serial.print("m_sv_rights:"); Serial.print(m_sv_rights); Serial.print(' ');
		Serial.println(' ');
		Serial.print("m_tilt_neg:"); Serial.print(m_tilt_neg ? 'o':'x'); Serial.print(' '); 
		Serial.println(' ');
		
		Serial.println("read  OK. now begin loop() ...\n");
	} else Serial.println("read  NG. now begin loop() ...\n");
	
	// 261015 temp debug
	m_sv_enable = 1;
	m_sv_lefts = 0;
	m_sv_rights = 180;
	m_sv_steps = 180;
	distance_config();
	c_tilt_neg = 'A';
	motion_config();
	
	
	while (Serial.available()) {
		char x = Serial.read();
	} // Serial.println("\n press enter to continue");
	
	//while (!Serial.available()) delay(1);
	//while (Serial.available()) char x = Serial.read();
	Serial.println("OK.");
#endif
}

void loop() {
#ifdef DEBUG
	debug();
#endif
	if (menu_state != 0 && menu_state != 2) {
		motion_loop();
		distance_loop();
		alert_loop();
	}
	control_loop();
	delay(10);
}

#ifdef DEBUG
void debug() {
	if (dbg_pos >= DBG_BUF_LEN) {
		dbg_pos = 0; Serial.println("cmd buffer overflow\n");
	} // buffer overflowed
	if (Serial.available()) {
		int n = Serial.read();
		if (n != -1 && n != '\r' && n != '\n') {
		dbg_buf[dbg_pos] = (char)n;
		dbg_pos = dbg_pos + 1; dbg_buf[dbg_pos] = 0x00;
		}
	}
	char* cmdend = strchr(dbg_buf, '.');
	if (cmdend) {
		*cmdend = 0x00;
		// process debug command
		switch (dbg_buf[0]) {
			case 'A': // test alerts
				if (dbg_buf[1] == 'X') {
					alert_stop(); Serial.print("Alerts cancelled.");
				} else if (dbg_buf[1] == 'U') {
					alert_start(ALERT_URGENT);
					Serial.print("Started urgent alert.");
				} else if (dbg_buf[1] == 'W') {
					alert_start(ALERT_WARN);
					Serial.print("Started warning alert.");
				} else if (dbg_buf[1] == 'I') {
					alert_start(ALERT_INFO);
					Serial.print("Started info alert.");
				} else if (dbg_buf[1] == 'E') {
					alert_start(ALERT_ERROR);
					Serial.print("Started error alert.");
				} else Serial.print("<?>"); 
				break;
			case 'C':
				control_debug(dbg_buf+1);
				break;
			case 'B': // test battery meter 
				m_battery = atoi(&dbg_buf[1]);
				Serial.print("m_battery: "); Serial.print(m_battery);
				break;
			case 'M': // force motion module
				if (dbg_buf[1] == 'T') {
					Serial.print("m_is_tilted "); 
					Serial.print(m_is_tilted ? 'o' : 'x');
					Serial.print("->");
					m_is_tilted = m_is_tilted ? false : true;
					Serial.print(m_is_tilted ? 'o' : 'x');
				} else if (dbg_buf[1] == 'A') {
					Serial.print("m_mtn_abnormal "); 
					Serial.print(m_mtn_abnormal ? 'o' : 'x');
					Serial.print("->");
					m_mtn_abnormal = m_mtn_abnormal ? false : true;
					Serial.print(m_mtn_abnormal ? 'o' : 'x');
				} else if (dbg_buf[1] == 'D') {
					motion_dismiss(); Serial.print("motion_dismiss()"); 
				} else Serial.print("<?>");
				break;
				
			case 'D': // force distance module
				unsigned int newval = atoi(&dbg_buf[2]);
				if (dbg_buf[1] == 'U') {
					m_us_cm = newval;
					Serial.print("m_us_cm -> "); Serial.print(newval);
				} else if (dbg_buf[1] == 'L') {
					m_ld_cm = newval;
					Serial.print("m_ld_cm -> "); Serial.print(newval);
				} else Serial.print("<?>");
				break;
				
			case 'e':
				char* comma = strchr(dbg_buf, ',');
				if (comma) {
					unsigned int addr = atoi(&dbg_buf[2]);
					unsigned int newval = atoi(comma+1);
					if (dbg_buf[1] == 'i') { // write int
						EEPROM.put(addr, newval);
						Serial.print(addr); Serial.print('='); Serial.println(newval);
					} else if (dbg_buf[1] == 'b') { // write bool
						bool x = newval; EEPROM.put(addr, x);
						Serial.print(addr); Serial.print('='); Serial.println(x ? '1' : '0');
					} else Serial.print("<?>");
				} break;
				
				
			default: Serial.print("<?>"); break;
		} 
		Serial.println(' ');
		// reset buffer
		dbg_pos = 0;
	}
	
	return;
}
#endif
