#include "menu_main_window.h"

#include "logger.h"
#include "menu.h"

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
        SDL_RenderTexture(renderer_, t->texture(), nullptr, &fr);
    }
}

void MenuMainWindow::motion(const SDL_MouseMotionEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    int depth = model_.size();
    for (auto &v : model_) {
        v->unhighlight();
    }
    for (auto v = model_.rbegin(); v != model_.rend(); v++) {
        if ((*v)->highlight(event.x, event.y)) {
            break;
        }
        depth--;
    }
    if (depth > 0 && depth < model_.size()) {
        model_.erase(std::next(model_.begin(), depth), model_.end());
    }
    auto submenu = model_.back()->get<MenuModelDataSubMenu>();
    if (submenu) {
        auto r = model_.back()->rect();
        r.y += model_.back()->getSelectedItemY();
        model_.push_back(std::make_unique<MenuModel>(submenu.value().children, r, r_, font_));
    }
    change();
}

void MenuMainWindow::button(const SDL_MouseButtonEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    auto action = model_.back()->getAction();
    if (action == ActionType::None) {
        return;
    }
    else if (action == ActionType::Site) {
        auto data = model_.back()->get<MenuModelDataActionWithArgs>();
        if (data && data->args.size() > 0) {
            std::vector<std::string> args;
            args.push_back(data->args[0]);
            if (data->args.size() >= 3) {
                args.push_back(data->args[2]);
            }
            parent_->enqueueDirectSSTP({{"EXECUTE", "VisitSite", {data->args[0]}}});
        }
    }
    else if (action == ActionType::StayOnTop) {
        // TODO stub
    }
    else if (action == ActionType::Preferences) {
        parent_->enqueueDirectSSTP({{"EXECUTE", "OpenPreferences", {}}});
    }
    else if (action == ActionType::Switch) {
        auto data = model_.back()->get<MenuModelDataActionWithArgs>();
        if (data && data->args.size() > 0) {
            parent_->enqueueDirectSSTP({{"EXECUTE", "ChangeGhost", {data->args[0]}}});
        }
    }
    else if (action == ActionType::Call) {
        auto data = model_.back()->get<MenuModelDataActionWithArgs>();
        if (data && data->args.size() > 0) {
            parent_->enqueueDirectSSTP({{"EXECUTE", "SummonGhost", {data->args[0]}}});
        }
    }
    else if (action == ActionType::Shell) {
        auto data = model_.back()->get<MenuModelDataActionWithArgs>();
        if (data && data->args.size() > 0) {
            parent_->enqueueDirectSSTP({{"EXECUTE", "ChangeShell", {data->args[0]}}});
        }
    }
    else if (action == ActionType::DressUp) {
        // TODO stub
    }
    else if (action == ActionType::Balloon) {
        auto data = model_.back()->get<MenuModelDataActionWithArgs>();
        if (data && data->args.size() > 0) {
            parent_->enqueueDirectSSTP({{"EXECUTE", "ChangeBalloon", {data->args[0]}}});
        }
    }
    else if (action == ActionType::BasewareVersion) {
        parent_->enqueueDirectSSTP({{"EXECUTE", "ShowBasewareVersion", {}}});
    }
    else if (action == ActionType::Close) {
        parent_->enqueueDirectSSTP({{"EXECUTE", "CloseGhost", {}}});
    }
    else if (action == ActionType::CloseAll) {
        parent_->enqueueDirectSSTP({{"EXECUTE", "CloseAllGhost", {}}});
    }
    else if (action == ActionType::ScriptInputBox) {
        parent_->enqueueDirectSSTP({{"EXECUTE", "OpenScriptInputBox", {}}});
    }
    parent_->kill();
}

void MenuMainWindow::wheel(const SDL_MouseWheelEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
}
