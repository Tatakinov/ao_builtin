#include "ao.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#include <ws2tcpip.h>
#include <afunix.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef max
#undef min
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif // WIN32

#include "sorakado.h"
#include "logger.h"
#include "misc.h"
#include "sstp.h"
#include "util.h"
#include "window.h"

namespace {
#ifndef IS_WINDOWS
    inline int closesocket(int fd) {
        return close(fd);
    }
#endif
}

Ao::~Ao() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        alive_ = false;
    }
    th_send_->join();
    th_recv_->join();
    characters_.clear();
#ifdef IS_WINDOWS
    WSACleanup();
#endif // Windows
}

bool Ao::init() {
#ifdef IS_WINDOWS
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif // Windows

    th_recv_ = std::make_unique<std::thread>([&]() {
        uint32_t len;
        while (true) {
            std::cin.read(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            if (std::cin.eof() || len == 0) {
                break;
            }
            char *buffer = new char[len];
            std::cin.read(buffer, len);
            std::string request(buffer, len);
            delete[] buffer;
            if (std::cin.gcount() < len) {
                break;
            }
            auto req = sorakado::Request::parse(request);
            Logger::log(request);
            auto event = req().value();

            sorakado::Response res {204, "No Content"};

            if (event == "Initialize" && req(0)) {
                std::string tmp;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    tmp = req(0).value();
                }
                std::u8string dir(tmp.begin(), tmp.end());
                ao_dir_ = dir;
            }
            else if (event == "Endpoint" && req(0) && req(1)) {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    path_ = req(0).value();
                    uuid_ = req(1).value();
                }
                loaded_ = true;
                cond_.notify_one();
            }
            else if (event == "IsPlayingAnimation" && req(0) && req(1)) {
                int side, id;
                util::to_x(req(0).value(), side);
                util::to_x(req(1).value(), id);
                res = {200, "OK"};
                bool playing = isPlayingAnimation(side, id);
                res() = static_cast<int>(playing);
            }
            else {
                std::vector<std::string> args;
                args.push_back(event);
                for (int i = 0; ; i++) {
                    if (req(i)) {
                        args.push_back(req(i).value());
                    }
                    else {
                        break;
                    }
                }
                std::unique_lock<std::mutex> lock(mutex_);
                queue_.push(args);
            }

            res["Charset"] = "UTF-8";

            std::string response = res;
            Logger::log(response);
            len = response.size();
            std::cout.write(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            std::cout.write(response.c_str(), len);
        }
        {
            std::unique_lock<std::mutex> lock(mutex_);
            loaded_ = true;
            alive_ = false;
            event_queue_.push({{"", "", {}}});
        }
        cond_.notify_one();
    });

#if !defined(DEBUG)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [&] { return loaded_; });
    }
#endif // DEBUG

    {
        std::string descript = util::readDescript(ao_dir_ / "descript.txt");
        std::istringstream iss(descript);
        std::string value;
        while (std::getline(iss, value, '\n')) {
            auto pos = value.find(',');
            if (pos == std::string::npos) {
                continue;
            }
            auto key = value.substr(0, pos);
            value = value.substr(pos + 1);
            info_[key] = value;
            do {
                std::string tmp, group, name, category, part;
                int side = -1, id = -1;
                {
                    std::istringstream iss(key);
                    std::getline(iss, tmp, '.');
                    if (tmp == "sakura") {
                        side = 0;
                    }
                    else if (tmp == "kero") {
                        side = 1;
                    }
                    else if (tmp.starts_with("char")) {
                        util::to_x(tmp.substr(4), side);
                    }
                    else {
                        break;
                    }
                    std::getline(iss, tmp, '.');
                    if (!tmp.starts_with("bindgroup")) {
                        break;
                    }
                    util::to_x(tmp.substr(9), id);
                    std::getline(iss, tmp, '.');
                    if (tmp != "name") {
                        break;
                    }
                }
                {
                    std::istringstream iss(value);
                    std::getline(iss, category, ',');
                    if (category.empty()) {
                        break;
                    }
                    std::getline(iss, part, ',');
                    if (part.empty()) {
                        break;
                    }
                }
                key = category + "," + part;
                bind_id_[side][key] = id;
            } while (false);
        }
    }

#ifdef IS_WINDOWS
    std::wstring exe_path;
    exe_path.resize(1024);
    GetModuleFileName(nullptr, exe_path.data(), 1024);
#else
    std::string exe_path;
    exe_path.resize(1024);
    readlink("/proc/self/exe", exe_path.data(), 1024);
