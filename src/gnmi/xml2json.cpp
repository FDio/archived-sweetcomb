/*
 * Copyright (c) 2019 PANTHEON.tech.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xml2json.h"
#include "log.h"

#include <exception>
#include <sstream>
#include <iostream>

XML2JSON::XML2JSON(const XML2JSON& other)
{

}

XML2JSON::~XML2JSON()
{

}

void XML2JSON::setData(const std::string& data, XML2JSON::string_type_t type)
{
    switch (type) {
        case string_type_t::JSONSTRING:
            {
                Json::Reader read;
                DEBUG("JSONSTRING: %s", data.c_str());

                read.parse(data, jsonRoot);
                DEBUG("Root size: %d", jsonRoot.size());
            }
            break;

        case string_type_t::XMLSTRING:
            {
                int rc = 0;
                rc = xmlDoc.load_string(data.c_str(),
                                   pugi::parse_default | pugi::parse_comments);
                if (0 != rc) {
                    ERROR("Error parse xml string.");
                    throw std::logic_error("Error parse xml string.");
                }
            }
            break;

        default:
            throw std::logic_error("Unknown type.");
            break;
    }
}

void XML2JSON::setPrefix(const std::string& prefix)
{
    this->prefix = prefix;
}

std::string XML2JSON::getJson()
{
    switch (type) {
        case string_type_t::JSONSTRING:
            //TODO:
            return std::string("");

        case string_type_t::XMLSTRING:
            //TODO:
            return std::string("");

        default:
            break;
    }

    throw std::logic_error("Unknown type.");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
std::string XML2JSON::getXML()
{
    std::ostringstream stream;

    switch (type) {
        case string_type_t::JSONSTRING:
            createXML();

        case string_type_t::XMLSTRING:
            xmlDoc.print(stream);
            DEBUG("XML doc: %s", xmlDoc.path().c_str());
            return stream.str();

        default:
            break;
    }

    throw std::logic_error("Unknown type.");
}
#pragma GCC diagnostic pop

const std::list<gNMIData> & XML2JSON::getgNMIData()
{
    createGNMIData();

    return gnmiData;
}

void XML2JSON::parseJSON()
{
//     parseJSONValueType(jsonRoot);
}

void XML2JSON::parseJSONValueType(const Json::Value& value,
                                  pugi::xml_node &node)
{
    switch (value.type()) {
        case Json::nullValue:
            break;

        case Json::intValue:
            DEBUG("Value: %d", value.asInt());
            node.append_child(pugi::node_pcdata).set_value(
                                        std::to_string(value.asInt()).c_str());
            break;

        case Json::uintValue:
            DEBUG("Value: %d", value.asUInt());
            node.append_child(pugi::node_pcdata).set_value(
                                        std::to_string(value.asUInt()).c_str());
            break;

        case Json::realValue:
            DEBUG("Value: %f", value.asFloat());
            node.append_child(pugi::node_pcdata).set_value(
                                    std::to_string(value.asFloat()).c_str());
            break;

        case Json::stringValue:
            DEBUG("Value: %s", value.asString().c_str());
            node.append_child(pugi::node_pcdata).set_value(
                                                    value.asString().c_str());
            break;

        case Json::booleanValue:
            DEBUG("Value: %d", value.asBool());
            node.append_child(pugi::node_pcdata).set_value(
                                        std::to_string(value.asBool()).c_str());
            break;

        case Json::arrayValue:
            parseJSONArray(value, node);
            break;

        case Json::objectValue:
            parseJSONObject(value, node);
            break;

        default:
            DEBUG("Unknown value type.");
            break;
    }
}

void XML2JSON::parseJSONValueType(const Json::Value& value,
                                  const std::string& prefix)
{
    switch (value.type()) {
        case Json::nullValue:
            {
                gNMIData data;
                std::string tmp(prefix);
                tmp.pop_back();
                data.setXPath(tmp);
                gnmiData.push_back(data);
            }
            break;

        case Json::intValue:
            //             DEBUG("Value: %d", value.asInt());
            {
                gNMIData data;
                std::string tmp(prefix);
                tmp.pop_back();
                data.setXPath(tmp);
                data.setValue(value.asInt());
                gnmiData.push_back(data);
            }
            break;

        case Json::uintValue:
            //             DEBUG("Value: %d", value.asUInt());
            {
                gNMIData data;
                std::string tmp(prefix);
                tmp.pop_back();
                data.setXPath(tmp);
                data.setValue(value.asUInt());
                gnmiData.push_back(data);
            }
            break;

        case Json::realValue:
            //             DEBUG("Value: %f", value.asFloat());
            {
                gNMIData data;
                std::string tmp(prefix);
                tmp.pop_back();
                data.setXPath(tmp);
                data.setValue(value.asFloat());
                gnmiData.push_back(data);
            }
            break;

        case Json::stringValue:
            //             DEBUG("Value: %s", value.asString().c_str());
            {
                gNMIData data;
                std::string tmp(prefix);
                tmp.pop_back();
                data.setXPath(tmp);
                data.setValue(value.asString());
                gnmiData.push_back(data);
            }
            break;

        case Json::booleanValue:
            //             DEBUG("Value: %d", value.asBool());
        {
                gNMIData data;
                std::string tmp(prefix);
                tmp.pop_back();
                data.setXPath(tmp);
                data.setValue(value.asBool() ? "true" : "false");
                gnmiData.push_back(data);
            }
            break;

        case Json::arrayValue:
            parseJSONArray(value, prefix);
            break;

        case Json::objectValue:
            parseJSONObject(value, prefix);
            break;

        default:
            DEBUG("Unknown value type.");
            break;
    }
}

void XML2JSON::parseJSONArray(const Json::Value& value, pugi::xml_node &node)
{
    if (Json::arrayValue != value.type()) {
        ERROR("Wrong JSON Value type");
        throw std::logic_error("Wrong JSON Value type");
    }

    for (const auto &elm : value) {
        parseJSONValueType(elm, node);
    }
}

void XML2JSON::parseJSONArray(const Json::Value& value,
                              const std::string& prefix)
{
    if (Json::arrayValue != value.type()) {
        ERROR("Wrong JSON Value type");
        throw std::logic_error("Wrong JSON Value type");
    }

    for (const auto &elm : value) {
        parseJSONValueType(elm, prefix);
    }
}

void XML2JSON::parseJSONObject(const Json::Value& value, pugi::xml_node &node)
{
    if (Json::objectValue != value.type()) {
        ERROR("Wrong JSON Value type");
        throw std::logic_error("Wrong JSON Value type");
    }

    for (const auto &name : value.getMemberNames()) {
        DEBUG("Name: %s", name.c_str());
        auto child = node.append_child(name.c_str());
        parseJSONValueType(value.get(name, value), child);
    }
}

void XML2JSON::parseJSONObject(const Json::Value& value,
                               const std::string& prefix)
{
    if (Json::objectValue != value.type()) {
        ERROR("Wrong JSON Value type");
        throw std::logic_error("Wrong JSON Value type");
    }

    for (const auto &name : value.getMemberNames()) {
        DEBUG("Name: %s", name.c_str());
        std::string tmp = prefix + name + "/";
        parseJSONValueType(value.get(name, value), tmp);
    }
}

void XML2JSON::createXML()
{
//     auto elm = xmlDoc.append_child(pugi::xml_node_type::node_document);
    parseJSONValueType(jsonRoot, xmlDoc);
}

void XML2JSON::createGNMIData()
{
    std::string prefix = this->prefix + "/";
    parseJSONValueType(jsonRoot, prefix);
}
