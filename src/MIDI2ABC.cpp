
/****************************************************************************\

                          D E C L A R A T I O N S

\****************************************************************************/
#define VERSION "v1.01 - October 18, 2008"

#define MAX_TRACKS 32
#define MAX_CHANNELS 16
#define MAX_PROGRAMS 127
#define MAX_NOTES 127

#define MAX_STRING_SIZE 255
#define MAX_NOTE_LETTER_SIZE 5
#define MAX_NOTE_DEFINITION_SIZE 12

/* These are MIDI note values. N_C is middle-C, aka C4. These should not vary
   from keyboard to keyboard, so these should never change. */
#define N_C4  (0x3c)
#define N_C4S (N_C4+1)
#define N_D4  (N_C4+2)
#define N_D4S (N_C4+3)
#define N_E4  (N_C4+4)
#define N_E4S (N_C4+5)
#define N_F4  (N_C4+5)
#define N_F4S (N_C4+6)
#define N_G4  (N_C4+7)
#define N_G4S (N_C4+8)
#define N_A5  (N_C4+9)
#define N_A5S (N_C4+10)
#define N_B5  (N_C4+11)
#define N_B5S  (N_C4+12)

/* Since I'm too lazy to actually write out the MIDI note values for each
   octave as above, I use this macro to "offset" the octave given the note
   as identified by one of the macros above (n parameter), and a signed
   value indicating the relative octave from that (o parameter). For
   example:

   OCTAVE(N_DS,-1) is the same as what we'd otherwise get by doing N_DS, but
   one octave lower. WARNING: No bounds checking! (OMG, heathen!) */
#define OCTAVE(n,o) ((n)+(12*(o)))

#define LUTE_NOTE_MIN OCTAVE(N_C4,-2)
#define LUTE_NOTE_MAX OCTAVE(N_C4,1)

#define HARP_NOTE_MIN OCTAVE(N_C4,-2)
#define HARP_NOTE_MAX OCTAVE(N_C4,1)

#define THEORBO_NOTE_MIN OCTAVE(N_C4,-2)
#define THEORBO_NOTE_MAX OCTAVE(N_C4,1)

#define HORN_NOTE_MIN OCTAVE(N_C4S,-2)
#define HORN_NOTE_MAX OCTAVE(N_C4,1)

#define FLUTE_NOTE_MIN OCTAVE(N_C4,-2)
#define FLUTE_NOTE_MAX OCTAVE(N_C4,1)

#define BAGPIPES_NOTE_MIN OCTAVE(N_C4,-2)
#define BAGPIPES_NOTE_MAX OCTAVE(N_C4,1)

#define CLARINET_NOTE_MIN OCTAVE(N_D4,-2)
#define CLARINET_NOTE_MAX OCTAVE(N_C4,1)


/****************************************************************************\
                              HEADER INCLUDES
\****************************************************************************/
#include <iostream>
#include <tchar.h>
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include <mmreg.h>
#include <msacm.h>
#include "MIDI2ABC.h"


/****************************************************************************\
                                DEBUG CODE
\****************************************************************************/
#ifdef _DEBUG
void DebugMsg(const char * const file, int line, const char * const text)
{
  /*
	char fmt[] = "%s %d: %s";
	char i[255];
	_snprintf(i,254,fmt,file,line,text);
  */
	char fmt[] = "%s";
	char i[255];
	_snprintf_s(i,254,fmt,text);
	OutputDebugString(i);
}
#endif


/****************************************************************************\
                             TYPE DEFINITIONS
\****************************************************************************/
/* Can't imagine having more than 50 MIDI output devices in a system. (wow!)
   This controls the max we can support since we're using static allocators
   for each MIDI device in the system. */
#define MAX_SUPPORTED_DEVICES 50

/* MIDI device handles. Could be several since we just cart-blanche open all
   MIDI output devices. */
HMIDIOUT hMidiOut[MAX_SUPPORTED_DEVICES];

/* Identifies which MIDI output devices we've opened. */
bool devOpen[MAX_SUPPORTED_DEVICES] = {false};

/* Gets filled with the number of MIDI input devices in the system. */
unsigned int numDevices = 0;


#pragma pack(1)
typedef struct {
  char           ID[4];
  unsigned long  length;
} midiChunkHdrType;

typedef struct {
    midiChunkHdrType  hdr;
   /* Here are the 6 bytes */
   unsigned short     format;
   unsigned short     numTracks;
   unsigned short     ppqn;
} midiChunkMThdType;

/* This is a generic way to deconstruct dwParam1 into a MIDI event. The 
   "event" will determine what param1 and param2 do. At that point, you can
   cast this to one of the more specific types. */
typedef struct {
  DWORD channel:4;
  DWORD event:4;
  DWORD param1:8;
  DWORD param2:8;
} midiEventType;

/* A more specific deconstruction of dwParam1 from the MIDI driver for use in
   both note on and note off events. */
typedef struct {
  DWORD channel:4;
  DWORD event:4;
  DWORD note:8;
  DWORD velocity:8;
} midiNoteOnEventType;
#pragma pack()


typedef struct
{
  void * prev;
  void * next;

  unsigned char note;

  unsigned long startTick; // milliseconds from file start
  unsigned long endTick;   // milliseconds from file start

  unsigned char program;
  unsigned char channel;
  unsigned char track;
} noteListItemType;


typedef struct
{
  void * prev;
  void * next;

  char * name;
  char * instrument;
  unsigned int track;
  bool   active;
  bool   lead;
  int    transpose;
  bool   flip_oor;
  bool   transpose_autoselect;
  unsigned int program;
} trackListItemType;

typedef struct
{
  unsigned long tick;     // milliseconds from file start
  char          name[MAX_STRING_SIZE];
  char          instrument[MAX_STRING_SIZE];
  unsigned int  track;
  unsigned int  program;
} trackStatusType;

typedef struct
{
  unsigned char on;
  unsigned long startTick;// milliseconds from file start
  unsigned char program;
} noteStatusType;


typedef struct
{
  unsigned char      currentProgram;
  noteStatusType     noteStatus[MAX_NOTES];
} channelStatusType;

typedef struct
{
  unsigned char     lastStatus;
  unsigned int      ppqn;
  unsigned int      numTracks;
  unsigned long     format;
  unsigned char     currentTrack;
  unsigned long     currentTempo;

  trackStatusType   trackStatus[MAX_TRACKS];
  channelStatusType channelStatus[MAX_CHANNELS];
} fileStatusType;

/****************************************************************************\
                     VARIABLE AND MACRO DECLARATIONS
\****************************************************************************/
noteListItemType  * noteList = NULL;
noteListItemType  * noteFilteredList = NULL;
trackListItemType * trackList = NULL;
trackListItemType * trackFilteredList = NULL;
#ifdef _DEBUG
unsigned long lastNoteOnTick = 0;
#endif
float gStretch = 1.0f;
trackListItemType * selectedTrack = 0;
unsigned char instMaxNote = 0;
unsigned char instMinNote = 0;

/****************************************************************************\
                  LOCAL FUNCTION FORWARD-DECLARATIONS
\****************************************************************************/

/****************************************************************************\

                F U N C T I O N   D E F I N I T I O N S

\****************************************************************************/

/*
** FUNCTION midiCloseDevices
**
** DESCRIPTION
**   Stops and closes all open MIDI output devices. Call this before exiting 
**   the application!
**
*****************************************************************************/
void midiCloseDevices()
{
  MMRESULT mmr;
  for( unsigned int i = 0; i < numDevices; i++ )
  {
    if ( devOpen[i] )
    {
      midiOutReset(hMidiOut[i]);
      mmr = midiOutClose(hMidiOut[i]);
      devOpen[i] = false;
    }
  }
}

/*
** FUNCTION midiOpenDevice
**
** DESCRIPTION
**   Opens and starts capture on the given MIDI output device. Devices are
**   enumerated from 0 to one less than the number of devices in the system.
**
*****************************************************************************/
bool midiOpenDevice( unsigned int devid )
{
  MMRESULT mmr;
  if ( !devOpen[devid] )
  {
    mmr = midiOutOpen(
      &hMidiOut[devid],          
      devid,
      NULL,
      NULL, /* client data, which we don't need (yet) */
      CALLBACK_NULL
    );
    if( mmr == MMSYSERR_NOERROR )
    {
      devOpen[devid] = true;

      //mmr = midiOutStart(hMidiIn[devid]);
    }
    else
    {
      return FALSE;
    }
  }
  return TRUE;
}


/*
** FUNCTION print_header_info
**
** DESCRIPTION
** 
**
*****************************************************************************/
void print_header_info ( void )
{
  printf("\nMIDI2ABC: A tool for converting .mid MIDI files into .abc ABC notation files");
  printf("\nfor LOTRO's music system.\n");
  printf("\nWritten by Aaron Teague <aaron@mindexpressions.com>.");
  printf("\nCopyright (c) 2007-2008 by Aaron Teague. All rights reserved.");
  printf("\nVersion: " VERSION);
  printf("\n\n");
}

/*
** FUNCTION clrscn
**
** DESCRIPTION
** http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1031963460&id=1043284385  
**
*****************************************************************************/
void clrscr ( void )
{
  DWORD n;                         /* Number of characters written */
  DWORD size;                      /* number of visible characters */
  COORD coord = {0};               /* Top left screen position */
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  /* Get a handle to the console */
  HANDLE h = GetStdHandle ( STD_OUTPUT_HANDLE );

  GetConsoleScreenBufferInfo ( h, &csbi );

  /* Find the number of characters to overwrite */
  size = csbi.dwSize.X * csbi.dwSize.Y;

  /* Overwrite the screen buffer with whitespace */
  FillConsoleOutputCharacter ( h, TEXT ( ' ' ), size, coord, &n );
  GetConsoleScreenBufferInfo ( h, &csbi );
  FillConsoleOutputAttribute ( h, csbi.wAttributes, size, coord, &n );

  /* Reset the cursor to the top left position */
  SetConsoleCursorPosition ( h, coord );
}

extern const char * GetInstrumentName( unsigned int inst );

