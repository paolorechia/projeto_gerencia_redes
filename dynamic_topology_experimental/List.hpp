#ifndef LIST_H
#define LIST_H

#include "Basic.hpp"

// ==================================================================== //

template <class T>
class List{
	public:
		Uint Quant, Capacity;
		T* Cont;

		// ================================ //

		List(){
			Quant = Capacity = 0;
			Cont = NULL;
		}

		// -------------------------------- //

		void Free(){
			//if( Cont )
			//	free( Cont );

			Quant = Capacity = 0;
			Cont = NULL;
		}

		// -------------------------------- //

		void Clear(){
			Byte* Temp = NULL;
			Uint Nn, Nm;

			Nm = sizeof( T ) * Capacity;
			Temp = (Byte*)Cont;
			for( Nn = 0 ; Nn < Nm ; Nn++ )
				Temp[ Nn ] = 0;

			Quant = 0;
		}

		// -------------------------------- //

		T& Get( Uint Pos ){
			while( Capacity <= Pos )
				this->Resize();

			Quant = Max( Quant, Pos + 1 );

			return( Cont[ Pos ] );
		}

		// -------------------------------- //		

		inline T& operator[]( Uint Pos ){
			return( this->Get( Pos ) );
		}

		// -------------------------------- //

		void Insert( T _Cont, Uint Pos ){
			Uint Nn;

			Nn = Max( Pos, Quant );
			while( Capacity <= Nn )
				this->Resize();

			Quant = Nn + 1;
			//for( ; Nn + 1 > Pos ; Nn-- )
			//	Cont[ Nn + 1 ] = Cont[ Nn ];

			Cont[ Pos ] = _Cont;
		}

		// -------------------------------- //

		inline void Push( T _Cont ){
			this->Insert( _Cont, Quant );
		}

		// -------------------------------- //

		inline List& operator<<( T _Cont ){
			this->Push( _Cont );
			return( *this );
		}

		// -------------------------------- //

		void Resize(){
			T* _Cont = NULL;
			Uint Nn, Nm;

			Nn = Max( Capacity << 1, 1 );
			_Cont = new T[ Nn ];
			if( !_Cont )
				exit( 0 );

			for( Nm = 0 ; Nm < Quant ; Nm++ )
				_Cont[ Nm ] = Cont[ Nm ];

			//if( Cont )
			//	free( Cont );
			Cont = _Cont;
			Capacity = Nn;
		}

		// -------------------------------- //

		void Remove( Uint Pos ){
			Uint Nn;

			if( Quant <= Pos )
				return;

			for( Nn = Pos ; Nn < ( Quant - 1 ) ; Nn++ )
				Cont[ Nn ] =  Cont[ Nn + 1 ];
			Quant--;
		}

		// -------------------------------- //

		List& operator>>( Uint Pos ){
			this->Remove( Pos );
			return( *this );
		}
};

// ==================================================================== //

#endif