#endif // OS
    std::filesystem::path exe_dir = exe_path;
    exe_dir = exe_dir.parent_path();
    bool use_self_alpha = (getInfo("seriko.use_self_alpha", false) == "1");
    cache_ = std::make_unique<ImageCache>(exe_dir, use_self_alpha);

    th_send_ = std::make_unique<std::thread>([&]() {
        while (true) {
            std::vector<Request> list;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock, [this] { return !event_queue_.empty(); });
                if (!alive_) {
                    break;
                }
                list = event_queue_.front();
                event_queue_.pop();
            }
            for (auto &request : list) {
                auto res = sstp::Response::parse(sendDirectSSTP(request.method, request.command, request.args));
                if (res.getStatusCode() != 204) {
                    break;
                }
            }
        }
    });

#if !defined(DEBUG)
    surfaces_ = std::make_unique<Surfaces>(ao_dir_);
    surfaces_->dump();
#endif // DEBUG

    auto fontfamily = fontlist::get_default_font();
    font_ = std::make_unique<WrapFont>(fontfamily);

    return true;
}

std::string Ao::getInfo(std::string key, bool fallback) {
    if (info_.contains(key)) {
        return info_.at(key);
    }
    if (fallback) {
        auto res = sstp::Response::parse(Ao::sendDirectSSTP("EXECUTE", "GetDescript", {key}));
        std::string content = res.getContent();
        if (content.empty()) {
            Logger::log("info(", key, "): not found");
            return "";
        }
        return content;
    }
    return "";
}


void Ao::create(int side) {
    if (characters_.contains(side)) {
        return;
    }
    auto s = util::side2str(side);
    auto name = getInfo(s + ".name", true);
    characters_.emplace(side, std::make_unique<Character>(this, side, name.c_str(), surfaces_->getSeriko()));
    // TODO dynamic
    if (util::isWayland() && getenv("NINIX_ENABLE_MULTI_MONITOR")) {
        int count = 0;
        auto *monitors = SDL_GetDisplays(&count);
        for (int i = 0; i < count; i++) {
            characters_.at(side)->create(monitors[i]);
        }
        SDL_free(monitors);
    }
    else if (util::isWayland()) {
        SDL_DisplayID id = util::getCurrentDisplayID();
        characters_.at(side)->create(id);
    }
    else {
        characters_.at(side)->create(0);
    }
    return;
}

void Ao::show(int side) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->show();
}

void Ao::hide(int side) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->hide();
}

void Ao::setSurface(int side, int id) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->setSurface(id);
}

void Ao::startAnimation(int side, int id) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->startAnimation(id);
}

bool Ao::isPlayingAnimation(int side, int id) {
    if (!characters_.contains(side)) {
        return false;
    }
    return characters_.at(side)->isPlayingAnimation(id);
}

void Ao::bind(int side, int id, std::string from, BindFlag flag) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->bind(id, from, flag);
}

void Ao::clearCache() {
    cache_->clearCache();
    for (auto &[_, v] : characters_) {
        v->clearCache();
    }
}

