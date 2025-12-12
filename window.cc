#include "window.h"

#include <cassert>
#include <cmath>
#include <string>
#include <unordered_map>

#include "character.h"
#include "logger.h"
#include "misc.h"
#include "sstp.h"

#define MOUSE_BUTTON_LEFT 1
#define MOUSE_BUTTON_MIDDLE 2
#define MOUSE_BUTTON_RIGHT 3

namespace {
    std::unordered_map<SDL_Keycode, int> key_count;
    std::unordered_map<SDL_Keycode, std::string> key2s = {
        { SDLK_0, "0" },
        { SDLK_1, "1" },
        { SDLK_2, "2" },
        { SDLK_3, "3" },
        { SDLK_4, "4" },
        { SDLK_5, "5" },
        { SDLK_6, "6" },
        { SDLK_7, "7" },
        { SDLK_8, "8" },
        { SDLK_9, "9" },
        { SDLK_A, "a" },
        { SDLK_B, "b" },
        { SDLK_C, "c" },
        { SDLK_D, "d" },
        { SDLK_E, "e" },
        { SDLK_F, "f" },
        { SDLK_G, "g" },
        { SDLK_H, "h" },
        { SDLK_I, "i" },
        { SDLK_J, "j" },
        { SDLK_K, "k" },
        { SDLK_L, "l" },
        { SDLK_M, "m" },
        { SDLK_N, "n" },
        { SDLK_O, "o" },
        { SDLK_P, "p" },
        { SDLK_Q, "q" },
        { SDLK_R, "r" },
        { SDLK_S, "s" },
        { SDLK_T, "t" },
        { SDLK_U, "u" },
        { SDLK_V, "v" },
        { SDLK_W, "w" },
        { SDLK_X, "x" },
        { SDLK_Y, "y" },
        { SDLK_Z, "z" },
    };
}

Window::Window(Character *parent, SDL_DisplayID id)
    : window_(nullptr), size_({0, 0}),
    position_({0, 0}), parent_(parent),
    adjust_(false),
    counter_(0), offset_({0, 0}), renderer_(nullptr), redrawn_(false) {
    if (util::isWayland()) {
        SDL_Rect r;
        SDL_GetDisplayBounds(id, &r);
        monitor_rect_ = { r.x, r.y, r.w, r.h };
    }
    else {
        monitor_rect_ = { 0, 0, 1, 1 };
    }
    if (util::isWayland()) {
        SDL_PropertiesID p = SDL_CreateProperties();
        assert(p);
        SDL_SetStringProperty(p, SDL_PROP_WINDOW_CREATE_TITLE_STRING, parent_->name().c_str());
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN, true);
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_UNDEFINED_DISPLAY(id));
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_UNDEFINED_DISPLAY(id));
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, monitor_rect_.width);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, monitor_rect_.height);
        window_ = SDL_CreateWindowWithProperties(p);
    }
#if 0
    else if (util::isWayland()) {
        int w = 0, h = 0;
        if (w == 0 || h == 0) {
            w = h = 200;
        }
        window_ = SDL_CreateWindow(parent_->name().c_str(), w, h, SDL_WINDOW_TRANSPARENT);
        //SDL_MaximizeWindow(window_);
        SDL_SyncWindow(window_);
        SDL_GetWindowSize(window_, &w, &h);
        monitor_rect_ = { 0, 0, w, h };
    }
#endif
    else {
        window_ = SDL_CreateWindow(parent_->name().c_str(), 200, 200, SDL_WINDOW_TRANSPARENT | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS);
    }
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    SDL_SetRenderVSync(renderer_, 1);
    texture_cache_ = std::make_unique<TextureCache>();
}

Window::~Window() {
    if (window_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
    }
}

void Window::resize(int width, int height) {
    std::unique_lock<std::mutex> lock(mutex_);
    size_ = {width, height};
}

void Window::focus(int focused) {
    //focused_ = (focused == GLFW_TRUE);
}

void Window::position(int x, int y) {
    if (!util::isWayland()) {
        SDL_SetWindowPosition(window_, x, y);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        position_ = {x, y};
    }
}

