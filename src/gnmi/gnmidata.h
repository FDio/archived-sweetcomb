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

#ifndef GNMIDATA_H
#define GNMIDATA_H

#include <string>

/**
 * @todo write docs
 */
class gNMIData
{
public:
    enum class ValueType {
        dStringVal,
        dIntVal,
        UnknownVal
    };

    enum class xPathType {
        gNMIPath,
        sysrepoPath,
    };

public:
    gNMIData() = default;
    ~gNMIData() = default;

    ValueType dataType() const;
    void clean();

    void setXPath(const std::string &str, xPathType type = xPathType::gNMIPath);
    void setValue(const std::string &str);
    void setValue(int val);

    std::string getXPath(xPathType type = xPathType::gNMIPath) const;
    std::string getStr() const;
    int getInt() const;

private:
    std::string convertToSyrepoPath() const;

private:
    ValueType dtype = ValueType::UnknownVal;
    int intData;
    std::string strData;
    std::string xpath;
};

#endif // GNMIDATA_H
