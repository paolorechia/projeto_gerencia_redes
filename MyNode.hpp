#ifndef MYNODE_H
#define MYNODE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
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

class MyNode{
	public:
		Ptr<Node> ns3node;
		unsigned int devices;
		List<const char*> devices_types;
		List<Ipv4Address> devices_ipv4;

	public:
		MyNode();
		void create( Ptr<Node> _ns3node );
		void free();
		void addDevice( const char* device_types, Ipv4Address device_ipv4 );
		const char* DeviceType( unsigned int device_num );
		Ipv4Address& DeviceAddress( unsigned int device_num );
};

// -------------------------------------------------------------------- //

MyNode::MyNode(){
	ns3node = NULL;
	devices = 0;
}

// -------------------------------------------------------------------- //

void MyNode::create( Ptr<Node> _ns3node ){

	this->free();
	ns3node = _ns3node;
}

// -------------------------------------------------------------------- //

void MyNode::free(){

	devices_types.Free();
	devices_ipv4.Free();
	devices = 0;
	ns3node = NULL;
}

// -------------------------------------------------------------------- //

void MyNode::addDevice( const char* device_types, Ipv4Address device_ipv4 ){

	devices_types << device_types;
	devices_ipv4 << device_ipv4;
	devices++;
}

// -------------------------------------------------------------------- //

inline const char* MyNode::DeviceType( unsigned int device_num ){

	return( devices_types[ device_num ] );
}

// -------------------------------------------------------------------- //

inline Ipv4Address& MyNode::DeviceAddress( unsigned int device_num ){

	return( devices_ipv4[ device_num ] );
}

// ==================================================================== //

#endif