void NewScreen(bool showTracks)
{
  clrscr();
  print_header_info();
  
  if( showTracks )
  {
    trackListItemType * trackItem = trackList;
    while( trackItem != NULL )
    {
      char markedChar = ' ';
      if( trackItem->lead )
      {
        markedChar = '#';
      }
      else if( trackItem->active )
      {
        markedChar = '+';
      }

      if( trackItem == selectedTrack )
      {
        if( strlen( trackItem->instrument ) > 0 )
        {
          printf("-[%c %d. %s (%s), transpose=%d ]-\n",markedChar,trackItem->track, trackItem->name, trackItem->instrument, trackItem->transpose);
        }
        else
        {
          printf("-[%c %d. %s (%s), transpose=%d ]-\n",markedChar,trackItem->track, trackItem->name, GetInstrumentName(trackItem->program), trackItem->transpose);
        }
      }
      else
      {
        if( strlen( trackItem->instrument ) > 0 )
        {
          printf("  %c %d. %s (%s), transpose=%d \n",markedChar,trackItem->track, trackItem->name, trackItem->instrument, trackItem->transpose);
        }
        else
        {
          printf("  %c %d. %s (%s), transpose=%d \n",markedChar,trackItem->track, trackItem->name, GetInstrumentName(trackItem->program), trackItem->transpose);
        }
      }
      trackItem = (trackListItemType *)trackItem->next;
    }
  }

}

/*
** FUNCTION ResetTrackList
**
** DESCRIPTION
**   
**
*****************************************************************************/
void ResetTrackList(trackListItemType * &thisTrackList)
{
  trackListItemType * trackItem = thisTrackList;

  while( trackItem != NULL )
  {
    trackListItemType * nextTrackItem = (trackListItemType *)trackItem->next;
    if( trackItem->name != NULL )
    {
      delete [] trackItem->name;
    }
    if( trackItem->instrument != NULL )
    {
      delete [] trackItem->instrument;
    }
    delete trackItem;
    trackItem = nextTrackItem;
  }

  thisTrackList = NULL;
}

/*
** FUNCTION ResetTrackList
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned int CountNotes(
  noteListItemType * &thisNoteList,
  unsigned int track
  )
{
  unsigned int count = 0;
  noteListItemType * noteItem = thisNoteList;

  while( noteItem != NULL )
  {
    if( noteItem->track == track )
    {
      count++;
    }
    noteItem = (noteListItemType *)noteItem->next;
  }

  return count;
}

/*
** FUNCTION DeleteTrack
**
** DESCRIPTION
**   
**
*****************************************************************************/
trackListItemType * DeleteTrack(
  trackListItemType * &thisTrackList,
  trackListItemType * delme
  )
{
  trackListItemType * trackItem = thisTrackList;
  trackListItemType * nextTrackItem = NULL;
  bool reassignLead = false;

  while( trackItem != NULL )
  {
    nextTrackItem = (trackListItemType *)trackItem->next;
    if( trackItem == delme )
    {
      if( trackItem->lead )
      {
        reassignLead = true;
      }

      if( thisTrackList == trackItem )
      {
        thisTrackList = nextTrackItem;
      }
      if( trackItem->prev != NULL )
      {
        ((trackListItemType *)trackItem->prev)->next = (void *)nextTrackItem;
      }
      if( nextTrackItem != NULL )
      {
        nextTrackItem->prev = trackItem->prev;
      }


      if( trackItem->name != NULL )
      {
        delete [] trackItem->name;
      }
      if( trackItem->instrument != NULL )
      {
        delete [] trackItem->instrument;
      }
      delete trackItem;
      break;
    }
    trackItem = nextTrackItem;
  }

  if( reassignLead )
  {
    if( thisTrackList != NULL )
    {
      thisTrackList->lead = true;
    }
  }
  
  return nextTrackItem;
}

/*
** FUNCTION GetTrack
**
** DESCRIPTION
**   
**
*****************************************************************************/
trackListItemType * GetTrack(
  trackListItemType * &thisTrackList,
  unsigned int track
  )
{
  trackListItemType * retval = NULL;
  trackListItemType * trackItem = thisTrackList;

  while( trackItem != NULL )
  {
    if( trackItem->track == track )
    {
      retval = trackItem;
      break;
    }
    trackItem = (trackListItemType *)trackItem->next;
  }

  return retval;
}

/*
** FUNCTION AddTrack
**
** DESCRIPTION
**   
**
*****************************************************************************/
void AddTrack(
  trackListItemType * &thisTrackList,
  trackListItemType * cpyTrack,
  unsigned int track,
  unsigned int program,
  char       * name,
  char       * instrument
  )
{
  trackListItemType * trackItem = new trackListItemType;
  trackListItemType * trackItemHead = thisTrackList;

  if( cpyTrack == NULL )
  {
    ZeroMemory( trackItem, sizeof( trackListItemType ) );

    trackItem->active = true;

    if( thisTrackList == NULL )
    {
      trackItem->lead = true;
    }
    else
    {
      trackItem->lead = false;
    }
    trackItem->transpose = 0;
    trackItem->track = track;
    trackItem->flip_oor = true;
    trackItem->transpose_autoselect = true;
    trackItem->program = program;
  
    unsigned int name_sz = (unsigned int)strlen(name);
    unsigned int inst_sz = (unsigned int)strlen(instrument);

    trackItem->name = new char[name_sz+1];
    trackItem->instrument = new char[inst_sz+1];

    strcpy_s(trackItem->name, name_sz + 1, name);
    strcpy_s(trackItem->instrument, inst_sz + 1, instrument);
  }
  else
  {
    memcpy( trackItem, cpyTrack, sizeof( trackListItemType ) );

    trackItem->next = NULL;
    trackItem->prev = NULL;

    unsigned int name_sz = (unsigned int)strlen(cpyTrack->name);
    unsigned int inst_sz = (unsigned int)strlen(cpyTrack->instrument);

    trackItem->name = new char[name_sz+1];
    trackItem->instrument = new char[inst_sz+1];

    strcpy_s(trackItem->name, name_sz + 1, cpyTrack->name);
    strcpy_s(trackItem->instrument, inst_sz + 1, cpyTrack->instrument);
    /* TODO: do lead */
  }

  if( thisTrackList == NULL )
  {
    thisTrackList = trackItem;
  }
  else
  {
    while( trackItemHead->next != NULL )
    {
      trackItemHead = (trackListItemType *)trackItemHead->next;
    }

    trackItem->prev = (void *)trackItemHead;
    trackItemHead->next = (void *)trackItem;
  }
}


/*
** FUNCTION ResetNoteList
**
** DESCRIPTION
**   
**
*****************************************************************************/
void ResetNoteList(noteListItemType * &thisNoteList)
{
  noteListItemType * noteItem = thisNoteList;

  while( noteItem != NULL )
  {
    noteListItemType * nextNoteItem = (noteListItemType *)noteItem->next;
    delete noteItem;
    noteItem = nextNoteItem;
  }

  thisNoteList = NULL;
}

/*
** FUNCTION DeleteNote
**
** DESCRIPTION
**   
**
*****************************************************************************/
noteListItemType * DeleteNote(
  noteListItemType * &thisNoteList,
  noteListItemType * delme
  )
{
  noteListItemType * noteItem = thisNoteList;
  noteListItemType * nextNoteItem = NULL;

  while( noteItem != NULL )
  {
    nextNoteItem = (noteListItemType *)noteItem->next;
    if( noteItem == delme )
    {
      if( thisNoteList == noteItem )
      {
        thisNoteList = nextNoteItem;
      }
      if( noteItem->prev != NULL )
      {
        ((noteListItemType *)noteItem->prev)->next = (void *)nextNoteItem;
      }
      if( nextNoteItem != NULL )
      {
        nextNoteItem->prev = noteItem->prev;
      }

      delete noteItem;
      break;
    }
    noteItem = nextNoteItem;
  }

  return nextNoteItem;
}

/*
** FUNCTION SortNoteListByNote
**
** DESCRIPTION
**   
**
*****************************************************************************/
void SortNoteListByNote(noteListItemType * &thisNoteList)
{
  int setChanged = 1;

  while( setChanged > 0 )
  {
    noteListItemType * noteItem = thisNoteList;

    setChanged = 0;

    while( noteItem != NULL && noteItem->next != NULL )
    {
      if( noteItem->note > ((noteListItemType *)(noteItem->next))->note )
      {
        noteListItemType * nextNoteItem = (noteListItemType *)noteItem->next;
        setChanged = 1;

        if( thisNoteList == noteItem )
        {
          thisNoteList = nextNoteItem;
        }
        if( noteItem->prev != NULL )
        {
          ((noteListItemType *)noteItem->prev)->next = (void *)nextNoteItem;
        }
        if( nextNoteItem->next != NULL )
        {
          ((noteListItemType *)nextNoteItem->next)->prev = (void *)noteItem;
        }

        nextNoteItem->prev = (noteListItemType *)noteItem->prev;
        noteItem->next = (noteListItemType *)nextNoteItem->next;
        nextNoteItem->next = (void *)noteItem;
        noteItem->prev = (void *)nextNoteItem;
      }
      else
      {
        noteItem = (noteListItemType *)noteItem->next;
      }
    }
  }
}

/*
** FUNCTION SortNoteListByStart
**
** DESCRIPTION
**   
**
*****************************************************************************/
void SortNoteListByStart(noteListItemType * &thisNoteList)
{
  int setChanged = 1;

  while( setChanged > 0 )
  {
    noteListItemType * noteItem = thisNoteList;

    setChanged = 0;

    while( noteItem != NULL && noteItem->next != NULL )
    {
      if( noteItem->startTick > ((noteListItemType *)(noteItem->next))->startTick )
      {
        noteListItemType * nextNoteItem = (noteListItemType *)noteItem->next;
        setChanged = 1;

        if( thisNoteList == noteItem )
        {
          thisNoteList = nextNoteItem;
        }
        if( noteItem->prev != NULL )
        {
          ((noteListItemType *)noteItem->prev)->next = (void *)nextNoteItem;
        }
        if( nextNoteItem->next != NULL )
        {
          ((noteListItemType *)nextNoteItem->next)->prev = (void *)noteItem;
        }

        nextNoteItem->prev = (noteListItemType *)noteItem->prev;
        noteItem->next = (noteListItemType *)nextNoteItem->next;
        nextNoteItem->next = (void *)noteItem;
        noteItem->prev = (void *)nextNoteItem;
      }
      else
      {
        noteItem = (noteListItemType *)noteItem->next;
      }
    }
  }
}

