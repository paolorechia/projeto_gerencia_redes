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

#define NODE_ERROR 16666
#define CHANNEL_TABLE_REFRESH_RATE 0.1

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpMultipathRouterApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpMultipathRouter);

/* ChannelTable methods */
ChannelTableEntry::ChannelTableEntry ( uint32_t id, uint32_t capacity )
{
  channel_id = id;
  channel_capacity = capacity;
  current_use = 0;
  last_measure = Simulator::Now();
  byte_counter = 0;
  dropped_packets = 0;
  byte_counter_sum = 0;
  dropped_packets_sum = 0;
  // data rate in mbps * 1024 = data rate in kbps
  // kbps / 8 = KB/s
  // multiplied by second fraction
  // yields max kilobits per refresh_rate, the desired drop threshold
  drop_threshold =  (capacity * 1024 / 8) * CHANNEL_TABLE_REFRESH_RATE;
}
ChannelTable::ChannelTable()
{
}
void
ChannelTable::AddChannelEntry ( uint32_t id, uint32_t capacity )  {
  entries.push_back( ChannelTableEntry ( id, capacity ) );
}

// TODO: Maybe use mutex here
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
  std::list<ChannelTableEntry>::iterator log_it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    // Do time diff
    double time_diff = current_time.GetSeconds() -  (*it).last_measure.GetSeconds();
    // Compute use in last time interval
    // Converts kilobyte counter to kilobits and then to megabits
    // uint64_t use_in_last_interval = (((*it).byte_counter * 8) / 1024) / time_diff;
    // Average last measure with current measure
    // (*it).current_use = ( (*it).current_use + use_in_last_interval ) / 2;
    (*it).current_use = (((*it).byte_counter * 8) / 1024) / time_diff;
    // update last_measure
    (*it).last_measure = current_time;
    // schedule new update
  }
  LogChannelTable ();
  for (it = entries.begin(); it != entries.end(); ++it) {
    (*it).byte_counter_sum += (*it).byte_counter;
    (*it).dropped_packets_sum += (*it).dropped_packets;
    // reset byte_counter
    (*it).byte_counter = 0;
    (*it).dropped_packets = 0;
  }
  ChannelTable::ScheduleChannelTableUpdate( Seconds ( CHANNEL_TABLE_REFRESH_RATE ) );
}

void 
ChannelTable::ScheduleChannelLog( ) {
//  LogChannelTable ();
  Simulator::Schedule ( Seconds ( 1.0 ), &ChannelTable::ScheduleChannelLog, this );
}

void
ChannelTable::LogChannelTable( ) {
  std::list<ChannelTableEntry>::iterator it;
    NS_LOG_INFO( "===========================================" );
    NS_LOG_INFO( "ChannelTable at time: " << Simulator::Now() );
    NS_LOG_INFO( 
  "| id | \tcapacity| \tuse | \tlast_measure |" "\t kilobyte_counter | \t packet_loss | "
  << "\t total_byte_count | \t total_dropped_packets |"
                );
  for (it = entries.begin(); it != entries.end(); ++it) {
    NS_LOG_INFO(
                 "|#ID:" << (*it).channel_id << "|\t" << (*it).channel_capacity  << "\t|\t"
                      << (*it).current_use << "|" << (*it).last_measure << "|\t" << (*it).byte_counter
                      << "\t" << (*it).dropped_packets << "\t" << (*it).byte_counter_sum
                      << "\t" << (*it).dropped_packets_sum << "|"
                );
  }
}

void
ChannelTable::ScheduleChannelTableUpdate ( Time dt ) {
  Simulator::Schedule ( dt, &ChannelTable::UpdateChannelsCurrentUse, this );
}

uint32_t
ChannelTable::GetChannelAvailableCapacity(uint32_t channel_id)
{
  std::list<ChannelTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ((*it).channel_id == channel_id) {
      return (*it).channel_capacity > (*it).current_use ? ( (*it).channel_capacity - (*it).current_use ) : 0;
    }
  }
  return 0;
}

uint32_t
ChannelTable::GetAvailableBytes(uint32_t channel_id)
{
  std::list<ChannelTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ((*it).channel_id == channel_id) {
      return (*it).drop_threshold > (*it).byte_counter ? ( (*it).drop_threshold- (*it).byte_counter) : 0;
    }
  }
  return 0;
}

