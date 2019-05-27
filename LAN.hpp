#ifndef LAN_H
#define LAN_H

#include "MyNode.hpp"

// ==================================================================== //

class LAN{
  public:
    unsigned int num_of_nodes, links;
    NodeContainer ns3nodes;
		MyNode* nodes;

	  PointToPointHelper p2p;
		CsmaHelper csma;

	  YansWifiChannelHelper wifichannel;
	  YansWifiPhyHelper phy;
		WifiHelper wifi;
		WifiMacHelper wifimac;
		Ssid ssid;
		MobilityHelper mobility;

	  Ipv4AddressHelper ipv4_n;

  public:
    LAN();
    int read( char* path );

		int p2p_connect( List<MyNode*> auxNodes );
		int csma_connect( List<MyNode*> auxNodes );
		int wifi_connect( List<MyNode*> auxNodes );
		int wifiM_connect( List<MyNode*> auxNodes );

		void info();
};

// -------------------------------------------------------------------- //

LAN::LAN(){

	num_of_nodes = links = 0;
	nodes = NULL;
	
  Time::SetResolution( Time::NS );
  LogComponentEnable( "UdpEchoClientApplication", LOG_LEVEL_INFO );
  LogComponentEnable( "UdpEchoServerApplication", LOG_LEVEL_INFO );

  p2p.SetDeviceAttribute( "DataRate", StringValue( "5Mbps" ) );
  p2p.SetChannelAttribute( "Delay", StringValue( "2ms" ) );

  csma.SetChannelAttribute( "DataRate", StringValue( "100Mbps" ) );
  csma.SetChannelAttribute( "Delay", TimeValue( NanoSeconds( 6560 ) ) );

	wifichannel = YansWifiChannelHelper::Default();
  phy = YansWifiPhyHelper::Default();
  phy.SetChannel( wifichannel.Create() );

	wifi.SetRemoteStationManager( "ns3::AarfWifiManager" );

  ssid = Ssid( "ns-3-ssid" );

  mobility.SetPositionAllocator( "ns3::GridPositionAllocator",
                                 "MinX", DoubleValue( 0.0 ),
                                 "MinY", DoubleValue( 0.0 ),
                                 "DeltaX", DoubleValue( 5.0 ),
                                 "DeltaY", DoubleValue( 10.0 ),
                                 "GridWidth", UintegerValue( 3 ),
                                 "LayoutType", StringValue( "RowFirst" ) );
}

// -------------------------------------------------------------------- //

int LAN::read( char* path ){
	List<MyNode*> auxNodes;
	char technology[ 16 ];
  FILE* fl = NULL;
	int nodeNum;

  if( !path )
    return( 0 );

  fl = fopen( path, "r" );
  if( !fl )
    return( 0 );

  fscanf( fl, "%u\n", &num_of_nodes );
  if( !num_of_nodes )
    return( 0 );

  ns3nodes.Create( num_of_nodes );
  InternetStackHelper stack;
  stack.Install( ns3nodes );

	nodes = new MyNode[ num_of_nodes ];
	if( !nodes )
		return( 0 );
	for( Uint n = 0 ; n < num_of_nodes ; n++ )
		nodes[ n ].create( ns3nodes.Get( n ) );

	while( !feof( fl ) ){

		memset( technology, 0, 16 );
		fscanf( fl, "%16s", technology );
		lowercase( technology );

		auxNodes.Free();
		while( readint( nodeNum, fl ) )
			auxNodes << &nodes[ nodeNum ];
		auxNodes << &nodes[ nodeNum ];

		if( !strcmp( technology, P2P_DEVICE ) )
			this->p2p_connect( auxNodes );

		else if( !strcmp( technology, CSMA_DEVICE ) )
			this->csma_connect( auxNodes );

		else if( !strcmp( technology, WIFI_DEVICE ) )
			this->wifi_connect( auxNodes );

		else if( !strcmp( technology, WIFIM_DEVICE ) )
			this->wifiM_connect( auxNodes );

		else
			break;
	}

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  return( 1 );
}

// -------------------------------------------------------------------- //

