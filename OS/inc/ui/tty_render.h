#pragma once

/* Render the existing CLI TTY into a framebuffer */
void tty_render_fullscreen(void);
void tty_render_region(int x, int y, int width, int height);
