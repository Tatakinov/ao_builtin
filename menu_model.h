#ifndef MENU_MODEL_H_
#define MENU_MODEL_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "font.h"
#include "misc.h"
#include "texture.h"

namespace {
    const int invalid = -1;
}

enum class ActionType {
    None, Site, StayOnTop, Preferences, Switch, Call, Shell, DressUp,
    Balloon, BasewareVersion, Close, CloseAll, ScriptInputBox,
};

struct MenuModelDataAction {
    ActionType action;
    bool valid;
    std::string caption;
};

struct MenuModelDataActionWithBoolean {
    ActionType action;
    bool valid;
    std::string caption;
    bool state;
};

struct MenuModelDataActionWithArgs {
    ActionType action;
    bool valid;
    std::string caption;
    std::vector<std::string> args;
};

struct MenuModelDataSubMenu {
    ActionType action;
    std::string caption;
    std::vector<std::variant<MenuModelDataAction, MenuModelDataActionWithArgs, MenuModelDataActionWithBoolean, MenuModelDataSubMenu>> children;
};

using MenuModelData = std::variant<MenuModelDataAction, MenuModelDataActionWithArgs, MenuModelDataActionWithBoolean, MenuModelDataSubMenu>;

class MenuItem {
    private:
        MenuModelData data_;
        std::unique_ptr<WrapFont> &font_;
        SDL_Surface *surface_;
        bool highlight_;
    public:
        MenuItem(MenuModelData &data, std::unique_ptr<WrapFont> &font);
        ~MenuItem();
        std::optional<std::vector<MenuModelData>> getModel();
        template <typename T>
            std::optional<T> get() {
                if (std::holds_alternative<T>(data_)) {
                    return std::get<T>(data_);
                }
                return std::nullopt;
            }
        ActionType getAction() const {
            ActionType type = ActionType::None;
            std::visit([&](const auto &x) {
                type = x.action;
            }, data_);
            return type;
        }
        int width() const;
        int height() const;
        SDL_Surface *surface();
        bool highlight(int y);
        void unhighlight();
};

class MenuModel {
    private:
        std::vector<std::unique_ptr<MenuItem>> item_list_;
        Rect r_;
        int height_;
        int scroll_;
        bool changed_;
        std::unique_ptr<WrapSurface> cache_;
        int index_;
    public:
        MenuModel(std::vector<MenuModelData> &data, const Rect parent_r, const Rect display_r, std::unique_ptr<WrapFont> &font);
        ~MenuModel();
        int getSelectedItemY();
        template <typename T>
            std::optional<T> get() {
                if (index_ == invalid) {
                    return std::nullopt;
                }
                return item_list_[index_]->get<T>();
            }
        ActionType getAction() const {
            if (index_ == invalid) {
                return ActionType::None;
            }
            return item_list_[index_]->getAction();
        }
        Rect rect() {
            return r_;
        }
        bool highlight(int x, int y);
        void unhighlight();
        void change();
        std::unique_ptr<WrapSurface> &getSurface();
};

#endif // MENU_MODEL_H_
