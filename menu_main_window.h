#ifndef MENU_MAIN_WINDOW_H_
#define MENU_MAIN_WINDOW_H_

#include <list>

#include "menu_model.h"
#include "menu_window.h"

class MenuMainWindow : public MenuWindow {
    private:
        int origin_x_, origin_y_;
    public:
        MenuMainWindow(Menu *menu, SDL_DisplayID id, int x, int y);
        ~MenuMainWindow();
        void drawContent() override;
        void motion(const SDL_MouseMotionEvent &event) override;
        void button(const SDL_MouseButtonEvent &event) override;
        void wheel(const SDL_MouseWheelEvent &event) override;
};

#endif // MENU_MAIN_WINDOW_H_
