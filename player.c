#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "vs1003b.h"
#include "integer.h"
#include "spi.h"
#include "pff.h"
#include "diskio.h"
#include "player.h"
#include "serial.h"

#define RECV_WAIT_START   1
#define RECV_WAIT_CMD     2
#define RECV_WAIT_DATA    3

#define COMMAND_IDLE      0
#define COMMAND_PLAY      1
#define COMMAND_BEEP      2
#define COMMAND_VOLUME    3
#define COMMAND_CHDIR     4
#define COMMAND_PLAYMODE  5
#define COMMAND_PAUSE     6

#define PLAY_MODE_ONESHOT    0
#define PLAY_MODE_CONTINUOUS 1

#define FILE_TYPE_UNK 0
#define FILE_TYPE_MP3 1
#define FILE_TYPE_WAV 2
#define FILE_TYPE_MID 3
#define FILE_TYPE_WMA 4

#define CMD_ARG_MAX 12
#define FAT_BUF_SIZE 512
#define DEFAULT_DIR "/"
#define INVALID_FILE_ATTR ( AM_DIR | AM_LFN | AM_VOL )

#define PLAYER_LED_PORT   PORTC
#define PLAYER_LED_DDR    DDRC
#define PLAYER_LED        3
#define PLAYER_LED_ON()   PLAYER_LED_PORT &= ~_BV(PLAYER_LED)
#define PLAYER_LED_OFF()  PLAYER_LED_PORT |=  _BV(PLAYER_LED)

#define COMMS_SELECT_PORT     PORTD
#define COMMS_SELECT_DDR      DDRD
#define COMMS_SELECT          4
#define COMMS_SELECT_PIN      PIND
#define COMMS_I2C_SELECTED() ((COMMS_SELECT_PIN & _BV(COMMS_SELECT)) !=0)


static FATFS fs;
static BYTE g_fatBuffer[ FAT_BUF_SIZE ];
static DIR g_dir;

static BYTE g_recvState = RECV_WAIT_START;
static BYTE g_pendingCommand = COMMAND_IDLE;
static BYTE g_curCommand = COMMAND_IDLE;

static BYTE g_cmdArgPos = 0;
char g_cmdArg[ CMD_ARG_MAX + 1 ];

static BYTE g_exitFlag = 0;
static BYTE g_pauseFlag = 0;
static BYTE g_volume = 20;
static BYTE g_playMode = PLAY_MODE_ONESHOT;

BYTE player_handleInput( char ch );

void responseCallback( char* msg )
{
  serial_println( msg );
}

unsigned char inputCallback()
{
  if( serial_available() > 0 )
  {
    char ch = serial_read();
    return( player_handleInput( ch ) );
  }
  return( 0 );
}

BYTE file_type_from_filename( char* filename )
{
  if( strstr( filename, ".MP3") != 0)
  {
    return( FILE_TYPE_MP3 );
  }
  else if( strstr( filename, ".WAV") != 0)
  {
    return( FILE_TYPE_WAV );
  }
  else if( strstr( filename, ".MID") != 0)
  {
    return( FILE_TYPE_MID );
  }
  else if( strstr( filename, ".WMA") != 0)
  {
    return( FILE_TYPE_WMA );
  }
  return( FILE_TYPE_UNK );
}


