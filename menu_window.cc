#include "menu_window.h"

#include <cassert>

#include "logger.h"
#include "menu.h"

MenuWindow::MenuWindow(Menu *menu, SDL_DisplayID id): changed_(true), focus_(false), parent_(menu) {
    SDL_Rect r;
    SDL_GetDisplayBounds(id, &r);
    SDL_PropertiesID p = SDL_CreateProperties();
    assert(p);
    SDL_SetStringProperty(p, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Menu");
    SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN, true);
    SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
    SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_UNDEFINED_DISPLAY(id));
    SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_UNDEFINED_DISPLAY(id));
    SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, r.w);
    SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, r.h);
    window_ = SDL_CreateWindowWithProperties(p);
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    SDL_SetRenderVSync(renderer_, 1);
}

MenuWindow::~MenuWindow() {
    if (window_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
    }
}

void MenuWindow::draw() {
    if (changed_) {
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer_);
        drawContent();
    }
}

void MenuWindow::setMenuModel(std::vector<MenuModelData> &model) {
    // nop
}

void MenuWindow::drawContent() {
}

bool MenuWindow::swapBuffers() {
    if (changed_) {
        changed_ = false;
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_RenderPresent(renderer_);
        return true;
    }
    return false;
}

void MenuWindow::motion(const SDL_MouseMotionEvent &event) {
    // nop
}

void MenuWindow::button(const SDL_MouseButtonEvent &event) {
    if (event.down) {
        parent_->kill();
    }
}

void MenuWindow::wheel(const SDL_MouseWheelEvent &event) {
    // nop
}

void MenuWindow::focus(bool focus) {
    // kill if focus lost after focus gained
    if (focus_ && !focus) {
        parent_->kill();
    }
    focus_ = focus;
}

void MenuWindow::change() {
    changed_ = true;
}
