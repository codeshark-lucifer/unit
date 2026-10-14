#include <engine/engine.h>
#include <iostream>

int main()
{
    try
    {
        engine_init(
            "unit - engine",
            Vec2(956, 540),
            false, true);
            
        while (engine_poll())
        {
            engine_draw_frame();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    engine_clean();

    return EXIT_SUCCESS;
}