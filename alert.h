// alert.h, header file for alert submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

#ifndef ALERT_H
#define ALERT_H

// TODO: change to PIN_BZZ
#define BUZZ_BZZ 7 // connect to transistor base through resistor
#define BUZZ_LED 6 // connect to buzzer led

#define ALERT_NONE 	 0
#define ALERT_URGENT 1
#define ALERT_WARN   2
#define ALERT_INFO   3
#define ALERT_ERROR  4

// forward decs, to be moved to header
int alert_setup();
void alert_start(int severity, int timeout = 0);
void alert_stop();
void alert_loop();


#endif
