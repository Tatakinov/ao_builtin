#ifndef MENU_WINDOW_H_
#define MENU_WINDOW_H_

#include <vector>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "menu_model.h"

class Menu;

class MenuWindow {
    private:
        bool changed_;
    protected:
        Menu *parent_;
        SDL_Window *window_;
        SDL_Renderer *renderer_;
    public:
        MenuWindow(Menu *menu, SDL_DisplayID id);
        virtual ~MenuWindow();
        void draw();
        virtual void setMenuModel(std::vector<MenuModelData> &model);
        virtual void drawContent();
        bool swapBuffers();
        virtual void motion(const SDL_MouseMotionEvent &event);
        virtual void button(const SDL_MouseButtonEvent &event);
        virtual void wheel(const SDL_MouseWheelEvent &event);
        void kill();
        void change();
};

#endif // MENU_WINDOW_H_
