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

/* SocketWrapper */
SocketWrapper::SocketWrapper()
{
}

SocketWrapper::SocketWrapper ( Ptr<Socket> s, uint16_t p )
{
  socket = s;
  listen_port = p;
}

/* ChannelTable methods */
ChannelTableEntry::ChannelTableEntry ( uint32_t id, uint32_t capacity )
{
  channel_id = id;
  channel_capacity = capacity;
  current_use = 0;
  last_measure = Simulator::Now();
  byte_counter = 0;
}
ChannelTable::ChannelTable()
{
}
void
ChannelTable::AddChannelEntry ( uint32_t id, uint32_t capacity )  {
  entries.push_back( ChannelTableEntry ( id, capacity ) );
}

// TODO: use Mutex here
void
ChannelTable::UpdateChannelByteCounter( uint32_t id, uint32_t routed_bytes ) {
  NS_LOG_LOGIC(" Updating byte counter " );
  std::list<ChannelTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ( (*it).channel_id == id ) {
      NS_LOG_LOGIC( " Found channel id " << id );
      (*it).byte_counter += routed_bytes;
      return;
    }
  }
  NS_LOG_LOGIC( " Did not find channel id " << id );
}

void
ChannelTable::UpdateChannelsCurrentUse( ) {
  Time current_time = Simulator::Now();
  std::list<ChannelTableEntry>::iterator it;
  ChannelTable::LogChannelTable ( );
  for (it = entries.begin(); it != entries.end(); ++it) {
    // Do time diff
    double time_diff = current_time.GetSeconds() -  (*it).last_measure.GetSeconds();
    // Compute actual use
    // Converts kilobyte counter to kilobits and then to megabits
    (*it).current_use = (((*it).byte_counter * 8) / 1024) / time_diff; // Time diff should be 1s;
    // reset byte_counter
    (*it).byte_counter = 0;
    // update last_measure
    (*it).last_measure = current_time;
    // schedule new update
  }
  ChannelTable::LogChannelTable ( );
  ChannelTable::ScheduleChannelTableUpdate( Seconds ( 1.0 ) );
}

void
ChannelTable::LogChannelTable( ) {
  std::list<ChannelTableEntry>::iterator it;
    NS_LOG_INFO( "===========================================" );
    NS_LOG_INFO( "ChannelTable at time: " << Simulator::Now() );
  for (it = entries.begin(); it != entries.end(); ++it) {
    NS_LOG_INFO( "| id | \tcapacity| \tuse | \tlast_measure | \t kilobyte_counter " );
    NS_LOG_INFO( "| " << (*it).channel_id << "|\t" << (*it).channel_capacity  << "\t|\t"
                    << (*it).current_use << "|" << (*it).last_measure << "|\t" << (*it).byte_counter );
  }
}

void
ChannelTable::ScheduleChannelTableUpdate ( Time dt ) {
  Simulator::Schedule ( dt, &ChannelTable::UpdateChannelsCurrentUse, this );
}

/* NodeTable methods */
NodeTableEntry::NodeTableEntry(uint32_t node, Ipv4Address addr, 
                     uint16_t port, Ptr<Socket> socket,
                     uint32_t channel)
{
  node_id = node;
  dest_addr = addr;
  dest_port = port;
  dest_socket = socket;
  channel_id = channel;
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
  incoming_sw_0.socket = 0;
  incoming_sw_1.socket = 0;
  incoming_sw_2.socket = 0;
  m_sending_socket_0 = 0;
  m_sending_socket_1 = 0;
  m_sending_socket_2 = 0;
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
  ChannelTable::DoDispose ();
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
          NS_LOG_INFO ("Initialized sending socket with IP: " << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
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
  incoming_sw_0.socket = UdpMultipathRouter::initReceivingSocket( incoming_sw_0.socket, incoming_sw_0.listen_port); 
  incoming_sw_1.socket = UdpMultipathRouter::initReceivingSocket (incoming_sw_1.socket, incoming_sw_1.listen_port); 
  incoming_sw_2.socket = UdpMultipathRouter::initReceivingSocket (incoming_sw_2.socket, incoming_sw_2.listen_port); 
  m_sending_socket_0 = UdpMultipathRouter::initSendingSocket (m_sending_socket_0, m_sending_port_0, m_sending_address_0); 
  m_sending_socket_1 = UdpMultipathRouter::initSendingSocket (m_sending_socket_1, m_sending_port_1, m_sending_address_1); 
  m_sending_socket_2 = UdpMultipathRouter::initSendingSocket (m_sending_socket_2, m_sending_port_2, m_sending_address_2); 
  NS_LOG_INFO("Initialized socket addresses..." << incoming_sw_0.socket << incoming_sw_1.socket << m_sending_socket_0 << m_sending_socket_1 << incoming_sw_2.socket << m_sending_socket_2);
  channelTable.ScheduleChannelTableUpdate( Seconds ( 1.0 ) );
}

void 
UdpMultipathRouter::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  UdpMultipathRouter::closeReceivingSocket (incoming_sw_0.socket);
  UdpMultipathRouter::closeReceivingSocket (incoming_sw_1.socket);
  UdpMultipathRouter::closeReceivingSocket (incoming_sw_1.socket);
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
          NS_LOG_LOGIC ("At time " << Simulator::Now ().GetSeconds () 
            << "s router received " << packet->GetSize () << " bytes from "
            << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port "
            << InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else {
        NS_ASSERT_MSG (false, "Incompatible address type: " << from);
      }
      // TODO: find a way to get the listen port here
      UdpMultipathRouter::RoutePacket(packet->GetSize(), from, socket);
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();
  }
}
void
UdpMultipathRouter::RoutePacket (uint32_t packet_size, Address from, Ptr<Socket> socket)
{
      NS_LOG_LOGIC("Routing packet to destination... TODO details");
//      uint16_t connection_port = InetSocketAddress::ConvertFrom (from).GetPort ();
      Address tmp_addr;
      Ptr<Socket> tmp_socket;
      uint16_t send_port;
      uint16_t listen_port;
      // TODO: build a better way to find the socket listening port)
      // Problem: using socket memory address as comparison and not a proper value for this
      // Planned to do (search in a list)
      if (socket == incoming_sw_0.socket)
        {
          listen_port = incoming_sw_0.listen_port;
        }
      else if (socket == incoming_sw_1.socket)
        {
          listen_port = incoming_sw_1.listen_port;
        }
      else if (socket == incoming_sw_2.socket)
        {
          listen_port = incoming_sw_2.listen_port;
        }
      else
        {
          NS_ASSERT_MSG (false, "Could not find listen port");
        }
      
      // TODO: actually use the tables to route packet to correct destination
      // for now just sending to hardcoded destination address for testing Send Function
      // (This is essentially a routing table)
      if (listen_port == 9)
        {
          tmp_addr = Address(m_sending_address_0);
          tmp_socket = m_sending_socket_0;
          send_port = m_sending_port_0;
        }
      else if (listen_port == 10)
        {
          tmp_addr = Address(m_sending_address_1);
          tmp_socket = m_sending_socket_1;
          send_port = m_sending_port_1;
        }
      else if (listen_port == 11 )
        {
          tmp_addr = Address(m_sending_address_2);
          tmp_socket = m_sending_socket_2;
          send_port = m_sending_port_2;
        }


      CheckIpv4(tmp_addr, send_port);
      NS_LOG_LOGIC ("At time " << Simulator::Now ().GetSeconds () << " routing of packet (" << packet_size 
                    << " bytes) from " << InetSocketAddress::ConvertFrom(from).GetIpv4() 
                    << " port: " << listen_port
                    << " to "  << Ipv4Address::ConvertFrom (tmp_addr) << " port: " << send_port);
      UdpMultipathRouter::Send (packet_size, tmp_socket, tmp_addr, send_port);
//      UdpMultipathRouter::ScheduleTransmit (Simulator::Now (), packet_size, tmp_socket, tmp_addr, send_port);
}


