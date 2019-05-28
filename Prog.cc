
/*

./waf --run 'scratch/Prog Dados.txt'

 */

#include "LAN.hpp"


NS_LOG_COMPONENT_DEFINE( "MainProgram" );

// ==================================================================== //

int main( int argc, char** argv ){
  LAN lan;

  if( argc < 2 ){
    puts( "Faltam argumentos!!\n\tPor favor informe o arquivo de entrada!!" );
    return( 0 );
  }

  if( !lan.read( argv[ 1 ] ) ){
    puts( "Não foi possível ler o arquivo!!" );
    return( 0 );
  }

	lan.info();
	puts( "\n\n==========================================\n\n" );

  UdpEchoServerHelper echoServer( 9 );

  ApplicationContainer serverApps = echoServer.Install( lan.ns3nodes.Get( 0 ) );
  serverApps.Start( Seconds( 1.0 ) );
  serverApps.Stop( Seconds( 10.0 ) );

  UdpEchoClientHelper echoClient( lan.nodes[ 0 ].DeviceAddress( 1 ), 9 );
  echoClient.SetAttribute( "MaxPackets", UintegerValue( 1 ) );
  echoClient.SetAttribute( "Interval", TimeValue( Seconds( 1.0 ) ) );
  echoClient.SetAttribute( "PacketSize", UintegerValue( 1024 ) );

  ApplicationContainer clientApps = echoClient.Install( lan.ns3nodes.Get( lan.num_of_nodes - 1 ) );
  clientApps.Start( Seconds( 2.0 ) );
  clientApps.Stop( Seconds( 10.0 ) );

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

// ==================================================================== //

