#pragma once

#ifdef _WIN32
    #if defined(ENGINE_BUILD) || defined(engine_EXPORTS)
        #define ENGINE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
    #endif
#else
    #define ENGINE_API
#endif