/*
** FUNCTION AddNote
**
** DESCRIPTION
**   
**
*****************************************************************************/
noteListItemType * AddNote(
  noteListItemType * &thisNoteList,
  unsigned char       note,
  unsigned long       startTick,
  unsigned long       endTick,
  unsigned char       program,
  unsigned char       channel,
  unsigned char       track
  )
{
  noteListItemType * noteItem = new noteListItemType;
  noteListItemType * noteItemHead = thisNoteList;

  ZeroMemory( noteItem, sizeof( noteListItemType ) );

  noteItem->note = note;
  noteItem->startTick = startTick;
  noteItem->endTick = endTick;
  noteItem->program = program;
  noteItem->channel = channel;
  noteItem->track = track;

  if( thisNoteList == NULL )
  {
    thisNoteList = noteItem;
  }
  else
  {
    while( noteItemHead->next != NULL )
    {
      noteItemHead = (noteListItemType *)noteItemHead->next;
    }

    noteItem->prev = (void *)noteItemHead;
    noteItemHead->next = (void *)noteItem;
  }

  return noteItem;
}


/*
** FUNCTION OpenOutputFile
**
** DESCRIPTION
**   
**
*****************************************************************************/
#if 0
int OpenOutputFile(unsigned int trackNum)
{
  char  * cpos = NULL;

  if( trackNum > MAX_TRACKS )
  {
    return 0;
  }

  if( strlen( outFileName[trackNum-1] ) != 0 )
  {
    /* Generate output filenames. */  
    strcpy( outFileName[trackNum-1], inFileName );
    cpos = outFileName[trackNum-1] + strlen( outFileName[trackNum-1] );
    while( *cpos != '.' && cpos != outFileName[trackNum-1] ) cpos--;
    if ( cpos != outFileName[trackNum-1] )
    {
      *cpos = '\0';
    }

    strcat( outFileName[trackNum-1], ".abc" );

    /* Attempt to open all output files for writing. */
    outFilePtr[trackNum-1]    = fopen( outFileName[trackNum-1], "wb" );
    if ( outFilePtr[trackNum-1] == NULL )
    {
      printf( "ERROR: Can not open %s for writing. (Is there enough space?)\n", 
          outFileName[trackNum-1] );
      exit(-1);   
    }
  }

  return 1;
} 
#endif


/*
** FUNCTION reorder
**
** DESCRIPTION
**   
**
*****************************************************************************/

#define reorderFunc(type, tptr, size) \
do {\
  type *ptr = (type *)(tptr);\
  type *end = (type *)(((char *)(ptr)) + (size) - sizeof(type)); \
\
  while( end > (ptr) )\
  {\
    type v = *(ptr);\
    \
    *(ptr) = *end;\
    *end = v;\
\
    (ptr)++; end--;\
  }\
} while(0)



#define reorder1(ptr,size) reorderFunc(unsigned char, ptr, (unsigned int)size)
#define reorder2(ptr,size) reorderFunc(unsigned short, ptr, (unsigned int)size)


#if 0
#define WORD_WIDTH unsigned char
#define reorder(ptr,size) reorderFunc((WORD_WIDTH *)ptr,(unsigned int)size)
void reorderFunc( WORD_WIDTH* ptr, unsigned int size )
{
  WORD_WIDTH *end = (WORD_WIDTH *)(((char *)ptr) + size - sizeof(WORD_WIDTH));

  while( end > ptr )
  {
    WORD_WIDTH v = *ptr;
    
    *ptr = *end;
    *end = v;

    ptr++; end--;
  }

  return;
}
#endif

