#include "menu.h"

Menu::Menu(SDL_Window* parent) {
    window_ = SDL_CreatePopupWindow(parent, 0, 0, 200, 200, SDL_WINDOW_POPUP_MENU | SDL_WINDOW_ALWAYS_ON_TOP);
    renderer_ = SDL_CreateRenderer(window_, nullptr);
}

Menu::~Menu() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
}

void Menu::draw() {
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_SetRenderDrawColor(renderer_, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(renderer_);
}

void Menu::swapBuffers() {
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_RenderPresent(renderer_);
}
