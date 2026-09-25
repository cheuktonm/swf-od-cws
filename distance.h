// distance.h, header file for distance submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

#ifndef DISTANCE_H
#define DISTANCE_H

#define PIN_US_TRIG 9
#define PIN_US_ECHO 8
#define PIN_SERVO   10

int distance_setup();
int distance_config();
void distance_loop();

// public vars
extern int m_us_cm;
extern int m_ld_cm;

// loaded vars
extern bool m_sv_enable;
extern int m_sv_lefts;
extern int m_sv_rights;

#endif