/*
** FUNCTION readVarLen
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned long readVarLen(unsigned char * data, unsigned long bytesRemaining, unsigned long * val)
{
  unsigned long bytesConsumed = 0;
  unsigned long value = 0;

  if( data != NULL && val != NULL )
  {
    while( bytesConsumed <= ( sizeof( unsigned long ) - 1 ) 
          && bytesConsumed <= bytesRemaining
          && 0 != (*data & 0x80) )
    {
      value = (value << 7) | ( *data & 0x7f );
      //value &= *data;
      bytesConsumed++;
      data++;
    }

    if( bytesConsumed <= ( sizeof( unsigned long ) - 1 ) 
        && bytesConsumed <= bytesRemaining )
    {
      value = (value << 7) | ( *data & 0x7f );

      *val += value;

      bytesConsumed++;
    }
  }

  return bytesConsumed;
}

/*
** FUNCTION parseChannelEvent
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned long parseChannelEvent(unsigned char * data, unsigned long bytesRemaining, fileStatusType * fileStatus)
{
  unsigned long bytesConsumed = 0;
  unsigned char thisMsgType = 0;
  unsigned char channel = 0;

  if( fileStatus->lastStatus != 0 )
  {
    thisMsgType = fileStatus->lastStatus;
  }
  else
  {
    thisMsgType = *data;
    data++; bytesRemaining--; bytesConsumed++;
  }

  channel = thisMsgType & 0x0F;
  fileStatus->lastStatus = thisMsgType;

  if( (thisMsgType & 0xF0) == 0x80 )
  {
    TRACEH("[%d ms] Note OFF event at 0x%x: channel=%d, key=%d\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      (unsigned int)data,
      channel,
      *data,
      0
      );
    /* note off */
    if( fileStatus->channelStatus[channel].noteStatus[data[0]].on != 0 )
    {
      (void)AddNote(
        noteList,
        data[0],
        fileStatus->channelStatus[channel].noteStatus[data[0]].startTick,
        fileStatus->trackStatus[fileStatus->currentTrack].tick,
        fileStatus->channelStatus[channel].noteStatus[data[0]].program,
        channel,
        fileStatus->currentTrack
        );

      fileStatus->channelStatus[channel].noteStatus[data[0]].on = 0;
      fileStatus->channelStatus[channel].noteStatus[data[0]].startTick = 0;
    }
    bytesConsumed += 2;
  }
  /* MIDI NoteOn Event */
  else if( (thisMsgType & 0xF0) == 0x90 )
  {
    
    TRACEH("[%d ms] Note ON event at 0x%x: channel=%d, key=%d, velocity=%d\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      data,
      channel,
      *data,
      data[1]
      ); 
    if( data[1] != 0 )
    {
#ifdef _DEBUG
      if( fileStatus->trackStatus[fileStatus->currentTrack].tick > lastNoteOnTick )
      {
        lastNoteOnTick =
          fileStatus->trackStatus[fileStatus->currentTrack].tick;
      }
#endif
      /* note on */
      fileStatus->channelStatus[channel].noteStatus[data[0]].on = 1;
      fileStatus->channelStatus[channel].noteStatus[data[0]].program =
        fileStatus->channelStatus[channel].currentProgram;
      if( fileStatus->channelStatus[channel].noteStatus[data[0]].startTick == 0 )
      {
        fileStatus->channelStatus[channel].noteStatus[data[0]].startTick = 
          fileStatus->trackStatus[fileStatus->currentTrack].tick;
      }
    }
    else
    {
      /* note off */
      if( fileStatus->channelStatus[channel].noteStatus[data[0]].on != 0 )
      {
        (void)AddNote(
          noteList,
          data[0],
          fileStatus->channelStatus[channel].noteStatus[data[0]].startTick,
          fileStatus->trackStatus[fileStatus->currentTrack].tick,
          fileStatus->channelStatus[channel].noteStatus[data[0]].program,
          channel,
          fileStatus->currentTrack
          );

        fileStatus->channelStatus[channel].noteStatus[data[0]].on = 0;
        fileStatus->channelStatus[channel].noteStatus[data[0]].startTick = 0;
      }
    }
    bytesConsumed += 2;
  }
  /* MIDI Polyphonic Key Pressure Event */
  else if( (thisMsgType & 0xF0) == 0xA0 )
  {
    TRACE("[%d ms] Polyphonic Aftertouch event at 0x%llx, skipping\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      (unsigned __int64)data);
    bytesConsumed += 1;
  }
  /* MIDI Controller Event */
  else if( (thisMsgType & 0xF0) == 0xB0 )
  {
    TRACE("[%d ms] Controller event at 0x%llx, controller=0x%x = 0x%x, skipping\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      (unsigned __int64)data,*data,data[1]);
    bytesConsumed += 2;
  }
  /* MIDI Program Change Event */
  else if( (thisMsgType & 0xF0) == 0xC0 )
  {
    TRACE("[%d ms] Program Change event at 0x%llx: channel=%d, program=%d\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      (unsigned __int64)data,channel,*data);
    fileStatus->trackStatus[fileStatus->currentTrack].program = *data;
    fileStatus->channelStatus[channel].currentProgram = *data;
    bytesConsumed += 1;
  }
  /* MIDI Channel Key Pressure Event */
  else if( (thisMsgType & 0xF0) == 0xD0 )
  {
    TRACE("[%d ms] Channel Aftertouch event at 0x%llx, skipping\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      (unsigned __int64)data);
    bytesConsumed += 1;
  }
  /* MIDI Pitch Bend Event */
  else if( (thisMsgType & 0xF0) == 0xE0 )
  {
    TRACE("[%d ms] Pitch Bend event at 0x%llx, skipping\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      (unsigned __int64)data);
    bytesConsumed += 2;
  }
  else
  {
    TRACE("[%d ms] Bad event: 0x%x\n",
      fileStatus->trackStatus[fileStatus->currentTrack].tick,
      thisMsgType);

    fileStatus->lastStatus = 0;
  }

  return bytesConsumed;
}

/*
** FUNCTION parseEvent
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned long parseEvent(unsigned char * data, unsigned long bytesRemaining, fileStatusType * fileStatus)
{
  unsigned long deltaTime = 0, lastStatus = 0;
  unsigned long bytesConsumed = 0, tbC = 0;

  tbC = readVarLen( data, bytesRemaining, &deltaTime );

  if( tbC == 0 )
  {
    TRACE("Error reading deltaTime at 0x%llx: [%02x%02x%02x%02x]\n",
      (unsigned __int64)data,
      data[0],
      data[1],
      data[2],
      data[3]
      );
  }
  else
  {
    TRACEH("deltaTime [%02x%02x%02x%02x] = %d ticks",
      data[0],
      data[1],
      data[2],
      data[3],
      deltaTime);

    bytesRemaining -= tbC;
    data += tbC;
    bytesConsumed += tbC;

    deltaTime = (unsigned long)( 
      (unsigned long)(
        ((long double)deltaTime / (long double)fileStatus->ppqn) * (long double)fileStatus->currentTempo
      ) / 1000
    );

    TRACEH(" = %d ms, used = %d bytes\n",
      deltaTime,
      tbC);

    fileStatus->trackStatus[fileStatus->currentTrack].tick += deltaTime;

    if( *data & 0x80 )
    {
      fileStatus->lastStatus = 0;

      /* Sysex Event */
      if( *data == 0xF0 || *data == 0xF7 )
      {
        unsigned long val = 0;
        TRACE("[%d ms] Sysex event at 0x%llx, ",
          fileStatus->trackStatus[fileStatus->currentTrack].tick,
          (unsigned __int64)data);
        data++; bytesRemaining--; bytesConsumed++;
        tbC = readVarLen( data, bytesRemaining, &val );
        TRACE("length=%d\n",tbC);

        data += tbC + val; bytesRemaining -= tbC + val; bytesConsumed += tbC + val;
      } 
      /* Meta Event */
      else if( *data == 0xFF )
      {
        unsigned long val = 0;
        TRACE("[%d ms] Meta event at 0x%llx, type=0x%x\n",
          fileStatus->trackStatus[fileStatus->currentTrack].tick,
          (unsigned __int64)data,data[1]);
        if( data[1] == 0x51 )
        {
          unsigned long tmpo = 0;

          tmpo = (*((unsigned long *)&data[2]) & 0xffffff00l);
          reorder1(&tmpo,sizeof(unsigned long));

          TRACE("[%d ms] Tempo change, was %0.2f bpm, now %0.2f bpm\n",
            deltaTime,
            (double)1 / ((double)(fileStatus->currentTempo) / (double)60000000),
            (double)1 / ((double)(tmpo) / (double)60000000));
          fileStatus->currentTempo = tmpo;
        }
        else if( data[1] == 0x03 )
        { /* sequence name */
          unsigned long len = 0, len_bytes = 0;;

          len_bytes = readVarLen( &data[2], bytesRemaining - 2, &len );

          strncpy_s(fileStatus->trackStatus[fileStatus->currentTrack].name,(char *)&data[2+len_bytes],len);
        }
        else if( data[1] == 0x04 )
        { /* instrument name */
          unsigned long len = 0, len_bytes = 0;;

          len_bytes = readVarLen( &data[2], bytesRemaining - 2, &len );

          strncpy_s(fileStatus->trackStatus[fileStatus->currentTrack].instrument,(char *)&data[2+len_bytes],len);
        }
        data += 2; bytesRemaining -= 2; bytesConsumed += 2;
        tbC = readVarLen( data, bytesRemaining, &val );

        data += tbC + val; bytesRemaining -= tbC + val; bytesConsumed += tbC + val;
      }
      /* Channel Event */
      else if( (*data & 0xF0) >= 0x80 
              && (*data & 0xF0) <= 0xE0 )
      {
        tbC = parseChannelEvent( data, bytesRemaining, fileStatus );
        bytesRemaining -= tbC;
        data += tbC;
        bytesConsumed += tbC;
      }
      /* Unknown Event */
      else
      {
        TRACE("********* ERROR: Unknown event at 0x%llx: 0x%x\n",(unsigned __int64)data,*data);
      }
    }
    else
    {
      if( fileStatus->lastStatus != 0 )
      {
        tbC = parseChannelEvent( data, bytesRemaining, fileStatus );
        bytesRemaining -= tbC;
        data += tbC;
        bytesConsumed += tbC;
      }
      else
      {
        TRACE("********* ERROR: Unknown event at 0x%llx: 0x%x\n",(unsigned __int64)data,*data);
      }
    }
  }

  return bytesConsumed;
}


/*
** FUNCTION parseTrack
**
** DESCRIPTION
**   
**
*****************************************************************************/
void parseTrack(unsigned char * data, unsigned long bytesRemaining, fileStatusType * fileStatus)
{
  unsigned long bytesConsumed = 0, tbC = 0;

  while( bytesRemaining > 0 )
  {
    tbC = parseEvent( data, bytesRemaining, fileStatus );
    if( tbC == 0 )
    {
      break;
    }
    bytesConsumed += tbC; bytesRemaining -= tbC; data += tbC;
  }

  AddTrack(
    trackList,
    NULL,
    fileStatus->currentTrack,
    fileStatus->trackStatus[fileStatus->currentTrack].program,
    fileStatus->trackStatus[fileStatus->currentTrack].name,
    fileStatus->trackStatus[fileStatus->currentTrack].instrument
    );

  fileStatus->currentTrack++;

}

/*
** FUNCTION parseMIDIFile
**
** DESCRIPTION
**   
**
*****************************************************************************/

int parseMIDIFile(char * inFileName, unsigned int * format, unsigned int * numTracks)
{
  unsigned long     inFileSize = 0;
  FILE             *inFilePtr = NULL;
  fileStatusType    fileStatus;
  unsigned long     fpos = 0;

  ZeroMemory( &fileStatus, sizeof( fileStatusType ) );

  /* This is expressed in microseconds per quarter note (aka beat).
     120 beats/minute is default, so:

     1 minute / 120 qtr notes = 60000000 microseconds / 120 qtr notes 
      = 500000 microseconds / 1 qtr note */
  fileStatus.currentTempo = 500000;
  fileStatus.currentTrack = 1;

  fopen_s( &inFilePtr, inFileName, "rb" );
  if ( inFilePtr == NULL ) 
  {
    printf( "ERROR: Can not open %s for reading. (Did you get the filename and path correct?)\n", 
      inFileName );
    return -1;   
  }

  /* Validate size of input file. */
  fseek(inFilePtr,0, SEEK_END);
  inFileSize = ftell(inFilePtr);
  if( inFileSize < sizeof( midiChunkHdrType ) ) 
  {
    printf( "ERROR: %s is too small. (It is not a valid MIDI file.)\n", inFileName );
    return -1;
  }

  /* Show filenames to user. */
  printf( "[input]\nMIDI file: %s\n\n", inFileName );

  /*                                                                        *\
  ============================== Read MThd Header ============================
  \*                                                                        */
  {
    midiChunkMThdType MThdHeader = { 0 };

    /* Grab MThd header. */
    fseek( inFilePtr, 0, SEEK_SET);
    fread( &MThdHeader, sizeof( midiChunkMThdType ), 1, inFilePtr );

    /* Check hdr */
    if( 0 != strncmp( MThdHeader.hdr.ID, "MThd", 4 ) )
    {
      printf( "ERROR: Header chunk has invalid ID: %s. (It is not a valid MIDI file.)\n",
        MThdHeader.hdr.ID ); //TODO: cap length
      return -1;
    }

    /* Reorder bytes */
    reorder1( &MThdHeader.hdr.length, sizeof( MThdHeader.hdr.length ) );
    reorder1( &MThdHeader.ppqn, sizeof( MThdHeader.ppqn ) );
    reorder1( &MThdHeader.format, sizeof( MThdHeader.format ) );
    reorder1( &MThdHeader.numTracks, sizeof( MThdHeader.numTracks ) );

    if( format != NULL )
    {
      *format = MThdHeader.format;
    }

    /* Check size */
    if( MThdHeader.hdr.length != 6 )
    {
      printf( "ERROR: Header chunk has invalid size: %d. (It is not a valid MIDI file.)\n",
        MThdHeader.hdr.length );
      return -1;
    }

    /* Check format */
    if( MThdHeader.format != 0 && MThdHeader.format != 1 )
    {
      printf( "ERROR: Unsupported MIDI format: %d. (It is not a MIDI file that this tool supports.)\n",
        MThdHeader.format );
      return -1;
    }

    /* Check PPQN */
    if( MThdHeader.ppqn & (1 << 14) )
    {
      printf( "ERROR: PPQN SMPTE format unsupported: 0x%x. (It is not a MIDI file that this tool supports.)\n",
        MThdHeader.ppqn );
      return -1;
    }

    printf( "Number of tracks: %d\nPPQN: %d\nFormat: %d\n\n",
      MThdHeader.numTracks, MThdHeader.ppqn, MThdHeader.format );

    fileStatus.ppqn = MThdHeader.ppqn;
    fileStatus.numTracks = MThdHeader.numTracks;
    fpos = sizeof( midiChunkMThdType );

  }

  /*                                                                        *\
  ================================== Read MTrks ==============================
  \*                                                                        */
  while( fpos < inFileSize )
  {
    unsigned char * data = NULL;
    midiChunkHdrType  thisHeader = { 0 };

    fseek( inFilePtr, fpos, SEEK_SET);
    fread( &thisHeader, sizeof( thisHeader ), 1, inFilePtr );

    /* Check hdr */
    if( 0 != strncmp( thisHeader.ID, "MTrk", 4 ) )
    {
      char tID[5] = { 0 };

      strncpy_s(tID,thisHeader.ID,4);
      tID[4] = 0;

      printf( "ERROR: Track chunk has invalid ID: %s. (It is not a valid MIDI file.)\n",
        tID );
      return -1;
    }

    reorder1( &thisHeader.length, sizeof( thisHeader.length ) );
    
    /* Check size */
    if( thisHeader.length > (inFileSize - fpos) )
    {
      printf( "ERROR: Track chunk has invalid size: %d of %d remaining in the file. (It is not a valid MIDI file.)\n",
        thisHeader.length, (inFileSize - fpos) );
      return -1;
    }

    data = new unsigned char[thisHeader.length];

    ZeroMemory( data, thisHeader.length );

    fseek( inFilePtr, fpos + sizeof( thisHeader ), SEEK_SET);
    fread( data, thisHeader.length, 1, inFilePtr );


    TRACE("Track started at 0x%llx\n",(unsigned __int64)data);
    parseTrack( data, thisHeader.length, &fileStatus );
    TRACE("Track ended at 0x%llx\n",(unsigned __int64)data);

    delete [] data;
    data = NULL;

    fpos += thisHeader.length + sizeof( midiChunkHdrType );
  }

  if( numTracks != NULL )
  {
    *numTracks = fileStatus.currentTrack;
  }

  return 0;
}


/*
** FUNCTION transposeNote
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned char transposeNote(unsigned char note, char transpose)
{
  unsigned char thisNote = note;

  if( (int)thisNote + (int)transpose < 0 )
  {
    thisNote = 0;
  }
  else if( (int)thisNote + (int)transpose > 127 )
  {
    thisNote = 127;
  }
  else
  {
    thisNote = (unsigned char)((char)thisNote + transpose);
  }

  return thisNote;
}

/*
** FUNCTION analyzeNotes
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned int analyzeNotes(
  unsigned char instMin,
  unsigned char instMax,
  unsigned char track,
           char transpose,

  unsigned char * out_noteMin,
  unsigned int  * out_numTooLow,
  unsigned char * out_noteMax,
  unsigned int  * out_numTooHigh,
           char * out_rmsAdjust,
           char * out_medianAdjust,
           char * out_noteAverage
  )
{
  noteListItemType * noteItem = noteList;

  unsigned int numNotes = 0;
  unsigned char minNote = 127;
  unsigned int numTooLow = 0;
  unsigned char maxNote = 0;
  unsigned int numTooHigh = 0;
  unsigned int noteAverage = 0;
  long double rmsNote = 0;
  long double midNote = ((long double)instMin + (((long double)instMax - (long double)instMin) / (long double)2));

  while( noteItem != NULL )
  {
    unsigned char thisNote = transposeNote(noteItem->note,transpose);

    if( noteItem->track != track )
    {
      noteItem = (noteListItemType *)noteItem->next;
      continue;
    }

    if( thisNote > maxNote )
    {
      maxNote = thisNote;
    }

    if( thisNote < minNote )
    {
      minNote = thisNote;
    }

    if( thisNote > instMax )
    {
      numTooHigh++;
    }

    if( thisNote < instMin )
    {
      numTooLow++;
    }

    noteAverage += thisNote;

    rmsNote += (((long double)thisNote - midNote) * ((long double)thisNote - midNote));

    numNotes++;

    noteItem = (noteListItemType *)noteItem->next;
  }

  *out_noteMin = minNote;
  *out_noteMax = maxNote;
  *out_numTooLow = numTooLow;
  *out_numTooHigh = numTooHigh;

  if( numNotes > 0 )
  {

    *out_noteAverage = (char)(noteAverage / numNotes);

    rmsNote /= (long double)numNotes;
    rmsNote = (long double)instMin + sqrt( rmsNote );

    *out_rmsAdjust = (char)(rmsNote - midNote);


    if( numNotes & 1 )
    {
      unsigned int medianIndex = numNotes / 2;

      noteItem = noteList;
      while( medianIndex > 0 || noteItem->track != track )
      {
        if( noteItem->track == track )
        {
          medianIndex--;
        }
        noteItem = (noteListItemType *)noteItem->next;
      }

      //printf("Median note: %d\n",transposeNote(noteItem->note,transpose));
      *out_medianAdjust = (char)((int)transposeNote(noteItem->note,transpose) - (int)midNote);
    }
    else
    {
      unsigned int medianIndex = (numNotes / 2) - 1;
      unsigned char A = 0;
      unsigned char B = 0;

      noteItem = noteList;
      while( medianIndex > 0 || noteItem->track != track )
      {
        if( noteItem->track == track )
        {
          medianIndex--;
        }
        noteItem = (noteListItemType *)noteItem->next;
      }
      A = transposeNote(noteItem->note,transpose);
      B = transposeNote(((noteListItemType *)noteItem->next)->note,transpose);

      *out_medianAdjust = (char)(
          ( (int)A + ( ( (int)B - (int)A ) / 2 ) ) - (int)midNote
        );
    }
  }
  else
  {
    *out_rmsAdjust = 0;
  }

  return numNotes;
}

/*
** FUNCTION adjustTrackTranspose
**
** DESCRIPTION
**   
**
*****************************************************************************/
void adjustTrackTranspose(
  int transpose
  )
{
  trackListItemType * trackItem = trackList;

  while( trackItem != NULL )
  {
    trackItem->transpose += transpose;

    trackItem = (trackListItemType *)trackItem->next;
  }
}


/*
** FUNCTION adaptNoteData_AutoFit
**
** DESCRIPTION
**   
**
*****************************************************************************/
int adaptNoteData_AutoFit(
  trackListItemType * trackItem,
  unsigned char instMinNote,
  unsigned char instMaxNote
  )
{
  unsigned char minNote = 127;
  unsigned char maxNote = 0;
  unsigned int  numNotes = 0;
  unsigned int  numTooLow = 0;
  unsigned int  numTooHigh = 0;
           char rmsAdjust = 0;
           char medianAdjust = 0;
           char noteAverage = 0;

           int  transpose = 0;
           int  percentTooLow = 0;
           int  percentTooHigh = 0;

           char midNote = (instMinNote + ((instMaxNote - instMinNote) / 2));

  SortNoteListByNote(noteList);

  numNotes = analyzeNotes(
    instMinNote, instMaxNote, trackItem->track, trackItem->transpose,
    &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
    &noteAverage
    );

  percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes)); 
  percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));

  if( trackItem->lead )
  {
    transpose = (((instMaxNote - midNote) / 2) + midNote) - noteAverage;

    numNotes = analyzeNotes(
      instMinNote, instMaxNote, trackItem->track, (trackItem->transpose + transpose),
      &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
      &noteAverage
      );

    percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes));
    percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));
  }
  else
  {
    while( numTooLow == 0 )
    {
      transpose -= 12;

      numNotes = analyzeNotes(
        instMinNote, instMaxNote, trackItem->track, (trackItem->transpose + transpose),
        &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
        &noteAverage
        );

      percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes));
      percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));
    }

    while( numTooLow > 0 )
    {
      transpose += 12;

      numNotes = analyzeNotes(
        instMinNote, instMaxNote, trackItem->track, (trackItem->transpose + transpose),
        &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
        &noteAverage
        );

      percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes));
      percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));
    }
  }

  while( percentTooHigh > 10 )
  {
    if( !trackItem->lead )
    {
      transpose -= 12;
    }
    else
    {
      transpose--;
    }


    numNotes = analyzeNotes(
      instMinNote, instMaxNote, trackItem->track, (trackItem->transpose + transpose),
      &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
      &noteAverage
      );

    percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes));
    percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));
  }

  if( trackItem->lead )
  {
    adjustTrackTranspose( transpose );
  }
  else
  {
    trackItem->transpose += transpose;
  }

  return 0;
}

