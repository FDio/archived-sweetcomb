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

#ifndef GNMISERVER_H
#define GNMISERVER_H

#include <grpcpp/grpcpp.h>

#include "gnmi.pb.h"
#include "gnmi.grpc.pb.h"
#include "sysrepoapi.h"
#include "gnmidata.h"
#include "eventreciverbase.h"

#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using gnmi::gNMI;
using gnmi::CapabilityRequest;
using gnmi::CapabilityResponse;
using gnmi::GetRequest;
using gnmi::GetResponse;
using gnmi::SetRequest;
using gnmi::SetResponse;
using gnmi::SubscribeRequest;
using gnmi::SubscribeResponse;

/**
 * @todo write docs
 */
class gNMIServer final : public gNMI::Service,
                                        public virtual EventReceiver<SysrepoAPI>
{
public:
    gNMIServer(SysrepoAPI &sysrepo);

    Status Capabilities(ServerContext* context,
                        const CapabilityRequest* request,
                        CapabilityResponse* reply) override;
    Status Get(ServerContext* context, const GetRequest* request,
               GetResponse* reply) override;
    Status Set(ServerContext* context, const SetRequest* request,
               SetResponse* reply) override;
    Status Subscribe(ServerContext* context,
                     ServerReaderWriter<SubscribeResponse,
                     SubscribeRequest>* stream) override;

    void receiveWriteEvent(SysrepoAPI * psender) override;

private:
    void parsePathMsg(const gnmi::Path &path);
    void printPath(const gnmi::Path &path);
    std::string convertToXPath(const gnmi::Path &path);

    void subscibeList(const gnmi::SubscriptionList &slist);

    void handleUpdateMessage(const gnmi::Update &msg);
    void handleTypeValueMsg(const gnmi::TypedValue &msg);

    void xpathTogNMIEl(const std::string &str, gnmi::Path &path);

    uint64_t getTimeNanosec();

private:
    SysrepoAPI &sysrepo;
    gNMIData vData;
    std::string dataPrefix;
};

#endif // GNMISERVER_H
