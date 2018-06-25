// ///////////////////////////////////////////////////////////////////////// //
//                                                                           //
//   Copyright (C) 2018 by Oleg Polivets                                     //
//   jsbot@ya.ru                                                             //
//                                                                           //
//   This program is free software; you can redistribute it and/or modify    //
//   it under the terms of the GNU General Public License as published by    //
//   the Free Software Foundation; either version 2 of the License, or       //
//   (at your option) any later version.                                     //
//                                                                           //
//   This program is distributed in the hope that it will be useful,         //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//   GNU General Public License for more details.                            //
//                                                                           //
// ///////////////////////////////////////////////////////////////////////// //

#include <iostream>
#include <sstream>
#include <fstream>
#include "mflow.hpp"
#include "pcapdumper.hpp"
#include "version.h"

struct FlowsContext {
    op::VariantPtr mNodePtr;
    bool mRequest;
    FlowsContext()
    {}
    FlowsContext(op::VariantPtr ptr, bool request)
        : mNodePtr(ptr)
        , mRequest(request)
    {}
};
typedef std::map< timeval, FlowsContext > FlowsByTimeStamp;

bool operator< (const timeval & a, const timeval & b) {
    return a.tv_sec  < b.tv_sec ||
          (a.tv_sec == b.tv_sec && a.tv_usec < b.tv_usec);
}

bool dumpFlows(const op::MFlowParser & parsedFlows, const std::string & outPath) {
    // create dumper object
    op::PCapDumper dumper(outPath);
    if (!dumper.isOK()) {
        std::cerr << "ERR: " << dumper.errorString() << std::endl;
        return false;
    }

    // sort requests/responses for each flow by timestamp
    FlowsByTimeStamp flows; {
        op::ValuesVector & v = parsedFlows.itemsVec();
        for (unsigned i = 0; i < v.size(); ++i) {
            op::KeyValueMap & obj = v.at(i)->asMap();
            const std::string & type = obj["type"]->asString();
            if (type.compare("http") != 0) {
                std::cerr << "WARN: ignored flow with type '" << type << "'" << std::endl;
                continue;
            }

            struct timeval ts;
            op::KeyValueMap & req = obj["request"]->asMap();
            sscanf(req["timestamp_start"]->asString().c_str(), "%10ld.%06ld", &ts.tv_sec, &ts.tv_usec);
            flows[ts] = FlowsContext(v.at(i), true);

            op::KeyValueMap & resp = obj["response"]->asMap();
            sscanf(resp["timestamp_start"]->asString().c_str(), "%10ld.%06ld", &ts.tv_sec, &ts.tv_usec);
            flows[ts] = FlowsContext(v.at(i), false);
        }
    }

    // dump each HTTP request/response according its timestamps
    for (FlowsByTimeStamp::const_iterator it = flows.begin(); it != flows.end(); ++it) {
        op::KeyValueMap & obj = it->second.mNodePtr->asMap();
        {
            // parse ip:port of source and destination
            op::KeyValueMap & server_conn = obj["server_conn"]->asMap();
            if (server_conn["ip_address"]->isMap()) {
                // old versions
                op::ValuesVector & addrSrv = server_conn["ip_address"]->asMap()["address"]->asVector();
                op::ValuesVector & addrCli = server_conn["source_address"]->asMap()["address"]->asVector();
                assert(addrSrv.size() >= 2);
                assert(addrCli.size() >= 2);
                dumper.setAddrs(addrSrv[0]->asString(), addrSrv[1]->asString(),
                        addrCli[0]->asString(), addrCli[1]->asString());
#if (0)
                std::string k (addrSrv[0]->asString() + ":" + addrSrv[1]->asString() + ":" +
                        addrCli[0]->asString() + ":" + addrCli[1]->asString());
                std::cout << it->first.tv_sec << "." << it->first.tv_usec
                          << " " << it->second.mRequest
                          << " " << k
                          << std::endl;
#endif
            } else {
                // new version of flow
                op::ValuesVector & addrSrv = server_conn["ip_address"]->asVector();
                op::ValuesVector & addrCli = server_conn["source_address"]->asVector();
                assert(addrSrv.size() >= 2);
                assert(addrCli.size() >= 2);
                dumper.setAddrs(addrSrv[0]->asString(), addrSrv[1]->asString(),
                        addrCli[0]->asString(), addrCli[1]->asString());
#if (0)
                std::string k (addrSrv[0]->asString() + ":" + addrSrv[1]->asString() + ":" +
                        addrCli[0]->asString() + ":" + addrCli[1]->asString());
                std::cout << it->first.tv_sec << "." << it->first.tv_usec
                          << " " << it->second.mRequest
                          << " " << k
                          << std::endl;
#endif
            }
        }{
            // rebuild HTTP request/response
            std::stringstream ss;
            if (it->second.mRequest == true) {
                op::KeyValueMap & req = obj["request"]->asMap();
                ss << req["method"]->asString() << " ";
                ss << req["path"]->asString() << " ";
                ss << req["http_version"]->asString() << "\r\n";
                {
                    op::ValuesVector & h = req["headers"]->asVector();
                    for (unsigned i = 0; i < h.size(); ++i) {
                        op::ValuesVector & hh = h[i]->asVector();
                        assert(hh.size() >= 2);
                        ss << hh[0]->asString() << ": "
                           << hh[1]->asString() << "\r\n";
                    }
                }
                ss << "\r\n";
                ss << req["content"]->asString();
            } else {
                op::KeyValueMap & resp = obj["response"]->asMap();
                ss << resp["http_version"]->asString() << " ";
                ss << resp["status_code"]->asString() << " ";
                ss << resp["reason"]->asString() << "\r\n";
                {
                    op::ValuesVector & h = resp["headers"]->asVector();
                    for (unsigned i = 0; i < h.size(); ++i) {
                        op::ValuesVector & hh = h[i]->asVector();
                        assert(hh.size() >= 2);
                        ss << hh[0]->asString() << ": "
                           << hh[1]->asString() << "\r\n";
                    }
                }
                ss << "\r\n";
                ss << resp["content"]->asString();
            }
            // ... and dump it
            const std::string & http = ss.str();
            dumper.dump((const u_char*) http.c_str(), http.size(), it->first, it->second.mRequest);
        }
    }
    return true;
} // dumpFlows

