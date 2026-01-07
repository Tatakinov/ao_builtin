#include "menu.h"

#include "ao.h"
#include "logger.h"
#include "util.h"

Menu::Menu(Ao *parent, int side, int x, int y, std::unique_ptr<WrapFont> &font, std::vector<MenuModelData> &model) : parent_(parent), alive_(true), side_(side), model_(model) {
    if (util::isWayland() && !getenv("NINIX_ENABLE_MULTI_MONITOR")) {
        main_display_ = util::getCurrentDisplayID();
        SDL_Rect r;
        SDL_GetDisplayBounds(main_display_, &r);
        if (x < r.x || x > r.x + r.w || y < r.y || y > r.y + r.h) {
            Logger::log("invalid rect");
            alive_ = false;
            return;
        }
        windows_[main_display_] = std::make_unique<MenuMainWindow>(this, main_display_, x - r.x, y - r.y, font);
    }
    else {
        main_display_ = util::getNearestDisplay(x, y);
        SDL_Rect r;
        SDL_GetDisplayBounds(main_display_, &r);
        if (x < r.x || x > r.x + r.w || y < r.y || y > r.y + r.h) {
            alive_ = false;
            return;
        }
        int count;
        auto *displays = SDL_GetDisplays(&count);
        for (int i = 0; i < count; i++) {
            if (main_display_ == displays[i]) {
                windows_[displays[i]] = std::make_unique<MenuMainWindow>(this, displays[i], x - r.x, y - r.y, font);
            }
            else {
                windows_[displays[i]] = std::make_unique<MenuWindow>(this, displays[i]);
            }
        }
        SDL_free(displays);
    }
    windows_.at(main_display_)->setMenuModel(model_);
}

Menu::~Menu() {
    windows_.clear();
}

void Menu::kill() {
    alive_ = false;
}

bool Menu::alive() const {
    return alive_;
}

void Menu::create(SDL_DisplayID id) {
    windows_[id] = std::make_unique<MenuWindow>(this, id);
}

void Menu::destroy(SDL_DisplayID id) {
    if (id == main_display_) {
        alive_ = false;
    }
    windows_.erase(id);
}

void Menu::draw() {
    for (auto &[_, v] : windows_) {
        v->draw();
    }
}

bool Menu::swapBuffers() {
    bool redrawn = false;
    for (auto &[_, v] : windows_) {
        redrawn = v->swapBuffers() || redrawn;
    }
    return redrawn;
}

void Menu::motion(const SDL_MouseMotionEvent &event) {
    for (auto &[_, v] : windows_) {
        v->motion(event);
    }
}

void Menu::button(const SDL_MouseButtonEvent &event) {
    for (auto &[_, v] : windows_) {
        v->button(event);
    }
}

void Menu::wheel(const SDL_MouseWheelEvent &event) {
    for (auto &[_, v] : windows_) {
        v->wheel(event);
    }
}

void Menu::enqueueDirectSSTP(std::vector<Request> list) {
    parent_->enqueueDirectSSTP(list);
}
