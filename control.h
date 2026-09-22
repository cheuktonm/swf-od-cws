// control.h, header file for control submodule
// for ELEN30013 Semester 2, 2026
// C. Mong (cheukton.mong@student.unimelb.edu.au)

#ifndef CONTROL_H
#define CONTROL_H

#define PIN_LCD_RS 12
#define PIN_LCD_EN 11
#define PIN_LCD_D4 5
#define PIN_LCD_D5 4
#define PIN_LCD_D6 3
#define PIN_LCD_D7 2

#define PIN_BTN_OK 0
#define PIN_BTN_LEFT 0
#define PIN_BTN_RIGHT 0

int control_setup();
int control_config();
void control_loop();

void control_errmsg(char* msg1, char* msg2);

// main.ino
extern unsigned int m_battery;

#endif
