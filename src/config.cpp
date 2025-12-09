#include "config.h"
#include <stdexcept>
#include <iostream>

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

    // --- Parse UDS Settings ---
    XMLElement *uds_node = root->FirstChildElement("uds");
    if (uds_node)
    {
        for (XMLElement *el = uds_node->FirstChildElement(); el != nullptr; el = el->NextSiblingElement())
        {
            std::string tag = el->Name();
            if (tag == "server")
            {
                const char *path = el->GetText();
                if (path)
                    config.uds_servers.push_back(path);
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
        throw std::runtime_error("Missing <uds> section");
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

    return config;
}
