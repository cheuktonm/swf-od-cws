// distance.cpp, implementation of the distance sensors submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

// 261018 - merge lidar implementation from Calder, cleanup
// 261001 - implement angle-distance array
// 260914 - write the distance module, ultrasonic, servo

#include <Arduino.h>
#include "distance.h"
#include "motion.h"
#include "alert.h"

#include <Servo.h>
#include <VL53L1X.h> //Download Vl53L1X Pololu library

// pin defines in distance.h

#define ULTRASONIC_CONV 58

// urgent alert thresholds are not user configurable
#define US_THRES_UR 300
#define LD_THRES_UR 100 //In long mode, VL53L1X can measure up to 360cm

#define LD_TIMEOUT 500 //500ms
#define LD_BUDGET 50000 //50,000us = 50ms
#define LD_PERIOD 50 //50ms

// public vars
int m_us_cm;
int m_ld_cm;
int m_sv_enable;
int m_sv_lefts;
int m_sv_rights;
int m_sv_steps;

int m_us_thres;
int m_ld_thres;

// private vars
Servo servo;
VL53L1X lidar;

int m_us_last;
int m_ld_last;
int sense_step, now_step;
int servo_timer;
bool lidar_ok;

int distance_setup() {
	// assumed i2c is alr setup at this point

	// setup lidar
	lidar.setTimeout(LD_TIMEOUT);
	//initialises and tests if lidar initialised
	if(!lidar.init()){
		Serial.println("Lidar failed init");
		lidar_ok = false;
		//return -1;
	} else lidar_ok = true;
	lidar.setDistanceMode(VL53L1X::Long); //DistanceMode: Short/Medium/Long. If the mode is set to longer distances, it measures longer distnaces better, but reading becomes more innacurate with ambient light
  	lidar.setMeasurementTimingBudget(LD_BUDGET);//does all the measurements in 50ms
  	lidar.startContinuous(LD_PERIOD);//starts measuring distance every 50ms. Since the timing budget is 50ms, and startContinous is 50ms, the moment one reading it done, the next one starts


	// setup ultrasonic
	pinMode(PIN_US_TRIG, OUTPUT);
	pinMode(PIN_US_ECHO, INPUT);
	
	// setup servo motor
	servo.attach(PIN_SERVO, 500, 2500);
	servo_timer = 0;
	
	// values to be configured from eeprom
	m_sv_enable = 0;
	m_sv_lefts = 0;
	m_sv_rights = 0;
	// calculated values
	sense_step = 0;
	now_step = 0;
	
	m_us_last = 0; m_ld_last = 0;
	
	m_us_cm = 0; m_ld_cm = 0;
	
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
	} else m_sv_enable = 1;

	sense_step = (m_sv_rights-m_sv_lefts)/m_sv_steps;
	//if (sense_step < 27) {
	//	sense_step = 27; // hard locked min to lidar fov
	//	m_sv_steps = 180/sense_step;
	//	sense_step = (m_sv_rights-m_sv_lefts)/m_sv_steps;
	//}
	now_step = 0; servo_timer = 0;
	
	return 0;
}

void distance_loop() {
	// grab ultrasonic measurement
	m_us_last = m_us_cm;
	digitalWrite(PIN_US_TRIG, HIGH); // send a pulse
	delayMicroseconds(10);
	digitalWrite(PIN_US_TRIG, LOW);
	m_us_cm = pulseIn(PIN_US_ECHO, HIGH)/ULTRASONIC_CONV;
	
	// grab lidar measurement
	if (lidar_ok) {
		m_ld_last = m_ld_cm;
		m_ld_cm = lidar.read()/10;
		int ld_status = lidar.ranging_data.range_status;
		bool m_ld_reliable = !lidar.timeoutOccurred() && (ld_status == VL53L1X::RangeValid || ld_status == VL53L1X::RangeValidMinRangeClipped);
	
		if(!m_ld_reliable) m_ld_cm = 0; // a value of 0 signals invalid reading.
	} else m_ld_cm = 0;
	
	// Might improve accuracy??
	// if (m_ld_cm && m_us_cm)
	// 	  m = (m_us_cm + m_ld_cm)/2;
	// else m = 0;
	
	// if the distance is below the *warning* threshold
	// raise the alarm
	if (m_us_cm && m_us_cm < m_us_thres && m_us_last < m_us_thres) 
		alert_start(ALERT_URGENT, 10);
	
	// if the distance is below the *urgent* threshold
	if (m_us_cm && m_us_cm < US_THRES_UR && m_us_last < US_THRES_UR) 
		alert_start(ALERT_URGENT, 10);
	// if (m_ld_cm && m_ld_cm < LD_THRES_UR && m_ld_last < LD_THRES_UR) 
	//	  alert_start(ALERT_URGENT, 10);
	
	// rotate platform
	int now = millis();
	if (m_sv_enable) {
		if (now-servo_timer > 500) {
			//Serial.print(m_sv_lefts);Serial.print(' ');
			//Serial.print(sense_step);Serial.print(' ');
			//Serial.print(now_step);Serial.print(' ');
			//Serial.print(m_sv_enable);Serial.println(' ');
			
			servo_timer = now;
			// our range is 180 degrees, divided by sense_step
			servo.write(min(m_sv_lefts + (sense_step*now_step),180));
			
			if (m_sv_enable == 1) {
				now_step++;
				if (now_step >= m_sv_steps) m_sv_enable = 2;
			} else if (m_sv_enable == 2) {
				now_step--;
				if (now_step <= 0) m_sv_enable = 1;
			} delay(5); // wait for the servo to get there
		} 
	}
	
	return;
}
