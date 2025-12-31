

#include <iostream>
#include "app.h"
#include "instance_utils.h"

int main(int argc, char *argv[])
{
    try
    {
        int instance = get_instance_from_args_env(argc, argv);
        AppConfig config = load_config("config.xml", instance);
        App app(config);
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "App error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
