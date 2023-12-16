#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <cstdio>
#include <map>
#include "./3DMaths.h"

enum MouseKeyState {
  MOUSE_BUTTON_NONE,
  MOUSE_BUTTON_PRESSED,
  MOUSE_BUTTON_DOWN,
  MOUSE_BUTTON_RELEASED,
};

#include "./main.cpp"

int main(int argc, char **argv) {
  int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  if (SDL_Init(SDL_INIT_EVERYTHING)) {
    return 0;
  }

  GameState *gameState = (GameState *)malloc(sizeof(GameState));
  memset(gameState, 0, sizeof(GameState));
  gameState->screenWidth = 800;
  gameState->aspectRatio_y_over_x = 1;
  gameState->mouseLeftBtn = MOUSE_BUTTON_NONE;

  SDL_Window *window;
  SDL_Renderer *renderer;
  if (SDL_CreateWindowAndRenderer(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x, flags, &window, &renderer)) {
    return 0;
  }

  SDL_Event event;
  SDL_Event e;
  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&e)) {
       if (e.type == SDL_QUIT) {
        quit = true;
      }
    }

    int w; 
    int h;
    if(SDL_GetRendererOutputSize(renderer, &w, &h) == 0) {
      gameState->screenWidth = w;
      gameState->aspectRatio_y_over_x = (float)h / (float)w;
    }

    int x; 
    int y;
    Uint32 mouseState = SDL_GetMouseState(&x, &y);
    gameState->mouseP_screenSpace.x = (float)x;
    gameState->mouseP_screenSpace.y = (float)(h - y); //NOTE: Bottom corner is origin 

    gameState->mouseP_01.x = gameState->mouseP_screenSpace.x / w;
    gameState->mouseP_01.y = gameState->mouseP_screenSpace.y / h;

  if(mouseState && SDL_BUTTON(1)) {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_NONE) {
      gameState->mouseLeftBtn = MOUSE_BUTTON_PRESSED;
    } else if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
      gameState->mouseLeftBtn = MOUSE_BUTTON_DOWN;
    }
  } else {
    if(gameState->mouseLeftBtn == MOUSE_BUTTON_DOWN || gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
      gameState->mouseLeftBtn = MOUSE_BUTTON_RELEASED;
    } else {
      gameState->mouseLeftBtn = MOUSE_BUTTON_NONE;
    }
  }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 1, 50, 32, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    updateGame(gameState, renderer);

    SDL_RenderPresent(renderer);
  }

  return 0;
}