void
ChannelTable::AddDroppedPacket(uint32_t channel_id)
{
  std::list<ChannelTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ((*it).channel_id == channel_id) {
      (*it).dropped_packets+= 1;
      return;
    }
  }
}

/* NodeTable methods */
NodeTableEntry::NodeTableEntry(uint32_t node, Address addr, 
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
uint32_t
NodeTable::FindSocketChannel( Ptr<Socket> socket )
{
  std::list<NodeTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ((*it).dest_socket == socket) {
      return (*it).channel_id;
    }
  }
  return NODE_ERROR;
}
void
NodeTable::AddNodeEntry ( uint32_t node, Address addr, uint16_t port, Ptr<Socket> socket, uint32_t channel)
{
  entries.push_back( NodeTableEntry ( node, addr, port, socket, channel ) );
}
std::list<NodeTableEntry>
NodeTable::GetAvailableChannels ( uint32_t id )
{
  std::list<NodeTableEntry> availableEntries;
  std::list<NodeTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ( (*it).node_id == id ) {
      availableEntries.push_back( ( *it ) );
      NS_LOG_LOGIC( "Found available channel id " << id << " for node " << id);
    }
  }
  return availableEntries;
}
void
NodeTable::LogNodeTable( ) {
  std::list<NodeTableEntry>::iterator it;
  NS_LOG_INFO( "===========================================" );
  NS_LOG_INFO( "NodeTable at time: " << Simulator::Now() );
  NS_LOG_INFO( "| node_id | dest_addr | dest_port | dest_socket | channel_id |" );
  for (it = entries.begin(); it != entries.end(); ++it) {
    InetSocketAddress socket_addr =
      InetSocketAddress (Ipv4Address::ConvertFrom((*it).dest_addr), (*it).dest_port);
    NS_LOG_INFO(   "|" << (*it).node_id
                << "|" << socket_addr.GetIpv4 ()
                << "|" << (*it).dest_port
                << "|" << (*it).dest_socket
                << "|" << (*it).channel_id  << "|" );
  }
  NS_LOG_INFO( "===========================================" );
}
NodeTableEntry
NodeTable::ChooseBestPath ( std::list<NodeTableEntry> available_pathes, BalancingAlgorithm algorithm, ChannelTable channelTable) {
  std::list<NodeTableEntry>::iterator it;
  it = available_pathes.begin();
  switch (algorithm) {
    case BalancingAlgorithm::NO_BALANCING: {
      return (*it);
    }
    case BalancingAlgorithm::TX_RATE: {
      uint32_t best_capacity = 0;
      NodeTableEntry bestPath= (*it);
      for (it = available_pathes.begin(); it != available_pathes.end(); ++it) {
        uint32_t channel_capacity = channelTable.GetChannelAvailableCapacity( (*it).channel_id );
        if ( channel_capacity > best_capacity ) {
          NS_LOG_LOGIC( " Best capacity " << channel_capacity << " channel id: " << (*it).channel_id);
          bestPath = (*it);
          best_capacity = channel_capacity;
        }
      }
      return bestPath;
    }
    case BalancingAlgorithm::TX_DROP_THRESHOLD: {
      uint32_t maximum = 0;
      NodeTableEntry bestPath= (*it);
      for (it = available_pathes.begin(); it != available_pathes.end(); ++it) {
        uint32_t available_bytes = channelTable.GetAvailableBytes( (*it).channel_id );
        if ( available_bytes > maximum) {
          NS_LOG_LOGIC( " Maximum " << available_bytes << " channel id: " << (*it).channel_id);
          bestPath = (*it);
          maximum = available_bytes;
        }
      }
      return bestPath;
      break;
    }
    default:
      NS_ASSERT_MSG (false, " Balancing Algorithm not implemented ");
  }
  return (*it);
}
/* PathTable methods */
PathTableEntry::PathTableEntry(Address addr, uint16_t port, uint32_t node, Ptr<Socket> socket)
{
  src_addr = addr;
  src_port = port;
  node_id = node;
  src_socket = socket;
}
PathTable::PathTable()
{
}
void
PathTable::AddPathTableEntry( Address src_addr, uint16_t src_port, uint32_t node_id, Ptr<Socket> socket )  {
  entries.push_back( PathTableEntry( src_addr, src_port, node_id, socket ) );
}
uint32_t
PathTable::FindDestinationNodeForPath ( Ipv4Address src_addr, uint16_t src_port ) {
  std::list<PathTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    InetSocketAddress socket_addr =
      InetSocketAddress (Ipv4Address::ConvertFrom((*it).src_addr), (*it).src_port);
    if (socket_addr.GetIpv4 () == src_addr && (*it).src_port == src_port) {
      return (*it).node_id;
    }
  }
  return NODE_ERROR;
}
void
PathTable::LogPathTable( ) {
  std::list<PathTableEntry>::iterator it;
  NS_LOG_INFO( "===========================================" );
  NS_LOG_INFO( "PathTable at time: " << Simulator::Now() );
  NS_LOG_INFO( "| src_addr | src_port | node_id |" );
  for (it = entries.begin(); it != entries.end(); ++it) {
    InetSocketAddress socket_addr =
      InetSocketAddress (Ipv4Address::ConvertFrom((*it).src_addr), (*it).src_port);
    NS_LOG_INFO(
                     "|" << socket_addr.GetIpv4 ()
                  << "|" << (*it).src_port
                  << "|" << (*it).node_id << "|" 
               );
  }
  NS_LOG_INFO( "===========================================" );
}


