#ifndef MENU_MAIN_WINDOW_H_
#define MENU_MAIN_WINDOW_H_

#include <list>
#include <memory>

#include "font.h"
#include "menu_model.h"
#include "menu_window.h"

class Menu;

class MenuMainWindow : public MenuWindow {
    private:
        int base_x_, base_y_;
        Rect r_;
        std::unique_ptr<WrapFont> &font_;
        std::list<std::unique_ptr<MenuModel>> model_;
    public:
        MenuMainWindow(Menu *menu, SDL_DisplayID id, int x, int y, std::unique_ptr<WrapFont> &font);
        ~MenuMainWindow();
        void setMenuModel(std::vector<MenuModelData> &model) override;
        void drawContent() override;
        void motion(const SDL_MouseMotionEvent &event) override;
        void button(const SDL_MouseButtonEvent &event) override;
        void wheel(const SDL_MouseWheelEvent &event) override;
};

#endif // MENU_MAIN_WINDOW_H_
