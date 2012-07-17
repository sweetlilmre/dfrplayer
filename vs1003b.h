#ifndef VS1003B_H
#define VS1003B_H

#include "integer.h"
  
#define VS1003_CS_DDR     DDRD
#define VS1003_CS_PORT    PORTD
#define VS1003_XDCS       6
#define VS1003_XCS        7

#define VS1003_XRST_DDR   DDRB
#define VS1003_XRST_PORT  PORTB
#define VS1003_XRST       0

#define VS1003_DREQ_DDR   DDRB
#define VS1003_DREQ_PORT  PORTB
#define VS1003_DREQ_PIN   PINB
#define VS1003_DREQ       1

#define LM_SHUTDOWN_DDR     DDRD
#define LM_SHUTDOWN_PORT    PORTD
#define LM_SHUTDOWN         5

#define LM_SHUTDOWN_SELECT()    LM_SHUTDOWN_PORT |=  _BV(LM_SHUTDOWN)
#define LM_SHUTDOWN_DESELECT()  LM_SHUTDOWN_PORT &= ~_BV(LM_SHUTDOWN)   // LM ENABLE = LOW

#define VS1003_DREQ_ACTIVE()    ((VS1003_DREQ_PIN & _BV(VS1003_DREQ)) !=0)

// VS1003 Controls
#define VS1003_XRST_SELECT()    VS1003_XRST_PORT &= ~_BV(VS1003_XRST)   // VS1003 XRST = LOW
#define VS1003_XRST_DESELECT()  VS1003_XRST_PORT |=  _BV(VS1003_XRST)

#define VS1003_XCS_SELECT()     VS1003_CS_PORT &= ~_BV(VS1003_XCS)         // VS1003 XCS Active = LOW
#define VS1003_XCS_DESELECT()   VS1003_CS_PORT |=  _BV(VS1003_XCS)

#define VS1003_XDCS_SELECT()    VS1003_CS_PORT &= ~_BV(VS1003_XDCS)        // VS1003 XDCS Active = LOW
#define VS1003_XDCS_DESELECT()  VS1003_CS_PORT |=  _BV(VS1003_XDCS)


// VS1003 Commands

#define VS1003_WRITE_COMMAND  0x02
#define VS1003_READ_COMMAND   0x03

// VS1003 Register definitions

#define SPI_MODE        0x0   // Mode control
#define SPI_STATUS      0x1   // Status of VS1003
#define SPI_BASS        0x2   // Built-in bass/treble enhancer
#define SPI_CLOCKF      0x3   // Clock freq + multiplier
#define SPI_DECODE_TIME 0x4   // Decode time in seconds
#define SPI_AUDATA      0x5   // Misc. audio data
#define SPI_WRAM        0x6   // RAM write/read
#define SPI_WRAMADDR    0x7   // Base address for RAM write/read
#define SPI_HDAT0       0x8   // Stream header data 0
#define SPI_HDAT1       0x9   // Stream header data 1
#define SPI_AIADDR      0xA   // Start address of application
#define SPI_VOL         0xB   // Volume control
#define SPI_AICTRL0     0xC   // Application control register 0
#define SPI_AICTRL1     0xD   // Application control register 0
#define SPI_AICTRL2     0xE   // Application control register 0
#define SPI_AICTRL3     0xF   // Application control register 0

#define SM_DIFF         0x01    // Differential
#define SM_JUMP         0x02    // Set to zero
#define SM_RESET        0x04    // Soft reset
#define SM_OUTOFWAV     0x08    // Jump out of WAV decoding
#define SM_PDOWN        0x10    // Powerdown
#define SM_TESTS        0x20    // Allow SDI tests
#define SM_STREAM       0x40    // Stream mode
#define SM_SETTOZERO2   0x80    // Set to zero?
#define SM_DACT         0x100   // DCLK active edge
#define SM_SDIORD       0x200   // SDI bit order
#define SM_SDISHARE     0x400   // Share SPI chip select
#define SM_SDINEW       0x800   // VS1002 native SPI modes
#define SM_ADPCM        0x1000  // ADPCM recording active
#define SM_ADPCM_HP     0x2000  // ADPCM high-pass filter active
#define SM_LINE_IN      0x4000  // ADPCM recording selector