uint16_t 
PathTable::FindPortFromSocket ( Ptr<Socket> socket ) {
  std::list<PathTableEntry>::iterator it;
  for (it = entries.begin(); it != entries.end(); ++it) {
    if ((*it).src_socket == socket) return (*it).src_port;
  }
  return 0;
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
  m_sendEvent = EventId ();
  balancingAlgorithm = BalancingAlgorithm::TX_RATE;
  dropMode = DropMode::TX_RATE;
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
UdpMultipathRouter::initReceivingSockets( ) {
  std::list<PathTableEntry>::iterator it;
  for ( it = pathTable.entries.begin(); it != pathTable.entries.end(); ++it ) {
    (*it).src_socket = UdpMultipathRouter::initReceivingSocket( (*it).src_socket, (*it).src_port);
  }
}

void
UdpMultipathRouter::initSendingSockets ( ) {
  std::list<NodeTableEntry>::iterator it;
  for ( it = nodeTable.entries.begin(); it != nodeTable.entries.end(); ++it ) {
    (*it).dest_socket = UdpMultipathRouter::initSendingSocket ( (*it).dest_socket,
                                                                (*it).dest_port, (*it).dest_addr ); 
  }
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
UdpMultipathRouter::closeReceivingSockets( ) {
  std::list<PathTableEntry>::iterator it;
  for ( it = pathTable.entries.begin(); it != pathTable.entries.end(); ++it ) {
    UdpMultipathRouter::closeReceivingSocket((*it).src_socket);
  }
}

void 
UdpMultipathRouter::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  UdpMultipathRouter::initReceivingSockets ( );
  UdpMultipathRouter::initSendingSockets ( );
  channelTable.ScheduleChannelTableUpdate( Seconds ( 1.0 ) );
  channelTable.ScheduleChannelLog( );
  nodeTable.LogNodeTable();
  pathTable.LogPathTable();
}

void 
UdpMultipathRouter::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  UdpMultipathRouter::closeReceivingSockets ( );
}

void
UdpMultipathRouter::SetLoadBalancing ( BalancingAlgorithm algorithm)
{
  UdpMultipathRouter::balancingAlgorithm = algorithm; 
};

void
UdpMultipathRouter::SetDropMode ( DropMode drop)
{
  UdpMultipathRouter::dropMode = drop; 
};


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
      NS_LOG_LOGIC("Routing packet to destination... ");
