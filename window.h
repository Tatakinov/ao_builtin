#ifndef WINDOW_H_
#define WINDOW_H_

#include "misc.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#if defined(IS__NIX)
#include <wayland-client.h>
#endif // Linux/Unix

#include "element.h"
#include "image_cache.h"
#include "logger.h"
#include "misc.h"
#include "util.h"

class Character;

class Window {
    struct State {
        bool press;
        bool drag;
        std::chrono::system_clock::time_point prev;
    };
    private:
        std::mutex mutex_;
        SDL_Window *window_;
        SDL_DisplayID id_;
        Position<int> size_;
        Position<int> position_;
        bool focused_;
        std::unordered_map<Uint8, State> mouse_state_;
        Position<double> cursor_position_;
        Character *parent_;
        Rect monitor_rect_;
        bool adjust_;
        int counter_;
        Offset offset_;
        std::optional<std::vector<int>> shape_;
        SDL_Renderer *renderer_;
        std::unique_ptr<TextureCache> texture_cache_;
        ElementWithChildren current_element_;
        std::unique_ptr<WrapTexture> current_texture_;
        bool redrawn_;
        bool changed_;
#if defined(IS__NIX)
        wl_registry *reg_;
        wl_compositor *compositor_;
#endif // Linux/Unix

    public:
        Window(Character *parent, SDL_DisplayID id);
        virtual ~Window();

#if defined(IS__NIX)
        void setCompositor(wl_compositor *compositor) {
            compositor_ = compositor;
        }

        wl_compositor *compositor() {
            return compositor_;
        }
#endif // Linux/Unix

        void resize(int width, int height);
        void position(int x, int y);
        void focus(int focused);

        void draw(std::unique_ptr<ImageCache> &image_cache, Offset offset, std::unique_ptr<WrapSurface> &surface, const ElementWithChildren &element, const bool changed);
        bool swapBuffers();

        void setPosition(int x, int y) {
            monitor_rect_.x = x;
            monitor_rect_.y = y;
        }

        void show(bool force) {
            if (force || (SDL_GetWindowFlags(window_) & SDL_WINDOW_HIDDEN)) {
                SDL_ShowWindow(window_);
            }
        }

        void hide() {
            SDL_HideWindow(window_);
        }

        Position<int> getPosition() {
            std::unique_lock<std::mutex> lock(mutex_);
            Position<int> p = {position_.x, position_.y};
            return p;
        }

        Rect getMonitorRect() const;

        void requestAdjust() {
            adjust_ = true;
        }

        bool isAdjusted() const {
            return !adjust_;
        }

        double distance(int x, int y) const;

        void clearCache();

        void raise();
        void key(const SDL_KeyboardEvent &event);
        void motion(const SDL_MouseMotionEvent &event);
        void button(const SDL_MouseButtonEvent &event);
        void wheel(const SDL_MouseWheelEvent &event);
        void maximized(const SDL_WindowEvent &event);
        int scale() const;
};

#endif // WINDOW_H_
