#include "menu.h"

#include "ao.h"
#include "logger.h"
#include "util.h"

Menu::Menu(Ao *parent, int side, int x, int y, int w, int h, std::unique_ptr<WrapFont> &font, std::vector<MenuModelData> &model) : parent_(parent), alive_(true), side_(side), model_(model) {
    auto id = util::getNearestDisplay(x, y);
    if (!util::isWayland()) {
        SDL_Rect r;
        SDL_GetDisplayBounds(id, &r);
        x -= r.x;
        y -= r.y;
        w = r.w;
        h = r.h;
    }
    window_ = std::make_unique<MenuMainWindow>(this, id, x, y, w, h, font);
    window_->setMenuModel(model_);
}

Menu::~Menu() {
    window_.reset();
}

void Menu::kill() {
    alive_ = false;
}

bool Menu::alive() const {
    return alive_;
}

void Menu::draw() {
    window_->draw();
}

bool Menu::swapBuffers() {
    return window_->swapBuffers();
}

void Menu::motion(const SDL_MouseMotionEvent &event) {
    window_->motion(event);
}

void Menu::button(const SDL_MouseButtonEvent &event) {
    window_->button(event);
}

void Menu::wheel(const SDL_MouseWheelEvent &event) {
    window_->wheel(event);
}

void Menu::focus(bool focus) {
    window_->focus(focus);
}

bool Menu::focused() const {
    return window_->focused();
}

void Menu::enqueueDirectSSTP(std::vector<Request> list) {
    parent_->enqueueDirectSSTP(list);
}
