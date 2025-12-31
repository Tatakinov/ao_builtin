#include "menu_main_window.h"

#include "logger.h"

MenuMainWindow::MenuMainWindow(Menu *menu, SDL_DisplayID id, int x, int y, std::unique_ptr<WrapFont> &font) : MenuWindow(menu, id), base_x_(x), base_y_(y), font_(font) {
    SDL_Rect r;
    SDL_GetDisplayBounds(id, &r);
    r_ = {r.x, r.y, r.w, r.h};
}

MenuMainWindow::~MenuMainWindow() {
}

void MenuMainWindow::setMenuModel(std::vector<MenuModelData> &model) {
    Rect r = {base_x_, base_y_, 0, 0};
    model_.push_back(std::make_unique<MenuModel>(model, r, r_, font_));
    change();
}

void MenuMainWindow::drawContent() {
    for (auto &v : model_) {
        auto &s = v->getSurface();
        auto t = std::make_unique<WrapTexture>(renderer_, s->surface(), true);
        auto r = v->rect();
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_FRect fr = { r.x, r.y, r.width, r.height };
        Logger::log("draw.rect:", r.x, r.y, r.width, r.height);
        SDL_RenderTexture(renderer_, t->texture(), nullptr, &fr);
    }
}

void MenuMainWindow::motion(const SDL_MouseMotionEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
}

void MenuMainWindow::button(const SDL_MouseButtonEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    MenuWindow::button(event);
}

void MenuMainWindow::wheel(const SDL_MouseWheelEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
}
