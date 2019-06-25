### UdpMultipathRouter

Para rodar o código no ns 3-29:


```
cp -r src/applications/* [caminho_instalacao_ns3]/ns-allinone-3.29/ns-3.29/src/applications/
cp udp_multipath_router_test.cc [caminho_instalacao_ns3]/ns-allinone-3.29/ns3-29/scratch

cd [caminho_instalacao_ns3]/ns-allinone-3.29/ns3-29/
export NS_LOG=UdpMultipathRouterApplication=level_info
./waf --run scratch/udp_multipath_router_test
```