// parsing command options
struct CommandOptions {
    std::string mInputPath;
    bool mPrint;
    bool mDump;
    bool mShowUsage;

    void usage() {
        std::cout
            << "mitmproxy2pcap v" << _VERSION_ << " " << _PROD_COPYRIGHT_ "\n"
            << "mitmproxy flow files converter to pcap.\n"
            << "\n"
            << "mitmproxy2pcap [OPTIONS] path_to_input_file\n"
            << "\n"
            << "OPTIONS:\n"
            << "--print  - just print json representation of parsed flows and exit.\n"
            << "--help   - this output.\n";
    }

    CommandOptions(int argc, char ** argv)
        : mPrint(false)
        , mDump(false)
        , mShowUsage(false)
    {
        for (int i = 1; i < argc; ++i) {
            if (!::strcmp(argv[i], "--help")) {
                mShowUsage = true;
            } else if (!::strcmp(argv[i], "--print")) {
                mPrint = true;
            } else {
                mInputPath = argv[i];
                mDump = true;
            }
        }
        // if input path not specifed then show usage message
        mShowUsage = mInputPath.empty();
    }

    ~CommandOptions() {
        if (mShowUsage) usage();
    }
}; // CommandOptions

// entry point of application
int main(int argc, char** argv) {
    CommandOptions cmdOptions(argc, argv);
    if (!cmdOptions.mShowUsage) {
        op::MFlowParser parsedFlows;
        std::ifstream is(cmdOptions.mInputPath.c_str(), std::ifstream::binary);
        parsedFlows.parse(is);
        if (cmdOptions.mPrint) {
            parsedFlows.rootItem()->print(std::cout);
        } else if (cmdOptions.mDump) {
            dumpFlows(parsedFlows, cmdOptions.mInputPath + ".pcap");
        }
    }
} // main
