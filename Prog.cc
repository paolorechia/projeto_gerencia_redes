
/*

./waf
./waf --run 'scratch/Prog Dados.txt'

 */

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

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define char_of_number( ch )		( ( ch >= '0' ) && ( ch <= '9') )

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

// ==================================================================== //

void lowercase( char* string ){

	if( string )
		for( int n = 0 ; string[ n ] ; n++ )
			string[ n ] = ( ( ( string[ n ] >= 'A' ) && ( string[ n ] <= 'Z' ) ) ? string[ n ] + 32 : string[ n ] );
}

// -------------------------------------------------------------------- //

int readint( int& val, FILE* fl ){
	char ch;

	val = 0;
	ch = fgetc( fl );
	while( !char_of_number( ch ) )
		ch = fgetc( fl );

	while( char_of_number( ch ) ){
		val = ( val * 10 ) + ( ch - '0' );
		ch = fgetc( fl );
	}

	if( ch == '\n' )
		return( 1 );
	return( 0 );
}

// ==================================================================== //

class LAN{
  public:
    unsigned int num_of_nodes, links;
    NodeContainer nodes;

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

		int p2p_connect( NodeContainer auxNodes );
		int csma_connect( NodeContainer auxNodes );
		int wifi_connect( NodeContainer auxNodes );
		int wifiM_connect( NodeContainer auxNodes );

		void info();
};

// ==================================================================== //

LAN::LAN(){

	num_of_nodes = links = 0;
	
  Time::SetResolution( Time::NS );
  LogComponentEnable( "UdpEchoClientApplication", LOG_LEVEL_INFO );
  LogComponentEnable( "UdpEchoServerApplication", LOG_LEVEL_INFO );

  p2p.SetDeviceAttribute( "DataRate", StringValue( "5Mbps" ) );
  p2p.SetChannelAttribute( "Delay", StringValue( "2ms" ) );

  csma.SetChannelAttribute( "DataRate", StringValue( "100Mbps" ) );
  csma.SetChannelAttribute( "Delay", TimeValue( NanoSeconds( 6560 ) ) );

	wifichannel = YansWifiChannelHelper::Default ();
  phy = YansWifiPhyHelper::Default ();
  phy.SetChannel( wifichannel.Create() );

	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  ssid = Ssid( "ns-3-ssid" );

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
}

// -------------------------------------------------------------------- //

int LAN::read( char* path ){
	char technology[ 16 ];
	int nodeNum, endofline;
  FILE* fl = NULL;

  if( !path )
    return( 0 );

  fl = fopen( path, "r" );
  if( !fl )
    return( 0 );

  fscanf( fl, "%u\n", &num_of_nodes );
  if( !num_of_nodes )
    return( 0 );

  nodes.Create( num_of_nodes );
  //printf( "\n\n%d nodos criados!!\n", num_of_nodes );
  InternetStackHelper stack;
  stack.Install( nodes );

	while( !feof( fl ) ){

		memset( technology, 0, 16 );
		fscanf( fl, "%16s", technology );
		lowercase( technology );

		if( !strcmp( technology, "p2p" ) ){
			NodeContainer p2pNodes;

			//printf( "\t-Nodos " );

			readint( nodeNum, fl );
			//printf( "%d ", nodeNum );
			p2pNodes.Add( nodes.Get( nodeNum ) );

			readint( nodeNum, fl );
			//printf( "%d ", nodeNum );
			p2pNodes.Add( nodes.Get( nodeNum ) );

			//puts( "conectados por p2p." );
			this->p2p_connect( p2pNodes );
		}

		else if( !strcmp( technology, "csma" ) ){
			NodeContainer wifiNodes;

			//printf( "\t-Nodos " );
			do{
				endofline = readint( nodeNum, fl );
				//printf( "%d ", nodeNum );
				wifiNodes.Add( nodes.Get( nodeNum ) );
			}while( !endofline );

			//puts( "conectados por CSMA." );
			this->csma_connect( wifiNodes );
		}

		else if( !strcmp( technology, "wifi" ) ){
			NodeContainer wifiNodes;

			//printf( "\t-Nodos " );
			do{
				endofline = readint( nodeNum, fl );
				//printf( "%d ", nodeNum );
				wifiNodes.Add( nodes.Get( nodeNum ) );
			}while( !endofline );

			//puts( "conectados por wifi." );
			this->wifi_connect( wifiNodes );
		}


		else if( !strcmp( technology, "wifim" ) ){
			NodeContainer wifimNodes;

			readint( nodeNum, fl );
			wifimNodes.Add( nodes.Get( nodeNum ) );

			this->wifiM_connect( wifimNodes );
		}
		else
			break;
	}

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  return( 1 );
}

