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

#include "gnmiserver.h"
#include "log.h"
#include "xml2json.h"

#include <iostream>
#include <exception>
#include <chrono>
#include <ctime>
#include <ratio>
#include <list>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

gNMIServer::gNMIServer(SysrepoAPI& sysrepo)
    : sysrepo(sysrepo)
{
}

grpc::Status gNMIServer::Capabilities(grpc::ServerContext* context,
                                      const gnmi::CapabilityRequest* request,
                                      gnmi::CapabilityResponse* reply)
{
    DEBUG("Capabilities message");

    if (0 < request->extension_size()) {
        //TODO;
    }

    sysrepo.createSession();
    const auto &ss = sysrepo.getSchemas();

    for (const auto schema : ss) {
        auto model = reply->add_supported_models();
        model->set_name(schema.moduleName);
        model->set_version(schema.revision);
    }

    reply->add_supported_encodings(gnmi::Encoding::ASCII);

    //FIXME: I`m not sure, if this is a correct version of gnmi.
    reply->set_gnmi_version(std::to_string(gnmi::kGnmiServiceFieldNumber));

    sysrepo.closeSession();

    return Status::OK;
}

grpc::Status gNMIServer::Get(grpc::ServerContext* context,
                             const gnmi::GetRequest* request,
                             gnmi::GetResponse* reply)
{
    DEBUG("Get message");

    std::cout << gnmi::GetRequest_DataType_Name(request->type()) << std::endl;

    try {
        sysrepo.createSession();

        auto &prefix = request->prefix();
        dataPrefix = convertToXPath(prefix);
        DEBUG("Prefix: %s", dataPrefix.c_str());

        DEBUG("Data type: %s",
              request->DataType_Name(request->type()).c_str());
        DEBUG("Encoding: %s", gnmi::Encoding_Name(request->encoding()).c_str());

        for (const auto &model : request->use_models()) {
            DEBUG("Model, name: %s, organization: %s, version: %s",
                  model.name().c_str(), model.organization().c_str(),
                  model.version().c_str());
        }

//             for (const auto &extension : request->extension()) {
//                 DEBUG(ex);
//             }

        if (0 < request->path().size()) {
            for (const auto &path : request->path()) {
                std::string strPath = convertToXPath(path);
                vData.clean();
                vData.setXPath(dataPrefix + strPath);
                sysrepo.addData(vData);
                sysrepo.getItemMessage();

                auto notification = reply->add_notification();

                notification->set_timestamp(getTimeNanosec());
                auto elem = notification->mutable_prefix();
                xpathTogNMIEl(dataPrefix, *elem);

                const auto &oData = sysrepo.getOutputData();

                for (const auto &data : oData) {
                    auto nupdate = notification->add_update();
                    xpathTogNMIEl(data.getXPath(), *nupdate->mutable_path());
                    auto uval = nupdate->mutable_val();
                    uval->set_string_val(data.getStr());
                }
            }
        }

        sysrepo.cleanData();
        sysrepo.closeSession();
    } catch (...) {
        sysrepo.cleanData();
        sysrepo.closeSession();
        return Status::CANCELLED;
    }

    return Status::OK;
}

