/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2019 - Paolo, Eric 
 *
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

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "udp-multipath-router.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpMultipathRouterApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpMultipathRouter);

/* LinkTable methods */
LinkTableEntry::LinkTableEntry(uint32_t capacity, uint32_t use, Time measure, uint32_t counter)
{
  link_capacity = capacity;
  current_use = use;
  last_measure = Simulator::Now();
  packet_counter  = counter;
}
LinkTable::LinkTable()
{
}

/* NodeTable methods */
NodeTableEntry::NodeTableEntry(uint32_t node, Ipv4Address addr, 
                     uint16_t port, Ptr<Socket> socket,
                     uint32_t link)
{
  node_id = node;
  dest_addr = addr;
  dest_port = port;
  dest_socket = socket;
  link = link_id;
}
NodeTable::NodeTable()
{
}
/* PathTable methods */
PathTableEntry::PathTableEntry(Ipv4Address addr, uint16_t port, uint32_t node)
{
  src_addr = addr;
  src_port = port;
  node_id = node;
}
PathTable::PathTable()
{
}
/* UdpMultipathRouter methods */
TypeId
UdpMultipathRouter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpMultipathRouter")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpMultipathRouter> ()
    .AddAttribute ("IncomingPort0", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&UdpMultipathRouter::m_incoming_port_0),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("IncomingPort1", "Port on which we listen for incoming packets.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&UdpMultipathRouter::m_incoming_port_1),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SendingPort0", "Port on which we retransmit received packets.",
                   UintegerValue (11),
                   MakeUintegerAccessor (&UdpMultipathRouter::m_sending_port_0),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SendingPort1", "Port on which we retransmit received packets.",
                   UintegerValue (12),
                   MakeUintegerAccessor (&UdpMultipathRouter::m_sending_port_1),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SendingAddress0", "Temporary destination Address ",
                   AddressValue (),
                   MakeAddressAccessor (&UdpMultipathRouter::m_sending_address_0),
                   MakeAddressChecker ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpMultipathRouter::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpMultipathRouter::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UdpMultipathRouter::UdpMultipathRouter ()
{
  NS_LOG_FUNCTION (this);
  m_incoming_socket_0 = 0;
  m_incoming_socket_1 = 0;
  m_sending_socket_0 = 0;
  m_sending_socket_1 = 0;
  m_sendEvent = EventId ();
}

UdpMultipathRouter::UdpMultipathRouter (uint16_t iport_0, uint16_t iport_1, uint16_t sport_0, uint16_t sport_1)
{
  NS_LOG_FUNCTION (this);
  m_incoming_port_0 = iport_0;
  m_incoming_port_1 = iport_1;
  m_incoming_socket_0 = 0;
  m_incoming_socket_1 = 0;
  m_sending_socket_0 = 0;
  m_sending_socket_1 = 0;
  m_sendEvent = EventId ();
}

UdpMultipathRouter::~UdpMultipathRouter()
{
  NS_LOG_FUNCTION (this);
}

void
UdpMultipathRouter::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
  /*
  LinkTable::DoDispose ();
  PathTable::DoDispose ();
  NodeTable::DoDispose ();
  */
}

Ptr<Socket>
UdpMultipathRouter::initReceivingSocket (Ptr<Socket> socket, uint16_t m_port)
{
  if (socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }
  NS_LOG_INFO("Initialized receiving socket..." << socket);
  socket->SetRecvCallback (MakeCallback (&UdpMultipathRouter::HandleRead, this));
  return socket;
}

Ptr<Socket>
UdpMultipathRouter::initSendingSocket (Ptr<Socket> socket, uint16_t m_port, Address addr)
{
  NS_LOG_FUNCTION (this);
  if (socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(addr) == true)
        {
          if (socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(addr), m_port);
          socket->Connect (socket_addr);
          NS_LOG_INFO ("Initialized sending socket with IP: " << socket_addr);
          NS_LOG_INFO ("Another format: " << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << addr);
        }
    }
  NS_LOG_INFO("Initialized sending socket..." << socket);
  socket->SetAllowBroadcast (true);
  return socket;
}

void
UdpMultipathRouter::closeReceivingSocket(Ptr<Socket> socket)
{
  if (socket != 0) 
    {
      socket->Close ();
      socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void 
UdpMultipathRouter::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_incoming_socket_0 = UdpMultipathRouter::initReceivingSocket (m_incoming_socket_0, m_incoming_port_0); 
  m_incoming_socket_1 = UdpMultipathRouter::initReceivingSocket (m_incoming_socket_1, m_incoming_port_1); 
  m_sending_socket_0 = UdpMultipathRouter::initSendingSocket (m_sending_socket_0, m_sending_port_0, m_sending_address_0); 
  m_sending_socket_1 = UdpMultipathRouter::initSendingSocket (m_sending_socket_1, m_sending_port_1, m_sending_address_1); 
  NS_LOG_INFO("Initialized socket addresses..." << m_incoming_socket_0 << m_incoming_socket_1 << m_sending_socket_0 << m_sending_socket_1);
}

void 
UdpMultipathRouter::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  UdpMultipathRouter::closeReceivingSocket (m_incoming_socket_0);
  UdpMultipathRouter::closeReceivingSocket (m_incoming_socket_1);
}

