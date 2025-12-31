#ifndef MENU_MODEL_H_
#define MENU_MODEL_H_

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "font.h"
#include "misc.h"
#include "texture.h"

enum class MenuModelType {
    Action, ActionWithBoolean, ActionWithString, SubMenu
};

enum class ActionType {
    None, StayOnTop, Preferences, Switch, Call, Shell, DressUp,
    Balloon, BasewareVersion, Close, CloseAll,
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

struct MenuModelDataActionWithString {
    ActionType action;
    bool valid;
    std::string caption;
    std::string arg;
};

struct MenuModelDataSubMenu {
    std::string caption;
    std::vector<std::variant<MenuModelDataAction, MenuModelDataActionWithString, MenuModelDataActionWithBoolean, MenuModelDataSubMenu>> children;
};

using MenuModelData = std::variant<MenuModelDataAction, MenuModelDataActionWithString, MenuModelDataActionWithBoolean, MenuModelDataSubMenu>;

class MenuItem {
    private:
        MenuModelData &data_;
        std::unique_ptr<WrapFont> &font_;
        SDL_Surface *surface_;
    public:
        MenuItem(MenuModelData &data, std::unique_ptr<WrapFont> &font);
        ~MenuItem();
        MenuModelData &getModel() {
            return data_;
        }
        int width() const;
        int height() const;
        SDL_Surface *surface();
        void highlight();
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
    public:
        MenuModel(std::vector<MenuModelData> &data, const Rect parent_r, const Rect display_r, std::unique_ptr<WrapFont> &font);
        ~MenuModel();
        MenuModelData &get(int index);
        Rect rect() {
            return r_;
        }
        void highlight(int index);
        void unhighlight(int index);
        void change();
        std::unique_ptr<WrapSurface> &getSurface();
};

#endif // MENU_MODEL_H_