/*
** FUNCTION adaptNoteData_Transpose
**
** DESCRIPTION
**   
**
*****************************************************************************/
int adaptNoteData_Transpose(
  trackListItemType * trackItem,
  unsigned char instMinNote,
  unsigned char instMaxNote
  )
{
  unsigned char minNote = 127;
  unsigned char maxNote = 0;
  unsigned int  numNotes = 0;
  unsigned int  numTooLow = 0;
  unsigned int  numTooHigh = 0;
           char rmsAdjust = 0;
           char medianAdjust = 0;
           char noteAverage = 0;

           int  transpose = 0;
           int  percentTooLow = 0;
           int  percentTooHigh = 0;

  int usrInput = 0;

  SortNoteListByNote(noteList);

  if( trackItem->lead && trackItem->transpose_autoselect )
  {
    adaptNoteData_AutoFit(trackItem,instMinNote,instMaxNote);
    trackItem->transpose_autoselect = false;
  }

  numNotes = analyzeNotes(
    instMinNote, instMaxNote, trackItem->track, trackItem->transpose,
    &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
      &noteAverage
    );

  percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes)); 
  percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));

  NewScreen(true);

  /* Analyze and select the transposition */
  printf("\nTRANSPOSE\n");
  printf("Press [left|right] to adjust, [enter] to accept changes.\n\n");
  printf("    |----- too low ------|------ in range ----|----- too high -----|\n");
  while( usrInput != 13 )
  {

    if( numNotes > 0 )
    {
      int numLowDits = (int)ceil((double)percentTooLow / 5.0f);
      int numHighDits = (int)ceil((double)percentTooHigh / 5.0f);

      printf("\r");
      for( int x = 0; x < 20 - numLowDits; x++ )
      {
        printf(" ");
      }
      if( numLowDits > 0 )
      {
        printf("%3d%%|",percentTooLow);
        for( int x = 0; x < numLowDits; x++ )
        {
          printf("-");
        }
      }
      else
      {
        printf("   0%%");
      }
      printf("|-----%3d%% (%3d)-----|",100 - percentTooHigh - percentTooLow, (trackItem->transpose + transpose));
      if( numHighDits > 0 )
      {
        for( int x = 0; x < numHighDits; x++ )
        {
          printf("-");
        }
        printf("|%3d%%",percentTooHigh);
      }
      else
      {
        printf("0%%   ");
      }
      for( int x = 0; x < 20 - numHighDits; x++ )
      {
        printf(" ");
      }

    }
    else
    {
      printf("No notes on track %d\n",trackItem->track);
      return -1;
    }

    do
    {
      usrInput = _getch();
    } while( usrInput != 224 && usrInput != 13 );

    if( usrInput == 224 )
    {
      usrInput = _getch();

      if( usrInput == 72 || usrInput == 77 )
      {
        if( !trackItem->lead )
        {
          transpose += 12;
        }
        else
        {
          transpose++;
        }
      }
      else if( usrInput == 80 || usrInput == 75 )
      {
        if( !trackItem->lead )
        {
          transpose -= 12;
        }
        else
        {
          transpose--;
        }
      }
    }

    numNotes = analyzeNotes(
      instMinNote, instMaxNote, trackItem->track, (trackItem->transpose + transpose),
      &minNote, &numTooLow, &maxNote, &numTooHigh, &rmsAdjust, &medianAdjust,
      &noteAverage
      );

    percentTooLow = (int)ceil(100*((double)numTooLow / (double)numNotes));
    percentTooHigh = (int)ceil(100*((double)numTooHigh / (double)numNotes));
  }


  if( trackItem->lead )
  {
    adjustTrackTranspose( transpose );
  }
  else
  {
    trackItem->transpose += transpose;
  }

  printf("\n\nNotes out of range: %d. Do you wish to flip out of range notes? [y/n]",(numTooLow+numTooHigh));

  usrInput = _getch();

  if( usrInput == 'y' || usrInput == 'Y' )
  {
    trackItem->flip_oor = true;
  }
  else
  {
    trackItem->flip_oor = false;
  }

  return 0;
}

/*
** FUNCTION stretchNote
**
** DESCRIPTION
**   
**
*****************************************************************************/
unsigned long stretchNote(unsigned long time, float stretch)
{
  return (unsigned long)((double)time * (double)stretch);
}

/*
** FUNCTION analyzeTiming
**
** DESCRIPTION
**   
**
*****************************************************************************/
#define MIN_TIMING_MS 60

