#pragma once
#include <cstdlib>
#include <string>
#include <cstring>

inline int get_instance_from_args_env(int argc, char *argv[])
{
    int instance = -1;

    // check command line args
    for (int i = 1; i < argc; ++i)
    {
        if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--instance") == 0) && i + 1 < argc)
        {
            instance = std::atoi(argv[i + 1]);
            break;
        }
        // Accept the first positional integer argument (not an option)
        if (instance < 0 && argv[i][0] != '-' && std::atoi(argv[i]) >= 0)
        {
            instance = std::atoi(argv[i]);
            break;
        }
    }

    // if not set, check env
    if (instance < 0)
    {
        const char *env = std::getenv("STATEFULSET_INDEX");
        if (env)
            instance = std::atoi(env);
    }

    return instance;
}
