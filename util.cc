#include "util.h"

#include <cassert>
#include <cstdlib>
#include <random>

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
        return (getenv("XDG_SESSION_TYPE") != nullptr && wayland == getenv("XDG_SESSION_TYPE"));
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
}
