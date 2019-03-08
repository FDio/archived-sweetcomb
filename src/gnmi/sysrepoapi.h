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

#ifndef SYSREPOAPI_H
#define SYSREPOAPI_H

#include "gnmidata.h"
#include "eventreciverbase.h"

#include <sysrepo.h>
#include <sysrepo/values.h>
#include <string>
#include <list>

#include <iostream>

struct SysrepoValue
{
    void setXPath(const std::string &str) {
        std::string tmp = str;
        std::size_t pos = 0;

        while (std::string::npos != (pos = tmp.find(":", pos))) {
            tmp.replace(pos, std::string(":").length(), "/");
        }

        xpath = tmp;
    }

    std::string xpath;
    std::string value;
};

struct SysrepSchema
{
    std::string moduleName;
    std::string revision;
};

/**
 * @todo write docs
 */
class SysrepoAPI : public virtual BaseSender<SysrepoAPI>
{
public:
    /**
     * Default constructor
     */
    SysrepoAPI();

    /**
     * Destructor
     */
    ~SysrepoAPI();

    void setSysrepoName(const std::string &sysrepo_name);

    void connect();
    void createSession(sr_datastore_e sesType = SR_DS_RUNNING);
    void closeSession();
    void commit();

    void addData(const gNMIData &data);
    void getItemMessage();
    void setItemMessage();
    void rpcSend(const std::string &xpath, sr_val_t &input);
    void eventSubscribeMessage();
    const std::list<SysrepSchema> &getSchemas();
//     void print_value();

    std::list<gNMIData> getOutputData() const;
    void cleanData();

private:
    friend void event_notif_cb(const sr_ev_notif_type_t notif_type,
                               const char *xpath, const sr_val_t *values,
                               const size_t value_cnt, time_t timestamp,
                               void *private_ct);
    enum sr_type_e convergNMITypeToSysrepo(gNMIData::ValueType type);
    gNMIData::ValueType convergSysrepoTypeTogNMI(sr_type_e type);
    void setSrVal(sr_val_t &value, const gNMIData &gData);
    void setOutVal(const sr_val_t &value);

private:
    std::string sysrepo_name = "app";
    sr_conn_ctx_t *conn = nullptr;
    sr_session_ctx_t *sess = nullptr;
    sr_subscription_ctx_t *subscription = nullptr;

    std::list<gNMIData> inputData;
    std::list<gNMIData> outpuData;
    std::list<SysrepSchema> schemaList;

};

#endif // SYSREPOAPI_H
