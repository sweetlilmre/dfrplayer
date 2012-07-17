#include <avr/io.h>
#include <util/delay.h>
#include "integer.h"
#include "vs1003b.h"
#include "spi.h"


void VS1003_Init()
{
  VS1003_CS_PORT |= _BV( VS1003_XCS ) | _BV( VS1003_XDCS );  // all vs1003 outputs high
  VS1003_CS_DDR  |= _BV( VS1003_XCS ) | _BV( VS1003_XDCS );  // turns on CS pin as output

  VS1003_DREQ_DDR  &=~_BV( VS1003_DREQ );                   // set DREQ as input
  VS1003_DREQ_PORT |= _BV( VS1003_DREQ );                   // TODO: something
  
  LM_SHUTDOWN_DDR  |= _BV( LM_SHUTDOWN );
  LM_SHUTDOWN_SELECT();
  
  VS1003_XRST_DDR |= _BV( VS1003_XRST );
  VS1003_XRST_SELECT();
}

void VS1003_WriteRegister( BYTE addressByte, BYTE highByte, BYTE lowByte )
{
  VS1003_XDCS_DESELECT ();
  VS1003_XCS_SELECT ();
  SPI_Send( VS1003_WRITE_COMMAND );
  SPI_Send( addressByte );
  SPI_Send( highByte );
  SPI_Send( lowByte );
  VS1003_XCS_DESELECT ();

  while ( !VS1003_DREQ_ACTIVE() );
}

USHORT VS1003_ReadRegister( BYTE addressByte )
{
  USHORT regValue = 0;

  VS1003_XDCS_DESELECT ();
  VS1003_XCS_SELECT ();
  SPI_Send( VS1003_READ_COMMAND );
  SPI_Send( addressByte );

  regValue = SPI_Recv();
  regValue <<= 8;
  regValue |= SPI_Recv();

  VS1003_XCS_DESELECT();
  return regValue;
}

void VS1003_SoftReset( BYTE leftVol, BYTE rightVol )
{
  VS1003_WriteRegisterW( SPI_MODE, SM_SDINEW | SM_RESET );
  _delay_ms(1);       
  while( !VS1003_DREQ_ACTIVE() );
  
  VS1003_WriteRegisterW( SPI_CLOCKF, MAKE_SCI_CLOCKF( 0, 0, SC_MULT4_0X ) );
  
  VS1003_WriteRegisterW( SPI_AUDATA, MAKE_SCI_AUDATA( 44100, 1 ) );
  VS1003_WriteRegister( SPI_BASS, 0x00, 0x55 );
  VS1003_SetVolume( leftVol, rightVol );

  SPI_Speed_Fast();
  
  VS1003_XDCS_SELECT();
  SPI_Send( 0 );
  SPI_Send( 0 );
  SPI_Send( 0 );
  SPI_Send( 0 );
  VS1003_XDCS_DESELECT();
}

void VS1003_Reset( BYTE leftVol, BYTE rightVol )
{
  SPI_Speed_Slow();
  VS1003_XRST_SELECT();
  
  VS1003_XCS_DESELECT();
  VS1003_XDCS_DESELECT();
  VS1003_XRST_DESELECT();
  _delay_ms( 2 );  

  while( !VS1003_DREQ_ACTIVE() );
  VS1003_SoftReset( leftVol, rightVol );
}

void VS1003_SendData( BYTE data )
{
  VS1003_XDCS_SELECT();
  SPI_Send( data );
  VS1003_XDCS_DESELECT();
}

void VS1003_StreamData( BYTE* data, int size, StreamCallbackFunc callback )
{
  int i;

  while( !VS1003_DREQ_ACTIVE() );

  VS1003_XDCS_SELECT();
  for( i = 0; i < size; i++ )
  {
    while( !VS1003_DREQ_ACTIVE() )
    {
      if( callback && callback() )
      {
        return;
      }
    }
    SPI_Send( *data++ );
  }
  VS1003_XDCS_DESELECT();
}

void VS1003_SineTestStart( BYTE val )
{
  VS1003_WriteRegisterW( SPI_MODE,SM_SDINEW|SM_TESTS );
  while( !VS1003_DREQ_ACTIVE() );

  VS1003_XDCS_SELECT();
  SPI_Send( 0x53 );
  SPI_Send( 0xEF );
  SPI_Send( 0x6E );
  SPI_Send( val );
  SPI_Send( 0x00 );
  SPI_Send( 0x00 );
  SPI_Send( 0x00 );
  SPI_Send( 0x00 );
  VS1003_XDCS_DESELECT();
}

void VS1003_SineTestStop( void )
{
  VS1003_XDCS_SELECT();
  SPI_Send( 0x45 );
  SPI_Send( 0x78 );
  SPI_Send( 0x69 );
  SPI_Send( 0x74 );
  SPI_Send( 0x00 );
  SPI_Send( 0x00 );
  SPI_Send( 0x00 );
  SPI_Send( 0x00 );
  VS1003_XDCS_DESELECT();

  VS1003_WriteRegisterW( SPI_MODE, SM_SDINEW );
  while( !VS1003_DREQ_ACTIVE() );
}

void VS1003_SineTest( void )
{
  VS1003_Reset( 20, 20 );

  VS1003_SineTestStart( 0x7E );
  _delay_ms( 1000 );
  VS1003_SineTestStop();

  VS1003_SineTestStart( 0x44 );
  _delay_ms( 1000 );
  VS1003_SineTestStop();
}


void VS1003_EndSong( void )
{
  int i;

  while( !VS1003_DREQ_ACTIVE() );

  VS1003_XDCS_SELECT();
  for( i = 0; i < 2048; i++ )
  {
    while( !VS1003_DREQ_ACTIVE() );
    SPI_Send( 0 );
  }
  VS1003_XDCS_DESELECT();
}  


