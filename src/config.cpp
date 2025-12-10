#include "config.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

// Include TinyXML-2 header
// Ensure libtinyxml2-dev is installed on your PetaLinux rootfs
#include <tinyxml2.h>

using namespace tinyxml2;

AppConfig load_config(const char *filename)
{
    XMLDocument doc;
    XMLError result = doc.LoadFile(filename);
    if (result != XML_SUCCESS)
    {
        std::cerr << "TinyXML2 error loading '" << filename << "': " << doc.ErrorStr() << " (code " << result << ")" << std::endl;
        throw std::runtime_error("Failed to load XML file. Check if file exists and is valid.");
    }

    XMLElement *root = doc.FirstChildElement("config");
    if (root == nullptr)
        throw std::runtime_error("Invalid XML: Missing <config> root element");

    AppConfig config;

    // --- Parse UDP Settings ---
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

    // --- Parse Data Link UDS Settings ---
    XMLElement *uds_node = root->FirstChildElement("data_link_uds");
    if (uds_node)
    {
        for (XMLElement *el = uds_node->FirstChildElement(); el != nullptr; el = el->NextSiblingElement())
        {
            std::string tag = el->Name();
            if (tag == "server")
            {
                UdsServerConfig server_cfg;
                XMLElement *path_el = el->FirstChildElement("path");
                if (path_el && path_el->GetText())
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
                if (name && path)
                    config.uds_clients[name] = path;
            }
        }
    }
    else
    {
        throw std::runtime_error("Missing <data_link_uds> section");
    }

    // --- Parse UL UDS Mapping ---
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

    // --- Parse ctrl/status UDS for each app/tod under <ctrl_status> ---
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
            config.ctrl_uds[section] = ctrl_cfg;
        }
    }

    // --- Override with environment variables if set ---
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

    return config;
}
