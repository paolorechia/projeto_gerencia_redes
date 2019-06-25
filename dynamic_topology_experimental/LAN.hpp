#ifndef LAN_HPP
#define LAN_HPP

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

#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/dsdv-helper.h"

#include "ns3/animation-interface.h"
#include "MyIpv4GlobalRoutingHelper.hpp"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/olsr-helper.h"
#include "Basic.hpp"

using namespace ns3;

const char* CSMA_DEVICE  = "csma";
const char* P2P_DEVICE   = "p2p";
const char* WIFI_DEVICE  = "wifi";

#define GLOBAL_ROUTING			0
#define AODV_ROUTING				1
#define OLSR_ROUTING				2
#define DSDV_ROUTING				4
#define NIX_ROUTING					8
#define STATIC_ROUTING			16

// ==================================================================== //

class Device{
	public:
		Uint									Node;					// Número do nodo ao qual está associado
		Ptr<NetDevice>				DevicePtr;		// Apontador para o dispositivo
		const char* 					DeviceType;		// Indica os tipos das tecnologias
		Ipv4Address						DeviceIpv4;		// Indica os endereços das tecnologias
		Ptr<QueueDisc>				DeviceQueue;	// Mede a quantidade de dados transmitidos pelo dispositivo

	public:
		Device(){
			memset( this, 0 , sizeof( Device ) );
		}
};

// -------------------------------------------------------------------- //

class LAN{
	public:
		NodeContainer ns3nodes;
		Uint N_Nodes;
		Device* Devices;
		Uint N_Devices;
		Uint Links;

	  PointToPointHelper p2p;
		CsmaHelper csma;
		WifiHelper wifi;
		YansWifiPhyHelper wifiPhy;
	  YansWifiChannelHelper wifiChannel;
	  WifiMacHelper wifiMac;

		Ssid ssid;
		MobilityHelper mobility;
	  TrafficControlHelper tch;
	  Ipv4AddressHelper ipv4_n;

	  AnimationInterface *Anim;

	public:
		LAN();

		Ptr<Node> GetNode( Uint Nn );
		Uint      Nodes_Len();

		Device&   GetDevice( Uint Nn );
		Uint      Devices_Len();

		Uint N_NodeDevices( Uint Num );

    int  read( const char* path );
    int  read( const char* path, Uint Flags );
		void p2p_connect( NodeContainer tempNodes );
		void csma_connect( NodeContainer tempNodes );
		void wifi_connect( NodeContainer tempNodes );
		void info();

		void setEcho( Uint Quant, double Time );
};

// ==================================================================== //

LAN::LAN(){

	Devices = new Device[ 512 ];
	if( !Devices ){
		puts( "Falha de alocação!!" );
		exit( 0 );
	}
	N_Devices = N_Nodes = 0;
	Links = 0;
	
  Time::SetResolution( Time::NS );
  LogComponentEnable( "UdpEchoClientApplication", LOG_LEVEL_INFO );
  LogComponentEnable( "UdpEchoServerApplication", LOG_LEVEL_INFO );

  p2p.SetDeviceAttribute( "DataRate", StringValue( "5Mbps" ) );
  p2p.SetChannelAttribute( "Delay", StringValue( "2ms" ) );
  p2p.SetQueue( "ns3::DropTailQueue", "MaxSize", StringValue( "1p" ) );

  csma.SetChannelAttribute( "DataRate", StringValue( "100Mbps" ) );
  csma.SetChannelAttribute( "Delay", TimeValue( NanoSeconds( 6560 ) ) );
  csma.SetQueue( "ns3::DropTailQueue", "MaxSize", StringValue( "1p" ) );

  wifi.SetStandard( WIFI_PHY_STANDARD_80211b );

  wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.Set( "RxGain", DoubleValue( 0 ) );
  wifiPhy.SetPcapDataLinkType( WifiPhyHelper::DLT_IEEE802_11_RADIO );

  wifiChannel.SetPropagationDelay( "ns3::ConstantSpeedPropagationDelayModel" );
  wifiChannel.AddPropagationLoss( "ns3::FixedRssLossModel","Rss",DoubleValue( -80 ) );
  wifiPhy.SetChannel( wifiChannel.Create() );

  wifi.SetRemoteStationManager( "ns3::ConstantRateWifiManager", "DataMode", StringValue( "DsssRate1Mbps" ), "ControlMode", StringValue( "DsssRate1Mbps" ) );
  wifiMac.SetType ("ns3::AdhocWifiMac");

  tch.SetRootQueueDisc( "ns3::RedQueueDisc" );

  ssid = Ssid( "ns-3-ssid" );

  mobility.SetPositionAllocator( "ns3::GridPositionAllocator",
                                 "MinX", DoubleValue( 0.0 ),
                                 "MinY", DoubleValue( 0.0 ),
                                 "DeltaX", DoubleValue( 5.0 ),
                                 "DeltaY", DoubleValue( 10.0 ),
                                 "GridWidth", UintegerValue( 5 ),
                                 "LayoutType", StringValue( "RowFirst" ) );

	Anim = NULL;
}