BYTE player_handleInput( char ch )
{
  if( ch >= 'a' && ch <= 'z' )
  {
    ch = ( ch - 'a' ) + 'A';
  }
  
  switch( g_recvState )
  {
    case RECV_WAIT_START:
    {
      if( ch == ':' )
      {
        g_recvState = RECV_WAIT_CMD;
      }
      break;
    }
    
    case RECV_WAIT_CMD:
    {
      switch( ch )
      {
        case 'P':
        {
          g_pendingCommand = COMMAND_PLAY;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'S':
        {
          g_pendingCommand = COMMAND_IDLE;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'V':
        {
          g_pendingCommand = COMMAND_VOLUME;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'B':
        {
          g_pendingCommand = COMMAND_BEEP;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'C':
        {
          g_pendingCommand = COMMAND_CHDIR;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'M':
        {
          g_pendingCommand = COMMAND_PLAYMODE;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        case 'U':
        {
          g_pendingCommand = COMMAND_PAUSE;
          g_recvState = RECV_WAIT_DATA;
          break;
        }
        default:
        {
          g_recvState = RECV_WAIT_START;
          break;
        }
      }
      break;
    }
    
    case RECV_WAIT_DATA:
    {
      if( ch != '\n' )
      {
        if( ch == '\r' )
        {
          // ignore carrige return
          return( 0 );
        }
        if( g_cmdArgPos < CMD_ARG_MAX )
        {
          g_cmdArg[ g_cmdArgPos++ ] = ch;
        }
        else
        {
          // command is too long, ignore and reset for next command
          g_recvState = RECV_WAIT_START;
          g_cmdArgPos = 0;
          g_cmdArg[ 0 ] = 0;
        }
      }
      else
      {
        // null terminate and reset state
        g_cmdArg[ g_cmdArgPos ] = 0;
        g_cmdArgPos = 0;
        g_recvState = RECV_WAIT_START;
        
        switch( g_pendingCommand )
        {
          case COMMAND_VOLUME:
          {
            // COMMAND_VOLUME is immediate and does not interrupt the play state.
            int vol = atoi( g_cmdArg );
            if( vol > 0 && vol < 256 )
            {
              vol--;
              g_volume = 254 - vol;
              VS1003_SetVolume( g_volume, g_volume );
            }
            g_pendingCommand = COMMAND_IDLE;
            break;
          }
          case COMMAND_PLAYMODE:
          {
            // COMMAND_PLAYMODE is immediate and does not interrupt the play state.
            int mode = atoi( g_cmdArg );
            if( mode )
            {
              g_playMode = PLAY_MODE_CONTINUOUS;
            }
            else
            {
              g_playMode = PLAY_MODE_ONESHOT;
            }
            g_pendingCommand = COMMAND_IDLE;
            break;
          }
          case COMMAND_PAUSE:
          {
            g_pauseFlag = !g_pauseFlag;
            g_pendingCommand = COMMAND_IDLE;
            break;
          }
          default:
          {
            g_curCommand = g_pendingCommand;
            g_exitFlag = 1;
            break;
          }
        }
      }
      break;
    }
  }
  return( 0 );
}


int find_first_file( char* pFile, FILINFO* pfile_info )
{
  FRESULT res;

  if( !pfile_info )
  {
    return( 0 );
  }
  // rewind directory to first file
  pf_readdir( &g_dir, 0 );
  
  while( 1 )
  {
    pfile_info->fname[0] = 0;
    res = pf_readdir( &g_dir, pfile_info );
    if( res != FR_OK || !pfile_info->fname[0] )
    {
      break;
    }

    if( !( pfile_info->fattrib & INVALID_FILE_ATTR ) )
    {
      responseCallback( pfile_info->fname );
      if( file_type_from_filename( pfile_info->fname ) != FILE_TYPE_UNK )
      {
        if( !pFile || *pFile == 0 || !strncmp( pfile_info->fname, pFile, 12 ) )
        {
          // found the file we were looking for or a playable file
          return( 1 );
        }
      }
    }
  }
  return( 0 );
}

int find_next_file( FILINFO* pfile_info )
{
  FRESULT res;
  BYTE count = 0;

  if( !pfile_info )
  {
    return( 0 );
  }
  
  while( 1 )
  {
    pfile_info->fname[0] = 0;
    res = pf_readdir( &g_dir, pfile_info );
    if( res != FR_OK )
    {
      break;
    }
    
    if( !pfile_info->fname[0] )
    {
      if( count++ < 1 )
      {
        // rewind directory to first file and start from the top
        pf_readdir( &g_dir, 0 );
        continue;
      }
      return( 0 );
    }

    if( !( pfile_info->fattrib & INVALID_FILE_ATTR ) )
    {
      responseCallback( pfile_info->fname );
      if( file_type_from_filename( pfile_info->fname ) != FILE_TYPE_UNK )
      {
        return( 1 );
      }
    }
  }
  return( 0 );
}

BYTE play_file( char* pFile )
{
  FRESULT res;
  WORD br;
  FILINFO file_info;
  
  g_exitFlag = 0;
  g_pauseFlag = 0;
  
  if( !find_first_file( pFile, &file_info ) )
  {
    responseCallback( ":rpno file" );
    return( 1 );
  }

  while( 1 ) 
  {
    res = pf_open( file_info.fname );
    if( res == FR_OK )
    {
      do
      {
        WORD count = 0;
        res = pf_read( (void*) g_fatBuffer, FAT_BUF_SIZE, &br );
        if( res == FR_OK )
        {
          while( count < br )
          {
            // give the VS1003 as much at it can handle
            if( VS1003_DREQ_ACTIVE() && !g_pauseFlag )
            {
              VS1003_SendData( g_fatBuffer[ count++ ] );
            }
            else
            {
              if( g_exitFlag )
              {
                return( 0 );
              }
              inputCallback();
            }
          }
        }
      }
      while ( ( res == FR_OK ) && ( br == FAT_BUF_SIZE ) );
    }
    
    if( g_playMode != PLAY_MODE_CONTINUOUS || !find_next_file( &file_info ) )
    {
      break;
    }
  }
  
  // Finished playing so return to IDLE command
  g_curCommand = COMMAND_IDLE;
  return( 0 );
}


void play_beep( int hz, int ms_delay )
{
  VS1003_SineTestStart( hz );
  _delay_ms( ms_delay );
  VS1003_SineTestStop();
  g_curCommand = COMMAND_IDLE;
}

FRESULT change_dir( char* path )
{
  FRESULT res;
  
  res = pf_chdir( path );
  if( res == FR_OK )
  {
    res = pf_opendir( &g_dir, "." );
  }
  g_curCommand = COMMAND_IDLE;
  return( res );
}

void blink_led( int delay, int times )
{
  int i;
  for( i = 0; i < times; i++ )
  {
    PLAYER_LED_ON();
    _delay_ms( delay );
    PLAYER_LED_OFF();
    _delay_ms( delay );
  }    
}

int player_hardwareSetup( void )
{
  FRESULT res;
  
  COMMS_SELECT_DDR  &=~_BV( COMMS_SELECT );
  PLAYER_LED_DDR  |= _BV( PLAYER_LED );
  PLAYER_LED_OFF();

  VS1003_Init();
  SPI_Init();
  SPI_Speed_Slow();
  SPI_Send (0xFF);
  
  //_delay_ms( 200 );
  
  if( COMMS_I2C_SELECTED() )
  {
    blink_led( 200, 1 );
  }
  else
  {
    serial_init();
    blink_led( 200, 2 );
  }
  
  res = pf_mount( &fs );
  if( res == FR_OK )
  {
    res = pf_opendir( &g_dir, DEFAULT_DIR );
  }
  else
  {
    responseCallback(":risd mount failed");
  }
    
  VS1003_Reset( g_volume, g_volume );
  // enable the speakers
  LM_SHUTDOWN_DESELECT();

  return( res == FR_OK );
}



void player_run()
{
  player_hardwareSetup( );
  responseCallback( "run" );

  while( 1 )
  {
    switch( g_curCommand )
    {
      case COMMAND_PLAY:
      {
        responseCallback( g_cmdArg );
        play_file( g_cmdArg );
        VS1003_EndSong();
        VS1003_SoftReset( g_volume, g_volume );
        break;
      }
      case COMMAND_BEEP:
      {
        responseCallback( g_cmdArg );
        play_beep( 0x7E, 1000 );
        VS1003_Reset( g_volume, g_volume );
        break;
      }
      case COMMAND_CHDIR:
      {
        responseCallback( g_cmdArg );
        if( change_dir( g_cmdArg ) != FR_OK )
        {
          responseCallback( ":rc0");
        }
        break;
      }
      default:
        inputCallback();
      break;
    }
  }
}
