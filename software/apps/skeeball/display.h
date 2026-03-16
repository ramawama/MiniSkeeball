#pragma once

#include <stdint.h>

void display_init(void);

// Fill screen black
void display_clear(void);

// Show a text string centered on screen (white on black, scale 1)
void display_show_text(const char *s);

// Brief flash — call on score event
void display_flash(void);
