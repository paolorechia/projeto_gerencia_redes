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
                   /*
    .AddAttribute ("SendingPort2", "Port on which we retransmit received packets.",
                   UintegerValue (13),
                   MakeUintegerAccessor (&UdpMultipathRouter::m_sending_port_2),
                   MakeUintegerChecker<uint16_t> ())
                   */
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

void
UdpMultipathRouter::initReceivingSocket (Ptr<Socket> m_socket, uint16_t m_port)
{
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
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
  m_socket->SetRecvCallback (MakeCallback (&UdpMultipathRouter::HandleRead, this));
}

void
UdpMultipathRouter::closeReceivingSocket(Ptr<Socket> m_socket)
{
  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void 
UdpMultipathRouter::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  UdpMultipathRouter::initReceivingSocket(m_incoming_socket_0, m_incoming_port_0); 
  UdpMultipathRouter::initReceivingSocket(m_incoming_socket_1, m_incoming_port_1); 
}

void 
UdpMultipathRouter::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  UdpMultipathRouter::closeReceivingSocket(m_incoming_socket_0);
  UdpMultipathRouter::closeReceivingSocket(m_incoming_socket_1);
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
//      RoutePacket(packet, socket);
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();
  }
}
void
UdpMultipathRouter::RoutePacket (Ptr<Packet> packet, Ptr<Socket> socket)
{

      NS_LOG_INFO("Routing packet to destination");
      /*
      socket->SendTo (packet, 0, from);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s router sent " << packet->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      */
}


void 
UdpMultipathRouter::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &UdpMultipathRouter::Send, this);
}

void 
UdpMultipathRouter::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "UdpEchoClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "UdpEchoClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that it doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding attribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }
  Address localAddress;
  m_socket->GetSockName (localAddress);
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      m_txTraceWithAddresses (p, localAddress, InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      m_txTraceWithAddresses (p, localAddress, Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
    }
  m_socket->Send (p);
  ++m_sent;

  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Inet6SocketAddress::ConvertFrom (m_peerAddress).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }

  if (m_sent < m_count) 
    {
      ScheduleTransmit (m_interval);
    }
}

} // Namespace ns3