void Ao::run() {
    SDL_Event event;
    while ((redrawn_) ? (SDL_PollEvent(&event)) : (SDL_WaitEventTimeout(&event, 10))) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                alive_ = false;
                return;
            case SDL_EVENT_DISPLAY_ADDED:
                if (util::isWayland() && getenv("NINIX_ENABLE_MULTI_MONITOR")) {
                    for (auto &[_, v] : characters_) {
                        v->create(event.display.displayID);
                    }
                    if (menu_) {
                        menu_->create(event.display.displayID);
                    }
                }
                break;
            case SDL_EVENT_DISPLAY_REMOVED:
                if (util::isWayland() && getenv("NINIX_ENABLE_MULTI_MONITOR")) {
                    for (auto &[_, v] : characters_) {
                        v->destroy(event.display.displayID);
                    }
                    if (menu_) {
                        menu_->destroy(event.display.displayID);
                    }
                }
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                for (auto &[_, v] : characters_) {
                    v->key(event.key);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                for (auto &[_, v] : characters_) {
                    v->motion(event.motion);
                }
                if (menu_) {
                    menu_->motion(event.motion);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                for (auto &[_, v] : characters_) {
                    v->button(event.button);
                }
                if (menu_) {
                    menu_->button(event.button);
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                for (auto &[_, v] : characters_) {
                    v->wheel(event.wheel);
                }
                if (menu_) {
                    menu_->wheel(event.wheel);
                }
                break;
            case SDL_EVENT_DROP_FILE:
                break;
            default:
                break;
        }
    }
    if (menu_ && !menu_->alive()) {
        menu_.reset();
    }
    std::queue<std::vector<std::string>> queue;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue.push(queue_.front());
            queue_.pop();
        }
    }
    bool changed = false;
    while (!queue.empty()) {
        std::vector<std::string> args = queue.front();
        queue.pop();
        if (args[0] == "Create") {
            int side;
            std::istringstream iss(args[1]);
            iss >> side;
            create(side);
        }
        else if (args[0] == "Show") {
            int side;
            std::istringstream iss(args[1]);
            iss >> side;
            show(side);
        }
        else if (args[0] == "SetSurfaceID") {
            int side, id;
            util::to_x(args[1], side);
            util::to_x(args[2], id);
            setSurface(side, id);
        }
        else if (args[0] == "ConfigurationChanged") {
            for (int i = 1; i < args.size(); i++) {
                auto value = args[i];
                auto pos = value.find(',');
                if (pos == std::string::npos) {
                    continue;
                }
                auto key = value.substr(0, pos);
                value = value.substr(pos + 1);
                if (key == "scale") {
                    int scale;
                    util::to_x(value, scale);
                    if (scale == scale_ || scale < 10) {
                        continue;
                    }
                    scale_ = scale;
                    clearCache();
                    cache_->setScale(scale);
                    changed = true;
                }
            }
        }
        else if (args[0] == "NotifyMenuInfo" && args.size() == 2) {
            Json::Reader reader;
            Json::Value value;
            reader.parse(args[1], value);
            auto data = parseMenuInfo(value);
            menu_ = std::make_unique<Menu>(this, menu_init_info_.side, menu_init_info_.x, menu_init_info_.y, font_, data);
        }
        else if (args[0] == "StartAnimation" && args.size() == 3) {
            int side, id;
            util::to_x(args[1], side);
            util::to_x(args[2], id);
            startAnimation(side, id);
        }
        else if (args[0] == "Bind" && args.size() == 6) {
            int side;
            BindFlag flag = BindFlag::Toggle;
            auto arg = args[5];
            if (arg == "true") {
                flag = BindFlag::True;
            }
            else if (arg == "false") {
                flag = BindFlag::False;
            }
            util::to_x(args[1], side);
            do {
                auto key = args[2] + "," + args[3];
                if (!bind_id_.contains(side)) {
                    break;
                }
                if (!bind_id_.at(side).contains(key)) {
                    break;
                }
                int id = bind_id_.at(side).at(key);
                bind(side, id, args[4], flag);
            } while (false);
        }
        else if (args[0] == "ScriptBegin") {
            raise();
        }
        else if (args[0] == "ScriptEnd") {
        }
    }
    std::vector<int> keys;
    for (auto &[k, _] : characters_) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    for (auto k : keys) {
        characters_.at(k)->draw(cache_, changed);
    }
    if (menu_) {
        menu_->draw();
    }
    redrawn_ = false;
    for (auto k : keys) {
        redrawn_ = characters_.at(k)->swapBuffers() || redrawn_;
    }
    if (menu_) {
        redrawn_ = menu_->swapBuffers() || redrawn_;
    }
}

void Ao::raise() {
    for (auto &[_, v] : characters_) {
        v->raise();
    }
}

namespace {
    std::vector<std::string> toList(Json::Value &value) {
        std::vector<std::string> list;
        for (int i = 0; !value[i].isNull(); i++) {
            list.push_back(value[i].asString());
        }
        return list;
    }
}