bool Window::draw(std::unique_ptr<ImageCache> &image_cache, Offset offset, std::unique_ptr<WrapSurface> &surface, const ElementWithChildren &element, const bool use_self_alpha) {
    auto m = getMonitorRect();
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer_);
    if (current_element_ == element && offset_ == offset) {
        redrawn_ = false;
        return redrawn_;
    }
    current_texture_ = element.getTexture(renderer_, texture_cache_, image_cache);
    if (current_texture_) {
        while (adjust_) {
            int side = parent_->side();
            int origin_x = m.x + m.width;
            if (side > 0) {
                auto o = parent_->getCharacterOffset(side - 1);
                if (!o) {
                    break;
                }
                if (o->x < origin_x) {
                    origin_x = o->x;
                }
            }
            origin_x -= current_texture_->width();
            if (origin_x < m.x) {
                origin_x = m.x;
            }
            int origin_y = m.y + m.height;
            origin_y -= current_texture_->height();
            parent_->setOffset(origin_x, origin_y);
            offset = {origin_x, origin_y};

            adjust_ = false;
        }
        if (!util::isWayland()) {
            SDL_SetWindowSize(window_, current_texture_->width(), current_texture_->height());
        }
        parent_->setSize(current_texture_->width(), current_texture_->height());
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
        SDL_SetTextureBlendMode(current_texture_->texture(), mode);
        SDL_FRect r = { offset.x - m.x, offset.y - m.y, current_texture_->width(), current_texture_->height() };
        SDL_RenderTexture(renderer_, current_texture_->texture(), nullptr, &r);
    }
    if (surface) {
        std::vector<int> shape;
        {
            SDL_LockSurface(surface->surface());
            for (int i = 0; i < surface->width() * surface->height(); i++) {
                unsigned char *p = static_cast<unsigned char *>(surface->surface()->pixels);
                if (p[4 * i + 3]) {
                    shape.push_back(i);
                }
            }
            SDL_UnlockSurface(surface->surface());
        }
        if (!shape_ || shape_ != shape || offset_ != offset) {
            auto s = std::make_unique<WrapSurface>(m.width, m.height);
            SDL_ClearSurface(s->surface(), 0, 0, 0, 0);
            SDL_Rect r = { offset.x - m.x, offset.y - m.y, surface->width(), surface->height() };
            SDL_BlitSurface(surface->surface(), nullptr, s->surface(), &r);
            SDL_SetWindowShape(window_, s->surface());
            shape_ = shape;
        }
    }
    else if (!shape_ || shape_->size() > 0) {
        shape_ = std::make_optional<std::vector<int>>();
        auto s = std::make_unique<WrapSurface>(m.width, m.height);
        SDL_ClearSurface(s->surface(), 0, 0, 0, 0);
        SDL_SetWindowShape(window_, s->surface());
    }
    current_element_ = element;
    offset_ = offset;
    redrawn_ = true;
    return redrawn_;
}

void Window::swapBuffers() {
    if (redrawn_) {
        SDL_SetRenderTarget(renderer_, nullptr);
        SDL_RenderPresent(renderer_);
    }
}

double Window::distance(int x, int y) const {
    if (monitor_rect_.x <= x && monitor_rect_.x + monitor_rect_.width >= x &&
            monitor_rect_.y <= y && monitor_rect_.y + monitor_rect_.height >= y) {
        return 0;
    }
    if (monitor_rect_.x <= x && monitor_rect_.x + monitor_rect_.width >= x) {
        return std::min(std::abs(monitor_rect_.x - x), std::abs(monitor_rect_.x + monitor_rect_.width - x));
    }
    if (monitor_rect_.y <= y && monitor_rect_.y + monitor_rect_.height >= y) {
        return std::min(std::abs(monitor_rect_.y - y), std::abs(monitor_rect_.y + monitor_rect_.height - y));
    }
    double d;
    {
        int dx = monitor_rect_.x - x;
        int dy = monitor_rect_.y - y;
        d = sqrt(dx * dx + dy * dy);
    }
    {
        int dx = monitor_rect_.x + monitor_rect_.width - x;
        int dy = monitor_rect_.y - y;
        d = std::min(d, sqrt(dx * dx + dy * dy));
    }
    {
        int dx = monitor_rect_.x + monitor_rect_.width - x;
        int dy = monitor_rect_.y + monitor_rect_.height - y;
        d = std::min(d, sqrt(dx * dx + dy * dy));
    }
    {
        int dx = monitor_rect_.x - x;
        int dy = monitor_rect_.y + monitor_rect_.height - y;
        d = std::min(d, sqrt(dx * dx + dy * dy));
    }
    return d;
}

void Window::clearCache() {
    texture_cache_->clear();
}

void Window::key(const SDL_KeyboardEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    // TODO stub
    std::vector<std::string> args = { key2s[event.key], util::to_s(event.key) };
    Request req = {"NOTIFY", "OnKeyPress", args};
    parent_->enqueueDirectSSTP({req});
}

