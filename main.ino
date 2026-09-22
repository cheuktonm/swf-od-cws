// main.ino, implementation of the SWF device, central controlling hub
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 260916 - implement debug module
// 260915 - laid out execution flow

#include "control.h"
#include "alert.h"
#include "motion.h"
#include "distance.h"

#define DEBUG

#ifdef DEBUG
#define DBG_BUF_LEN 50
char dbg_buf[DBG_BUF_LEN];
int dbg_pos;
#endif

unsigned int m_battery;

void setup() {
	m_battery = 0;
#ifdef DEBUG
	memset(dbg_buf, 0x00, DBG_BUF_LEN); dbg_pos = 0;
	Serial.begin(9600);
	Serial.println("\nSWF-OD-CWS is starting...");

	if (control_setup()) Serial.println("control_setup NG");
	if (alert_setup()) Serial.println("alert_setup NG");
	if (motion_setup()) Serial.println("motion_setup NG");
	if (distance_setup()) Serial.println("distance_setup NG");
	
#else // non debug behaviour
	int ok = true;
	if (control_setup()) ok = false;
	if (alert_setup()) ok = false;
	if (motion_setup()) ok = false
	if (distance_setup()) ok = false;
	
	if (!ok) {
		control_errmsg("Device error.", "");
		alert_start(ALERT_ERROR);
		delay(3600000);
	}
#endif
	
#ifdef DEBUG
	Serial.println("setup OK. now read EEPROM  ...");
	Serial.println("read  OK. now begin loop() ...\n");
#endif
}

void loop() {
#ifdef DEBUG
	debug();
#endif
	motion_loop();
	distance_loop();
	alert_loop();
	control_loop();
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
			default: Serial.print("<?>"); break;
		} 
		Serial.println(' ');
		// reset buffer
		dbg_pos = 0;
	}
	
	return;
}
#endif
