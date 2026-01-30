#ifndef MENU_H_
#define MENU_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_video.h>

#include "menu_model.h"
#include "menu_window.h"
#include "menu_main_window.h"

class Ao;

class Menu {
    private:
        Ao *parent_;
        bool alive_;
        int side_;
        SDL_DisplayID main_display_;
        std::unique_ptr<MenuWindow> window_;
        std::vector<MenuModelData> model_;
    public:
        Menu(Ao *parent, int side, int x, int y, int w, int h, std::unique_ptr<WrapFont> &font, std::vector<MenuModelData> &model);
        ~Menu();
        int side() const {
            return side_;
        }
        void kill();
        bool alive() const;
        void motion(const SDL_MouseMotionEvent &event);
        void button(const SDL_MouseButtonEvent &event);
        void wheel(const SDL_MouseWheelEvent &event);
        void focus(bool focus);
        bool focused() const;
        void draw();
        bool swapBuffers();

        void enqueueDirectSSTP(std::vector<Request> list);
};

#endif // MENU_H_