unsigned int analyzeTiming(
          float stretch,

            int * out_maxQError,
          float * out_biasLevel,
          float * out_errorPower
  )
{
  noteListItemType * noteItem = noteList;

  unsigned int numNotes = 0;
  long maxQError = 0;
  long biasLevel = 0;
  long errorPower = 0;

  while( noteItem != NULL )
  {
    unsigned long thisStart = stretchNote(noteItem->startTick,stretch);
    unsigned long thisEnd = stretchNote(noteItem->endTick,stretch);

    long thisStartDelta = ((long)thisStart) % (long)MIN_TIMING_MS;

    if( thisStartDelta > ((long)MIN_TIMING_MS / 2) )
    {
      thisStartDelta -= (long)MIN_TIMING_MS;
    }

    long thisEndDelta = ((long)thisEnd) % (long)MIN_TIMING_MS;

    if( thisEndDelta > ((long)MIN_TIMING_MS / 2) )
    {
      thisEndDelta -= (long)MIN_TIMING_MS;
    }


    if( abs(thisStartDelta) > maxQError )
    {
      maxQError = abs(thisStartDelta);
    }
    if( abs(thisEndDelta) > maxQError )
    {
      maxQError = abs(thisEndDelta);
    }

    errorPower += (thisStartDelta * thisStartDelta) + (thisEndDelta * thisEndDelta);
    biasLevel += thisStartDelta + thisEndDelta;

    numNotes++;

    noteItem = (noteListItemType *)noteItem->next;
  }

  *out_maxQError = (int)maxQError;

  if( numNotes > 0 )
  {
    //*out_biasLevel = (int)(biasLevel);
    *out_biasLevel = (float)((double)biasLevel / ((double)numNotes * (double)2));
    *out_errorPower = (float)sqrt((double)errorPower / ((double)numNotes * (double)2));
  }
  else
  {
    *out_errorPower = 0;
    *out_biasLevel = 0;
  }

  return numNotes;
}


/*
** FUNCTION adaptNoteData_Quantize
**
** DESCRIPTION
**   
**
*****************************************************************************/
int adaptNoteData_Quantize()
{
  int           usrInput = 0;
  float         stretch = 1.0f;
  unsigned int  numNotes = 0;
  int           maxQError = 0;
  float         biasLevel = 0.0f;
  float         errorPower = 0.0f;
  unsigned long lastLeadTrackStart = 0;
  unsigned long nextLeadTrackStart = 0;
  
  /* Analyze and select the stretch */
  while( usrInput != 'c' && usrInput != 'C' )
  {
    numNotes = analyzeTiming(
      stretch,
      &maxQError, &biasLevel, &errorPower
      );

    if( numNotes > 0 )
    {
      printf("\nSTRETCH[duration = %0.1fx]\n",stretch);
      printf("Max Q Error: %d, Bias Level: %0.03f, Error Power: %0.03f\n",
        maxQError,
        biasLevel,
        errorPower
        );
    }
    else
    {
      printf("No notes.\n");
      return -1;
    }

    printf("Press [c] to accept, or [up|down] to stretch|shrink, respectively.");
    do
    {
      usrInput = _getch();
    } while( usrInput != 224 && usrInput != 'c' && usrInput != 'C' );

    printf("\r                                                                  \r");
    if( usrInput == 224 )
    {
      usrInput = _getch();

      if( usrInput == 72 )
      {
        stretch += 0.1f;
      }
      else if( usrInput == 80 )
      {
        stretch -= 0.1f;
      }
    }
  }

  gStretch = stretch;

  return 0;
}


/*
** FUNCTION adaptNoteData
**
** DESCRIPTION
**   
**
*****************************************************************************/
int adaptNoteData(bool includeLeadTrack)
{
  trackListItemType * trackItem = trackList;

  if( includeLeadTrack )
  {
    while( trackItem != NULL )
    {
      if( trackItem->lead )
      {
        break;
      }
      trackItem = (trackListItemType *)trackItem->next;
    }

    adaptNoteData_AutoFit(trackItem,instMinNote,instMaxNote);
  }

  trackItem = trackList;

  while( trackItem != NULL )
  {
    if( !trackItem->lead )
    {
      adaptNoteData_AutoFit(trackItem,instMinNote,instMaxNote);
    }
    trackItem = (trackListItemType *)trackItem->next;
  }

  return 0;
}

/*
** FUNCTION filterNoteData
**
** DESCRIPTION
**   
**
*****************************************************************************/
int filterNoteData(unsigned int * o_NumDeleted, unsigned int * o_NumAdjusted)
{
  noteListItemType * noteItem;

  unsigned int numDeleted = 0;
  unsigned int numAdjusted = 0;
  unsigned long numMsToDelete = 0;
  unsigned long lastOnTime[MAX_TRACKS] = { 0 };
  unsigned long lastLeadTrackStart = 0;

  SortNoteListByStart( noteList );
  ResetNoteList( noteFilteredList );
  ResetTrackList( trackFilteredList );

  noteItem = noteList;

  if( noteItem != NULL )
  {
    numMsToDelete = noteItem->startTick;
  }

  trackListItemType * trackItem = trackList;
  /* delete inactive tracks */
  while( trackItem != NULL )
  {
    if( trackItem->active )
    {
      AddTrack(
        trackFilteredList,
        trackItem,
        0,
        0,
        NULL,
        NULL
        );
    }
    
    trackItem = (trackListItemType *)trackItem->next;
  }

  /* transpose notes */
  while( noteItem != NULL )
  {
    if( noteItem->channel == 9 )
    {
      noteItem = (noteListItemType *)noteItem->next;
      continue;
    }

    trackItem = GetTrack( 
      trackFilteredList,
      noteItem->track );

    if( trackItem == NULL )
    {
      noteItem = (noteListItemType *)noteItem->next;
      continue;
    }

    unsigned int tnote = transposeNote(noteItem->note,trackItem->transpose);

    noteListItemType * noteFilteredItem = AddNote(
      noteFilteredList,
      tnote,
      noteItem->startTick - numMsToDelete,
      noteItem->endTick - numMsToDelete,
      noteItem->program,
      noteItem->channel,
      noteItem->track
      );

    //noteItem->startTick -= numMsToDelete;
    //noteItem->endTick -= numMsToDelete;

    //noteItem->note = transposeNote(noteItem->note,trackItem->transpose);

    if( trackItem->flip_oor )
    {
      unsigned long diffSinceLast = noteFilteredItem->startTick - lastOnTime[noteFilteredItem->track];

      if( diffSinceLast != 0
          && diffSinceLast <= ( MIN_TIMING_MS * 1 ) )
      {
        numDeleted++;
        noteFilteredItem = DeleteNote( noteFilteredList, noteFilteredItem );
        noteItem = (noteListItemType *)noteItem->next;
        continue;
      }

      lastOnTime[noteFilteredItem->track] = noteFilteredItem->startTick;

      /* Flip note back into range */
      bool adjusted = false;

      while( noteFilteredItem->note < instMinNote )
      {
        noteFilteredItem->note = transposeNote(noteFilteredItem->note,12);
        adjusted = true;
      }
      while( noteFilteredItem->note > instMaxNote )
      {
        noteFilteredItem->note = transposeNote(noteFilteredItem->note,-12);
        adjusted = true;
      }
      if( adjusted )
      {
        numAdjusted++;
      }


      unsigned long thisStart = stretchNote(noteFilteredItem->startTick,gStretch);
      unsigned long thisEnd = stretchNote(noteFilteredItem->endTick,gStretch);

      //unsigned long origMinTiming = 


      long thisStartDelta = ((long)thisStart) % (long)MIN_TIMING_MS;

      if( thisStartDelta > ((long)MIN_TIMING_MS / 2) )
      {
        thisStartDelta -= (long)MIN_TIMING_MS;
      }

      thisStart = thisStart - thisStartDelta;
      thisEnd = thisEnd - thisStartDelta;

      noteFilteredItem->startTick = thisStart;


      /* snap to lead */
      trackListItemType * trackItem = GetTrack(
        trackFilteredList,
        noteFilteredItem->track);

      if( trackItem->lead )
      {
        lastLeadTrackStart = noteFilteredItem->startTick;
      }
      else
      {
        if( noteFilteredItem->startTick - lastLeadTrackStart <= (MIN_TIMING_MS * 2) )
        {
          noteFilteredItem->startTick = lastLeadTrackStart;
        }
      }


      long thisDurationDelta = ((long)thisEnd - (long)thisStart) % (long)MIN_TIMING_MS;

      if( thisDurationDelta > ((long)MIN_TIMING_MS / 2) )
      {
        thisDurationDelta -= (long)MIN_TIMING_MS;
      }

      noteFilteredItem->endTick = thisEnd - thisDurationDelta;

      if( noteFilteredItem->endTick <= noteFilteredItem->startTick )
      {
        noteFilteredItem->endTick += MIN_TIMING_MS;
      }

    }
    else
    {
      /* Filter out of range notes. */
      if( noteFilteredItem->note < instMinNote || noteFilteredItem->note > instMaxNote )
      {
        numDeleted++;
        noteFilteredItem = DeleteNote( noteFilteredList, noteFilteredItem );
        noteItem = (noteListItemType *)noteItem->next;
        continue;
      }
    }

    noteItem = (noteListItemType *)noteItem->next;
  }

  if( o_NumDeleted != NULL )
  {
    *o_NumDeleted = numDeleted;
  }
  if( o_NumAdjusted != NULL )
  {
    *o_NumAdjusted = numAdjusted;
  }

  return 0;
}

/*
** FUNCTION getNoteLetter
**
** DESCRIPTION
**   
**
*****************************************************************************/
void getNoteLetter(unsigned char note, char * letter)
{
#define CENTER_OCTAVE 4
  char octave = (char)((float)note / 12.0f);

  switch( note % 12 )
  {
  case 0:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=C,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=c");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=C");
    }
    break;
  case 1:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^C,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^c");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^C");
    }
    break;
  case 2:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=D,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=d");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=D");
    }
    break;
  case 3:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^D,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^d");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^D");
    }
    break;
  case 4:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=E,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=e");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=E");
    }
    break;
  case 5:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=F,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=f");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=F");
    }
    break;
  case 6:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^F,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^f");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^F");
    }
    break;
  case 7:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=G,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=g");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=G");
    }
    break;
  case 8:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^G,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^g");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^G");
    }
    break;
  case 9:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=A,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=a");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=A");
    }
    break;
  case 10:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^A,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^a");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "^A");
    }
    break;
  case 11:
    if(octave < CENTER_OCTAVE)
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=B,");
    }
    else if( octave > CENTER_OCTAVE )
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=b");
    }
    else
    {
      strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "=B");
    }
    break;
  default:
    strcpy_s(letter, MAX_NOTE_LETTER_SIZE, "OMG");
    break;

  }
}

