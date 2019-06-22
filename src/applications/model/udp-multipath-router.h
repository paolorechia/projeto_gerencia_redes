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

#define NUMBER_PORTS 4

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Socket;
class Packet;

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

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void closeReceivingSocket(Ptr<Socket> m_socket);
  void initReceivingSocket (Ptr<Socket> m_socket, uint16_t m_port);

  uint16_t m_incoming_port_0; //!< Port on which we listen for incoming packets.
  uint16_t m_incoming_port_1; //!< Port on which we listen for incoming packets.
  uint16_t m_sending_port_0; //!< Port on which we retransmit packets.
  uint16_t m_sending_port_1; //!< Port on which we retransmit packets.
//  uint16_t m_sending_port_2; //!< Port on which we retransmit packets.
  Ptr<Socket> m_incoming_socket_0; //!< IPv4 Socket
  Ptr<Socket> m_incoming_socket_1; //!< IPv4 Socket
  Ptr<Socket> m_sending_socket_0; //!< IPv4 Socket
  Ptr<Socket> m_sending_socket_1; //!< IPv4 Socket
//  Ptr<Socket> m_sending_socketsocket_3; //!< IPv4 Socket
  Address m_local; //!< local multicast address

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;
  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_MULTIPATH_ROUTER */

