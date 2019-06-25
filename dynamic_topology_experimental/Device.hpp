#ifndef DEVICE_H
#define DEVICE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include "List.hpp"

using namespace ns3;

const char* CSMA_DEVICE = "csma";
const char* P2P_DEVICE = "p2p";
const char* WIFI_DEVICE = "wifi";
const char* WIFIM_DEVICE = "wifim";

// ==================================================================== //

class Device{
	public:
		Ptr<NetDevice>				device_ptr;			// Apontador para o dispositivo
		const char* 					device_type;		// Indica os tipos das tecnologias
		Ipv4Address						device_ipv4;		// Indica os endereços das tecnologias
		Ptr<QueueDisc>				device_queue;		// Mede a quantidade de dados transmitidos pelo dispositivo
		//ApplicationContainer	SinkApp;				// Variáveis que medem o trafego de mensagens pelo nodo

	public:
		Device();

		void create( Ptr<NetDevice> _device, const char* _device_type, Ipv4Address _device_ipv4, Ptr<QueueDisc> _device_queue );
		Ptr<NetDevice> device();
		const char* type();
		Ipv4Address& address();

		uint32_t receivedPackets();
		uint64_t receivedBytes();
};

// -------------------------------------------------------------------- //

Device::Device(){
	memset( this, 0, sizeof( Device ) );
}

// -------------------------------------------------------------------- //

void Device::create( Ptr<NetDevice> _device, const char* _device_type, Ipv4Address _device_ipv4, Ptr<QueueDisc> _device_queue ){
	device_ptr  = _device;
	device_type = _device_type;
	device_ipv4 = _device_ipv4;
	device_queue = _device_queue;
}

// -------------------------------------------------------------------- //

inline Ptr<NetDevice> Device::device(){
	return( device_ptr );
}

// -------------------------------------------------------------------- //

inline const char* Device::type(){
	return( device_type );
}

// -------------------------------------------------------------------- //

inline Ipv4Address& Device::address(){
	return( device_ipv4 );
}

// -------------------------------------------------------------------- //

inline uint32_t Device::receivedPackets(){
	return( device_queue->GetStats().nTotalReceivedBytes );
}

// -------------------------------------------------------------------- //

inline uint64_t Device::receivedBytes(){
	return( device_queue->GetStats().nTotalReceivedPackets );
}

// ==================================================================== //

#endif

