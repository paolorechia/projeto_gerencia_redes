#ifndef BASIC_H
#define BASIC_H

#include <new>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined( WIN32 ) || defined( _WIN32 )
#include <windows.h>
#endif

#if defined( __unix__ )
#include <unistd.h>
#endif

// ========================================================================== //

#define PI 			3.14159265
#define PVAL		( PI / 180 )

#define Max( n1, n2 )												( ( n1 ) > ( n2 ) ? ( n1 ) : ( n2 ) )
#define Min( n1, n2 )												( ( n1 ) < ( n2 ) ? ( n1 ) : ( n2 ) )

#define Abs( n )														( ( (Int)n ) > 0 ? ( (Int)n ) : ( - ( (Int)n ) ) )

#define Between( Nx, Px, Pw )								( ( Nx >= Px ) && ( Nx < Px + Pw ) )

#define char_to_number( ch )								( ( ch >= '0' ) && ( ch <= '9') )

// ========================================================================== //

typedef char*			String;
typedef uint8_t		Byte;
typedef int8_t		SByte;
typedef int16_t		Short;
typedef uint16_t	Ushort;
typedef int32_t		Int;
typedef uint32_t	Uint;
typedef int64_t		Long;
typedef uint64_t	Ulong;
typedef void*			Any;

// ========================================================================== //

void lowercase( char* string ){

	if( string )
		for( int n = 0 ; string[ n ] ; n++ )
			string[ n ] = ( ( ( string[ n ] >= 'A' ) && ( string[ n ] <= 'Z' ) ) ? string[ n ] + 32 : string[ n ] );
}

// -------------------------------------------------------------------------- //

int readint( int& val, FILE* fl ){
	char ch;

	val = 0;
	ch = fgetc( fl );
	while( !char_to_number( ch ) && !feof( fl ) )
		ch = fgetc( fl );

	while( char_to_number( ch )  && !feof( fl ) ){
		val = ( val * 10 ) + ( ch - '0' );
		ch = fgetc( fl );
	}

	if( ch == '\n' || feof( fl ) )
		return( 0 );
	return( 1 );
}

// ========================================================================== //

#endif