/*
** FUNCTION writeABCFile
**
** DESCRIPTION
**   
**
*****************************************************************************/
int writeABCFile(char * inFileName, char * instName)
{
  char  * cpos = NULL;
  char    outFileNameBase[MAX_STRING_SIZE] = { 0 }, outFileName[MAX_STRING_SIZE] = { 0 }, outFileNamePart[MAX_STRING_SIZE] = { 0 };
  FILE  * outFilePtr = NULL;

  boolean fixedLength = false;

  if( 0 == strncmp(instName, "lute", 4) )
  {
    fixedLength = true;
  }
  else if( 0 == strncmp(instName, "theorbo", 7) )
  {
    fixedLength = true;
  }
  else if( 0 == strncmp(instName, "horn", 4) )
  {
    fixedLength = false;
  }
  else if( 0 == strncmp(instName, "flute", 5) )
  {
    fixedLength = false;
  }
  else if( 0 == strncmp(instName, "bagpipes", 8) )
  {
    fixedLength = false;
  }
  else if( 0 == strncmp(instName, "clarinet", 8) )
  {
    fixedLength = false;
  }

  /* Generate output filenames. */  
  strcpy_s( outFileNameBase, MAX_STRING_SIZE, inFileName );
  cpos = outFileNameBase + strlen( outFileNameBase );
  while( *cpos != '.' && cpos != outFileNameBase ) cpos--;
  if ( cpos != outFileNameBase )
  {
    *cpos = '\0';
  }

  sprintf_s(outFileNamePart, MAX_STRING_SIZE, "_%s.abc",instName);

  strcpy_s( outFileName, MAX_STRING_SIZE, outFileNameBase );
  strcat_s( outFileName, MAX_STRING_SIZE, outFileNamePart );

  /* Attempt to open all output files for writing. */
  fopen_s( &outFilePtr, outFileName, "wb" );
  if ( outFilePtr == NULL )
  {
    printf( "ERROR: Can not open %s for writing. (Is there enough space?)\n", 
        outFileName );
    exit(-1);   
  }

  /* Show filenames to user. */
  printf( "\n\n[output]\nABC file: %s\n\n", outFileName );

  /*                                                                        *\
  ============================== Write ABC Header ============================
  \*                                                                        */
  fprintf(outFilePtr, "X: 1\r\n");
  fprintf(outFilePtr, "T: %s %s\r\n",outFileNameBase,instName);
  fprintf(outFilePtr, "Z: Converted by MIDI2ABC: http://www.mindexpressions.com/users/lotro/\r\n");
  fprintf(outFilePtr, "%%  From MIDI: %s\r\n",inFileName);
  fprintf(outFilePtr, "%%  Transposed by: %d, Time scaled by: %0.02f\r\n", 0, gStretch);
  fprintf(outFilePtr, "L: 1/16\r\n");
  fprintf(outFilePtr, "Q: 1/4=250\r\n");
  fprintf(outFilePtr, "K: C\r\n\r\n");

  SortNoteListByStart(noteFilteredList);
  
  noteListItemType * noteItem = noteFilteredList;
  char noteChordDef[256] = { 0 };

  //unsigned long notesOn[127] = { 0 };

  int numPrinted = 0;
  int minChordLength = 0;
  int maxChordLength = 0;
  unsigned long inChord = 0;
  unsigned long lastStartTick = 0;
  unsigned long lastEndTick = 0;

  int numChordNotes = 0;
  while( noteItem != NULL )
  {
    char noteDef[MAX_NOTE_DEFINITION_SIZE] = { 0 };
    char noteLetter[MAX_NOTE_LETTER_SIZE] = { 0 };

    if( lastStartTick != noteItem->startTick )
    {
      int silenceDuration = (int)((noteItem->startTick - lastEndTick) / MIN_TIMING_MS);

      if( silenceDuration > 0 )
      {
        while( silenceDuration > 16 )
        {
          fprintf(outFilePtr,"z%d",16);
          numPrinted++; silenceDuration -= 16;
        }

        fprintf(outFilePtr,"z%d",silenceDuration);
        numPrinted++;

      }
    }

    if( minChordLength != maxChordLength && inChord == 0 )
    {
      int chordMakeupZ = maxChordLength - minChordLength;

      minChordLength = 0;
      maxChordLength = 0;

      while( chordMakeupZ > 16 )
      {
        fprintf(outFilePtr,"z%d",16);
        numPrinted++; chordMakeupZ -= 16;
      }
      fprintf(outFilePtr,"z%d",chordMakeupZ);
      numPrinted++;
    }

    if( fixedLength )
    {
      lastEndTick = noteItem->startTick + 60;
    }


    getNoteLetter(noteItem->note,noteLetter);

    int noteDuration = (int)((noteItem->endTick - noteItem->startTick) / MIN_TIMING_MS);

    if( fixedLength )
    {
      noteDuration = 1;
    }

    if( noteItem->next != NULL
        && noteItem->startTick == ((noteListItemType *)noteItem->next)->startTick )
    {
      if( !inChord )
      {
        inChord = 1;
        minChordLength = noteDuration;
        maxChordLength = noteDuration;
        fprintf(outFilePtr,"[");
      }
    }

    lastStartTick = noteItem->startTick;

    sprintf_s(noteDef, MAX_NOTE_DEFINITION_SIZE, "%s", noteLetter);

    if( inChord )
    {
      numChordNotes++;

      if( minChordLength > noteDuration )
      {
        minChordLength = noteDuration;
      }
      if( maxChordLength < noteDuration )
      {
        maxChordLength = noteDuration;
      }
    }

    if( numChordNotes < 6 )
    {
      while( noteDuration > 16 )
      {
        fprintf(outFilePtr,"%s%d",noteDef,16);
        if( !inChord )
        {
          fprintf(outFilePtr,"-");
        }
        numPrinted++; noteDuration -= 16;
      }

      fprintf(outFilePtr,"%s%d",noteDef,noteDuration);
      numPrinted++;
    }

    if( noteItem->next == NULL
        || noteItem->startTick != ((noteListItemType *)noteItem->next)->startTick )
    {
      if( inChord )
      {
        inChord = 0; numChordNotes = 0;
        fprintf(outFilePtr,"]");
      }
    }

    if(!inChord && numPrinted >= 32)
    {
      fprintf(outFilePtr,"\\\r\n");
      numPrinted = 0;
    }

    noteItem = (noteListItemType *)noteItem->next;
  }

  fclose(outFilePtr);
  return 0;
}

/*
** FUNCTION sendMidiMessage
**
** DESCRIPTION
**   
**
*****************************************************************************/
inline void sendMidiMessage(midiEventType noteEvent)
{
  MMRESULT mmr;
  DWORD * val = (DWORD *)&noteEvent;

  for( unsigned int i = 0; i < numDevices; i++ )
  {
    if ( devOpen[i] )
    {
      mmr = midiOutShortMsg(hMidiOut[i],*val);
    }
  }
}

/*
** FUNCTION sendMidiReset
**
** DESCRIPTION
**   
**
*****************************************************************************/
inline void sendMidiReset()
{
  MMRESULT mmr;

  for( unsigned int i = 0; i < numDevices; i++ )
  {
    if ( devOpen[i] )
    {
      mmr = midiOutReset(hMidiOut[i]);
    }
  }
}

/*
** FUNCTION sendMidiVolume
**
** DESCRIPTION
**   
**
*****************************************************************************/
inline void sendMidiVolume(DWORD vol)
{
  MMRESULT mmr;
  for( unsigned int i = 0; i < numDevices; i++ )
  {
    if ( devOpen[i] )
    {
      mmr = midiOutSetVolume(hMidiOut[i],vol);
    }
  }
}

/*
** FUNCTION _tmain
**
** DESCRIPTION
**   Program entry-point.
**
*****************************************************************************/
int previewTrack(unsigned int track)
{
  char inputBuffer[256] = { 0 };
  bool play = true;

  noteListItemType * noteItem;

  SortNoteListByStart(noteFilteredList);

  noteItem = noteFilteredList;

  if( track > 0 )
  {
    printf("\nPreviewing track: %d\nPress any key to stop.\n",track);
  }
  else
  {
    printf("\nPreviewing file.\nPress any key to stop.\n");
  }

  DWORD startTime = GetTickCount();
  DWORD fileTime = 0;

  /* Get a handle to the console */
  HANDLE h = GetStdHandle ( STD_INPUT_HANDLE );

  midiEventType noteOn,noteOff,genMsg;

  sendMidiVolume(0);

  genMsg.channel = 1;
  genMsg.event = 0xC; // program change
  genMsg.param1 = 25; // nylon guitar
  genMsg.param2 = 0;

  sendMidiMessage(genMsg);

#if 0
  genMsg.event = 0xC; // program change
  genMsg.param1 = 25; // nylon guitar
  genMsg.param2 = 0;

  sendMidiMessage(genMsg);
#endif

  genMsg.event = 0xB; // controller change
  genMsg.param1 = 0x40; // sustain
  genMsg.param2 = 0x7f; // on

  sendMidiMessage(genMsg);

  noteOn.channel = 1;
  noteOn.event = 9;
  noteOn.param2 = 127;
  noteOn.param1 = 0;

  noteOff.channel = 1;
  noteOff.event = 8;
  noteOff.param2 = 0x40;
  noteOff.param1 = 0;

  sendMidiVolume(0xFFFF);

  DWORD nextPrint = 0;

  SetConsoleMode(h,ENABLE_PROCESSED_INPUT);
  FlushConsoleInputBuffer(h);

  while( play && noteItem != NULL )
  {
    if( ( track != 0 
          && noteItem->track != track )
        || noteItem->channel == 9
        )
    {
      noteItem = (noteListItemType *)noteItem->next;
      continue;
    }

    while( play && noteItem->startTick > fileTime )
    {
      DWORD numInputRead = 0;

      if( fileTime > nextPrint )
      {
        unsigned int sec = 0, min = 0;

        min = ((fileTime / 60) / 1000);
        sec = (fileTime / 1000) - (60 * min);
        printf("\r[%02d:%02d]",min,sec);
        nextPrint += 500;
      }

      Sleep(5);

      //ReadConsole(h,&inputBuffer,1,&numInputRead,NULL);
      GetNumberOfConsoleInputEvents(h,&numInputRead);
      if( numInputRead > 0 )
      {
        INPUT_RECORD inputRecord;
        DWORD t_numInputRead = 0;

        for( unsigned int x = 0; x < numInputRead; x++ )
        {
          ReadConsoleInput(h,&inputRecord,1,&t_numInputRead);

          if( t_numInputRead > 0
              && inputRecord.EventType == KEY_EVENT )
          {
            if( inputRecord.Event.KeyEvent.bKeyDown )
            {
              play = false;
              break;
            }
          }
        }
        numInputRead = 0;
      }

      fileTime = GetTickCount() - startTime;
    }

    if( !play )
    {
      break;
    }

    if( noteItem->note < N_C4 ) //(unsigned char)(OCTAVE((int)N_C4,(int)-1)) )
    {
      noteOn.param1 = noteItem->note - 12;
      noteOff.param1 = noteItem->note - 12;
    }
    else
    {
      noteOn.param1 = noteItem->note;
      noteOff.param1 = noteItem->note;
    }

    sendMidiMessage(noteOn);

    /*
    if( noteItem->next != NULL &&
      noteItem->startTick != ((noteListItemType *)noteItem->next)->startTick )
    {
       Sleep(60);
    }
    */

    sendMidiMessage(noteOff);
/*    Sleep(1000);
    break;
*/
    noteItem = (noteListItemType *)noteItem->next;
  }

  printf("\r                ");
  sendMidiVolume(0);
  sendMidiReset();
  return 0;
}

