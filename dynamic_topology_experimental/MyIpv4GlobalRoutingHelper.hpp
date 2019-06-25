#ifndef MYIPV4GLOBALROUTINGHELPER_HPP
#define MYIPV4GLOBALROUTINGHELPER_HPP

#include "MyGlobalRouteManager.hpp"

#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/global-router-interface.h"
#include "ns3/ipv4-global-routing.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/log.h"

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE( "MyIpv4GlobalRoutingHelper" );

// ==================================================================== //

class MyIpv4GlobalRoutingHelper : public Ipv4RoutingHelper{
	public:
		MyIpv4GlobalRoutingHelper();
		MyIpv4GlobalRoutingHelper( const MyIpv4GlobalRoutingHelper& );

		MyIpv4GlobalRoutingHelper* Copy() const;
	  virtual Ptr<Ipv4RoutingProtocol> Create( Ptr<Node> node ) const;
	  static void PopulateRoutingTables();
	  static void RecomputeRoutingTables();

	private:
	  MyIpv4GlobalRoutingHelper& operator=( const MyIpv4GlobalRoutingHelper& );
};

// ==================================================================== //

MyIpv4GlobalRoutingHelper::MyIpv4GlobalRoutingHelper(){

}

// -------------------------------------------------------------------- //

MyIpv4GlobalRoutingHelper::MyIpv4GlobalRoutingHelper( const MyIpv4GlobalRoutingHelper& o ){

}

// -------------------------------------------------------------------- //

MyIpv4GlobalRoutingHelper* MyIpv4GlobalRoutingHelper::Copy() const{

  return new MyIpv4GlobalRoutingHelper(*this);
}

// -------------------------------------------------------------------- //

Ptr<Ipv4RoutingProtocol> MyIpv4GlobalRoutingHelper::Create( Ptr<Node> node ) const{

  NS_LOG_LOGIC( "Adding GlobalRouter interface to node " << node->GetId() );

  Ptr<GlobalRouter> globalRouter = CreateObject<GlobalRouter>();
  node->AggregateObject( globalRouter );

  NS_LOG_LOGIC( "Adding GlobalRouting Protocol to node " << node->GetId() );
  Ptr<Ipv4GlobalRouting> globalRouting = CreateObject<Ipv4GlobalRouting>();
  globalRouter->SetRoutingProtocol( globalRouting );

  return globalRouting;
}

// -------------------------------------------------------------------- //

void MyIpv4GlobalRoutingHelper::PopulateRoutingTables(){

  MyGlobalRouteManager::BuildGlobalRoutingDatabase ();
  MyGlobalRouteManager::InitializeRoutes ();
}

// -------------------------------------------------------------------- //

void MyIpv4GlobalRoutingHelper::RecomputeRoutingTables(){

  MyGlobalRouteManager::DeleteGlobalRoutes ();
  MyGlobalRouteManager::BuildGlobalRoutingDatabase ();
  MyGlobalRouteManager::InitializeRoutes ();
}

// ==================================================================== //

}

#endif
