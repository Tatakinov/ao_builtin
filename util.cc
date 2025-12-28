#include "util.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <random>

#include "logger.h"

namespace {
    std::random_device rd;
    std::mt19937 mt(rd());
}

namespace util {
    double random() {
        std::uniform_real_distribution<> dist(0, 1);
        return dist(mt);
    }

    int random(int a, int b) {
        std::uniform_int_distribution<> dist(a, b);
        return dist(mt);
    }

    std::string side2str(int side) {
        if (side == 0) {
            return "sakura";
        }
        else if (side == 1) {
            return "kero";
        }
        else if (side >= 2) {
            std::ostringstream oss;
            oss << "char" << side;
            return oss.str();
        }
        // unreachable
        assert(false);
    }

    bool isWayland() {
        std::string wayland = "wayland";
        return (SDL_GetCurrentVideoDriver() != nullptr && wayland == SDL_GetCurrentVideoDriver());
    }

    SDL_DisplayID getNearestDisplay(int x, int y) {
        SDL_DisplayID id = 0;
        int count = 0;
        double distance = -1;
        auto *displays = SDL_GetDisplays(&count);
        for (int i = 0; i < count; i++) {
            SDL_Rect r;
            SDL_GetDisplayBounds(displays[i], &r);
            double d;
            if (r.x <= x && r.x + r.w >= x &&
                    r.y <= y && r.y + r.h >= y) {
                d = 0;
            }
            else if (r.x <= x && r.x + r.w >= x) {
                d = std::min(std::abs(r.x - x), std::abs(r.x + r.w - x));
            }
            else if (r.y <= y && r.y + r.h >= y) {
                d = std::min(std::abs(r.y - y), std::abs(r.y + r.h - y));
            }
            else {
                {
                    int dx = r.x - x;
                    int dy = r.y - y;
                    d = sqrt(dx * dx + dy * dy);
                }
                {
                    int dx = r.x + r.w - x;
                    int dy = r.y - y;
                    d = std::min(d, sqrt(dx * dx + dy * dy));
                }
                {
                    int dx = r.x + r.w - x;
                    int dy = r.y + r.h - y;
                    d = std::min(d, sqrt(dx * dx + dy * dy));
                }
                {
                    int dx = r.x - x;
                    int dy = r.y + r.h - y;
                    d = std::min(d, sqrt(dx * dx + dy * dy));
                }
            }
            if (distance == -1 || distance > d) {
                distance = d;
                id = displays[i];
            }
        }
        SDL_free(displays);
        return id;
    }

    std::string readDescript(std::filesystem::path path) {
        std::ifstream ifs(path, std::ios_base::binary);
        if (!ifs) {
            return "";
        }
        std::ostringstream oss;
        std::string buffer;
        std::string charset = "Shift_JIS";
        while (std::getline(ifs, buffer, '\n')) {
            if (buffer.ends_with('\r')) {
                oss << buffer.substr(0, buffer.length() - 1) << '\n';
            }
            else {
                oss << buffer << '\n';
            }
            if (buffer.starts_with("charset,")) {
                int begin = 8;
                int end = buffer.length() - 1;
                for (; begin < buffer.length(); begin++) {
                    if (buffer[begin] == ' ') {
                        continue;
                    }
                    break;
                }
                for (; end > 0; end--) {
                    if (buffer[begin] == ' ') {
                        continue;
                    }
                    break;
                }
                charset = buffer.substr(begin, end - begin);
                Logger::log("charset in ", path, ": ", charset);
            }
        }
        buffer = oss.str();
        if (charset == "UTF-8" || buffer.empty()) {
            return buffer;
        }
        std::string converted;
        converted.resize(buffer.length());
        do {
            converted.resize(converted.length() * 2);
            const char *in = buffer.data();
            size_t in_length = buffer.length();
            char *out = converted.data();
            size_t out_length = converted.length();
            SDL_iconv_t cd = SDL_iconv_open("UTF-8", charset.c_str());
            if (reinterpret_cast<size_t>(cd) == SDL_ICONV_ERROR) {
                Logger::log("iconv_open error");
                return buffer;
            }
            auto err = SDL_iconv(cd, &in, &in_length, &out, &out_length);
            SDL_iconv_close(cd);
            if (err == -2) {
                continue;
            }
            else if (err < 0) {
                Logger::log("iconv error");
                return buffer;
            }
            else {
                converted.resize(out_length);
                break;
            }
        } while (true);
        return converted;
    }

    SDL_DisplayID getCurrentDisplayID() {
        // TODO stub
        return 0;
    }
}
