// motion.cpp, implementation of the motion sensors submodule
// for ELEN30013 Semester 2, 2026
// C. Mong 1606303 (cheukton.mong@student.unimelb.edu.au)

// 261018 - merge mpu implementation from Calder
// 261017 - add motion module disable option
// 260924 - fix tilt logic
// 260915 - write the motion module

#include <Arduino.h>
#include <Wire.h>
#include "motion.h"
#include "alert.h"

// pin defines in motion.h

#define THRES_TILT_BITS 12 // require x/16 bits set to trigger
#define ACC_THRES 2 // in g
#define GYRO_THRES 180 // in degrees/sec

// public bars
bool m_is_tilted;
bool m_mtn_abnormal;
bool m_tilt_neg;
char c_tilt_neg;
int m_mtn_angle;

// private vars
int MPU6050 = 0x68; // MPU-6050 I2C address
float X_a, Y_a, Z_a; // Accelerometer outputs (in g)
float GX, GY, GZ; // Gyroscope outputs (in deg/sec)
bool tilt_enable;
int tilt_thres, tilt_trig, tilt_cnt;
int tilt; // array of 16 bits

int motion_setup() {
	// assumed i2c is alr setup at this point
	
	//configure MPU6050
	Wire.beginTransmission(MPU6050); // sends this to MPU6050 address
  	Wire.write(0x6B);   // PWR_MGMT_1 register
  	Wire.write(0x00);   // Wake the sensor up (it starts in sleep mode by default)
  	Wire.endTransmission();
  	delay(10); // give time to boot up device
	
	Wire.beginTransmission(MPU6050);
	Wire.write(0x1C);   // ACCEL_CONFIG register (adjusts range of measureable acceleration (default is 2))
	Wire.write(0x10);   // sets the range to 16 which gives the range ±8g 
	Wire.endTransmission();

	// setup tilt sensor
	pinMode(PIN_TILT, INPUT);
	
	tilt_enable = true;
	tilt_thres = 0;
	tilt_trig = 0;
	tilt_cnt = 0;
	tilt = 0;
	
	m_is_tilted = false;

	m_mtn_abnormal = false;

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
		case 'D': // disabled
				tilt_enable = false;
				m_is_tilted = false;
				m_mtn_abnormal = false;
				break; 
	}

	//Serial.print("motion_config(): ");
	//Serial.print("tn="); Serial.print(c_tilt_neg); 
	//Serial.print("->"); Serial.println(m_tilt_neg ? 'O' : 'X');
	
	tilt = 0;
	return 0;
}

void motion_loop() {
	if (c_tilt_neg == 'D') return;
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
	Wire.beginTransmission(MPU6050);
  	Wire.write(0x3B); // Starts at ACCEL_XOUT_H
  	Wire.endTransmission(false);
  	Wire.requestFrom(MPU6050, 6, true);

	X_a = (Wire.read() << 8 | Wire.read());
  	Y_a = (Wire.read() << 8 | Wire.read());
  	Z_a = (Wire.read() << 8 | Wire.read());

	// Convert to g (range is +/-8g -> 4096.0 per g)
	X_a = X_a / 4096.0;
  	Y_a = Y_a / 4096.0;
  	Z_a = Z_a / 4096.0;

	Wire.beginTransmission(MPU6050);
  	Wire.write(0x43); // Start at GYRO_XOUT_H
  	Wire.endTransmission(false);
  	Wire.requestFrom(MPU6050, 6, true);

  	GX = (Wire.read() << 8 | Wire.read());
  	GY = (Wire.read() << 8 | Wire.read());
  	GZ = (Wire.read() << 8 | Wire.read());

  	// Convert to deg/sec (default range is +/-250 deg/s -> 131 LSB per deg/s)
  	GX = GX / 131.0;
  	GY = GY / 131.0;
  	GZ = GZ / 131.0;
	
	float acc_mag = sqrt(X_a*X_a + Y_a*Y_a + Z_a*Z_a); // magnitude of acceleration
	if(acc_mag> ACC_THRES) m_mtn_abnormal = true;

	float gyro_mag = sqrt(GX*GX + GY*GY + GZ*GZ); // magnitude of gyro
	if(gyro_mag > GYRO_THRES) m_mtn_abnormal = true;
	
	// Serial.print(acc_mag); Serial.print(' '); Serial.print(gyro_mag); Serial.println(' ');

	return;
}

void motion_dismiss() {
	if (c_tilt_neg == 'D') return;
	tilt_enable = true; 
	m_is_tilted = false; 
	tilt = 0; tilt_cnt = 0;
	m_mtn_abnormal = false;
	return;
}
