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

#ifndef XML2JSON_H
#define XML2JSON_H

#include <string>
#include <list>

#include <json/json.h>
#include <pugixml.hpp>

#include "gnmidata.h"

/**
 * @todo write docs
 */
class XML2JSON
{
public:
    enum class string_type_t {
        XMLSTRING,
        JSONSTRING
    };

public:
    /**
     * Default constructor
     */
    XML2JSON() = default;

    /**
     * Copy Constructor
     *
     * @param other TODO
     */
    XML2JSON(const XML2JSON& other);

    /**
     * Destructor
     */
    ~XML2JSON();

    /**
     * @todo write docs
     *
     * @param other TODO
     * @return TODO
     */
//     XML2JSON& operator=(const XML2JSON& other);

    void setData(const std::string &data, string_type_t type = string_type_t::JSONSTRING);
    void setPrefix(const std::string &prefix);
    std::string getJson();
    std::string getXML();
    const std::list<gNMIData> &getgNMIData();

private:
    void parseJSON();
    void parseJSONValueType(const Json::Value &value, pugi::xml_node &node);
    void parseJSONValueType(const Json::Value &value, const std::string &prefix);
    void parseJSONArray(const Json::Value &value, pugi::xml_node &node);
    void parseJSONArray(const Json::Value &value, const std::string &prefix);
    void parseJSONObject(const Json::Value &value, pugi::xml_node &node);
    void parseJSONObject(const Json::Value &value, const std::string &prefix);

    void createXML();
    void createGNMIData();

private:
    std::string prefix = "/";
    string_type_t type = string_type_t::JSONSTRING;
    Json::Value jsonRoot;
    pugi::xml_document xmlDoc;
    std::list<gNMIData> gnmiData;
};

#endif // XML2JSON_H
