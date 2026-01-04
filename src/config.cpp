#include "config.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include "tinyxml2.h"
#include "instance_utils.h"

using namespace tinyxml2;

// override_config_from_env: Override config fields from environment variables if set
// Priority: environment > config.xml
// Supported env vars: FSL_SENSOR_ID, FSL_LOCAL_PORT, FSL_REMOTE_IP, FSL_REMOTE_PORT, LOGGING_LEVEL
void override_config_from_env(AppConfig &config)
{
    if (const char *env = std::getenv("FSL_SENSOR_ID"))
    {
        config.sensor_id = std::atoi(env);
    }

    if (const char *env = std::getenv("FSL_LOCAL_PORT"))
    {
        config.udp_local_port = std::atoi(env);
    }

    if (const char *env = std::getenv("FSL_REMOTE_IP"))
    {
        config.udp_remote_ip = env;
    }

    if (const char *env = std::getenv("FSL_REMOTE_PORT"))
    {
        config.udp_remote_port = std::atoi(env);
    }

    if (const char *env = std::getenv("LOGGING_LEVEL"))
    {
        std::string lvl(env);
        // Convert to uppercase for normalization
        for (auto &c : lvl)
            c = toupper(c);
        config.logging_level = lvl;
    }
}

// rewrite_uds_paths: Rewrite UDS paths to be unique per instance
// Used for multi-instance deployments (e.g., k8s, multiple sensors)
// Prepends /tmp/sensor-{instance}/ to UDS paths
void rewrite_uds_paths(AppConfig &config, int instance)
{
    auto prefix = std::string("/tmp/sensor-") + std::to_string(instance) + "/";

    // UDS servers
    for (auto &server : config.uds_servers)
    {
        if (!server.path.empty() && server.path.rfind("/tmp/", 0) == 0)
        {
            // Replace /tmp/ with /tmp/sensor-{instance}/
            server.path = prefix + server.path.substr(5);
        }
    }

    // UDS clients
    for (auto &client : config.uds_clients)
    {
        if (!client.second.empty() && client.second.rfind("/tmp/", 0) == 0)
        {
            client.second = prefix + client.second.substr(5);
        }
    }

    // Ctrl/Status UDS
    for (auto &entry : config.ctrl_uds_name)
    {
        auto &ctrl_cfg = entry.second;
        if (!ctrl_cfg.request_path.empty() && ctrl_cfg.request_path.rfind("/tmp/", 0) == 0)
        {
            ctrl_cfg.request_path = prefix + ctrl_cfg.request_path.substr(5);
        }

        if (!ctrl_cfg.response_path.empty() && ctrl_cfg.response_path.rfind("/tmp/", 0) == 0)
        {
            ctrl_cfg.response_path = prefix + ctrl_cfg.response_path.substr(5);
        }
    }
}

