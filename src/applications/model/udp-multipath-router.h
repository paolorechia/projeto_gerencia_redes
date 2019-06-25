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

#ifndef UDP_MULTIPATH_ROUTER
#define UDP_MULTIPATH_ROUTER

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include <list>
#include <iterator>

namespace ns3 {

class Socket;
class Packet;
class Time;

enum class BalancingAlgorithm { NO_BALANCING, TX_RATE, TX_DROP_THRESHOLD };
enum class DropMode { NO_DROPPING, TX_RATE, TX_DROP_THRESHOLD };

class ChannelTableEntry
{
public:
  ChannelTableEntry (uint32_t link_id, uint32_t capacity);
  uint32_t channel_id;
  uint32_t channel_capacity; // megabits/s
  uint32_t current_use;      // megabits/s
  Time last_measure;         // should update every second
  uint32_t byte_counter;     // counts in kilobytes
  uint64_t drop_threshold;   // not used (yet) - number of sent kilobytes before dropping
  uint32_t dropped_packets;  // packet loss (usually kilobytes)
  uint64_t byte_counter_sum; // keep byte counter history
  uint64_t dropped_packets_sum; // keep dropped packets history
};

class ChannelTable
{
public:
  ChannelTable ();
  void AddChannelEntry (uint32_t id, uint32_t capacity);
  void UpdateChannelByteCounter (uint32_t id, uint32_t routed_bytes);
  void UpdateChannelsCurrentUse();
  void LogChannelTable () ;
  void ScheduleChannelTableUpdate( Time dt );
  void ScheduleChannelLog( );
  uint32_t GetChannelAvailableCapacity(uint32_t channel_id);
  uint32_t GetAvailableBytes(uint32_t channel_id);
  void AddDroppedPacket(uint32_t channel_id);

private:
  std::list<ChannelTableEntry> entries;
};

class NodeTableEntry
{
public:
  NodeTableEntry(uint32_t node, Address addr, 
                     uint16_t port, Ptr<Socket> socket,
                     uint32_t link);
  uint32_t node_id;
  Address dest_addr;
  uint16_t dest_port;
  Ptr<Socket> dest_socket;
  uint32_t channel_id;
};


class NodeTable
{
public:
  NodeTable ();
  uint32_t FindSocketChannel( Ptr<Socket> ); // returns channel_id
  void AddNodeEntry( uint32_t node, Address addr, uint16_t port, Ptr<Socket> dest_socket, uint32_t channel_id );
  std::list<NodeTableEntry> GetAvailableChannels ( uint32_t node_id );
  void LogNodeTable( void );
  NodeTableEntry ChooseBestPath ( std::list<NodeTableEntry>, BalancingAlgorithm algorithm, ChannelTable channelTable );
  std::list<NodeTableEntry> entries;
};

class PathTableEntry
{
public:
  PathTableEntry(Address addr, uint16_t port, uint32_t node, Ptr<Socket> socket);
  Address src_addr;
  uint16_t src_port;
  uint32_t node_id;
  Ptr<Socket> src_socket;
};

class PathTable
{
public:
  PathTable ();
  void AddPathTableEntry( Address src_addr, uint16_t src_port, uint32_t node_id, Ptr<Socket> socket);
  uint32_t FindDestinationNodeForPath( Ipv4Address src_addr, uint16_t src_port );
  void LogPathTable( void );
  uint16_t FindPortFromSocket ( Ptr<Socket> socket );
  std::list<PathTableEntry> entries;
};

/**
 * \ingroup applications 
 * \defgroup udpmultipathrouter
 */

/**
 * \ingroup udpmultipathrouter 
 * \brief A Udp Multipath Router
 *
 * Every packet received is routed to another node 
 */


class UdpMultipathRouter : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  UdpMultipathRouter ();
  virtual ~UdpMultipathRouter ();
  void CreatePath (Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port,
                   uint32_t node_id, uint32_t channel_id);
  void SetLoadBalancing( BalancingAlgorithm algorithm );
  void SetDropMode ( DropMode drop_mode);
  // Tables
  ChannelTable channelTable;
  ChannelTable historicChannelTable; // Used for logging purposes only
  NodeTable nodeTable;
  PathTable pathTable;

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void closeReceivingSocket(Ptr<Socket> m_socket);
  void closeReceivingSockets (void);
  Ptr<Socket> initReceivingSocket (Ptr<Socket> m_socket, uint16_t m_port);
  void initReceivingSockets (void);
  Ptr<Socket> initSendingSocket (Ptr<Socket> m_socket, uint16_t m_port, Address address);
  void initSendingSockets (void);

  void RoutePacket (uint32_t packet_size, Address address, Ptr<Socket> socket);

  void CheckIpv4 (Address ipv4address, uint16_t m_port);

  void Send (uint32_t packet_size, Ptr<Socket> m_socket, Address dest_addr, uint16_t dest_port);
  void ScheduleTransmit (Time dt, uint32_t packet_size, Ptr<Socket> s, Address addr, uint16_t port);

  BalancingAlgorithm balancingAlgorithm; 
  DropMode dropMode; 
  EventId m_sendEvent; //!< Event to send the next packet
//  Ptr<Socket> m_sending_socketsocket_3; //!< IPv4 Socket
  Address m_local; //!< local multicast address

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;
  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;
  /// Callbacks for tracing the packet Tx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_MULTIPATH_ROUTER */

