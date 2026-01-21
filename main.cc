#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_main.h>

#include "ao.h"
#include "logger.h"

int main(int argc, char **argv) {
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "io.github.tatakinov.ninix-kagari.ao.ao_builtin");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }
    atexit(SDL_Quit);
    if (!TTF_Init()) {
        return 1;
    }
    atexit(TTF_Quit);

    Ao ao;

#if defined(DEBUG)
    ao.create(0);
    ao.show(0);
    ao.setSurface(0, 0);
#endif // DEBUG

    while (ao) {
        ao.run();
    }

	return 0;
}