void Window::motion(const SDL_MouseMotionEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    if (!parent_->drag().has_value()) {
        int xi = event.x, yi = event.y;
        if (util::isWayland()) {
            auto r = getMonitorRect();
            xi = xi + r.x;
            yi = yi + r.y;
        }
        auto name = parent_->getHitBoxName(xi, yi);
        if (name.empty()) {
            SDL_Cursor *cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
            SDL_SetCursor(cursor);
        }
        else {
            SDL_Cursor *cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
            SDL_SetCursor(cursor);
        }
    }
    if (!parent_->drag().has_value() && mouse_state_[MOUSE_BUTTON_LEFT].press) {
        if (util::isWayland()) {
            parent_->setDrag(cursor_position_.x + monitor_rect_.x, cursor_position_.y + monitor_rect_.y);
        }
        else {
            parent_->setDrag(cursor_position_.x, cursor_position_.y);
        }
    }
    cursor_position_ = {event.x, event.y};
    if (parent_->drag().has_value()) {
        auto [dx, dy, px, py] = parent_->drag().value();
        auto x = event.x;
        auto y = event.y;
        if (util::isWayland()) {
            x = x + monitor_rect_.x;
            y = y + monitor_rect_.y;
        }
        parent_->setOffset(px + x - dx, py + y - dy);
    }
    for (auto &[k, v] : mouse_state_) {
        if (v.press) {
            v.drag = true;
        }
    }
}

void Window::button(const SDL_MouseButtonEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    mouse_state_[event.button].press = event.down;
    if (event.button == MOUSE_BUTTON_LEFT && !mouse_state_[event.button].press) {
        parent_->resetDrag();
    }
    if (!mouse_state_[event.button].press && !mouse_state_[event.button].drag) {
        int x = cursor_position_.x;
        int y = cursor_position_.y;
        int b = -1;
        switch (event.button) {
            case 1:
                b = 0;
                break;
            case 2:
                b = 2;
                break;
            case 3:
                b = 1;
                break;
            default:
                break;
        }

        if (util::isWayland()) {
            auto r = getMonitorRect();
            x = x + r.x;
            y = y + r.y;
        }
        auto name = parent_->getHitBoxName(x, y);

        std::vector<std::string> args;
        Offset offset = parent_->getOffset();
        x = x - offset.x;
        y = y - offset.y;
        args = {util::to_s(x), util::to_s(y), util::to_s(0), util::to_s(parent_->side()), name, util::to_s(b)};

        if (event.clicks % 2 == 0) {
            Request req = {"NOTIFY", "OnMouseDoubleClick", args};
            parent_->enqueueDirectSSTP({req});
        }
        else if (event.button != MOUSE_BUTTON_RIGHT) {
            Request up = {"NOTIFY", "OnMouseUp", args};
            Request click = {"NOTIFY", "OnMouseClick", args};
            parent_->enqueueDirectSSTP({up, click});
            parent_->reserveMenuParent(window_);
        }
        else {
            Request up = {"NOTIFY", "OnMouseUp", args};
            Request click = {"NOTIFY", "OnMouseClick", args};
#if 0
            // 右クリックメニューを呼び出す
            args = {util::to_s(parent_->side()), util::to_s(x), util::to_s(y)};
            Request menu = {"EXECUTE", "OpenMenu", args};
            parent_->enqueueDirectSSTP({up, click, menu});
#else
            parent_->enqueueDirectSSTP({up, click});
#endif
        }
    }
    else if (event.down) {
        int x = cursor_position_.x;
        int y = cursor_position_.y;
        int b = -1;
        switch (event.button) {
            case 1:
                b = 0;
                break;
            case 2:
                b = 2;
                break;
            case 3:
                b = 1;
                break;
            default:
                break;
        }

        if (util::isWayland()) {
            auto r = getMonitorRect();
            x = x + r.x;
            y = y + r.y;
        }
        auto name = parent_->getHitBoxName(x, y);

        std::vector<std::string> args;
        Offset offset = parent_->getOffset();
        x = x - offset.x;
        y = y - offset.y;
        args = {util::to_s(x), util::to_s(y), util::to_s(0), util::to_s(parent_->side()), name, util::to_s(b)};
        Request req = {"NOTIFY", "OnMouseDown", args};
        parent_->enqueueDirectSSTP({req});
    }
    for (auto &[k, v] : mouse_state_) {
        if (!v.press) {
            v.drag = false;
        }
    }
}

void Window::wheel(const SDL_MouseWheelEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    // TODO stub
}
