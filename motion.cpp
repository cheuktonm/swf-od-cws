// motion.cpp, implementation of the motion sensors submodule
// for ELEN30013 Semester 2, 2026
// C. Mong 1606303 (cheukton.mong@student.unimelb.edu.au)

// 260924 - fix tilt logic
// 260915 - write the motion module

#include <Arduino.h>
#include "motion.h"
#include "alert.h"

//#include "MPU6050.h"

// pin defines in motion.h

#define THRES_TILT_BITS 12 // require x/16 bits set to trigger

// public bars
bool m_is_tilted;
bool m_mtn_abnormal;
bool m_tilt_neg;
char c_tilt_neg;
int m_mtn_angle;

// private vars
//MPU6050 mpu;
bool tilt_enable;
int tilt_thres, tilt_trig, tilt_cnt;
int tilt; // array of 16 bits

int motion_setup() {
	// assumed i2c is alr setup at this point
	// init accelerometer
	//mpu.initialize();
	//if (mpu.testConnection() == false) return -1;
	
	// setup tilt sensor
	pinMode(PIN_TILT, INPUT);
	
	tilt_enable = true;
	tilt_thres = 0;
	tilt_trig = 0;
	tilt_cnt = 0;
	tilt = 0;
	
	m_is_tilted = false;
	m_mtn_angle = 0;

	return 0;
}

bool read_tilt(bool cycle) {
	if (cycle) { // read a full 16 bits
		for (int i = 0; i < 2*sizeof(int)*8; i++) {
			tilt = (tilt<<1) + digitalRead(PIN_TILT);
			delay(10); // takes 2*sizeof(int)*10 ms in total
		} return __builtin_popcount(tilt) > THRES_TILT_BITS;
	}
	// reads tilt sensor, updates history, returns if threshold reached
	tilt = (tilt<<1) + (digitalRead(PIN_TILT) ^ m_tilt_neg); // XOR provides m_tilt_neg logic
	return __builtin_popcount(tilt) > THRES_TILT_BITS;
}

int motion_config() {
	// TODO: abnormal movement detection thresholds
	
	// cfg_negtilt: A=Auto, H=horiz, V=vert
	// if the setting for the tilt sensor is Auto
	// then the current reading @ poweron becomes the OFF value
	switch (c_tilt_neg) {
		default: c_tilt_neg = 'A';
		case 'A': m_tilt_neg = read_tilt(true); break;
		case 'H': m_tilt_neg = false; break;
		case 'V': m_tilt_neg = true; break;
	}

	Serial.print("motion_config(): ");
	Serial.print("tn="); Serial.print(c_tilt_neg); 
	Serial.print("->"); Serial.println(m_tilt_neg ? 'O' : 'X');
	
	tilt = 0;
	return 0;
}

void motion_loop() {
	// tilt switch
	if (tilt_enable) {
		int now = millis();
		if (now - tilt_cnt > 1000) { // 1s*16bit = 16 seconds of history
			tilt_cnt = now;
			if (read_tilt()) {
				m_is_tilted = true;
				// alert_start(ALERT_URGENT, 0);
				// tilt does not rearm until deasserted
				tilt_enable = false; 
			}
		}
	}

	
	// read accelerometer, determine abnormal state

	return;
}

void motion_dismiss() {
	tilt_enable = true; 
	m_is_tilted = false; 
	tilt = 0; tilt_cnt = 0;
	return;
}
