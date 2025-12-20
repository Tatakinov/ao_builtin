#include "surface.h"

#include "image_cache.h"
#include "logger.h"

std::unique_ptr<WrapTexture> Element::getTexture(SDL_Renderer *renderer, std::unique_ptr<TextureCache> &texture_cache, std::unique_ptr<ImageCache> &image_cache, int scale) const {
    auto &src = texture_cache->get(filename, renderer, image_cache);
    auto dst = std::make_unique<WrapTexture>(renderer, src->width(), src->height(), src->isUpconverted());
    SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
    switch (method) {
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
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
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
    SDL_SetRenderTarget(renderer, dst->texture());
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    SDL_SetTextureBlendMode(src->texture(), mode);
    SDL_FRect r = { 0, 0, src->width(), src->height() };
    SDL_RenderTexture(renderer, src->texture(), nullptr, &r);
    SDL_SetRenderTarget(renderer, nullptr);
    return dst;
}

std::unique_ptr<WrapSurface> Element::getSurface(std::unique_ptr<ImageCache> &cache, int scale) const {
    auto &info = cache->get(filename);
    if (!info) {
        Logger::log("invalid info");
        std::unique_ptr<WrapSurface> invalid;
        return invalid;
    }
    WrapSurface src(info.value());
    auto dst = std::make_unique<WrapSurface>((x * scale) / 100 + src.width(), (y * scale) / 100 + src.height());
    SDL_ClearSurface(dst->surface(), 0, 0, 0, 0);
    SDL_SetSurfaceBlendMode(src.surface(), SDL_BLENDMODE_BLEND);
    SDL_Rect r = { (x * scale) / 100, (y * scale) / 100, src.width(), src.height() };
    SDL_BlitSurface(src.surface(), nullptr, dst->surface(), &r);
    return dst;
}
