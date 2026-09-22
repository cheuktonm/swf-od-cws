// alert.cpp, implementation of the alert submodule
// for ELEN30013 Semester 2, 2026
// C. Mong 1606303 (cheukton.mong@student.unimelb.edu.au)

// 260912 - write the alert module code

#include <Arduino.h>
#include "alert.h"

// pin defines in alert.h

#define ALERT_INT_LED_URGENT 10 
#define ALERT_INT_BZZ_URGENT 10
// urgent buzzer - alternate 1000/500 hz 
#define ALERT_BZZ_URG_A 1000 
#define ALERT_BZZ_URG_B 500 

#define ALERT_INT_LED_WARN   50
#define ALERT_INT_BZZ_WARN   50

#define ALERT_INT_BZZ_INFO   50

#define ALERT_INT_BZZ_ERROR  100
// error buzzer - alternate 1000/500 hz 
#define ALERT_BZZ_ERR_A 1000 
#define ALERT_BZZ_ERR_B 500 

// private vars
int alert_state;
int alert_timeout;
int alert_led_state;
int alert_bzz_state;
unsigned long alert_cnt;
unsigned long alert_cnt_led;
unsigned long alert_cnt_bzz;

// implementation

// setup the alert module
int alert_setup() {
	pinMode(BUZZ_BZZ, OUTPUT);
	pinMode(BUZZ_LED, OUTPUT);
	alert_state = ALERT_NONE;
	alert_led_state = false;
	alert_bzz_state = false;
	alert_timeout = 0;
	alert_cnt = 0;
	alert_cnt_led = 0;
	alert_cnt_bzz = 0;
	noTone(BUZZ_BZZ);
	return 0;
}

// begin an alert
void alert_start(int severity, int timeout) {
	alert_cnt = millis(); // reset the counter
	switch (severity) {
		case ALERT_URGENT:
			// highest level warning
			// buzzer = full wail on
			// led    = fast toggle
			alert_state = ALERT_URGENT;
			
			break;
		case ALERT_WARN:
			// medium level warning
			// buzzer = triple beep, short pause
			// led    = fast blink
			alert_state = ALERT_WARN;
	
			break;
			
		case ALERT_INFO:
			// lowest level warning
			// buzzer = double bip, pause
			// led    = off 
			alert_state = ALERT_INFO;
	
			break;
			
		case ALERT_ERROR:
			// indicate an error to the user
			// buzzer = high, low, pause
			// led    = on
			alert_state = ALERT_ERROR;
			digitalWrite(BUZZ_LED, HIGH);

			break;
			
		default: alert_stop();
	} 
	if (alert_state){ // start counters
		alert_cnt_led = alert_cnt;
		alert_cnt_bzz = alert_cnt;
		alert_timeout = timeout ? millis()+timeout*1000 : 0;
	} return;
}

// stop any alerts
void alert_stop() {
	alert_state = ALERT_NONE;
	noTone(BUZZ_BZZ);
	alert_led_state = false;
	digitalWrite(BUZZ_LED, LOW);
	alert_bzz_state = false;
	return;
}

void alert_loop() {
	unsigned long now = millis();
	if (alert_timeout && now > alert_timeout) {
		alert_state = ALERT_NONE;
		alert_timeout = 0;
		alert_stop(); 
		return;
	}
	
	switch (alert_state) {
		case ALERT_URGENT:
			if (now - alert_cnt_led > ALERT_INT_LED_URGENT) {
				// update LED - urgent
				alert_led_state = alert_led_state ? false : true;
				digitalWrite(BUZZ_LED, alert_led_state);
				alert_cnt_led = now;
			}
			if (now - alert_cnt_bzz > ALERT_INT_BZZ_URGENT) {
				// update buzzer - urgent
				alert_bzz_state = alert_bzz_state ? false : true;
				tone(BUZZ_BZZ, alert_bzz_state ? ALERT_BZZ_URG_A : ALERT_BZZ_URG_B);
				alert_cnt_bzz = now;
			} break;
			
		case ALERT_WARN:
			if (now - alert_cnt_led > ALERT_INT_LED_WARN) {
				// update LED - warning
				alert_led_state = alert_led_state ? false : true;
				digitalWrite(BUZZ_LED, alert_led_state);
				alert_cnt_led = now;
			}
			if (now - alert_cnt_bzz > ALERT_INT_BZZ_WARN) {
				// update buzzer - warning
				if (alert_bzz_state) { // triple bip
					tone(BUZZ_BZZ, 1000); delay(1);
					noTone(BUZZ_BZZ); delay(1);
					tone(BUZZ_BZZ, 1000); delay(1);
					noTone(BUZZ_BZZ); delay(1);
					tone(BUZZ_BZZ, 1000); delay(1);
					noTone(BUZZ_BZZ);
				} alert_bzz_state = alert_bzz_state ? false : true;
				alert_cnt_bzz = now;
			} break;
			
		case ALERT_INFO: 
			// LED is OFF
			if (now - alert_cnt_bzz > ALERT_INT_BZZ_INFO) {
				// update buzzer - warning
				if (alert_bzz_state) { // double bip
					tone(BUZZ_BZZ, 1000); delay(1);
					noTone(BUZZ_BZZ); delay(1);
					tone(BUZZ_BZZ, 1000); delay(1);
					noTone(BUZZ_BZZ);
				} alert_bzz_state = alert_bzz_state ? false : true;
				alert_cnt_bzz = now;
			} break;
			break;
			
		case ALERT_ERROR:
			// LED is solid on, no update
			if (now - alert_cnt_bzz > ALERT_INT_BZZ_ERROR) {
				// update buzzer - error
				switch (alert_bzz_state) {
					case 0: tone(BUZZ_BZZ, ALERT_BZZ_ERR_A); break;
					case 1: tone(BUZZ_BZZ, ALERT_BZZ_ERR_B); break;
					case 2: noTone(BUZZ_BZZ); break;
					default: break;
				} alert_bzz_state = (alert_bzz_state == 2) ? 0 : alert_bzz_state++;
				alert_cnt_bzz = now;
			} break;
			
		default: break;
	} return;
}
