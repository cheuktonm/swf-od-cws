// alert.h, header file for alert submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

#ifndef ALERT_H
#define ALERT_H

#define PIN_BZZ_BZZ 6 // connect to transistor base through resistor
#define PIN_BZZ_LED 7 // connect to buzzer led

#define ALERT_NONE 	 0
#define ALERT_INFO   1
#define ALERT_WARN   2
#define ALERT_URGENT 3
#define ALERT_ERROR  4

int alert_setup();
void alert_start(int severity, int timeout = 0);
void alert_stop();
void alert_loop();

// public var
extern char m_bzz_mode;

#endif