std::vector<MenuModelData> Ao::parseMenuInfo(Json::Value &value) {
    std::vector<MenuModelData> data;
    for (int i = 0; !value[i].isNull(); i++) {
        auto &v = value[i];
        auto type = v["type"].asString();
        if (type == "submenu") {
            MenuModelDataSubMenu submenu = {
                .action = ActionType::None,
                .caption = v["caption"].asString(),
                .children = parseMenuInfo(v["list"]),
            };
            data.push_back(submenu);
            assert(std::holds_alternative<MenuModelDataSubMenu>(data.back()));
        }
        if (type == "site") {
            MenuModelDataActionWithArgs args = {
                .action = ActionType::Site,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
                .args = toList(v["list"]),
            };
            data.push_back(args);
        }
        if (type == "check") {
            MenuModelDataActionWithBoolean check = {
                .action = ActionType::StayOnTop,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
                .state = v["state"].asBool(),
            };
            data.push_back(check);
        }
        if (type == "preferences") {
            MenuModelDataAction action = {
                .action = ActionType::Preferences,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
            };
            data.push_back(action);
        }
        if (type == "scriptinputbox") {
            MenuModelDataAction action = {
                .action = ActionType::ScriptInputBox,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
            };
            data.push_back(action);
        }
        if (type == "switch") {
            MenuModelDataActionWithArgs action = {
                .action = ActionType::Switch,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
                .args = toList(v["list"])
            };
            data.push_back(action);
        }
        if (type == "call") {
            MenuModelDataActionWithArgs action = {
                .action = ActionType::Call,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
                .args = toList(v["list"])
            };
            data.push_back(action);
        }
        if (type == "shell") {
            MenuModelDataActionWithArgs action = {
                .action = ActionType::Call,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
                .args = toList(v["list"])
            };
            data.push_back(action);
        }
        if (type == "dressup") {
            MenuModelDataSubMenu dressup = {
                .caption = v["caption"].asString(),
                .children = getDressUpList(),
            };
            data.push_back(dressup);
        }
        if (type == "balloon") {
            MenuModelDataActionWithArgs action = {
                .action = ActionType::Balloon,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
                .args = toList(v["list"])
            };
            data.push_back(action);
        }
        if (type == "basewareversion") {
            MenuModelDataAction action = {
                .action = ActionType::BasewareVersion,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
            };
            data.push_back(action);
        }
        if (type == "close") {
            MenuModelDataAction action = {
                .action = ActionType::Close,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
            };
            data.push_back(action);
        }
        if (type == "close_all") {
            MenuModelDataAction action = {
                .action = ActionType::CloseAll,
                .valid = v["valid"].asBool(),
                .caption = v["caption"].asString(),
            };
            data.push_back(action);
        }
    }
    return data;
}

std::vector<MenuModelData> Ao::getDressUpList() {
    std::vector<MenuModelData> data;
    if (!menu_) {
        return data;
    }
    int side = menu_->side();
    for (auto &[k, _] : bind_id_[side]) {
        // FIXME clickable
        MenuModelDataActionWithBoolean action = {
            .action = ActionType::DressUp,
            .valid = false,
            .caption = k,
            .state = false,
        };
        data.push_back(action);
    }
    return data;
}

std::optional<Offset> Ao::getCharacterOffset(int side) {
    int s = side;
    std::optional<Offset> ret = std::nullopt;
    for (; s >= 0; s--) {
        if (characters_.contains(s)) {
            break;
        }
    }
    if (s == -1) {
        ret = {0, 0};
    }
    else if (characters_.at(s)->isAdjusted()) {
        ret = characters_.at(s)->getOffset();
    }
    return ret;
}

std::string Ao::sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args) {
    sstp::Request req {method};
    sstp::Response res {500, "Internal Server Error"};
    req["Charset"] = "UTF-8";
    req["Ao"] = uuid_;
    if (path_.empty()) {
        return res;
    }
    req["Sender"] = "AYU_PoC";
    req["Option"] = "nodescript";
    if (req.getCommand() == "EXECUTE") {
        req["Command"] = command;
    }
    else if (req.getCommand() == "NOTIFY") {
        req["Event"] = command;
    }
    for (int i = 0; i < args.size(); i++) {
        req(i) = args[i];
    }
    sockaddr_un addr;
    if (path_.length() >= sizeof(addr.sun_path)) {
        return res;
    }
    int soc = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc == -1) {
        return res;
    }
    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    // null-terminatedも書き込ませる
    strncpy(addr.sun_path, path_.c_str(), path_.length() + 1);
    if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
        return res;
    }
    std::string request = req;
    if (send(soc, request.c_str(), request.size(), 0) != request.size()) {
        closesocket(soc);
        return res;
    }
    char buffer[BUFFER_SIZE] = {};
    std::string data;
    while (true) {
        int ret = recv(soc, buffer, BUFFER_SIZE, 0);
        if (ret == -1) {
            closesocket(soc);
            return res;
        }
        if (ret == 0) {
            closesocket(soc);
            break;
        }
        data.append(buffer, ret);
    }
    return data;
}

void Ao::enqueueDirectSSTP(std::vector<Request> list) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        event_queue_.push(list);
    }
    cond_.notify_one();
}

void Ao::reserveMenuParent(int side, int x, int y) {
    menu_init_info_ = {side, x, y};
}
