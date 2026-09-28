// distance.cpp, implementation of the distance sensors submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 260914 - write the distance module, ultrasonic, servo

#include <Arduino.h>
#include "distance.h"

#include <Servo.h>

// pin defines in distance.h

#define ULTRASONIC_CONV 58

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
	
	
	return 0; // all good
}

int distance_config() {
	if (!m_sv_enable) {
		sense_step = 0;
		now_step = 0;
		return 0;
	}

	sense_step = (m_sv_rights-m_sv_lefts)/m_sv_steps;
	if (sense_step < 27) sense_step = 27; // hard locked min to lidar fov
	now_step = 0;
	
	return 0;
}

void distance_loop() {
	// rotate platform
	if (m_sv_enable) {
		// our range is 180 degrees, divided by sense_step
		servo.write(m_sv_lefts + (sense_step*now_step));
		now_step = (now_step == m_sv_steps) ? 0 : now_step + 1;
		delay(5); // wait for the servo to get there
	}

	// grab ultrasonic measurement
	digitalWrite(PIN_US_TRIG, HIGH); // send a pulse
	delayMicroseconds(10);
	digitalWrite(PIN_US_TRIG, LOW);
	m_us_cm = pulseIn(PIN_US_ECHO, HIGH)/ULTRASONIC_CONV;
	
	// grab lidar measurement
	
	
	return;
}

