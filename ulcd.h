#ifndef ULCD_UI_H
#define ULCD_UI_H

enum Wave { SIN, TRI, SAW, SQU };

void ulcd_init();
void ulcd_update_wave(Wave wave);
void ulcd_update_volume(uint16_t vol);

#endif // ULCD_UI_H