//TODO: Need handle INVALID operation, somehow, but how????
grpc::Status gNMIServer::Set(grpc::ServerContext* context,
                             const gnmi::SetRequest* request,
                             gnmi::SetResponse* reply)
{
    DEBUG("Set message");
    std::string path;

    reply->set_timestamp(getTimeNanosec());

    auto prefix = reply->mutable_prefix();
    prefix->add_elem();

    try {
        sysrepo.createSession(SR_DS_RUNNING);

        dataPrefix = convertToXPath(request->prefix());

        if (0 < request->delete__size()) {
            for (const auto &del : request->delete_()) {
                vData.clean();
                path = convertToXPath(del);
                vData.setXPath(dataPrefix + path);
                DEBUG("Delete: %s", vData.getXPath().c_str());
                sysrepo.addData(vData);
            }
            sysrepo.setItemMessage();

            const auto &oData = sysrepo.getOutputData();
            for (const auto &data : oData) {
                auto updateResult = reply->add_response();
                updateResult->set_op(gnmi::UpdateResult::DELETE);
                xpathTogNMIEl(data.getXPath(), *updateResult->mutable_path());
            }

            sysrepo.cleanData();
        }

        if (0 < request->replace_size()) {
            for (const auto &replace : request->replace()) {
                vData.clean();
                handleUpdateMessage(replace);
                DEBUG("Replace : %s", vData.getXPath().c_str());
                sysrepo.addData(vData);
            }
            sysrepo.setItemMessage();

            const auto &oData = sysrepo.getOutputData();
            for (const auto &data : oData) {
                auto updateResult = reply->add_response();
                updateResult->set_op(gnmi::UpdateResult::REPLACE);
                xpathTogNMIEl(data.getXPath(), *updateResult->mutable_path());
            }

            sysrepo.cleanData();
        }

        if (0 < request->update_size()) {
            for (const auto &update : request->update()) {
                vData.clean();
                handleUpdateMessage(update);
                DEBUG("Update: %s", vData.getXPath().c_str());
//                 sysrepo.addData(vData);
                XML2JSON json;
                json.setData(vData.getStr());
                json.setPrefix(vData.getXPath());
                auto &gdatas = json.getgNMIData();
                for (const auto &gdata : gdatas) {
                    DEBUG("Data xpath: %s, data: %s", gdata.getXPath().c_str(),
                          gdata.getStr().c_str());
                    sysrepo.addData(gdata);
                }
//                 DEBUG("%s", json.getXML().c_str());
            }
            sysrepo.setItemMessage();

            const auto &oData = sysrepo.getOutputData();
            for (const auto &data : oData) {
                auto updateResult = reply->add_response();
                updateResult->set_op(gnmi::UpdateResult::UPDATE);
                xpathTogNMIEl(data.getXPath(), *updateResult->mutable_path());
            }

            sysrepo.cleanData();
        }

        //TODO: Need some special handling for sysrepo
        sysrepo.commit();
        sysrepo.closeSession();
    } catch (...) {
        sysrepo.cleanData();
        sysrepo.closeSession();
        return Status::CANCELLED;
    }

    return Status::OK;
}

grpc::Status gNMIServer::Subscribe(grpc::ServerContext* context,
                                   ServerReaderWriter<gnmi::SubscribeResponse,
                                   gnmi::SubscribeRequest>* stream)
{
    DEBUG("Subscribe message");

    gnmi::SubscribeRequest request;

    try {
        sysrepo.registerReciver<gNMIServer>(this);
        sysrepo.registerStream(stream);

        sysrepo.createSession(SR_DS_RUNNING);
        while (stream->Read(&request)) {
            switch (request.request_case()) {
                case gnmi::SubscribeRequest::kAliases:
                    break;

                case gnmi::SubscribeRequest::kPoll:
                    break;

                case gnmi::SubscribeRequest::kSubscribe:
                    subscibeList(request.subscribe());
                    break;

                case gnmi::SubscribeRequest::REQUEST_NOT_SET:
                    break;

                default:
                    DEBUG("Unknown request case.");
                    break;
            }
        }
    } catch (...) {
        sysrepo.cleanData();
        sysrepo.closeSession();
        return Status::CANCELLED;
    }

    sysrepo.cleanData();
    sysrepo.closeSession();
    DEBUG("Subscribe message end");

    return Status::OK;
}

void gNMIServer::subscibeList(const gnmi::SubscriptionList& slist)
{
    std::string prefix = convertToXPath(slist.prefix());

    if (0 < slist.subscription_size()) {
        for (const auto &sub : slist.subscription()) {
            vData.clean();
            vData.setXPath(prefix + convertToXPath(sub.path()));
            //TODO: Call sysrepo event

            DEBUG("Subs Mode: %s",
                  gnmi::SubscriptionMode_Name(sub.mode()).c_str());
            DEBUG("Subs xpath: %s", vData.getXPath().c_str());
            sysrepo.addData(vData);
            sysrepo.eventSubscribeMessage();
        }
    }

    DEBUG("Subscribe list mode: %s", slist.Mode_Name(slist.mode()).c_str());

    if (0 < slist.use_models_size()) {
        for (const auto &umod : slist.use_models()) {
            DEBUG("Name: %s, Organization: %s, Version: %s",
                  umod.name().c_str(), umod.organization().c_str(),
                  umod.version().c_str());
        }
    }

    DEBUG("Encoding: %s", gnmi::Encoding_Name(slist.encoding()).c_str());

    DEBUG("Update only: %d", slist.updates_only());
}

