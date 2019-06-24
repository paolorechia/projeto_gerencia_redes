
/*

./waf --run 'scratch/Prog --file=scratch/Dados.txt --traffic=0 --time=12'

 */

#include "LAN.hpp"


NS_LOG_COMPONENT_DEFINE( "MainProgram" );

// ==================================================================== //

int main( int argc, char** argv ){
  std::string File( "" );
	double Time = 10.0;
	bool Stats = false;
	Uint Traffic = 0;
  LAN lan;

  CommandLine cmd;
  cmd.AddValue( "traffic", "Traffic", Traffic );
  cmd.AddValue( "file", "File", File );
  cmd.AddValue( "time", "Time", Time );
  cmd.AddValue( "stats", "Packest stats", Stats );
  cmd.Parse (argc, argv);

  if( !lan.read( File.c_str() ) ){
    puts( "Não foi possível ler o arquivo!!" );
    return( 0 );
  }

	lan.info();
	puts( "\n\n==========================================\n\n" );

	lan.setEcho( Traffic, Time );

  Simulator::Run();
  Simulator::Destroy();

	if( Stats == true ){
		puts( "Pacotes recebidos por cada dispositivo:" );
		for( Uint Nn = 0 ; Nn < lan.N_Nodes ; Nn++ ){
			printf( "\tNodo %d\n", Nn );
			for( Uint Nm = 0 ; Nm < lan.N_Devices ; Nm++ )
				if( lan.Devices[ Nm ].Node == Nn ){
					std::cout << "\t  -- " << lan.Devices[ Nm ].DeviceIpv4;
					printf( " = %u\n", lan.Devices[ Nm ].DeviceQueue->GetStats().nTotalReceivedPackets );
				}
		}
	}

  return 0;
}

// ==================================================================== //

