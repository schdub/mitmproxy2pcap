# mitmproxy2pcap

Utility that able to parse mitmproxy flows file (modified netstring serialization format), build IP, TCP and HTTP packets for each request and response and output it in .pcap file 

# Dependency

libpcap - Packet Capture library
https://www.tcpdump.org/manpages/pcap.3pcap.html

Installing under Debian-based Linux distros is trivial:
```
sudo apt install libpcap-dev
```

# Building using CMake
```
mkdir build && cd $_ && cmake .. && make -j4
```
# Building using QMake
```
qmake && make -j4
```
# Usage
```
mitmproxy2pcap v0.1.20.12 (c) Oleg V. Polivets, 2018.
mitmproxy flow files converter to pcap.

mitmproxy2pcap [OPTIONS] path_to_input_file

OPTIONS:
--print  - just print json representation of parsed flows and exit.
--help   - this output.
```