void gNMIServer::receiveWriteEvent(SysrepoAPI* psender)
{
    gnmi::SubscribeResponse response;
    ServerReaderWriter<gnmi::SubscribeResponse, gnmi::SubscribeRequest>* stream;

    stream = (ServerReaderWriter<gnmi::SubscribeResponse,
              gnmi::SubscribeRequest>*) psender->getStream();

    auto sData = psender->getOutputData();
    auto update = response.mutable_update();

    update->set_timestamp(getTimeNanosec());
    auto elem = update->mutable_prefix();

    xpathTogNMIEl("/", *elem);
    for (const auto &data : sData) {
        DEBUG("XPath: %s, value: %s",
              data.getXPath().c_str(), data.getStr().c_str());
        auto nupdate = update->add_update();
        xpathTogNMIEl(data.getXPath(), *nupdate->mutable_path());
        auto val = nupdate->mutable_val();
        val->set_string_val(data.getStr());
    }

    psender->cleanData();
    stream->Write(response);
}

void gNMIServer::parsePathMsg(const gnmi::Path& path)
{
    if (0 >= path.elem_size()) {
        DEBUG("Path is empty");
        return;
    }

}

std::string gNMIServer::convertToXPath(const gnmi::Path& path)
{
    std::string str = "/";

    if (0 < path.elem_size()) {
        for (const auto &elm : path.elem()) {
            str += elm.name();

            for (const auto &el : elm.key()) {
                str += "[" + el.first + "='" + el.second + "']";
            }

            str += "/";
        }
    }
    str.pop_back();

    return str;
}

void gNMIServer::printPath(const gnmi::Path& path)
{
    if (0 < path.element().size()) {
        for (const auto &element : path.element()) {
            DEBUG("Element: %s", element.c_str());
        }
    }

    if (0 < path.elem_size()) {
        for (const auto &elm : path.elem()) {
            DEBUG("Elm name: %ss", elm.name().c_str());
            for (const auto &el : elm.key()) {
                DEBUG("El key: %s, val: %s", el.first.c_str(),
                      el.second.c_str());
            }
        }
    }
}

void gNMIServer::handleUpdateMessage(const gnmi::Update& msg)
{
    vData.setXPath(dataPrefix + convertToXPath(msg.path()));

    if (msg.has_val()) {
        handleTypeValueMsg(msg.val());
    }
}

void gNMIServer::handleTypeValueMsg(const gnmi::TypedValue& msg)
{
    DEBUG("Val case: %d", msg.value_case());

    switch (msg.value_case()) {
        case gnmi::TypedValue::ValueCase::kStringVal:
            vData.setValue(msg.string_val());
            break;

        case gnmi::TypedValue::ValueCase::kIntVal:
            break;

        case gnmi::TypedValue::ValueCase::kUintVal:
            break;

        case gnmi::TypedValue::ValueCase::kBoolVal:
            break;

        case gnmi::TypedValue::ValueCase::kBytesVal:
            break;

        case gnmi::TypedValue::ValueCase::kFloatVal:
            break;

        case gnmi::TypedValue::ValueCase::kDecimalVal:
            break;

        case gnmi::TypedValue::ValueCase::kLeaflistVal:
            break;

        case gnmi::TypedValue::ValueCase::kAnyVal:
            break;

        case gnmi::TypedValue::ValueCase::kJsonVal:
            break;

        case gnmi::TypedValue::ValueCase::kJsonIetfVal:
            vData.setValue(msg.json_ietf_val());
            break;

        case gnmi::TypedValue::ValueCase::kAsciiVal:
            break;

        case gnmi::TypedValue::ValueCase::kProtoBytes:
            break;

        case gnmi::TypedValue::ValueCase::VALUE_NOT_SET:
            break;

        default:
            DEBUG("Unknown type.");
            break;
    }
}

void gNMIServer::xpathTogNMIEl(const std::string& str, gnmi::Path& path)
{
    sr_xpath_ctx_t state;
    std::string tmp(str);
    const char *xpath = tmp.c_str();
    char *name;

    if ((name = sr_xpath_next_node((char *)xpath, &state)) == NULL) {
        DEBUG("Empty XPATH, xpath: %s", xpath);
        return;
    }

    do {
        auto pathEl = path.add_elem();
        pathEl->set_name(name);

        const char *key_name;
        while ((key_name = sr_xpath_next_key_name(NULL, &state)) != NULL) {
            std::string key(key_name);
            const char *key_value = sr_xpath_next_key_value(NULL, &state);
            (*pathEl->mutable_key())[key] = key_value;
        }
    } while ((name = sr_xpath_next_node(NULL, &state)) != NULL);

    sr_xpath_recover(&state);
}


uint64_t gNMIServer::getTimeNanosec()
{
    using namespace std::chrono;

    std::uint64_t tm = high_resolution_clock::now().time_since_epoch() /
                                                                nanoseconds(1);

    return tm;
}
