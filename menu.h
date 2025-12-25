#ifndef MENU_H_
#define MENU_H_

#include <memory>
#include <unordered_map>

#include <SDL3/SDL_video.h>

#include "menu_model.h"
#include "menu_window.h"
#include "menu_main_window.h"

class Menu {
    private:
        bool alive_;
        SDL_DisplayID main_display_;
        std::unordered_map<SDL_DisplayID, std::unique_ptr<MenuWindow>> windows_;
    public:
        Menu(int x, int y);
        ~Menu();
        void create(SDL_DisplayID id);
        void destroy(SDL_DisplayID id);
        void kill();
        bool alive() const;
        void setMenuModel(MenuModel &model);
        void motion(const SDL_MouseMotionEvent &event);
        void button(const SDL_MouseButtonEvent &event);
        void wheel(const SDL_MouseWheelEvent &event);
        void draw();
        bool swapBuffers();
};

#endif // MENU_H_
