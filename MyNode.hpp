#ifndef MYNODE_H
#define MYNODE_H

#include "Device.hpp"


// ==================================================================== //

class MyNode{
	public:
		Ptr<Node> ns3node;											// Aponta para o endereço do nodo do simulador ns3
		List<Device> devices;										// Lista de dispositivos de transmissão que este nodo possui

	public:
		MyNode();
		void create( Ptr<Node> _ns3node );
		void free();
		void addDevice( Ptr<NetDevice> device, const char* device_type, Ipv4Address device_ipv4, Ptr<QueueDisc> _device_queue );
		Device& getDevice( unsigned int num );
		unsigned int num_of_devices();
};

// -------------------------------------------------------------------- //

MyNode::MyNode(){
	ns3node = NULL;
}

// -------------------------------------------------------------------- //

void MyNode::create( Ptr<Node> _ns3node ){

	this->free();
	ns3node = _ns3node;
}

// -------------------------------------------------------------------- //

void MyNode::free(){

	devices.Free();
	ns3node = NULL;
}

// -------------------------------------------------------------------- //

inline void MyNode::addDevice( Ptr<NetDevice> device, const char* device_type, Ipv4Address device_ipv4, Ptr<QueueDisc> _device_queue ){
	devices[ devices.Quant ].create( device, device_type, device_ipv4, _device_queue );
}

// -------------------------------------------------------------------- //

inline Device& MyNode::getDevice( unsigned int num ){
	return( devices[ num ] );
}

// -------------------------------------------------------------------- //

inline unsigned int MyNode::num_of_devices(){
	return( devices.Quant );
}

// ==================================================================== //

#endif

