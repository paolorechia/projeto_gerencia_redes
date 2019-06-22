#ifndef MYGLOBALROUTEMANAGER_HPP
#define MYGLOBALROUTEMANAGER_HPP

#include "MyGlobalRouteManagerImpl.hpp"
//#include "ns3/global-route-manager-impl.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulation-singleton.h"
#include "ns3/global-route-manager.h"

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE( "MyGlobalRouteManager" );

// ==================================================================== //

class MyGlobalRouteManager{
	public:
		static uint32_t AllocateRouterId();
		static void DeleteGlobalRoutes();
		static void BuildGlobalRoutingDatabase();
		static void InitializeRoutes();

	private:
		MyGlobalRouteManager( GlobalRouteManager& srm );
		MyGlobalRouteManager& operator=( GlobalRouteManager& srm );
};

// ==================================================================== //

void MyGlobalRouteManager::DeleteGlobalRoutes(){
  NS_LOG_FUNCTION_NOARGS();

  SimulationSingleton<MyGlobalRouteManagerImpl>::Get()->DeleteGlobalRoutes();
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManager::BuildGlobalRoutingDatabase(){
  NS_LOG_FUNCTION_NOARGS();

  SimulationSingleton<MyGlobalRouteManagerImpl>::Get()->BuildGlobalRoutingDatabase();
}

// -------------------------------------------------------------------- //

void MyGlobalRouteManager::InitializeRoutes(){
  NS_LOG_FUNCTION_NOARGS();

  SimulationSingleton<MyGlobalRouteManagerImpl>::Get()->InitializeRoutes();
}

// -------------------------------------------------------------------- //

uint32_t MyGlobalRouteManager::AllocateRouterId(){
  NS_LOG_FUNCTION_NOARGS ();

  static uint32_t routerId = 0;
  return routerId++;
}

// ==================================================================== //

}

#endif
