// main.ino, implementation of the SWF device, central controlling hub
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 260925 - implement eeprom function
// 260916 - implement debug module
// 260915 - laid out execution flow

#define DEBUG

#include <EEPROM.h>

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
	
	// tilt orientation
	EEPROM.get(b+i, m_tilt_neg); 	i = i+sizeof(m_tilt_neg);
	

	return 0;
}

int save_eeprom(int b) {
	if (EEPROM.length - b < 256) {
		control_errmsg(F("Device error."), F("Save fail"));
		return -1;
	}
	
	int i = 0;
	
	// servo config
	EEPROM.put(b+i, m_sv_enable); 	i = i+sizeof(m_sv_enable);
	EEPROM.put(b+i, m_sv_lefts); 	i = i+sizeof(m_sv_lefts);
	EEPROM.put(b+i, m_sv_rights); 	i = i+sizeof(m_sv_rights);
	
	// tilt orientation
	EEPROM.put(b+i, m_tilt_neg); 	i = i+sizeof(m_tilt_neg);
	
	
	return 0;
}

void setup() {
	m_battery = 0;
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
	
	if (!ok) {
		control_errmsg(F("Device error."), atoi(e));
		alert_start(ALERT_ERROR);
		delay(3600000);
	}
#endif

	bool eeprom_ok = false;
	// load configuration data from EEPROM
	if (EEPROM.length() > 256) {
		// first, verify the eeprom has config data
		char ep_head[3]; 
		for (int i = 0; i < 3; i++) {
			ep_head[i] = EEPROM.read(64+i);
		} // i hope that gets unrolled
		if (ep_head[0] != 's' || ep_head[1] != 'w' ep_head[2] != 'f') {
			control_errmsg(F("Device error."), F("No config"), true);
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
	} else Serial.println("read FAIL. now begin loop() ...\n");
	
	while (Serial.available()) {
		char x = Serial.read();
	} Serial.println("\n press enter to continue");
	
	while (!Serial.available()) delay(1);
	while (Serial.available()) char x = Serial.read();
	Serial.println("OK.");
#endif
}

void loop() {
#ifdef DEBUG
	debug();
#endif
	//motion_loop();
	//distance_loop();
	//alert_loop();
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
			case 'A':
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
