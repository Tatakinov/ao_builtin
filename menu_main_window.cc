#include "menu_main_window.h"

MenuMainWindow::MenuMainWindow(Menu *menu, SDL_DisplayID id, int x, int y) : MenuWindow(menu, id), origin_x_(x), origin_y_(y) {
}

MenuMainWindow::~MenuMainWindow() {
}

void MenuMainWindow::drawContent() {
}

void MenuMainWindow::motion(const SDL_MouseMotionEvent &event) {
}

void MenuMainWindow::button(const SDL_MouseButtonEvent &event) {
}

void MenuMainWindow::wheel(const SDL_MouseWheelEvent &event) {
}