// load_config: Parse config.xml and return AppConfig
// Throws std::runtime_error on error
// Applies environment overrides and instance-specific UDS/UDP settings
AppConfig load_config(const char *filename, int instance)
{
    // Load XML config file
    XMLDocument doc;
    XMLError result = doc.LoadFile(filename);
    if (result != XML_SUCCESS)
    {
        std::cerr << "TinyXML2 error loading '" << filename << "': " << doc.ErrorStr() << " (code " << result << ")" << std::endl;
        throw std::runtime_error("Failed to load XML file. Check if file exists and is valid.");
    }

    // Get <config> root element
    XMLElement *root = doc.FirstChildElement("config");
    if (root == nullptr)
        throw std::runtime_error("Invalid XML: Missing <config> root element");

    AppConfig config;

    // --- Parse Logging Level ---
    // <logging><level>INFO|DEBUG|ERROR</level></logging>
    XMLElement *logging_node = root->FirstChildElement("logging");
    if (logging_node)
    {
        XMLElement *level_node = logging_node->FirstChildElement("level");
        if (level_node && level_node->GetText())
            config.logging_level = level_node->GetText();
    }

    // --- Parse UDP Settings ---
    // <udp><local_port>...</local_port><remote_ip>...</remote_ip><remote_port>...</remote_port></udp>
    XMLElement *udp_node = root->FirstChildElement("udp");
    if (udp_node)
    {
        if (udp_node->FirstChildElement("local_port")->QueryIntText(&config.udp_local_port) != XML_SUCCESS)
            throw std::runtime_error("Missing or invalid <local_port>");
        if (udp_node->FirstChildElement("remote_port")->QueryIntText(&config.udp_remote_port) != XML_SUCCESS)
            throw std::runtime_error("Missing or invalid <remote_port>");
        const char *ip_text = udp_node->FirstChildElement("remote_ip")->GetText();
        if (ip_text)
            config.udp_remote_ip = ip_text;
        else
            throw std::runtime_error("Missing <remote_ip>");
    }
    else
    {
        throw std::runtime_error("Missing <udp> section");
    }

    // Parse sensor_id from top level (optional, default 1)
    int xml_sensor_id = 1;
    XMLElement *sensor_id_el = root->FirstChildElement("sensor_id");
    if (sensor_id_el)
        sensor_id_el->QueryIntText(&xml_sensor_id);

    // Set sensor_id in order: FSL_SENSOR_ID > instance > config.xml
    const char *env_sensor_id = std::getenv("FSL_SENSOR_ID");
    if (env_sensor_id)
    {
        config.sensor_id = std::atoi(env_sensor_id);
    }
    else if (instance >= 0)
    {
        config.sensor_id = instance;
    }
    else
    {
        config.sensor_id = xml_sensor_id;
    }

    // --- Parse Data Link UDS Settings ---
    // <data_link_uds><server .../><client .../></data_link_uds>
    XMLElement *uds_node = root->FirstChildElement("data_link_uds");
    if (uds_node)
    {
        for (XMLElement *el = uds_node->FirstChildElement(); el != nullptr; el = el->NextSiblingElement())
        {
            std::string tag = el->Name();
            if (tag == "server")
            {
                UdsServerConfig server_cfg;
                const char *name = el->Attribute("name");
                if (name)
                    server_cfg.name = name;
                XMLElement *path_el = el->FirstChildElement("path");
                if (!path_el || !path_el->GetText() || std::string(path_el->GetText()).empty())
                    throw std::runtime_error(std::string("UDS server '") + (name ? std::string(name) : "<unnamed>") + "' missing <path> element or value");
                server_cfg.path = path_el->GetText();
                XMLElement *buf_el = el->FirstChildElement("receive_buffer_size");
                if (buf_el)
                    buf_el->QueryIntText(&server_cfg.receive_buffer_size);
                if (!server_cfg.path.empty())
                    config.uds_servers.push_back(server_cfg);
            }
            else if (tag == "client")
            {
                const char *name = el->Attribute("name");
                const char *path = el->GetText();
                if (!name)
                    throw std::runtime_error("UDS client missing 'name' attribute");
                if (!path || std::string(path).empty())
                    throw std::runtime_error(std::string("UDS client '") + std::string(name) + "' missing path value");
                config.uds_clients[name] = path;
            }
        }
    }
    else
    {
        throw std::runtime_error("Missing <data_link_uds> section");
    }

    // --- Parse UL UDS Mapping ---
    // <ul_uds_mapping><mapping opcode="..." uds="..."/></ul_uds_mapping>
    XMLElement *mapping_root = root->FirstChildElement("ul_uds_mapping");
    if (mapping_root)
    {
        for (XMLElement *el = mapping_root->FirstChildElement("mapping"); el != nullptr; el = el->NextSiblingElement("mapping"))
        {
            int opcode = 0;
            const char *uds_name = el->Attribute("uds");
            el->QueryIntAttribute("opcode", &opcode);
            if (uds_name && opcode > 0)
            {
                config.ul_uds_mapping[static_cast<uint16_t>(opcode)] = uds_name;
            }
        }
    }

    // --- Parse ctrl/status UDS for each app under <ctrl_status_uds> ---
    // <ctrl_status_uds><FSW>...</FSW><PLMG>...</PLMG>...</ctrl_status_uds>
    XMLElement *ctrl_status_node = root->FirstChildElement("ctrl_status_uds");
    if (ctrl_status_node)
    {
        for (XMLElement *app_node = ctrl_status_node->FirstChildElement(); app_node != nullptr; app_node = app_node->NextSiblingElement())
        {
            std::string section = app_node->Name();
            CtrlUdsConfig ctrl_cfg;
            XMLElement *req = app_node->FirstChildElement("request");
            if (req)
            {
                XMLElement *path = req->FirstChildElement("path");
                if (path && path->GetText())
                    ctrl_cfg.request_path = path->GetText();
                XMLElement *buf = req->FirstChildElement("receive_buffer_size");
                if (buf)
                    buf->QueryIntText(&ctrl_cfg.request_buffer_size);
            }

            XMLElement *resp = app_node->FirstChildElement("response");
            if (resp)
            {
                XMLElement *path = resp->FirstChildElement("path");
                if (path && path->GetText())
                    ctrl_cfg.response_path = path->GetText();
                XMLElement *buf = resp->FirstChildElement("receive_buffer_size");
                if (buf)
                    buf->QueryIntText(&ctrl_cfg.response_buffer_size);
            }

            config.ctrl_uds_name[section] = ctrl_cfg;
        }
    }

    // Override config fields from environment variables if set
    override_config_from_env(config);

    // Replace FSL_REMOTE_IP template if present
    if (instance >= 0)
    {
        const std::string tmpl = "{i}";
        size_t pos = config.udp_remote_ip.find(tmpl);
        if (pos != std::string::npos)
        {
            config.udp_remote_ip.replace(pos, tmpl.length(), std::to_string(instance));
        }
    }

    // lilo:TODO [review]
    if (instance >= 0 && !is_k8s_mode())
    {
        // If instance >= 0, rewrite UDS paths for multi-instance support
        rewrite_uds_paths(config, instance);

        // If instance >= 0, assign unique UDP ports per instance (for multi-instance)
        config.udp_local_port = 9910 + instance;
        config.udp_remote_port = 9010 + instance;
    }

    return config;
}
