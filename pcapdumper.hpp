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

#pragma once 

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <pcap/pcap.h>
#include <string>
#include <memory>
#include <map>
#include <cassert>
#include <cstring>

namespace op {

#pragma pack(1)
struct hdrIPv4 {
    u_int8_t  iph_ihl:4,
              iph_ver:4;
    u_int8_t  iph_tos;
    u_int16_t iph_len;
    u_int16_t iph_ident;
    u_int16_t iph_offset:13,
              iph_flags:3;
    u_int8_t  iph_ttl;
    u_int8_t  iph_protocol;
    u_int16_t iph_chksum;
    u_int32_t iph_sourceip;
    u_int32_t iph_destip;
};
// TODO: add hdrIPv6
struct hdrTCP {
    u_int16_t tcph_srcport;
    u_int16_t tcph_destport;
    u_int32_t tcph_seqnum;
    u_int32_t tcph_acknum;
    u_int16_t tcph_ns:1,
              tcph_reserved:3,
              tcph_offset:4,
              tcph_fin:1,
              tcph_syn:1,
              tcph_rst:1,
              tcph_psh:1,
              tcph_ack:1,
              tcph_urg:1,
              tcph_ece:1,
              tcph_cwr:1;
    u_int16_t tcph_win;
    u_int16_t tcph_chksum;
    u_int16_t tcph_urgptr;
};
#pragma pack()

class PCapDumper {
private:
    static bool lookupIPv4(const char * host, struct in_addr & addr) {
        struct in_addr ret;
        if (inet_pton(AF_INET, host, &ret) == 1) {
            memcpy(&addr, &ret, sizeof(struct in_addr));
            return true;
        }
        struct addrinfo hints, *p = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
        hints.ai_socktype = SOCK_STREAM;
        int rv = getaddrinfo(host, NULL, &hints, &p);
        if (rv != 0) {
            //WARN("getaddrinfo: %s", gai_strerror(rv));
            return false;
        }
        memcpy(&addr, p->ai_addr, sizeof(struct in_addr));
        freeaddrinfo(p);
        return true;
    }

    static bool lookupIPv6(const char * host, struct in6_addr & addr) {
        struct in6_addr ret;
        if (inet_pton(AF_INET6, host, &ret) == 1) {
            memcpy(&addr, &ret, sizeof(struct in6_addr));
            return true;
        }
        struct addrinfo hints, *p = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6; // use AF_INET6 to force IPv6
        hints.ai_socktype = SOCK_STREAM;
        int rv = getaddrinfo(host, NULL, &hints, &p);
        if (rv != 0) {
            //WARN("getaddrinfo: %s", gai_strerror(rv));
            return false;
        }
        memcpy(&addr, p->ai_addr, sizeof(struct in6_addr));
        freeaddrinfo(p);
        return true;
    }

public:
    PCapDumper() : mHandle(nullptr), mDumper(nullptr) { }
    PCapDumper(const std::string & path) {
        mHandle = pcap_open_dead(DLT_RAW, 1 << 16);
        mDumper = pcap_dump_open(mHandle, path.c_str());
    }
    ~PCapDumper() {
        if (mDumper != nullptr) {
            pcap_dump_close(mDumper);
            mDumper = nullptr;
        }
        if (mHandle != nullptr) {
            pcap_close(mHandle);
            mHandle = nullptr;
        }
    }

    bool isOK() const {
        return mHandle != nullptr && mDumper != nullptr;
    }

    std::string errorString() const {
        if (mHandle == nullptr)
            return std::string("pcap_open_dead() failed.");
        return std::string(pcap_geterr(mHandle));
    }

    //
    bool setAddrs(const std::string & srv, const std::string & srvPort,
                  const std::string & cli, const std::string & cliPort) {
        mUseIPv4 = false;
        mUseIPv6 = false;
        mUseIPv4 = lookupIPv4(srv.c_str(), mIPv4Srv);
        if (mUseIPv4)
            mUseIPv4 = lookupIPv4(cli.c_str(), mIPv4Cli);
        if (!mUseIPv4) {
            mUseIPv6 = lookupIPv6(srv.c_str(), mIPv6Srv);
            if (mUseIPv6)
                mUseIPv6 = lookupIPv6(cli.c_str(), mIPv6Cli);
        }

        if (!mUseIPv4 && !mUseIPv6)
            return false;

        mPortSrv = atol(srvPort.c_str());
        mPortCli = atol(cliPort.c_str());

        std::string k(srv + ":" + srvPort + ":" + cli + ":" + cliPort);
        mTCPCtx = mTCPseqs[k];
        if (mTCPCtx.get() == nullptr) {
            mTCPCtx = std::shared_ptr<TCPContext>(new TCPContext);
            mTCPCtx->mReqSEQ = 0;
            mTCPCtx->mReqACK = 0;
            mTCPCtx->mRespSEQ = 0;
            mTCPCtx->mRespACK = 0;
            mTCPseqs[k] = mTCPCtx;
        }
#if (0)
        std::cerr << "DBG: " << k
                  << " " << mTCPCtx->mReqACK
                  << " " << mTCPCtx->mReqSEQ
                  << " " << mTCPCtx->mRespACK
                  << " " << mTCPCtx->mRespSEQ
                  << "\n";
#endif
        return true;
    }