// -------------------------------------------------------------------- //

Ptr<Node> LAN::GetNode( Uint Nn ){

	if( Nn >= this->Nodes_Len() )
		return( NULL );

	return( ns3nodes.Get( Nn ) );
}

// -------------------------------------------------------------------- //

inline Uint LAN::Nodes_Len(){

	return( N_Nodes );
}

// -------------------------------------------------------------------- //

inline Device& LAN::GetDevice( Uint Nn ){

	return( Devices[ Nn ] );
}

// -------------------------------------------------------------------- //

inline Uint LAN::Devices_Len(){

	return( N_Devices );
}

// -------------------------------------------------------------------- //

Uint LAN::N_NodeDevices( Uint Num ){
	Uint Nn, Nm;

	Nm = 0;
	for( Nn = 0 ; Nn < N_Devices ; Nn++ )
		if( Devices[ Nn ].Node == Num )
			Nm++;

	return( Nm );
}

// -------------------------------------------------------------------- //

inline int LAN::read( const char* path ){

	return( this->read( path, 0 ) );
}

// -------------------------------------------------------------------- //

int LAN::read( const char* path, Uint Flags ){
	char technology[ 16 ];
  FILE* fl = NULL;
	int NodeNum;

  if( !path )
    return( 0 );

  fl = fopen( path, "r" );
  if( !fl )
    return( 0 );

  fscanf( fl, "%u\n", &N_Nodes );
  if( !N_Nodes )
    return( 0 );

  ns3nodes.Create( N_Nodes );
  InternetStackHelper stack;

	if( Flags ){
		Ipv4ListRoutingHelper list;

		if( Flags & AODV_ROUTING ){
			AodvHelper aodv;
			list.Add( aodv, 10 );
		}

		if( Flags & OLSR_ROUTING ){
			OlsrHelper olsr;
			list.Add( olsr, 8 );
		}

		if( Flags & DSDV_ROUTING ){
			DsdvHelper dsdv;
			list.Add( dsdv, 5 );
		}

		if( Flags & NIX_ROUTING ){
			Ipv4NixVectorHelper nix;
			list.Add( nix, 4 );
		}

		if( Flags & STATIC_ROUTING ){
			Ipv4StaticRoutingHelper staticRouting;
			list.Add( staticRouting, 0 );
		}

		stack.SetRoutingHelper( list );
	}
  stack.Install( ns3nodes );

	while( !feof( fl ) ){

		memset( technology, 0, 16 );
		fscanf( fl, "%16s", technology );
		lowercase( technology );

		if( strlen( technology ) ){
			NodeContainer Temp;

			while( readint( NodeNum, fl ) )
				Temp.Add( ns3nodes.Get( NodeNum ) );
			Temp.Add( ns3nodes.Get( NodeNum ) );

			if( !strcmp( technology, P2P_DEVICE ) )
				this->p2p_connect( Temp );

			else if( !strcmp( technology, CSMA_DEVICE ) )
				this->csma_connect( Temp );

			else if( !strcmp( technology, WIFI_DEVICE ) )
				this->wifi_connect( Temp );

			else
				break;
		}
	}

	char Temp[ 128 ];
	strcpy( Temp, path );
	strcpy( Temp + strlen( Temp ) - 3, "xml" );
  Anim = new AnimationInterface( Temp );

  MyIpv4GlobalRoutingHelper::PopulateRoutingTables();

  return( 1 );
}

