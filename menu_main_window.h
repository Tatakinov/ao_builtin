#ifndef MENU_MAIN_WINDOW_H_
#define MENU_MAIN_WINDOW_H_

#include "misc.h"

#include <list>
#include <memory>

#if defined(IS__NIX)
#include <wayland-client.h>
#endif // Linux/Unix

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
#if defined(IS__NIX)
        wl_registry *reg_;
        wl_compositor *compositor_;
#endif // Linux/Unix
    public:
        MenuMainWindow(Menu *menu, SDL_DisplayID id, int x, int y, std::unique_ptr<WrapFont> &font);
        ~MenuMainWindow();
        void setMenuModel(std::vector<MenuModelData> &model) override;
        void drawContent() override;
        void motion(const SDL_MouseMotionEvent &event) override;
        void button(const SDL_MouseButtonEvent &event) override;
        void wheel(const SDL_MouseWheelEvent &event) override;
#if defined(IS__NIX)
        void setCompositor(wl_compositor *compositor) {
            compositor_ = compositor;
        }
#endif // Linux/Unix
};

#endif // MENU_MAIN_WINDOW_H_
