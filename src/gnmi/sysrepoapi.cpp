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

#include "sysrepoapi.h"
#include "log.h"

#include <exception>
#include <iostream>

void event_notif_cb(const sr_ev_notif_type_t notif_type,
                    const char *xpath, const sr_val_t *values,
                    const size_t value_cnt, time_t timestamp,
                    void *private_ct)
{
    if (NULL == xpath || NULL == values || NULL == private_ct) {
        ERROR("Error, NULL detect.");
        return;
    }

    SysrepoAPI *sapi = (SysrepoAPI*) private_ct;

    for (size_t i = 0; i < value_cnt; i++ ) {
        sapi->setOutVal(*(values + i));
    }

    sapi->sendEvent(sapi);
}

SysrepoAPI::SysrepoAPI()
{
    sr_log_stderr(SR_LL_DBG);
}

SysrepoAPI::~SysrepoAPI()
{
    if (nullptr != subscription) {
        sr_unsubscribe(sess, subscription);
    }

    if (nullptr != sess) {
        sr_session_stop(sess);
    }

    if (nullptr != conn) {
        sr_disconnect(conn);
    }
}

void SysrepoAPI::setSysrepoName(const std::string& sysrepo_name)
{
    this->sysrepo_name = sysrepo_name;
}

void SysrepoAPI::connect()
{
    int rc = sr_connect(sysrepo_name.c_str(), SR_CONN_DEFAULT, &conn);
    if (SR_ERR_OK != rc) {
        ERROR("Failed connect to sysrepo.");
        throw std::runtime_error("Failed connect to sysrepo.");
    }
}

void SysrepoAPI::createSession(sr_datastore_e sesType)
{
    int rc = SR_ERR_OK;

    if (nullptr == conn) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    closeSession();

    rc = sr_session_start(conn, sesType, SR_SESS_DEFAULT, &sess);
    if (SR_ERR_OK != rc) {
        ERROR("Failed connect to sysrepo session.");
        throw std::runtime_error("Failed connect to sysrepo session.");
    }
}

void SysrepoAPI::closeSession()
{
    if (nullptr != sess) {
        sr_session_stop(sess);
        sess = nullptr;
    }
}

void SysrepoAPI::commit()
{
    int rc = SR_ERR_OK;

    if (nullptr == sess) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    rc = sr_commit(sess);
    if (SR_ERR_OK != rc) {
        ERROR("Failed commit change to sysrepo.");
        throw std::runtime_error("Failed commit change to sysrepo.");
    }
}

void SysrepoAPI::addData(const gNMIData& data)
{
    inputData.push_back(data);
}

void SysrepoAPI::getItemMessage()
{
    if (nullptr == sess) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    for (const auto &data : inputData) {
        sr_val_t *value = nullptr;
        sr_val_iter_t *iter = nullptr;
//         size_t count;
        DEBUG("Path: %s", data.getXPath(gNMIData::xPathType::sysrepoPath).c_str());

        int rc = sr_get_items_iter(sess, data.getXPath(gNMIData::xPathType::sysrepoPath).c_str(),
                             &iter);
//         int rc = sr_get_item(sess, data.getXPath(gNMIData::xPathType::sysrepoPath).c_str(),
//                              &value);
        if (SR_ERR_OK != rc) {
            ERROR("Failed get item from sysrepo.");
            throw std::runtime_error("Failed get item from sysrepo.");
        }

        while (SR_ERR_OK == sr_get_item_next(sess, iter, &value)) {
            sr_print_val(value);

//         sr_print_val(value);

        gNMIData oData;

        oData.setXPath(value->xpath, gNMIData::xPathType::sysrepoPath);
        oData.setValue(sr_val_to_str(value));

        outpuData.push_back(oData);

        sr_free_val(value);
        }
    }
}

void SysrepoAPI::setItemMessage()
{
    int rc = SR_ERR_OK;

//     sr_xpath_ctx_t state = {0};

    if (nullptr == sess) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    for (const auto &data : inputData) {
        DEBUG("Set path: %s", data.getXPath(gNMIData::xPathType::sysrepoPath).c_str());
        DEBUG("Val: %s", data.getStr().c_str());

//         if (gNMIData::ValueType::dStringVal == data.dataType()) {
            rc = sr_set_item_str(sess,
                                 data.getXPath(gNMIData::xPathType::sysrepoPath).c_str(),
                                 data.getStr().c_str(), SR_EDIT_DEFAULT);
//         } else {
//             sr_val_t value = { 0 };
//             setSrVal(value, data);
// 
//             rc = sr_set_item(sess, data.getXPath(gNMIData::xPathType::sysrepoPath).c_str(),
//                              &value, SR_EDIT_DEFAULT);
//         }
        if (SR_ERR_OK != rc) {
            ERROR("Failed set item to sysrepo.");
            throw std::runtime_error("Failed set item to sysrepo.");
        }

        //TODO: Hmm, this is duplication copy, I`m not sure with this solution.
        outpuData.push_back(data);
    }
}

void SysrepoAPI::rpcSend(const std::string& xpath, sr_val_t &input)
{
    int rc = SR_ERR_OK;
    sr_val_t *output = nullptr;
    size_t output_cnt = 0;

    if (nullptr == sess) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    rc = sr_rpc_send(sess, xpath.c_str(), &input, 1, &output, &output_cnt);
    if (SR_ERR_OK != rc) {
        ERROR("RPC message failed: %s.", sr_strerror(rc));
        throw std::runtime_error("RPC message failed.");
    }
}

