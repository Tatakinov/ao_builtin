#ifndef MENU_MODEL_H_
#define MENU_MODEL_H_

#include <string>
#include <vector>

union MenuModelData;

struct MenuModelDataAction {
    int type;
    int action_type;
    bool valid;
    std::string title;
};

struct MenuModelDataActionWithBoolean {
    int type;
    int action_type;
    bool valid;
    std::string title;
    bool state;
};

struct MenuModelDataActionWithString {
    int type;
    int action_type;
    bool valid;
    std::string title;
    std::string arg;
};

struct MenuModelDataSubMenu {
    int type;
    std::vector<MenuModelData> children;
};

union MenuModelData {
    int type;
    MenuModelDataAction action;
    MenuModelDataActionWithString string;
    MenuModelDataActionWithBoolean boolean;
};

class MenuModel {
    private:
        std::vector<MenuModelData> &data_;
    public:
        MenuModel(std::vector<MenuModelData> &data);
};

#endif // MENU_MODEL_H_
