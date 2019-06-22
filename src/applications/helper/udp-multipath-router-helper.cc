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
 *
 * Author: Paolo
 */
#include "udp-multipath-router-helper.h"
#include "ns3/udp-multipath-router.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

UdpMultipathRouterHelper::UdpMultipathRouterHelper (uint16_t iport_0, uint16_t iport_1, uint16_t sport_0, uint16_t sport_1)
{
  m_factory.SetTypeId (UdpMultipathRouter::GetTypeId ());
  SetAttribute ("IncomingPort0", UintegerValue (iport_0));
  SetAttribute ("IncomingPort1", UintegerValue (iport_1));
  SetAttribute ("SendingPort0", UintegerValue (sport_0));
  SetAttribute ("SendingPort1", UintegerValue (sport_1));
}

void 
UdpMultipathRouterHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpMultipathRouterHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpMultipathRouterHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpMultipathRouterHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpMultipathRouterHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpMultipathRouter> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