// -------------------------------------------------------------------- //

void LAN::p2p_connect( NodeContainer tempNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	char base[ 32 ];

	sprintf( base, "10.1.%d.0", Links );
  ipv4_n.SetBase( base, "255.255.255.0" );
	Links++;

  n_devs = p2p.Install( tempNodes );
  QueueDiscContainer qdiscs = tch.Install( n_devs );
  interfaces = ipv4_n.Assign( n_devs );

  mobility.SetMobilityModel( "ns3::ConstantPositionMobilityModel" );
  mobility.Install( tempNodes );

	for( Uint Nn = 0 ; Nn < interfaces.GetN() ; Nn++ ){
		for( Uint Nm = 0 ; Nm < ns3nodes.GetN() ; Nm++ ){
			if( ns3nodes.Get( Nm ) == tempNodes.Get( Nn ) ){
				Devices[ N_Devices ].Node = Nm;
				Devices[ N_Devices ].DevicePtr = n_devs.Get( Nn );
				Devices[ N_Devices ].DeviceType = P2P_DEVICE;
				Devices[ N_Devices ].DeviceIpv4 = interfaces.GetAddress( Nn );
				Devices[ N_Devices ].DeviceQueue = qdiscs.Get( Nn );

				N_Devices++;
			}
		}
	}
}

// -------------------------------------------------------------------- //

void LAN::csma_connect( NodeContainer tempNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	char base[ 32 ];

	sprintf( base, "10.1.%d.0", Links );
  ipv4_n.SetBase( base, "255.255.255.0" );
	Links++;

  n_devs = csma.Install( tempNodes );
  QueueDiscContainer qdiscs = tch.Install( n_devs );
  interfaces = ipv4_n.Assign( n_devs );

  mobility.SetMobilityModel( "ns3::ConstantPositionMobilityModel" );
  mobility.Install( tempNodes );

	for( Uint Nn = 0 ; Nn < interfaces.GetN() ; Nn++ ){
		for( Uint Nm = 0 ; Nm < ns3nodes.GetN() ; Nm++ ){
			if( ns3nodes.Get( Nm ) == tempNodes.Get( Nn ) ){
				Devices[ N_Devices ].Node = Nm;
				Devices[ N_Devices ].DevicePtr = n_devs.Get( Nn );
				Devices[ N_Devices ].DeviceType = CSMA_DEVICE;
				Devices[ N_Devices ].DeviceIpv4 = interfaces.GetAddress( Nn );
				Devices[ N_Devices ].DeviceQueue = qdiscs.Get( Nn );

				N_Devices++;
			}
		}
	}
}

// -------------------------------------------------------------------- //

void LAN::wifi_connect( NodeContainer tempNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	char base[ 32 ];

	static Byte Verif = 0;
	if( Verif ){
		puts( "Cannot create another wifi instance!!" );
		return;
	}
	Verif++;

	sprintf( base, "10.1.%d.0", Links );
  ipv4_n.SetBase( base, "255.255.255.0" );
	Links++;

  n_devs = wifi.Install( wifiPhy, wifiMac, tempNodes );
  QueueDiscContainer qdiscs = tch.Install( n_devs );
  interfaces = ipv4_n.Assign( n_devs );

  mobility.SetMobilityModel( "ns3::ConstantPositionMobilityModel" );
  mobility.Install( tempNodes );

	for( Uint Nn = 0 ; Nn < n_devs.GetN() ; Nn++ ){
		for( Uint Nm = 0 ; Nm < ns3nodes.GetN() ; Nm++ ){
			if( ns3nodes.Get( Nm ) == tempNodes.Get( Nn ) ){
				Devices[ N_Devices ].Node = Nm;
				Devices[ N_Devices ].DevicePtr = n_devs.Get( Nn );
				Devices[ N_Devices ].DeviceType = WIFI_DEVICE;
				Devices[ N_Devices ].DeviceIpv4 = interfaces.GetAddress( Nn );
				Devices[ N_Devices ].DeviceQueue = qdiscs.Get( Nn );

				N_Devices++;
			}
		}
	}
}

