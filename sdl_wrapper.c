/*
 *
 * Nano RISC-V 32i emulator
 * Copyright (C) Dmitry 'MatrixS_Master' Solovyev, 2020
 *
 * This work is licensed under the MIT License. See included LICENSE file
 *
 * */

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "sdl_wrapper.h"

static SDL_Window* wnd = NULL;
static SDL_Renderer* ren = NULL;
static SDL_Texture* screen_tex = NULL;

int sdl_wrapper_init(int w, int h, const char* title)
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        puts("Can't open video");
        return 1;
    }

    if (SDL_CreateWindowAndRenderer(w,h,0,&wnd,&ren)) {
        puts("Can't create main window");
        return 2;
    }

    if (SDL_SetRenderDrawColor(ren,0,0,0,255)) {
        puts("Can't set up background color");
        return 3;
    }

    SDL_SetWindowTitle(wnd,title);
    SDL_RenderClear(ren);
    SDL_RenderPresent(ren);

    return 0;
}

void sdl_wrapper_destroy()
{
    if (ren) SDL_DestroyRenderer(ren);
    if (wnd) SDL_DestroyWindow(wnd);
    if (screen_tex) SDL_DestroyTexture(screen_tex);
    SDL_Quit();
}
