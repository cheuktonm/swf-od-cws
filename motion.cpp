// motion.cpp, implementation of the motion sensors submodule
// for ELEN30013 Semester 2, 2026
// C. Mong 1606303 (cheukton.mong@student.unimelb.edu.au)

// 260915 - write the motion module

#include <Arduino.h>
#include "motion.h"
#include "alert.h"

//#include "MPU6050.h"

// pin defines in motion.h

#define THRES_TILT_BITS 12 // require x/16 bits set to trigger

// public bars
bool m_is_tilted;

// private vars
//MPU6050 mpu;
bool tilt_enable, tilt_neg;
int tilt_thres, tilt_trig;
int tilt; // array of 16 bits

int motion_setup() {
	// assumed i2c is alr setup at this point
	// init accelerometer
	//mpu.initialize();
	//if (mpu.testConnection() == false) return -1;
	
	// setup tilt sensor
	pinMode(PIN_TILT, INPUT);
	
	tilt_enable = false;
	tilt_neg = false;
	tilt_thres = 0;
	tilt_trig = 0;
	tilt = 0;
	
	m_is_tilted = false;

	return 0;
}

bool read_tilt(bool cycle) {
	if (cycle) { // read a full 16 bits
		for (int i = 0; i < sizeof(int); i++) {
			tilt = tilt << digitalRead(PIN_TILT);
			delay(1); // takes 32ms in total
		} return __builtin_popcount(tilt) > THRES_TILT_BITS;
	}
	// reads tilt sensor, updates history, returns if threshold hit
	tilt = tilt << digitalRead(PIN_TILT); // 16 readings of debounce 
	return __builtin_popcount(tilt) > THRES_TILT_BITS;
}

int motion_config(char cfg_negtilt) {
	// TODO: abnormal movement detection thresholds
	
	// cfg_negtilt: A=Auto, H=horiz, V=vert
	// if the setting for the tilt sensor is Auto
	// then the current reading @ poweron becomes the OFF value
	if (cfg_negtilt == 'A') tilt_neg = read_tilt(true);
	else if (cfg_negtilt == 'H') tilt_neg = false;
	else if (cfg_negtilt == 'V') tilt_neg = true;
	
	return 0;
}

void motion_loop() {
	// tilt switch
	if (tilt_enable && read_tilt()) {
		if (tilt_trig && tilt_trig > tilt_thres) {
			m_is_tilted = true;
			alert_start(ALERT_URGENT, 0);
			// tilt does not rearm until deasserted
			tilt_enable = false; 
			tilt_trig = 0; 
		} else tilt_trig = millis();
	} else { m_is_tilted = false; tilt_trig = 0; }
	
	// read accelerometer, determine abnormal state

	return;
}
