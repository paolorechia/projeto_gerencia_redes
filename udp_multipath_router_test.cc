/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ssid.h"

#define PACKET_INTERVAL 0.0001

// Network Topology
//
//                        Wifi 10.1.3.0
//                   AP
//                   *          *
//       10.1.1.0    |          |
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MultipathUdpRouterTest");

// function from ns3 docs
Ptr<YansWifiPhy>
 GetYansWifiPhyPtr (const NetDeviceContainer &nc)
 {
   Ptr<WifiNetDevice> wnd = nc.Get (0)->GetObject<WifiNetDevice> ();
   Ptr<WifiPhy> wp = wnd->GetPhy ();
   return wp->GetObject<YansWifiPhy> ();
 }


int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 3;
//  uint32_t nWifi = 3;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  nCsma = nCsma == 0 ? 1 : nCsma;
  
  // Setup point-to-point nodes
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  // Setup CSMA nodes 
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // Setup AP Node and Wifi Channel
  NodeContainer wifiApNode = p2pNodes.Get (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  Ssid ssid = Ssid ("ns-3-ssid");
  WifiMacHelper mac;
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);


  // Setup Station nodes
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NodeContainer wifiStaNodes;
  // wifiStaNodes.Create (nWifi); // for now no extra wifi nodes
  wifiStaNodes.Add ( csmaNodes.Get (3) );

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  // With 1 antena and 20MHz channel... date rate of 72 megabits per second
  Ptr<YansWifiPhy> phySta = GetYansWifiPhyPtr ( staDevices.Get ( 0 ) );
  NS_ASSERT ( phySta->GetChannelNumber () == 1 );
  NS_ASSERT ( phySta->GetChannelWidth () == 20 );
  NS_ASSERT ( phySta->GetFrequency () == 2412 );


  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);
  // Redundant installs, but our topology may change, be aware!
//  stack.Install (wifiApNode);
//  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;
  // Point to point network
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);
  // CSMA network
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
  // Wifi network
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer apWifiInterfaces;
  apWifiInterfaces = address.Assign (apDevices);

  Ipv4InterfaceContainer staWifiInterfaces;
  staWifiInterfaces = address.Assign (staDevices);

  //int maxPackets = 500000;
  int maxPackets = 2;
  // Sender Client 1
  UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue ( maxPackets ));
  echoClient.SetAttribute ("Interval", TimeValue ( Seconds ( PACKET_INTERVAL ) ) );
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Sender Client 2
  UdpEchoClientHelper echoClient2 (p2pInterfaces.GetAddress (1), 10);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (2));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (0.01)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps = echoClient2.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.5));
  clientApps.Stop (Seconds (10.0));

  // Sender Client 3
  UdpEchoClientHelper echoClient3 (p2pInterfaces.GetAddress (1), 11);
  echoClient3.SetAttribute ("MaxPackets", UintegerValue (2));
  echoClient3.SetAttribute ("Interval", TimeValue (Seconds (0.01)));
  echoClient3.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps = echoClient3.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (3.0));
  clientApps.Stop (Seconds (10.0));

  // Receiving Client 1
  UdpEchoServerHelper echoServer1 (31);
  ApplicationContainer serverApps = echoServer1.Install (csmaNodes.Get (2));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Receiving Client 2
  UdpEchoServerHelper echoServer2 (32);
  serverApps = echoServer2.Install (csmaNodes.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Receiving Client 3
  UdpEchoServerHelper echoServer3 (33);
  serverApps = echoServer3.Install (wifiStaNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Setup Udp Multipath Router
  Ptr<UdpMultipathRouter> routingApp = CreateObject<UdpMultipathRouter> ();

  routingApp->channelTable.AddChannelEntry( 0, 100 ); // CSMA Channel
  routingApp->channelTable.AddChannelEntry( 1, 72 );  // Wi Fi 2.4 GHZ Channel

  routingApp->CreatePath(
                          p2pInterfaces.GetAddress ( 0 ),  // source address
                          9,                               // source port
                          csmaInterfaces.GetAddress (2),   // destination address
                          31,                              // destination port
                          0,                               // destination node id
                          0                                // channel id
                        );

  routingApp->CreatePath(
                          p2pInterfaces.GetAddress ( 0 ),  // source address
                          10,                              // source port
                          csmaInterfaces.GetAddress(3),    // destination address
                          32,                              // destination port
                          1,                               // destination node id
                          0                                // channel id
                        );

  routingApp->CreatePath( 
                        p2pInterfaces.GetAddress ( 0 ),  // source address
                        11,                              // source port
                        staWifiInterfaces.GetAddress(0), // destination address
                        33,                              // destination port
                        1,                               // destination node id
                        1                              // channel id
                       );

  p2pNodes.Get (1)->AddApplication(routingApp);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
              "MinX", DoubleValue (0.0), "MinY", DoubleValue (0.0),"DeltaX", DoubleValue (5.0), "DeltaY", DoubleValue (10.0),
               "GridWidth", UintegerValue (5), "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (p2pNodes);
  mobility.Install (csmaNodes);
  mobility.Install (wifiStaNodes);
  mobility.Install (wifiApNode);

  Simulator::Stop (Seconds (8.0));

  AnimationInterface anim  ("second_test.xml");
  anim.SetConstantPosition( p2pNodes.Get(0), 0, 5);
  anim.SetConstantPosition( p2pNodes.Get(1), 5, 5);
  anim.SetConstantPosition( csmaNodes.Get(1), 10, 0);
  anim.SetConstantPosition( csmaNodes.Get(2), 10, 5);
  anim.SetConstantPosition( csmaNodes.Get(3), 10, 10);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