// -------------------------------------------------------------------- //

int LAN::p2p_connect( NodeContainer auxNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	char base[ 32 ];
	
  links++;
	sprintf( base, "10.1.%d.0", links );
  ipv4_n.SetBase( base, "255.255.255.0" );

  n_devs = p2p.Install( auxNodes );
  interfaces = ipv4_n.Assign( n_devs );

	return( 0 );
}

// -------------------------------------------------------------------- //

int LAN::csma_connect( NodeContainer auxNodes ){
	Ipv4InterfaceContainer interfaces;
	NetDeviceContainer n_devs;
	char base[ 32 ];
	
  links++;
	sprintf( base, "10.1.%d.0", links );
  ipv4_n.SetBase( base, "255.255.255.0" );

  n_devs = csma.Install( auxNodes );
  interfaces = ipv4_n.Assign( n_devs );

	return( 0 );
}

// -------------------------------------------------------------------- //

int LAN::wifi_connect( NodeContainer auxNodes ){

  wifimac.SetType( "ns3::StaWifiMac", "Ssid", SsidValue( ssid ), "ActiveProbing", BooleanValue( false ) );
	wifi.Install( phy, wifimac, auxNodes );
  mobility.SetMobilityModel( "ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue( Rectangle( -50, 50, -50, 50 ) ) );
  mobility.Install( auxNodes );

	return( 0 );
}

// -------------------------------------------------------------------- //

int LAN::wifiM_connect( NodeContainer auxNodes ){

  wifimac.SetType( "ns3::ApWifiMac", "Ssid", SsidValue( ssid ) );
	wifi.Install( phy, wifimac, auxNodes );
  mobility.SetMobilityModel( "ns3::ConstantPositionMobilityModel" );
  mobility.Install( auxNodes );

	return( 0 );
}

// -------------------------------------------------------------------- //

void LAN::info(){
	NodeContainer::Iterator i;
	unsigned int n, m;

	printf( "A rede possui %d nodos:\n", nodes.GetN() );
	for( i = nodes.Begin(), n = 0 ; i != nodes.End() ; i++, n++ ){
    printf( "\tNodo %d possui %d conexões.\n", n, (*i)->GetNDevices() - 1 );
		for( m = 1 ; m < (*i)->GetNDevices() ; m++ ){
			printf( "\t\t[ %d ] - ", m );
			std::cout << (*i)->GetDevice( m )->GetAddress() << '\n';
		}
  }
}

// ==================================================================== //

int main( int argc, char** argv ){
  LAN lan;

/*
	char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  printf("Current working dir: %s\n", cwd);
*/

  if( argc < 2 ){
    puts( "Faltam argumentos!!\n\tPor favor informe o arquivo de entrada!!" );
    return( 0 );
  }

  if( !lan.read( argv[ 1 ] ) ){
    puts( "Não foi possível ler o arquivo!!" );
    return( 0 );
  }


	/*
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install( lan.nodes.Get( 0 ) );
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient( lan.nodes.Get( 0 )->GetDevice( 0 )->GetAddress( 1 ) , 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install( lan.nodes.Get(5) );
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
*/

	lan.info();

  //Simulator::Run();
  //Simulator::Destroy();

  return 0;
}

// ==================================================================== //