// -------------------------------------------------------------------- //

void LAN::info(){
	NodeContainer::Iterator i;
	unsigned int Nn, Nm;

	printf( "A rede possui %d nodos:\n", N_Nodes );
	for( Nn = 0 ; Nn < N_Nodes ; Nn++ ){
		Nm = this->N_NodeDevices( Nn );
    printf( "\tNodo %d possui %d conexões.\n", Nn, Nm );
		for( Nm = 0 ; Nm < N_Devices ; Nm++ ){
			if( Devices[ Nm ].Node == Nn ){
				printf( "\t\t%s : ", Devices[ Nm ].DeviceType );
				std::cout << Devices[ Nm ].DeviceIpv4 << '\n';
			}
		}
  }
}

// -------------------------------------------------------------------- //

void LAN::setEcho( Uint Quant, double Time ){

  UdpEchoServerHelper echoServer( 9 );

  ApplicationContainer serverApps = echoServer.Install( this->GetNode( 0 ) );
  serverApps.Start( Seconds( 0.0 ) );
  serverApps.Stop( Seconds( Time ) );

  UdpEchoClientHelper echoClient( this->GetDevice( 0 ).DeviceIpv4, 9 );
  echoClient.SetAttribute( "MaxPackets", UintegerValue( 0xffffffff ) );

	srand( 0 );

	echoClient.SetAttribute( "Interval", TimeValue( Seconds( 1.0 ) ) );
	echoClient.SetAttribute( "PacketSize", UintegerValue( 1024 ) );

  ApplicationContainer clientApps = echoClient.Install( this->GetNode( N_Nodes - 1 ) );
  clientApps.Start( Seconds( 0.0 ) );
  clientApps.Stop( Seconds( Time ) );


  Ptr<UdpMultipathRouter> routingApp = CreateObject<UdpMultipathRouter> ();
  routingApp->channelTable.AddChannelEntry( 0, 100 ); // CSMA Channel
  routingApp->channelTable.AddChannelEntry( 1, 72 );  // Wi Fi 2.4 GHZ Channel
  routingApp->CreatePath( this->GetDevice( 0 ).DeviceIpv4, 9, this->GetDevice( N_Nodes - 1 ).DeviceIpv4, 31, 0, 0 );

  ns3nodes.Get( N_Nodes - 1 )->AddApplication( routingApp );

	uint16_t port = 9;
	for( Uint Nn = 1 ; Nn < N_Nodes - 1 ; Nn++ ){
		for( Uint Nm = 0 ; Nm < Quant ; Nm++ ){
			OnOffHelper onoff( "ns3::UdpSocketFactory", InetSocketAddress( Devices[ ( rand() % ( N_Devices - 2 ) ) + 1 ].DeviceIpv4, port ) );
			onoff.SetAttribute( "OnTime", StringValue( "ns3::ConstantRandomVariable[Constant=1]" ) );
			onoff.SetAttribute( "OffTime", StringValue( "ns3::ConstantRandomVariable[Constant=0]" ) );
			onoff.SetAttribute( "DataRate", StringValue( "2kbps" ) );
			onoff.SetAttribute( "PacketSize", UintegerValue( 1024 ) );

			ApplicationContainer apps = onoff.Install( ns3nodes.Get( Nn ) );
			apps.Start( Seconds( 0.0 ) );
			apps.Stop( Seconds( Time ) );
		}
	}

  Simulator::Stop( Seconds( Time ) );
}

// ==================================================================== //

#endif
