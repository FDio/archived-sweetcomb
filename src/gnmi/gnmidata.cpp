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

#include "gnmidata.h"

gNMIData::ValueType gNMIData::dataType() const
{
    return dtype;
}

void gNMIData::clean()
{
    dtype = ValueType::UnknownVal;
    value = {};
    xpath = "";
}

void gNMIData::setXPath(const std::string& str, xPathType type)
{
    std::size_t pos = 0;

    xpath = str;

    if (xPathType::gNMIPath == type) {
        return;
    }

    while (std::string::npos != (pos = xpath.find(":", pos))) {
        xpath.replace(pos, std::string(":").length(), "/");
    }
}

void gNMIData::setValue(const std::string& str)
{
    dtype = ValueType::dStringVal;
    value = str;
}

void gNMIData::setValue(int val)
{
    dtype = ValueType::dIntVal;
    value = val;
}

std::string gNMIData::getXPath(gNMIData::xPathType type) const
{
    switch (type) {
        case xPathType::gNMIPath:
            return xpath;

        case xPathType::sysrepoPath:
            return convertToSyrepoPath();

        default:
            break;
    }

    return xpath;
}

std::string gNMIData::getStr() const
{
    switch (dtype) {
        case ValueType::dIntVal:
            return std::to_string(getInt());

        case ValueType::dStringVal:
            return std::get<std::string>(value);

        case ValueType::UnknownVal:
        default:
            //TODO: I`m not sure with N/Al
            return "N/A";
    }

    return "N/A";
}

int gNMIData::getInt() const
{
    return std::get<int>(value);
}

std::string gNMIData::convertToSyrepoPath() const
{
    std::string str = xpath;
    std::size_t pos = 0;
    int i = 0;

    while (std::string::npos != (pos = str.find("/", pos))) {
        if (1 == i++) {
            str.replace(pos, std::string("/").length(), ":");
            break;
        }
        pos++;
    }

    return str;
}
