// distance.cpp, implementation of the distance sensors submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 261001 - implement angle-distance array
// 260914 - write the distance module, ultrasonic, servo

#include <Arduino.h>
#include "distance.h"
#include "motion.h"
#include "alert.h"

#include <Servo.h>

// pin defines in distance.h

#define ULTRASONIC_CONV 58

#define US_THRES 1500
#define LD_THRES 1500

// public vars
int m_us_cm;
int m_ld_cm;
bool m_sv_enable;
int m_sv_lefts;
int m_sv_rights;
int m_sv_steps;

// private vars
Servo servo;
int sense_step, now_step;

struct dist_t {
	int m; // measurement
	//int a; // angle
	//int t; // time
};
dist_t us_a[180];//, ld_a[180]; // 180 slots

int distance_setup() {
	// assumed i2c is alr setup at this point
	// setup lidar
	
	// setup ultrasonic
	pinMode(PIN_US_TRIG, OUTPUT);
	pinMode(PIN_US_ECHO, INPUT);
	
	// setup servo motor
	servo.attach(PIN_SERVO);
	
	// values to be configured from eeprom
	m_sv_enable = false;
	m_sv_lefts = 0;
	m_sv_rights = 0;
	// calculated values
	sense_step = 0;
	now_step = 0;
	
	m_us_cm = 0; m_ld_cm = 0;
	
	memset(&us_a[0], 0x00, 180*sizeof(dist_t));
	// memset(&ld_a[0], 0x00, 180*sizeof(dist_t));
	
	return 0; // all good
}

int distance_config() {
	if (!m_sv_enable) {
		sense_step = 0;
		now_step = 0;
		// park the servo to the middle
		int angle = (m_sv_rights-m_sv_lefts)/2;
		servo.write(m_sv_lefts+angle);
		return 0;
	}

	sense_step = (m_sv_rights-m_sv_lefts)/m_sv_steps;
	if (sense_step < 27) sense_step = 27; // hard locked min to lidar fov
	now_step = 0;
	
	return 0;
}

void distance_loop() {
	// grab ultrasonic measurement
	digitalWrite(PIN_US_TRIG, HIGH); // send a pulse
	delayMicroseconds(10);
	digitalWrite(PIN_US_TRIG, LOW);
	m_us_cm = pulseIn(PIN_US_ECHO, HIGH)/ULTRASONIC_CONV;
	for (int i = 0; i < sense_step; i++) {
		us_a[(sense_step*now_step)+i].m = m_us_cm;
	}	
	
	// grab lidar measurement
	
	// if the distance is below the threshold
	// raise the alarm
	for (int i = 0; i < 180; i++) {
		if (us_a[i].m < US_THRES)
			alert_start(ALERT_URGENT, 10);
		//if (ld_a[i].m < LD_THRES)
		//	alert_start(ALERT_URGENT, 10);
	}
	
	// rotate platform
	if (m_sv_enable) {
		// our range is 180 degrees, divided by sense_step
		servo.write(m_sv_lefts + (sense_step*now_step));
		now_step = (now_step == m_sv_steps) ? 0 : now_step + 1;
		delay(5); // wait for the servo to get there
	}
	
	return;
}
