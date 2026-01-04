#include <iostream>
#include <unistd.h>
#include <fstream>
#include "catch.hpp"
#include "logger.h"
#include "test_utils.h"
#include "../src/config.h"

TEST_CASE("Config parsing: minimal valid config", "[config]")
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)))
    {
        Logger::debug(std::string("(test_config) CWD: ") + cwd);
    }

    std::string config_path = get_test_config_path();

    AppConfig cfg = load_config(config_path.c_str(), 0);
    REQUIRE(cfg.udp_local_port > 0);
    REQUIRE(cfg.udp_remote_ip == "127.0.0.1");
    REQUIRE(cfg.udp_remote_port > 0);
    REQUIRE(cfg.uds_servers.size() > 0);
    REQUIRE(cfg.uds_clients.size() > 0);
    REQUIRE(cfg.ul_uds_mapping.size() > 0);
    REQUIRE(cfg.ctrl_uds_name.size() > 0);
}
