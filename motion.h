// motion.h, header file for motion submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

#ifndef MOTION_H
#define MOTION_H

#define PIN_TILT 1

int motion_setup();
int motion_config(bool cfg_negtilt);
void motion_loop();

bool read_tilt(bool cycle = false);

// public vars
extern bool m_is_tilted;

#endif
