#include "menu_model.h"

#include "logger.h"

namespace {
    const int invalid = -1;
}

MenuItem::MenuItem(MenuModelData &data, std::unique_ptr<WrapFont> &font) : data_(data), font_(font), highlight_(false) {
    SDL_Color color = {0x00, 0x00, 0x00, 0xff};
    std::visit([&](const auto &d) {
        surface_ = TTF_RenderText_Blended(font_->font(), d.caption.data(), d.caption.length(), color);
    }, data_);
}

MenuItem::~MenuItem() {
    if (surface_ != nullptr) {
        SDL_DestroySurface(surface_);
    }
}

int MenuItem::width() const {
    if (surface_ == nullptr) {
        return 0;
    }
    return surface_->w;
}

int MenuItem::height() const {
    if (surface_ == nullptr) {
        return 0;
    }
    return surface_->h;
}

SDL_Surface *MenuItem::surface() {
    return surface_;
}

bool MenuItem::highlight(int x, int y) {
    if (x >= 0 && x < width() && y >= 0 && y < height()) {
        highlight_ = true;
        return true;
    }
    return false;
}

void MenuItem::unhighlight() {
    highlight_ = false;
}

std::optional<std::vector<MenuModelData>> MenuItem::getModel() {
    if (std::holds_alternative<MenuModelDataSubMenu>(data_)) {
        Logger::log("Submenu!");
        return std::make_optional<std::vector<MenuModelData>>(std::get<MenuModelDataSubMenu>(data_).children);
    }
    return std::nullopt;
}

ActionType MenuItem::getAction() {
    ActionType type = ActionType::None;
    std::visit([&](auto &x) {
        type = x.action;
    }, data_);
    return type;
}

MenuModel::MenuModel(std::vector<MenuModelData> &data, const Rect parent_r, const Rect display_r, std::unique_ptr<WrapFont> &font) : r_(0, 0, 0, 0), height_(display_r.height), scroll_(0), changed_(true), index_(invalid) {
    Logger::log("data.size:", data.size());
    for (auto &v : data) {
        item_list_.push_back(std::make_unique<MenuItem>(v, font));
        auto &last = item_list_.back();
        if (r_.width < last->width()) {
            r_.width = last->width();
        }
        r_.height += last->height();
    }
    r_.x = parent_r.x + parent_r.width;
    if (r_.x + r_.width > display_r.width) {
        if (parent_r.x - r_.width > 0 && parent_r.width > 0) {
            r_.x = parent_r.x - r_.width;
        }
        else {
            r_.x -= parent_r.x + parent_r.width + r_.width - display_r.width;
        }
    }
    r_.y = parent_r.y;
    if (r_.y + r_.height > display_r.height) {
        r_.y -= parent_r.y + r_.height - display_r.height;
    }
    if (r_.y < 0) {
        r_.y = 0;
    }
    Logger::log("menu.rect: ", r_.x, r_.y, r_.width, r_.height);
    Logger::log("parent.rect: ", parent_r.x, parent_r.y, parent_r.width, parent_r.height);
    Logger::log("display.rect: ", display_r.x, display_r.y, display_r.width, display_r.height);
}

MenuModel::~MenuModel() {
}

int MenuModel::getSelectedItemY() {
    if (index_ == invalid) {
        return 0;
    }
    int height = 0;
    for (int i = 0; i < index_; i++) {
        height += item_list_[index_]->height();
    }
    return height;
}

std::optional<std::vector<MenuModelData>> MenuModel::getSubModel() {
    if (index_ == invalid) {
        return std::nullopt;
    }
    return item_list_[index_]->getModel();
}

std::optional<ActionType> MenuModel::getAction() {
    if (index_ == invalid) {
        return std::nullopt;
    }
    return std::make_optional<ActionType>(item_list_[index_]->getAction());
}

bool MenuModel::highlight(int x, int y) {
    x -= r_.x;
    y -= r_.y;
    for (auto &item : item_list_) {
        index_++;
        if (item->highlight(x, y)) {
            return true;
        }
        y -= item->height();
    }
    index_ = invalid;
    return false;
}

void MenuModel::unhighlight() {
    index_ = invalid;
    for (auto &item : item_list_) {
        item->unhighlight();
    }
}

std::unique_ptr<WrapSurface> &MenuModel::getSurface() {
    if (cache_ && !changed_) {
        return cache_;
    }
    cache_ = std::make_unique<WrapSurface>(r_.width, r_.height);
    SDL_ClearSurface(cache_->surface(), 1, 1, 1, 1); // とりあえず白背景
    int height = 0;
    for (auto &item : item_list_) {
        SDL_Rect r = {0, height, item->width(), item->height()};
        SDL_BlitSurface(item->surface(), nullptr, cache_->surface(), &r);
        height += item->height();
    }
    return cache_;
}
