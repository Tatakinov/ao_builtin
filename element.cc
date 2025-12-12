#include "element.h"

#include <cassert>

#include "logger.h"

namespace {
    const int kInf = 1000000;
}

std::unique_ptr<WrapTexture> ElementWithChildren::getTexture(SDL_Renderer *renderer, std::unique_ptr<TextureCache> &texture_cache, std::unique_ptr<ImageCache> &image_cache, int scale) const {
    int w = 0, h = 0;
    bool upconverted = true;
    std::vector<std::optional<std::unique_ptr<WrapTexture>>> list;
    for (auto &element : children) {
        std::visit([&](const auto &e) {
            auto t = e.getTexture(renderer, texture_cache, image_cache, scale);
            if (!t) {
                list.push_back(std::nullopt);
                return;
            }
            upconverted = upconverted && t->isUpconverted();
            if (w < (e.x * scale) / 100 + t->width()) {
                w = (e.x * scale) / 100 + t->width();
            }
            if (h < (e.y * scale) / 100 + t->height()) {
                h = (e.y * scale) / 100 + t->height();
            }
            list.push_back(std::move(t));
        }, element);
    }
    if (w == 0 || h == 0) {
        std::unique_ptr<WrapTexture> invalid;
        Logger::log("no valid children");
        return invalid;
    }
    auto texture = std::make_unique<WrapTexture>(renderer, w, h, upconverted);
    SDL_SetRenderTarget(renderer, texture->texture());
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    for (int i = 0; i < children.size(); i++) {
        if (!list[i]) {
            continue;
        }
        SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
        std::visit([&](const auto &e) {
            switch (e.method) {
                case Method::Base:
                case Method::Add:
                case Method::Overlay:
                    break;
                case Method::OverlayFast:
                    mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case Method::OverlayMultiply:
                    // FIXME
                    mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case Method::Replace:
                    mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case Method::Interpolate:
                    mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                case Method::Reduce:
                    // FIXME
                    mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
                    break;
                default:
                    break;
            }
            auto &t = list[i].value();
            SDL_SetTextureBlendMode(t->texture(), mode);
            SDL_FRect r = { (e.x * scale / 100), (e.y * scale / 100), t->width(), t->height() };
            SDL_RenderTexture(renderer, t->texture(), nullptr, &r);
        }, children[i]);
    }
    SDL_SetRenderTarget(renderer, nullptr);
    return texture;
}

std::unique_ptr<WrapSurface> ElementWithChildren::getSurface(std::unique_ptr<ImageCache> &cache, int scale) const {
    int w = 0, h = 0;
    std::vector<std::optional<std::unique_ptr<WrapSurface>>> list;
    for (auto &element : children) {
        std::visit([&](const auto &e) {
            auto t = e.getSurface(cache, scale);
            if (!t) {
                list.push_back(std::nullopt);
                return;
            }
            if (w < (e.x * scale) / 100 + t->width()) {
                w = (e.x * scale) / 100 + t->width();
            }
            if (h < (e.y * scale) / 100 + t->height()) {
                h = (e.y * scale) / 100 + t->height();
            }
            list.push_back(std::move(t));
        }, element);
    }
    if (w == 0 || h == 0) {
        std::unique_ptr<WrapSurface> invalid;
        Logger::log("no valid children");
        return invalid;
    }
    auto surface = std::make_unique<WrapSurface>(w, h);
    SDL_ClearSurface(surface->surface(), 0, 0, 0, 0);
    for (int i = 0; i < list.size(); i++) {
        if (!list[i]) {
            continue;
        }
        std::visit([&](const auto &e) {
            auto &t = list[i].value();
            SDL_SetSurfaceBlendMode(t->surface(), SDL_BLENDMODE_BLEND);
            SDL_Rect r = { (e.x * scale) / 100, (e.y * scale) / 100, t->width(), t->height() };
            SDL_BlitSurface(t->surface(), nullptr, surface->surface(), &r);
        }, children[i]);
    }
    return surface;
}