#define SC_MULT0X       0x0000 // XTALI
#define SC_MULT1_5X     0x2000 // XTALI x 1:5
#define SC_MULT2_0X     0x4000 // XTALI x 2:0
#define SC_MULT2_5X     0x6000 // XTALI x 2:5
#define SC_MULT3_0X     0x8000 // XTALI x 3:0
#define SC_MULT3_5X     0xa000 // XTALI x 3:5
#define SC_MULT4_0X     0xc000 // XTALI x 4:0
#define SC_MULT4_5X     0xe000 // XTALI x 4:5

#define SC_ADD0         0x0000 // No modification is allowed
#define SC_ADD0_5       0x0800 // 0.5x
#define SC_ADD1_0       0x1000 // 1.0x
#define SC_ADD1_5       0x1800 // 1.5x

#define MAKE_SCI_CLOCKF( FREQ, ADD, MULT ) ( ( ( FREQ ) & 0x07FF ) | ( ADD ) | ( MULT ) )

#define AUDATA_RATE( AUDATA ) ( ( AUDATA ) & 0xFFFE )
#define AUDATA_IS_STEREO( AUDATA ) ( ( ( AUDATA ) & 0x01 ) == 0x01 )

#define MAKE_SCI_AUDATA( RATE, STEREO ) ( ( ( ( RATE ) >> 1 ) << 1 ) | ( ( STEREO ) ? 1 : 0 ) )

#define MAKE_SCI_VOL( LEFT, RIGHT ) ( ( ( ( LEFT ) & 0xFF ) << 8 ) | ( ( RIGHT ) & 0xFF ) )

#define MAKE_SINE_TEST( SAMPLE_RATE_INDEX, SINE_SKIP_SPEED ) ( ( SAMPLE_RATE_INDEX << 5 ) | ( SINE_SKIP_SPEED & 0x1F ) )
// Look into SCI_HDAT1

typedef struct
{
  USHORT SS_AVOL:2;      // Analog volume control
  USHORT SS_APDOWN1:1;   // Analog internal powerdown
  USHORT SS_APDOWN2:1;   // Analog driver powerdown
  USHORT SS_VER:3;       // Version
  USHORT UNUSED:11;
} STATUS;

typedef struct
{
  USHORT SB_FREQLIMIT:4; // Lower limit frequency in 10 Hz steps (2..15)
  USHORT SB_AMPLITUDE:4; // Bass Enhancement in 1 dB steps (0..15, 0 = off)
  USHORT ST_FREQLIMIT:4; // Lower limit frequency in 1000 Hz steps (0..15)
  USHORT ST_AMPLITUDE:4; // Treble Control in 1.5 dB steps (-8..7, 0 = off)
} BASS;

void VS1003_Init( void );
void VS1003_Reset( BYTE leftVol, BYTE rightVol );
void VS1003_SoftReset( BYTE leftVol, BYTE rightVol );
void VS1003_SineTest( void );
void VS1003_SineTestStart( BYTE val );
void VS1003_SineTestStop( void );

USHORT VS1003_ReadRegister (BYTE addressbyte);
void VS1003_WriteRegister( BYTE addressbyte, BYTE highbyte, BYTE lowbyte );

typedef BYTE ( *StreamCallbackFunc )( void );

void VS1003_SendData( BYTE data );
void VS1003_StreamData( BYTE* data, int size, StreamCallbackFunc callback);

#define VS1003_SetVolume( leftchannel, rightchannel ){ VS1003_WriteRegister( SPI_VOL, (leftchannel), (rightchannel) );}
#define VS1003_WriteRegisterW( addressbyte, data ){ VS1003_WriteRegister( addressbyte, ( ( data ) >> 8), ( ( data ) & 0xFF) ); }

void VS1003_EndSong( void );



#endif