void SysrepoAPI::eventSubscribeMessage()
{
    int rc = SR_ERR_OK;

    if (nullptr == sess) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    for (const auto &data : inputData) {
        DEBUG("Register subscribe path: %s",
              data.getXPath(gNMIData::xPathType::sysrepoPath).c_str());
        rc = sr_event_notif_subscribe(sess,
                                      data.getXPath(gNMIData::xPathType::sysrepoPath).c_str(),
                                      event_notif_cb, this, SR_SUBSCR_DEFAULT,
                                      &subscription);
        if (SR_ERR_OK != rc) {
            ERROR("Event notification subscribe failed: %s.", sr_strerror(rc));
            throw std::runtime_error("Event notification subscribe failed.");
        }
    }
}

const std::list<SysrepSchema> &SysrepoAPI::getSchemas()
{
    int rc = SR_ERR_OK;
    sr_schema_t *schemas = nullptr;
    size_t schema_cnt = 0;

    if (nullptr == sess) {
        ERROR("Error, nullptr detect.");
        throw std::runtime_error("Error, nullptr detect.");
    }

    rc = sr_list_schemas(sess, &schemas, &schema_cnt);
    if (SR_ERR_OK != rc) {
        ERROR("List schemas failed: %s.", sr_strerror(rc));
        throw std::runtime_error("List schemas failed failed.");
    }

    for (size_t i = 0; i < schema_cnt; i++) {
//         DEBUG("Module: %s, xpath: %s, prefix: %s, r yang: %s, revison: %s",
//               schemas[i].module_name, schemas[i].ns, schemas[i].prefix,
//               schemas[i].revision.file_path_yang, schemas[i].revision.revision);
        SysrepSchema schema;

        if (NULL != schemas[i].module_name) {
            schema.moduleName = std::string(schemas[i].module_name);
        }

        if (NULL != schemas[i].revision.revision) {
            schema.revision = std::string(schemas[i].revision.revision);
        }
        schemaList.push_back(schema);
    }

    sr_free_schemas(schemas, schema_cnt);

    return schemaList;
}

std::list<gNMIData> SysrepoAPI::getOutputData() const
{
    return outpuData;
}

void SysrepoAPI::cleanData()
{
    inputData.clear();
    outpuData.clear();
    schemaList.clear();
}

enum sr_type_e SysrepoAPI::convergNMITypeToSysrepo(gNMIData::ValueType type)
{
    switch (type) {
        case gNMIData::ValueType::dStringVal:
            return SR_STRING_T;

        case gNMIData::ValueType::dIntVal:
            return SR_INT32_T;

        case gNMIData::ValueType::UnknownVal:
        default:
            return SR_UNKNOWN_T;
    }

    return SR_UNKNOWN_T;
}

gNMIData::ValueType SysrepoAPI::convergSysrepoTypeTogNMI(sr_type_e type)
{
    switch (type) {
        case SR_STRING_T:
            return gNMIData::ValueType::dStringVal;

        case SR_INT8_T:
        case SR_INT16_T:
        case SR_INT32_T:
        case SR_INT64_T:
        case SR_UINT8_T:
        case SR_UINT16_T:
        case SR_UINT32_T:
        case SR_UINT64_T:
            return gNMIData::ValueType::dIntVal;

        case SR_ANYDATA_T:
        case SR_ANYXML_T:
        case SR_BINARY_T:
        case SR_BITS_T:
        case SR_BOOL_T:
        case SR_CONTAINER_PRESENCE_T:
        case SR_CONTAINER_T:
        case SR_DECIMAL64_T:
        case SR_ENUM_T:
        case SR_IDENTITYREF_T:
        case SR_INSTANCEID_T:
        case SR_LEAF_EMPTY_T:
        case SR_LIST_T:
        case SR_TREE_ITERATOR_T:
        case SR_UNKNOWN_T:
            return gNMIData::ValueType::UnknownVal;

        default:
            return gNMIData::ValueType::UnknownVal;
    }

    return gNMIData::ValueType::UnknownVal;
}

void SysrepoAPI::setSrVal(sr_val_t& value, const gNMIData& gData)
{
    value.type = convergNMITypeToSysrepo(gData.dataType());

    switch (gData.dataType()) {
        case gNMIData::ValueType::dStringVal:
            value.data.string_val = (char *) gData.getStr().c_str();
            DEBUG("Val: %s", value.data.string_val);
            break;

        case gNMIData::ValueType::dIntVal:
            value.data.int32_val = gData.getInt();
            DEBUG("Val: %d", value.data.int32_val);
            break;

        case gNMIData::ValueType::UnknownVal:
        default:
            DEBUG("Unknown value.");
            //TODO:
            break;
    }
}

void SysrepoAPI::setOutVal(const sr_val_t& value)
{
//     auto key = convergSysrepoTypeTogNMI(value.type);
    std::string str;

    //TODO: Need rewrite, only for test
    switch (value.type) {
        case SR_STRING_T:
            str = std::string(value.data.string_val);
            break;

        case SR_INT8_T:
            str = std::to_string(value.data.int8_val);
            break;

        case SR_INT16_T:
            str = std::to_string(value.data.int16_val);
            break;

        case SR_INT32_T:
            str = std::to_string(value.data.int32_val);
            break;

        case SR_INT64_T:
            str = std::to_string(value.data.int64_val);
            break;

        case SR_UINT8_T:
            str = std::to_string(value.data.uint8_val);
            break;
        case SR_UINT16_T:
            str = std::to_string(value.data.uint16_val);
            break;
        case SR_UINT32_T:
            str = std::to_string(value.data.uint32_val);
            break;
        case SR_UINT64_T:
            str = std::to_string(value.data.uint64_val);
            break;

        default:
            //TODO: Need implement.
            break;
    }

    gNMIData sData;
    sData.setXPath(value.xpath, gNMIData::xPathType::sysrepoPath);
    sData.setValue(str);

    outpuData.push_back(sData);
}
