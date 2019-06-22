#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  /* Configuration. */

  /* Build nodes. */
  NodeContainer term_0;
  term_0.Create (1);
  NodeContainer term_1;
  term_1.Create (1);
  NodeContainer term_2;
  term_2.Create (1);
  NodeContainer router_0;
  router_0.Create (1);
  NodeContainer station_0;
  station_0.Create (1);
  NodeContainer ap_0;
  ap_0.Create (1);

  /* Build link. */
  CsmaHelper csma_hub_0;
  csma_hub_0.SetChannelAttribute ("DataRate", DataRateValue (100000000));
  csma_hub_0.SetChannelAttribute ("Delay",  TimeValue (MilliSeconds (10000)));
  YansWifiPhyHelper wifiPhy_ap_0 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel_ap_0 = YansWifiChannelHelper::Default ();
  wifiPhy_ap_0.SetChannel (wifiChannel_ap_0.Create ());
  PointToPointHelper p2p_p2p_0;
  p2p_p2p_0.SetDeviceAttribute ("DataRate", DataRateValue (100000000));
  p2p_p2p_0.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10000)));

  /* Build link net device container. */
  NodeContainer all_hub_0;
  all_hub_0.Add (router_0);
  all_hub_0.Add (term_2);
  all_hub_0.Add (term_0);
  all_hub_0.Add (term_1);
  NetDeviceContainer ndc_hub_0 = csma_hub_0.Install (all_hub_0);
  NodeContainer all_ap_0;
  NetDeviceContainer ndc_ap_0;
  Ssid ssid_ap_0 = Ssid ("wifi-default-0");
  WifiHelper wifi_ap_0 = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac_ap_0 = NqosWifiMacHelper::Default ();
  wifi_ap_0.SetRemoteStationManager ("ns3::ArfWifiManager");
  wifiMac_ap_0.SetType ("ns3::ApWifiMac", 
     "Ssid", SsidValue (ssid_ap_0), 
     "BeaconGeneration", BooleanValue (true),
     "BeaconInterval", TimeValue (Seconds (2.5)));
  ndc_ap_0.Add (wifi_ap_0.Install (wifiPhy_ap_0, wifiMac_ap_0, ap_0));
  wifiMac_ap_0.SetType ("ns3::StaWifiMac",
     "Ssid", SsidValue (ssid_ap_0), 
     "ActiveProbing", BooleanValue (false));
  ndc_ap_0.Add (wifi_ap_0.Install (wifiPhy_ap_0, wifiMac_ap_0, all_ap_0 ));
  MobilityHelper mobility_ap_0;
  mobility_ap_0.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap_0.Install (ap_0);
  mobility_ap_0.Install(all_ap_0);
  NodeContainer all_p2p_0;
  all_p2p_0.Add (router_0);
  NetDeviceContainer ndc_p2p_0 = p2p_p2p_0.Install (all_p2p_0);

  /* Install the IP stack. */
  InternetStackHelper internetStackH;
  internetStackH.Install (term_0);
  internetStackH.Install (term_1);
  internetStackH.Install (term_2);
  internetStackH.Install (router_0);
  internetStackH.Install (station_0);
  internetStackH.Install (ap_0);

  /* IP assign. */
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_hub_0 = ipv4.Assign (ndc_hub_0);
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_ap_0 = ipv4.Assign (ndc_ap_0);
  ipv4.SetBase ("10.0.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_p2p_0 = ipv4.Assign (ndc_p2p_0);

  /* Generate Route. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Generate Application. */

  /* Simulation. */
  /* Pcap output. */
  /* Stop the simulation after x seconds. */
  uint32_t stopTime = 1;
  Simulator::Stop (Seconds (stopTime));
  /* Start and clean simulation. */
  Simulator::Run ();
  Simulator::Destroy ();
}