void 
UdpMultipathRouter::ScheduleTransmit (Time dt, uint32_t p, Ptr<Socket> s, Address addr, uint16_t port)
{ 
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &UdpMultipathRouter::Send, this, p, s, addr, port);
}

void 
UdpMultipathRouter::SetPath0(Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port)
{
  UdpMultipathRouter::CheckIpv4(source_ip, source_port);
  UdpMultipathRouter::CheckIpv4(dest_ip, dest_port);
  incoming_sw_0.listen_port = source_port;
  m_sending_address_0 = Address(dest_ip);
  m_sending_port_0 = dest_port;
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(m_sending_address_0), m_sending_port_0);
  NS_LOG_INFO ("Remote 0 IPv4:" << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
}

void
UdpMultipathRouter::SetPath1(Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port)
{
  UdpMultipathRouter::CheckIpv4(source_ip, source_port);
  UdpMultipathRouter::CheckIpv4(dest_ip, dest_port);
  incoming_sw_1.listen_port = source_port;
  m_sending_address_1 = Address(dest_ip);
  m_sending_port_1 = dest_port;
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(m_sending_address_1), m_sending_port_1);
  NS_LOG_INFO ("Remote 1 IPv4:" << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
}

void
UdpMultipathRouter::SetPath2(Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port)
{
  UdpMultipathRouter::CheckIpv4(source_ip, source_port);
  UdpMultipathRouter::CheckIpv4(dest_ip, dest_port);
  incoming_sw_2.listen_port = source_port;
  m_sending_address_2 = Address(dest_ip);
  m_sending_port_2 = dest_port;
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(m_sending_address_2), m_sending_port_2);
  NS_LOG_INFO ("Remote 2 IPv4:" << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
}

//TODO finish this part
void 
UdpMultipathRouter::Send (uint32_t packet_size, Ptr<Socket> socket, Address dest_addr, uint16_t dest_port)
{
  Ptr<Packet> packet = Create<Packet>(packet_size);
  channelTable.UpdateChannelByteCounter(0, packet_size / 1024);
  NS_LOG_FUNCTION (this);
  CheckIpv4(dest_addr, dest_port);
  Address localAddress;
  socket->GetSockName (localAddress);
  // TODO: check what packet tags are about in the docs
 // m_txTrace (packet);
  if (Ipv4Address::IsMatchingType (dest_addr))
    {
  //    m_txTraceWithAddresses (packet, localAddress, InetSocketAddress (Ipv4Address::ConvertFrom (dest_addr), dest_port));
    socket->Send (packet);
    NS_LOG_LOGIC ("At time " << Simulator::Now ().GetSeconds () << "s router sent " << packet_size << " bytes to " << Ipv4Address::ConvertFrom (dest_addr) << " port " << dest_port);
    }
}

void
UdpMultipathRouter::CheckIpv4(Address ipv4address, uint16_t m_port) {
  if (Ipv4Address::IsMatchingType(ipv4address) != true) {
    NS_ASSERT_MSG (false, "Incompatible address type: " << ipv4address);
  }
  /*
  NS_LOG_INFO ("Checking IP: "<< ipv4address);
  InetSocketAddress socket_addr = InetSocketAddress (Ipv4Address::ConvertFrom(ipv4address), m_port);
  NS_LOG_INFO ("IPv4: " << InetSocketAddress::ConvertFrom (socket_addr).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (socket_addr).GetPort ());
  */
}


} // Namespace ns3
