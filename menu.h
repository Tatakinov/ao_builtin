#ifndef MENU_H_
#define MENU_H_

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

class Menu {
    private:
        SDL_Window *window_;
        SDL_Renderer *renderer_;
    public:
        Menu(SDL_Window *parent);
        ~Menu();
        bool operator==(SDL_WindowID id) {
            return SDL_GetWindowID(window_) == id;
        }
        void draw();
        void swapBuffers();
};

#endif // MENU_H_
