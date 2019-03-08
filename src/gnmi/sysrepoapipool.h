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

#ifndef SYSREPOAPIPOOL_H
#define SYSREPOAPIPOOL_H

#include "sysrepoapi.h"

#include <map>
#include <string>

/**
 * @todo write docs
 */
class SysrepoApiPool
{
public:
    /**
     * Default constructor
     */
    SysrepoApiPool();

    /**
     * Destructor
     */
    ~SysrepoApiPool();

    SysrepoAPI &get(const std::string &name);
    void remove(const std::string &name);

private:
    std::map<std::string, SysrepoAPI> pool;
};

#endif // SYSREPOAPIPOOL_H