//      uint16_t connection_port = InetSocketAddress::ConvertFrom (from).GetPort ();
      uint16_t listen_port;
      // TODO: build a better way to find the socket listening port
      // Problem: using socket memory address as comparison and not a proper value for this
      listen_port = pathTable.FindPortFromSocket(socket);
      if (listen_port == 0 )
        {
          NS_ASSERT_MSG (false, "Could not find listen port");
        }
      NS_LOG_LOGIC("Found listen port: " << listen_port);
      // TODO: finish routing here
      uint32_t node_id = pathTable.FindDestinationNodeForPath( InetSocketAddress::ConvertFrom(from).GetIpv4( ),
                                                               listen_port );
      NS_ASSERT_MSG( (node_id != NODE_ERROR), "Could not find node to route to!" );
      NS_LOG_LOGIC("Found node ID: " << node_id);
      std::list<NodeTableEntry> available_channels = nodeTable.GetAvailableChannels ( node_id );
      NS_LOG_LOGIC("Found " << available_channels.size() << " available channels ");
      NodeTableEntry chosenPath = nodeTable.ChooseBestPath( available_channels,
                                                            UdpMultipathRouter::balancingAlgorithm,
                                                            channelTable );
      // Packet loss mechanism
      uint32_t drop_test = 0;
      switch (UdpMultipathRouter::dropMode) {
        case DropMode::NO_DROPPING: {
              drop_test = 1;
              break;
        }
        case DropMode::TX_RATE: {
          drop_test = channelTable.GetChannelAvailableCapacity(chosenPath.channel_id);
          NS_LOG_LOGIC (" Available capacity: " << drop_test);
          break;
        }
        case DropMode::TX_DROP_THRESHOLD: {
          drop_test = channelTable.GetAvailableBytes(chosenPath.channel_id);
          NS_LOG_LOGIC (" Available bytes : " << drop_test);
          break;
        }
        default:
          NS_ASSERT_MSG (false, "Invalid drop mode" );
      }
      
      if (drop_test == 0) {
        // Dropped Packet
        NS_LOG_LOGIC("Dropped packet... " << chosenPath.channel_id);
        channelTable.AddDroppedPacket( chosenPath.channel_id );
      } else {
      NS_LOG_LOGIC("Picked channel... " << chosenPath.channel_id);
      NS_LOG_LOGIC("Testing destination address and port");
      CheckIpv4(chosenPath.dest_addr, chosenPath.dest_port);
      NS_LOG_LOGIC (
                    "At time " << Simulator::Now ().GetSeconds () << " routing of packet (" << packet_size 
                    << " bytes) from " << InetSocketAddress::ConvertFrom(from).GetIpv4() 
                    << " port: " << listen_port
                    << " to "  << Ipv4Address::ConvertFrom (chosenPath.dest_addr) 
                    << " port: " << chosenPath.dest_port
                   );
      UdpMultipathRouter::Send (packet_size, chosenPath.dest_socket, chosenPath.dest_addr, chosenPath.dest_port);
//      UdpMultipathRouter::ScheduleTransmit (Simulator::Now (), packet_size, tmp_socket, tmp_addr, send_port);
      }
}


void 
UdpMultipathRouter::ScheduleTransmit (Time dt, uint32_t p, Ptr<Socket> s, Address addr, uint16_t port)
{ 
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &UdpMultipathRouter::Send, this, p, s, addr, port);
}

void
UdpMultipathRouter::CreatePath ( Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port,
                                  uint32_t node_id, uint32_t channel_id )
{
  UdpMultipathRouter::CheckIpv4(source_ip, source_port);
  UdpMultipathRouter::CheckIpv4(dest_ip, dest_port);
  nodeTable.AddNodeEntry(node_id, dest_ip, dest_port, 0, channel_id); // null socket
  pathTable.AddPathTableEntry(source_ip, source_port, node_id, 0); // null socket
}

void 
UdpMultipathRouter::Send (uint32_t packet_size, Ptr<Socket> socket, Address dest_addr, uint16_t dest_port)
{
  Ptr<Packet> packet = Create<Packet>(packet_size);
  // TODO: Find correct channel for this socket
  uint32_t channel_id = nodeTable.FindSocketChannel( socket );
  if (channel_id == NODE_ERROR) {
    NS_ASSERT_MSG (false, "Router could not find channel id for sending socket!!");
  }
  channelTable.UpdateChannelByteCounter(channel_id, packet_size / 1024);
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
}


} // Namespace ns3