    void dump(const u_char* data, size_t len, const struct timeval & ts, bool request) {
        const size_t MAX_MTU = (0xFFFF - 40);
        u_char buffer[MAX_MTU + 40];
        size_t total = 0, maxData = len;
        hdrIPv4 *pip4  = (hdrIPv4*) (buffer);
        hdrTCP  *ptcp  = (hdrTCP*)  (buffer + sizeof(hdrIPv4));
        u_char  *pdata = (buffer + sizeof(hdrIPv4) + sizeof(hdrTCP));
        u_int32_t fragments;
        size_t dataLen;
        struct pcap_pkthdr pcap_hdr;

        // load ACK and SEQ values
        u_int32_t SEQ, ACK;
        if (request) {
            SEQ = mTCPCtx->mReqSEQ;
            ACK = mTCPCtx->mRespACK;
        } else {
            SEQ = mTCPCtx->mRespSEQ;
            ACK = mTCPCtx->mReqACK;
        }

        bool fragmented;
        do {
            fragmented = (maxData - total > MAX_MTU);
            len = (fragmented ? MAX_MTU : (maxData - total));

            memset((void*)pip4, 0, sizeof(hdrIPv4));
            memset((void*)ptcp, 0, sizeof(hdrTCP));
            memcpy(pdata, data+total, len);
            fragments = total + len;
            dataLen = len;

            pip4->iph_ver = 4;
            pip4->iph_ihl = 5;
            pip4->iph_protocol = 6;
            pip4->iph_tos = 48;
            pip4->iph_ttl = 45;
            ptcp->tcph_offset = 5;

            if (request) {
                pip4->iph_sourceip  = mIPv4Cli.s_addr;
                pip4->iph_destip    = mIPv4Srv.s_addr;
                ptcp->tcph_srcport  = htons(mPortCli);
                ptcp->tcph_destport = htons(mPortSrv);
            } else {
                pip4->iph_sourceip  = mIPv4Srv.s_addr;
                pip4->iph_destip    = mIPv4Cli.s_addr;
                ptcp->tcph_srcport  = htons(mPortSrv);
                ptcp->tcph_destport = htons(mPortCli);
            }

            len += sizeof(hdrTCP);
            ptcp->tcph_win      = htons(len);
            ptcp->tcph_chksum   = 0;
            ptcp->tcph_ack      = 0;
            len += sizeof(hdrIPv4);
            pip4->iph_len       = htons(len);
            pip4->iph_chksum    = 0;
            ptcp->tcph_seqnum   = htonl(SEQ);
            ptcp->tcph_acknum   = 0;
            pip4->iph_flags     = 0;

            // TODO: calculate checksums before send

            pcap_hdr.caplen = len;
            pcap_hdr.len    = len;
            pcap_hdr.ts     = ts;
            pcap_dump((u_char*)mDumper, &pcap_hdr, buffer);
            total = fragments;

            SEQ = (SEQ + dataLen) % 0xffffffff;
            ACK = SEQ;

            // write TCP ACK from reciever

            std::swap(pip4->iph_sourceip, pip4->iph_destip);
            std::swap(ptcp->tcph_srcport, ptcp->tcph_destport);
            ptcp->tcph_win    = htons(1024);
            ptcp->tcph_chksum = 0;
            ptcp->tcph_ack    = 1;
            pip4->iph_len     = htons(40);
            pip4->iph_chksum  = 0;
            ptcp->tcph_seqnum = 0;
            ptcp->tcph_acknum = htonl(ACK);
            pcap_hdr.caplen   = 40;
            pcap_hdr.len      = 40;
            pcap_hdr.ts       = ts;
            // TODO: calculate checksums before send
            pcap_dump((u_char*)mDumper, &pcap_hdr, buffer);
        } while (fragmented);
        // store TCP ACK and SEQ values for using in next flows
        if (request) {
            mTCPCtx->mReqSEQ  = SEQ;
            mTCPCtx->mRespACK = ACK;
        } else {
            mTCPCtx->mRespSEQ = SEQ;
            mTCPCtx->mReqACK  = ACK;
        }
    } // dump()

private:
    pcap_t * mHandle;
    pcap_dumper_t * mDumper;
    in_addr mIPv4Srv, mIPv4Cli;
    in6_addr mIPv6Srv, mIPv6Cli;
    bool mUseIPv4, mUseIPv6;
    u_int16_t mPortSrv;
    u_int16_t mPortCli;
    struct TCPContext {
        u_int32_t mReqSEQ;
        u_int32_t mReqACK;
        u_int32_t mRespSEQ;
        u_int32_t mRespACK;
    };
    typedef std::shared_ptr<TCPContext> PTCPContext;
    std::map<std::string, PTCPContext> mTCPseqs;
    PTCPContext mTCPCtx;
}; // PCapDumper

} // namespace op
