#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../src/config.h"
#include <fstream>

TEST_CASE("Config parsing: minimal valid config", "[config]")
{
    const char *xml = R"(<config>
    <udp>
        <local_port>1234</local_port>
        <remote_ip>127.0.0.1</remote_ip>
        <remote_port>5678</remote_port>
    </udp>
    <data_link_uds>
        <server>
            <path>/tmp/test1.sock</path>
            <receive_buffer_size>1024</receive_buffer_size>
            </server>
        <client name="test.ul">/tmp/test2.sock</client>
    </data_link_uds>
    <ul_uds_mapping>
        <mapping opcode="1" uds="test.ul" />
    </ul_uds_mapping>
    <ctrl_status_uds>
        <appx>
            <request>
                <path>/tmp/appx_to_fsl.requests.sock</path>
                <receive_buffer_size>2048</receive_buffer_size>
            </request>
            <response>
                <path>/tmp/fsl_to_appx.responses.sock</path>
                <receive_buffer_size>4096</receive_buffer_size>
            </response>
        </appx>
    </ctrl_status_uds>
</config>)";
    std::ofstream f("test_config.xml");
    f << xml;
    f.close();

    AppConfig cfg = load_config("test_config.xml");
    REQUIRE(cfg.udp_local_port == 1234);
    REQUIRE(cfg.udp_remote_ip == "127.0.0.1");
    REQUIRE(cfg.udp_remote_port == 5678);
    REQUIRE(cfg.uds_servers.size() == 1);
    REQUIRE(cfg.uds_servers[0].path == "/tmp/test1.sock");
    REQUIRE(cfg.uds_clients.size() == 1);
    REQUIRE(cfg.uds_clients.at("test.ul") == "/tmp/test2.sock");
    REQUIRE(cfg.ul_uds_mapping.size() == 1);
    REQUIRE(cfg.ul_uds_mapping.at(1) == "test.ul");
    REQUIRE(cfg.ctrl_uds.size() == 1);
    REQUIRE(cfg.ctrl_uds.count("appx") == 1);
    REQUIRE(cfg.ctrl_uds.at("appx").request_path == "/tmp/appx_to_fsl.requests.sock");
    REQUIRE(cfg.ctrl_uds.at("appx").request_buffer_size == 2048);
    REQUIRE(cfg.ctrl_uds.at("appx").response_path == "/tmp/fsl_to_appx.responses.sock");
    REQUIRE(cfg.ctrl_uds.at("appx").response_buffer_size == 4096);
    remove("test_config.xml");
}