/*
** FUNCTION CleanUpExit
**
** DESCRIPTION
**   
**
*****************************************************************************/
void CleanUpExit(int retval)
{
  ResetNoteList( noteList );
  ResetNoteList( noteFilteredList );
  ResetTrackList( trackList );
  ResetTrackList( trackFilteredList );
  midiCloseDevices();
  exit(retval);
}

/*
** FUNCTION _tmain
**
** DESCRIPTION
**   Program entry-point.
**
*****************************************************************************/
int _tmain(int argc, _TCHAR* argv[])
{
  int retval = 0;
  unsigned int format = 0;
  unsigned int numTracks = 0;

  SetConsoleTitle("MIDI2ABC " VERSION );

  /* Print copyright information. */
  print_header_info();

  /* Check # of arguments. */
  if( argc != 3 ) 
  {
    if( argc > 3 )
    {
      printf("ERROR: Too many arguments.\n");
    }
    else
    {
      printf("ERROR: Not enough arguments.\n");
    }
    printf("\nUsage: MIDI2ABC <instrument> <inFileName>\n\n");
    printf("  instrument\t= One of the following:\n");
    printf("  \t\t    lute theorbo clarinet horn flute bagpipes\n");
    printf("  inFileName\t= The filename of the MIDI file.\n\n");
    exit(-1);
  }

  if( 0 == _strnicmp(argv[1], "lute", 4) )
  {
    instMaxNote = LUTE_NOTE_MAX;
    instMinNote = LUTE_NOTE_MIN;
  }
  else if( 0 == _strnicmp(argv[1], "harp", 4) )
  {
    instMaxNote = HARP_NOTE_MAX;
    instMinNote = HARP_NOTE_MIN;
  }
  else if( 0 == _strnicmp(argv[1], "theorbo", 7) )
  {
    instMaxNote = THEORBO_NOTE_MAX;
    instMinNote = THEORBO_NOTE_MIN;
  }
  else if( 0 == _strnicmp(argv[1], "horn", 4) )
  {
    instMaxNote = HORN_NOTE_MAX;
    instMinNote = HORN_NOTE_MIN;
  }
  else if( 0 == _strnicmp(argv[1], "flute", 5) )
  {
    instMaxNote = FLUTE_NOTE_MAX;
    instMinNote = FLUTE_NOTE_MIN;
  }
  else if( 0 == _strnicmp(argv[1], "bagpipes", 8) )
  {
    instMaxNote = BAGPIPES_NOTE_MAX;
    instMinNote = BAGPIPES_NOTE_MIN;
  }
  else if( 0 == _strnicmp(argv[1], "clarinet", 8) )
  {
    instMaxNote = CLARINET_NOTE_MAX;
    instMinNote = CLARINET_NOTE_MIN;
  }
  else
  {
    printf("ERROR: Bad instrument name.\n");
    CleanUpExit(-1);
  }

  /* Enumerate and open the MIDI devices. */
  numDevices = midiOutGetNumDevs();
  if( numDevices < 1 )
  {
    printf("ERROR: Sorry about this, but I can't seem to find any MIDI input devices installed in your computer. My advice: check to make sure you have drivers for it installed properly. Beyond that, I can't help ya, I'm afraid. :-(\n");
		CleanUpExit(-1);
  }

  unsigned int numOpened = 0;

  for( unsigned int i = 0; i < 1/* numDevices */; i++ )
  {
    if( midiOpenDevice(i) )
    {
      numOpened++;
    }
  }

  if( numOpened < 1 )
  {
    printf("ERROR: Sorry about this, but, though I did find at least one MIDI Input device to use, I can't seem to get control of any of them. My advice: make sure there are no other applications running that might use MIDI. Beyond that, I can't help ya, I'm afraid. :-(\n");
		CleanUpExit(-1);
  }

  if( numOpened != numDevices )
  {
    printf("NOTE: This may or may not cause a problem for you, but I was unable to get control over at least one of the MIDI Input devices in your system. I did manage to get control of at least one of them, so let's hope that it is the one you have your equipment plugged in to. :-) Otherwise, if you can't get this to work, check to make sure you don't have any other software running that would use the MIDI devices and try restarting this utility.\n");
    //exit(-1); 
    (void)_getch();
  }



  /* Parse MIDI file */
  if( -1 == parseMIDIFile( argv[2], &format, &numTracks ) )
  {
    CleanUpExit(-1);
  }

  trackListItemType * trackItem = trackList;
  while( trackItem != NULL )
  {
    if( format == 1 )
    {
      trackItem->track--;
    }

    if( trackItem->track == 0 )
    {
      trackItem = DeleteTrack( 
        trackList, trackItem );
    }
    else
    {
      trackItem = (trackListItemType *)trackItem->next;
    }
  }

  noteListItemType * noteItem = noteList;
  while( noteItem != NULL )
  {
    if( format == 1 )
    {
      noteItem->track--;
    }

    if( ( format == 1 && noteItem->track == 0 ) 
        || noteItem->channel == 9 )
    {
      noteItem = DeleteNote( noteList, noteItem );
    }
    else
    {
      noteItem = (noteListItemType *)noteItem->next;
    }
  }

  trackItem = trackList;
  while( trackItem != NULL )
  {
    if( CountNotes( noteList, trackItem->track ) == 0 )
    {
      trackItem = DeleteTrack( 
        trackList, trackItem );
    }
    else
    {
      trackItem = (trackListItemType *)trackItem->next;
    }
  }


  selectedTrack = trackList;

  adaptNoteData(true);

  int usrInput = 0;
  while( usrInput != 'w' && usrInput != 'W' )
  {
    usrInput = 0;

    NewScreen(true);

    printf("\n[up|down] = change track selection");
    printf("\n[enter] = modify track");
    printf("\n[m] = toggle inclusion of track in ABC file (+ = included)");
    printf("\n[s] = set as lead track (#) which affects how other tracks can be transposed to maintain harmony");
    printf("\n[p] = preview track");
    printf("\n[f] = preview the whole file");
    printf("\n[q] = quit without saving changes");
    printf("\n[w] = accept track changes and write ABC file\n");

    do
    {
      usrInput = _getch();
    } while( 
      usrInput != 224 
      && usrInput != 13 
      && usrInput != 'w' 
      && usrInput != 'W' 
      && usrInput != 'm'
      && usrInput != 'M'
      && usrInput != 's'
      && usrInput != 'S'
      && usrInput != 'p'
      && usrInput != 'P'
      && usrInput != 'f'
      && usrInput != 'F'
      && usrInput != 'q'
      && usrInput != 'Q'
      );

    if( usrInput == 224 )
    {
      usrInput = _getch();

      if( usrInput == 72 )
      {
        selectedTrack = (trackListItemType *)selectedTrack->prev;
      }
      else if( usrInput == 80 )
      {
        selectedTrack = (trackListItemType *)selectedTrack->next;
      }

      if( selectedTrack == NULL ) { selectedTrack = trackList; }
    }
    else if( usrInput == 'm' || usrInput == 'M' )
    {
      selectedTrack->active = !selectedTrack->active;
    }
    else if( usrInput == 's' || usrInput == 'S' )
    {
      trackListItemType * t_trackItem = trackList;

      selectedTrack->active = true;

      while( t_trackItem != NULL )
      {
        t_trackItem->lead = false;
        t_trackItem = (trackListItemType *)t_trackItem->next;
      }

      selectedTrack->lead = true;
    }
    else if( usrInput == 'p' || usrInput == 'P' )
    {
      filterNoteData( NULL, NULL );
      previewTrack( selectedTrack->track );
    }
    else if( usrInput == 'f' || usrInput == 'F' )
    {
      filterNoteData( NULL, NULL );
      previewTrack( 0 );
    }
    else if( usrInput == 'q' || usrInput == 'Q' )
    {
      printf("\nChanges abandoned, quitting.\n");
      CleanUpExit(-1);
    }
    else if( usrInput == 13 )
    {
      if( selectedTrack != NULL )
      {
        /* Perform adaptation process */
        if( -1 == adaptNoteData_Transpose( selectedTrack, instMinNote, instMaxNote ) )
        {
          printf("\nERROR adapting track: %d\n",selectedTrack->track);
          CleanUpExit(-1);
        }
      }
      else
      {
        printf("\nERROR: No such track: %d\n",selectedTrack->track);
        CleanUpExit(-1);
      }
    }
  }

  NewScreen(true);

  /* Filter note data */
  unsigned int numDeleted = 0, numAdjusted = 0;

  if( -1 == filterNoteData( &numDeleted, &numAdjusted ) )
  {
    CleanUpExit(-1);
  }

  printf("\nNotes removed: %d, notes adjusted: %d\n",numDeleted,numAdjusted);

  (void)adaptNoteData_Quantize();


#if 0
  {
    FILE * tstptr = fopen("out5.csv","w");

    fprintf(tstptr,"track,channel,note,startTick,endTick,duration\n");

    noteListItemType * noteItem = noteList;
    while( noteItem != NULL )
    {
      fprintf(tstptr,"%d,%d,%d,%d,%d,%d\n",
        noteItem->track,
        noteItem->channel,
        noteItem->note,
        noteItem->startTick,
        noteItem->endTick,
        noteItem->endTick - noteItem->startTick
        );
      noteItem = (noteListItemType *)noteItem->next;
    }
    fclose(tstptr);
  }
#endif

  /* Write ABC file */
  if( -1 == writeABCFile( argv[2], argv[1] ) )
  {
    CleanUpExit(-1);
  }

  return 0;
}

