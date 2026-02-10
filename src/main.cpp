#include "Application.h"

int main()
{
    static auto app = std::make_unique<Application>();

    if (!app->onInit())
        return 1;

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(
        [](void *userData)
        {
            reinterpret_cast<Application *>(userData)->onFrame();
        },
        app.get(),
        0, false);
#else
    while (app->isRunning())
    {
        app->onFrame();
    }
#endif

    // app.onFinish();

    return 0;
}