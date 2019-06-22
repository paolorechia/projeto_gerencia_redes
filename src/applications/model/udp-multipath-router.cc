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
                   UintegerValue (12),
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
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();
/*
      NS_LOG_LOGIC ("Echoing packet");
      socket->SendTo (packet, 0, from);

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
*/
  }
}

} // Namespace ns3
