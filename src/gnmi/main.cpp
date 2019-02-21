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

#include "log.h"
#include "gnmiserver.h"
#include "sysrepoapipool.h"

#include <argp.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <arpa/inet.h>

const char *argp_program_version = "0.0.1";
// const char *argp_program_bug_address = "<your@email.address>";
static char doc[] = "gNMI server implementation/integration for FD.io vpp ";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"address", 'a', "x.x.x.x", 0, "Destination IPv4 address"},
    {"port", 'p', "port", 0, "Destination port"},
    {"sysrepo", 's', "name", 0, "Sysrepo name"},
    {"ssl/tls", 't', NULL, 0, "Enable ssl/tls"},
    {"serverKey", 'k', "file", 0, "Server Key"},
    {"serverCerts", 'c', "file", 0, "Server Certs"},
    {"rootCert", 'r', "file", 0, "Root Cert"},
    { 0 }
};

struct arguments {
    std::string ip_address;
    std::string port;
    std::string sysrepo_name;
    bool enable_ssltls;
    std::string server_key;
    std::string server_certs;
    std::string root_cert;

    arguments() : ip_address{}, port{}, sysrepo_name{}, enable_ssltls{false},
    server_key{}, server_certs{}, root_cert{} {}
};

static error_t check_ip(const std::string &str)
{
    struct sockaddr_in sa;
    int rc = inet_pton(AF_INET, str.c_str(), &(sa.sin_addr));
    if (1 != rc) {
        ERROR("Wrong IPv4 address format.");
        return ARGP_KEY_ERROR;
    }

    return ARGP_KEY_ARG;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    error_t rc = 0;
    struct arguments *arguments = (struct arguments*) state->input;

    switch (key) {
        case 'a':
            arguments->ip_address = std::string(arg);
            rc = check_ip(arguments->ip_address);
            break;

        case 'p':
            arguments->port = std::string(arg);
            break;

        case 't':
            arguments->enable_ssltls = true;
            break;

        case 's':
            arguments->sysrepo_name = std::string(arg);
            break;

        case 'k':
            arguments->server_key = std::string(arg);
            break;

        case 'c':
            arguments->server_certs = std::string(arg);
            break;

        case 'r':
            arguments->root_cert = std::string(arg);
            break;

        case ARGP_KEY_ARG:
            return rc;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return rc;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };
SysrepoApiPool sysrepoPoll;

static std::string read_cert(const std::string &file)
{
    std::ifstream fo(file);
    std::stringstream str;

    str << fo.rdbuf();

    return str.str();
}

void RunServer(const struct arguments &arg)
{
    std::string server_address = std::string(arg.ip_address) +
    std::string(":") + std::string(arg.port);
    gNMIServer service(sysrepoPoll.get(arg.sysrepo_name));
    ServerBuilder builder;

    try {
        if (arg.enable_ssltls) {
            grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;

            pkcp.private_key = read_cert(arg.server_key);
            pkcp.cert_chain = read_cert(arg.server_certs);

            grpc::SslServerCredentialsOptions ssl_opts;
            ssl_opts.pem_root_certs = "";
            ssl_opts.pem_key_cert_pairs.push_back(pkcp);
            ssl_opts.pem_root_certs = read_cert(arg.root_cert);

            auto creds = grpc::SslServerCredentials(ssl_opts);

            builder.AddListeningPort(server_address, creds);
        } else {

            // Listen on the given address without any authentication mechanism.
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        }
        // Register "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *synchronous* service.
        builder.RegisterService(&service);
        // Finally assemble the server.
        //     builder.
        std::unique_ptr<Server> server(builder.BuildAndStart());

        if (server != nullptr)
        {
            std::cout << "Server listening on " << server_address << std::endl;
            // Wait for the server to shutdown. Note that some other thread must be
            // responsible for shutting down the server for this call to ever return.
            server->Wait();
        }
        else
        {
            throw std::logic_error("Failed to create Server.");
        }
    } catch(...) {
        ERROR("Failed to create Server.");
        exit(-1);
    }

}

int main(int argc, char **argv)
{
    struct arguments arguments;

    init_log(LOG_PERROR);

    error_t rc = argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if (ARGP_KEY_ARG != rc) {
        argp_help(&argp, stdout, ARGP_HELP_SEE, argv[0]);
        exit(-1);
    }

    if (arguments.ip_address.empty() || arguments.port.empty()) {
        ERROR("IP address and port must be set.");
        argp_help(&argp, stdout, ARGP_HELP_SEE, argv[0]);
        exit(-1);
    }

    if (arguments.enable_ssltls) {
        if (arguments.server_key.empty()) {
            ERROR("Server key must be set.");
            argp_help(&argp, stdout, ARGP_HELP_SEE, argv[0]);
            exit(-1);
        }

        if (arguments.server_certs.empty()) {
            ERROR("Server certificate must be set.");
            argp_help(&argp, stdout, ARGP_HELP_SEE, argv[0]);
            exit(-1);
        }
    }

    auto &sysrepo = sysrepoPoll.get(arguments.sysrepo_name);
    sysrepo.connect();

    std::string destination = std::string(arguments.ip_address) +
                                std::string(":") + std::string(arguments.port);

    RunServer(arguments);

    close_log();

    return 0;
}