void 
UdpMultipathRouter::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s router received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else {
        NS_ASSERT_MSG (false, "Incompatible address type: " << from);
      }
      // TODO: actually use the tables to route packet to correct destination
      // for now just sending to hardcoded destination address for testing Send Function
      // (This is essentially a routing table)
      UdpMultipathRouter::RoutePacket(packet->GetSize(), from);
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();
  }
}
void
UdpMultipathRouter::RoutePacket (uint32_t packet_size, Address from)
{
      NS_LOG_INFO("Routing packet to destination... TODO details");
      NS_LOG_INFO("Testing stored addresses...");
      CheckIpv4(m_sending_address_0, m_sending_port_0);
      CheckIpv4(m_sending_address_1, m_sending_port_1);
      uint16_t recv_port = InetSocketAddress::ConvertFrom (from).GetPort ();
      Address tmp_addr;
      Ptr<Socket> tmp_socket;
      uint16_t send_port;
      if (recv_port == 9)
        {
        tmp_addr = Address(m_sending_address_0);
        tmp_socket = m_sending_socket_0;
        send_port = m_sending_port_0;
        }
      else {
        tmp_addr = Address(m_sending_address_1);
        tmp_socket = m_sending_socket_1;
        send_port = m_sending_port_1;
      }
      NS_LOG_INFO("Testing chosen address...");
      CheckIpv4(tmp_addr, send_port);
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s router schedules transmition of " << packet_size << " bytes to somewhere...");
      UdpMultipathRouter::Send(packet_size, m_sending_socket_0, tmp_addr, send_port);
//      UdpMultipathRouter::ScheduleTransmit (Simulator::Now (), packet_size, tmp_socket, tmp_addr, send_port);
}


void 
UdpMultipathRouter::ScheduleTransmit (Time dt, uint32_t p, Ptr<Socket> s, Address addr, uint16_t port)
{ 
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &UdpMultipathRouter::Send, this, p, s, addr, port);
}

void 
UdpMultipathRouter::SetRemote0 (Address ip, uint16_t port)
{
  if (Ipv4Address::IsMatchingType(ip) != true) {
    NS_ASSERT_MSG (false, "Incompatible address type: " << ip);
  }
  UdpMultipathRouter::CheckIpv4(ip, port);
  m_sending_address_0 = Address(ip);
  m_sending_port_0 = port;
  NS_LOG_INFO ("Remote 0 IP: "<< ip << " Port: " << port);
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(m_sending_address_0), m_sending_port_0);
  NS_LOG_INFO ("Remote 0 IPv4:" << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
  //NS_LOG_INFO ("Stored remote 0 as " << InetSocketAddress::ConvertFrom (m_sending_address_0).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_sending_address_0).GetPort ());
}

void 
UdpMultipathRouter::SetRemote1 (Address ip, uint16_t port)
{
  if (Ipv4Address::IsMatchingType(ip) != true) {
    NS_ASSERT_MSG (false, "Incompatible address type: " << ip);
  }
  NS_LOG_INFO ("Setting remote 1 IP: "<< ip << " Port: " << port);
  m_sending_address_1 = Address(ip);
  m_sending_port_1 = port;
  NS_LOG_INFO ("Remote 1 IP: "<< ip << " Port: " << port);
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(m_sending_address_1), m_sending_port_1);
  NS_LOG_INFO ("Remote 1 IPv4:" << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
}

//TODO finish this part
void 
UdpMultipathRouter::Send (uint32_t packet_size, Ptr<Socket> socket, Address dest_addr, uint16_t dest_port)
{
  Ptr<Packet> packet = Create<Packet>(packet_size);
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Entering Send... dest_addr is: " << dest_addr);
  NS_LOG_INFO("Testing chosen address...");
  CheckIpv4(dest_addr, dest_port);
  NS_LOG_INFO("Trying to access local address...");
  Address localAddress;
  NS_LOG_INFO("Socket... " << m_sending_socket_0);
  NS_LOG_INFO("Socket... " << m_sending_socket_1);
  NS_LOG_INFO("Socket... " << m_incoming_socket_0);
  NS_LOG_INFO("Socket... " << m_incoming_socket_1);
  socket->GetSockName (localAddress);
  NS_LOG_INFO("Socket... " << socket);
  socket->GetSockName (localAddress);
  // TODO: check what packet tags are about in the docs
 // m_txTrace (packet);
  if (Ipv4Address::IsMatchingType (dest_addr))
    {
  //    m_txTraceWithAddresses (packet, localAddress, InetSocketAddress (Ipv4Address::ConvertFrom (dest_addr), dest_port));
    NS_LOG_INFO("Trying to send...");
    socket->Send (packet);
   // NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " << Ipv4Address::ConvertFrom (dest_addr) << " port " << dest_port);
    }
}

void
UdpMultipathRouter::CheckIpv4(Address ipv4address, uint16_t m_port) {
  if (Ipv4Address::IsMatchingType(ipv4address) != true) {
    NS_ASSERT_MSG (false, "Incompatible address type: " << ipv4address);
  }
  NS_LOG_INFO ("Checking IP: "<< ipv4address);
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(ipv4address), m_port);
  NS_LOG_INFO ("IPv4: " << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
}

} // Namespace ns3
