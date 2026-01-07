#ifndef AO_H_
#define AO_H_

#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <json/json.h>

#include "character.h"
#include "font.h"
#include "image_cache.h"
#include "menu.h"
#include "misc.h"
#include "surfaces.h"
#include "util.h"
#include "window.h"

class Character;
class Surfaces;

struct MenuInitInfo {
    int side, x, y;
};

class Ao {
    private:
        std::mutex mutex_;
        std::condition_variable cond_;
        std::queue<std::vector<std::string>> queue_;
        std::queue<std::vector<Request>> event_queue_;
        std::unique_ptr<std::thread> th_recv_;
        std::unique_ptr<std::thread> th_send_;
        std::filesystem::path ao_dir_;
        std::unordered_map<std::string, std::string> info_;
        std::unordered_map<int, std::unordered_map<std::string, int>> bind_id_;
        std::unordered_map<int, std::unique_ptr<Character>> characters_;
        std::unique_ptr<Surfaces> surfaces_;
        std::unique_ptr<ImageCache> cache_;
        std::string path_;
        std::string uuid_;
        bool alive_;
        int scale_;
        bool loaded_;
        bool redrawn_;
        std::unique_ptr<Menu> menu_;
        MenuInitInfo menu_init_info_;
        std::unique_ptr<WrapFont> font_;

    public:
        Ao() : alive_(true), scale_(100), loaded_(false), redrawn_(false) {
            init();
#if defined(DEBUG)
            ao_dir_ = "./shell/master";
            surfaces_ = std::make_unique<Surfaces>(ao_dir_);
#endif // DEBUG
        }
        ~Ao();

        bool init();

        void load();

        std::string getInfo(std::string key, bool fallback);

        void create(int side);

        Rect getRect(int side);

        void setBalloonOffset(int side, int x, int y);
        Offset getBalloonOffset(int side);

        std::optional<Offset> getCharacterOffset(int side);

        void show(int side);

        void hide(int side);

        void setSurface(int side, int id);

        void startAnimation(int side, int id);

        bool isPlayingAnimation(int side, int id);

        void bind(int side, int id, std::string from, BindFlag flag);

        void clearCache();

        operator bool() {
            return alive_;
        }

        void run();

        std::string sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args);

        void enqueueDirectSSTP(std::vector<Request> list);

        void reserveMenuParent(int side, int x, int y);
        std::vector<MenuModelData> parseMenuInfo(Json::Value &value);
        std::vector<MenuModelData> getDressUpList();

        int scale() const {
            return scale_;
        }
};

#endif // GL_AYU_H_
