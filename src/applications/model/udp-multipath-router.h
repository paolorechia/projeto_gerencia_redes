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

// #define NUMBER_PORTS 4

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

class SocketWrapper
{
public:
  SocketWrapper();
  SocketWrapper(Ptr<Socket> socket, uint16_t listen_port);
  Ptr<Socket> socket;
  uint16_t listen_port;
private:
};

class ChannelTableEntry
{
public:
  ChannelTableEntry (uint32_t link_id, uint32_t capacity);
  uint32_t channel_id;
  uint32_t channel_capacity; // megabits/s
  uint32_t current_use;      // megabits/s
  Time last_measure;
  uint32_t byte_counter; // counts in kilobytes
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
  void AddNodeEntry( uint32_t node, Address addr, uint16_t port, Ptr<Socket> dest_socket, uint32_t channel_id );
  std::list<NodeTableEntry> GetAvailableChannels ( uint32_t node_id );
  void LogNodeTable( void );

private:
  std::list<NodeTableEntry> entries;
  
};

class PathTableEntry
{
public:
  PathTableEntry(Address addr, uint16_t port, uint32_t node);

private:
  Address src_addr;
  uint16_t src_port;
  uint32_t node_id;
};

class PathTable
{
public:
  PathTable ();
  void AddPathTableEntry( Address src_addr, uint16_t src_port, uint32_t node_id );
private:
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
  void SetPath0 (Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port);
  void SetPath1 (Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port);
  void SetPath2 (Address source_ip, uint16_t source_port, Address dest_ip, uint16_t dest_port);
  
  // Tables
  ChannelTable channelTable;
  NodeTable nodeTable;
  PathTable pathTable;

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void closeReceivingSocket(Ptr<Socket> m_socket);
  Ptr<Socket> initReceivingSocket (Ptr<Socket> m_socket, uint16_t m_port);
  Ptr<Socket> initSendingSocket (Ptr<Socket> m_socket, uint16_t m_port, Address address);

  void RoutePacket (uint32_t packet_size, Address address, Ptr<Socket> socket);

  void CheckIpv4 (Address ipv4address, uint16_t m_port);
  
  SocketWrapper incoming_sw_0; //!< IPv4 Socket
  SocketWrapper incoming_sw_1; //!< IPv4 Socket
  SocketWrapper incoming_sw_2; //!< IPv4 Socket
  std::list<SocketWrapper> incoming_sw_list;

  void Send (uint32_t packet_size, Ptr<Socket> m_socket, Address dest_addr, uint16_t dest_port);
  void ScheduleTransmit (Time dt, uint32_t packet_size, Ptr<Socket> s, Address addr, uint16_t port);

  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time

  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint32_t m_sent; //!< Counter for sent packets

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