int LAN::p2p_connect( List<MyNode*> auxNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	NodeContainer tempNodes;
	char base[ 32 ];
	
	for( Uint Nn = 0 ; Nn < auxNodes.Quant ; Nn++ )
		tempNodes.Add( auxNodes[ Nn ]->ns3node );

  links++;
	sprintf( base, "10.1.%d.0", links );
  ipv4_n.SetBase( base, "255.255.255.0" );

  n_devs = p2p.Install( tempNodes );
  interfaces = ipv4_n.Assign( n_devs );

	for( Uint Nn = 0 ; Nn < auxNodes.Quant ; Nn++ )
		auxNodes[ Nn ]->addDevice( P2P_DEVICE, interfaces.GetAddress( Nn ) );

	return( 0 );
}

// -------------------------------------------------------------------- //

int LAN::csma_connect( List<MyNode*> auxNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	NodeContainer tempNodes;
	char base[ 32 ];
	
	for( Uint Nn = 0 ; Nn < auxNodes.Quant ; Nn++ )
		tempNodes.Add( auxNodes[ Nn ]->ns3node );

  links++;
	sprintf( base, "10.1.%d.0", links );
  ipv4_n.SetBase( base, "255.255.255.0" );

  n_devs = csma.Install( tempNodes );
  interfaces = ipv4_n.Assign( n_devs );

	for( Uint Nn = 0 ; Nn < auxNodes.Quant ; Nn++ )
		auxNodes[ Nn ]->addDevice( CSMA_DEVICE, interfaces.GetAddress( Nn ) );

	return( 0 );
}

// -------------------------------------------------------------------- //

int LAN::wifi_connect( List<MyNode*> auxNodes ){
	NodeContainer tempNodes;

	for( Uint Nn = 0 ; Nn < auxNodes.Quant ; Nn++ )
		tempNodes.Add( auxNodes[ Nn ]->ns3node );

  wifimac.SetType( "ns3::StaWifiMac", "Ssid", SsidValue( ssid ), "ActiveProbing", BooleanValue( false ) );
	wifi.Install( phy, wifimac, tempNodes );
  mobility.SetMobilityModel( "ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue( Rectangle( -50, 50, -50, 50 ) ) );
  mobility.Install( tempNodes );

	return( 0 );
}

// -------------------------------------------------------------------- //

int LAN::wifiM_connect( List<MyNode*> auxNodes ){
	NodeContainer tempNodes;

	for( Uint Nn = 0 ; Nn < auxNodes.Quant ; Nn++ )
		tempNodes.Add( auxNodes[ Nn ]->ns3node );

  wifimac.SetType( "ns3::ApWifiMac", "Ssid", SsidValue( ssid ) );
	wifi.Install( phy, wifimac, tempNodes );
  mobility.SetMobilityModel( "ns3::ConstantPositionMobilityModel" );
  mobility.Install( tempNodes );

	return( 0 );
}

// -------------------------------------------------------------------- //

void LAN::info(){
	NodeContainer::Iterator i;
	unsigned int n, m;

	/*
	printf( "A rede possui %d nodos:\n", ns3nodes.GetN() );
	for( i = ns3nodes.Begin(), n = 0 ; i != ns3nodes.End() ; i++, n++ ){
    printf( "\tNodo %d possui %d conexões.\n", n, (*i)->GetNDevices() - 1 );
		for( m = 1 ; m < (*i)->GetNDevices() ; m++ ){
			printf( "\t\t[ %d ] - ", m );
			std::cout << (*i)->GetDevice( m )->GetAddress() << '\n';
		}
  }
	*/

	printf( "A rede possui %d nodos:\n", num_of_nodes );
	for( n = 0 ; n < num_of_nodes ; n++ ){
    printf( "\tNodo %d possui %d conexões.\n", n, nodes[ n ].devices );
		for( m = 0 ; m < nodes[ n ].devices ; m++ ){
			printf( "\t\t[ %d ] - %s : ", m, nodes[ n ].DeviceType( m ) );
			std::cout << nodes[ n ].DeviceAddress( m ) << '\n';
		}
  }
}

// ==================================================================== //

#endif
