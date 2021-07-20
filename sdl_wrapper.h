/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020-2021
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#ifndef SDL_WRAPPER_H_
#define SDL_WRAPPER_H_

int sdl_wrapper_init(int w, int h, const char* title);
void sdl_wrapper_destroy();

#endif /* SDL_WRAPPER_H_ */
