
const char * GetInstrumentName( unsigned int inst )
{
  inst++;

  if( inst == 1 )
  {
    return "Acoustic Grand Piano";
  }
  if( inst == 65 )
  {
    return "Soprano Sax";
  }
  if( inst == 2 )
  {
    return "Bright Acoustic Piano";
  }
  if( inst == 66 )
  {
    return "Alto Sax";
  }
  if( inst == 3 )
  {
    return "Electric Grand Piano";
  }
  if( inst == 67 )
  {
    return "Tenor Sax";
  }
  if( inst == 4 )
  {
    return "Honky-tonk Piano";
  }
  if( inst == 68 )
  {
    return "Baritone Sax";
  }
  if( inst == 5 )
  {
    return "Electric Piano 1";
  }
  if( inst == 69 )
  {
    return "Oboe";
  }
  if( inst == 6 )
  {
    return "Electric Piano 2";
  }
  if( inst == 70 )
  {
    return "English Horn";
  }
  if( inst == 7 )
  {
    return "Harpsichord";
  }
  if( inst == 71 )
  {
    return "Bassoon";
  }
  if( inst == 8 )
  {
    return "Clavi";
  }
  if( inst == 72 )
  {
    return "Clarinet";
  }
  if( inst == 9 )
  {
    return "Celesta";
  }
  if( inst == 73 )
  {
    return "Piccolo";
  }
  if( inst == 10 )
  {
    return "Glockenspiel";
  }
  if( inst == 74 )
  {
    return "Flute";
  }
  if( inst == 11 )
  {
    return "Music Box";
  }
  if( inst == 75 )
  {
    return "Recorder";
  }
  if( inst == 12 )
  {
    return "Vibraphone";
  }
  if( inst == 76 )
  {
    return "Pan Flute";
  }
  if( inst == 13 )
  {
    return "Marimba";
  }
  if( inst == 77 )
  {
    return "Blown Bottle";
  }
  if( inst == 14 )
  {
    return "Xylophone";
  }
  if( inst == 78 )
  {
    return "Shakuhachi";
  }
  if( inst == 15 )
  {
    return "Tubular Bells";
  }
  if( inst == 79 )
  {
    return "Whistle";
  }
  if( inst == 16 )
  {
    return "Dulcimer";
  }
  if( inst == 80 )
  {
    return "Ocarina";
  }
  if( inst == 17 )
  {
    return "Drawbar Organ";
  }
  if( inst == 81 )
  {
    return "Lead 1 (square)";
  }
  if( inst == 18 )
  {
    return "Percussive Organ";
  }
  if( inst == 82 )
  {
    return "Lead 2 (sawtooth)";
  }
  if( inst == 19 )
  {
    return "Rock Organ";
  }
  if( inst == 83 )
  {
    return "Lead 3 (calliope)";
  }
  if( inst == 20 )
  {
    return "Church Organ";
  }
  if( inst == 84 )
  {
    return "Lead 4 (chiff)";
  }
  if( inst == 21 )
  {
    return "Reed Organ";
  }
  if( inst == 85 )
  {
    return "Lead 5 (charang)";
  }
  if( inst == 22 )
  {
    return "Accordion";
  }
  if( inst == 86 )
  {
    return "Lead 6 (voice)";
  }
  if( inst == 23 )
  {
    return "Harmonica";
  }
  if( inst == 87 )
  {
    return "Lead 7 (fifths)";
  }
  if( inst == 24 )
  {
    return "Tango Accordion";
  }
  if( inst == 88 )
  {
    return "Lead 8 (bass + lead)";
  }
  if( inst == 25 )
  {
    return "Acoustic Guitar (nylon)";
  }
  if( inst == 89 )
  {
    return "Pad 1 (new age)";
  }
  if( inst == 26 )
  {
    return "Acoustic Guitar (steel)";
  }
  if( inst == 90 )
  {
    return "Pad 2 (warm)";
  }
  if( inst == 27 )
  {
    return "Electric Guitar (jazz)";
  }
  if( inst == 91 )
  {
    return "Pad 3 (polysynth)";
  }
  if( inst == 28 )
  {
    return "Electric Guitar (clean)";
  }
  if( inst == 92 )
  {
    return "Pad 4 (choir)";
  }
  if( inst == 29 )
  {
    return "Electric Guitar (muted)";
  }
  if( inst == 93 )
  {
    return "Pad 5 (bowed)";
  }
  if( inst == 30 )
  {
    return "Overdriven Guitar";
  }
  if( inst == 94 )
  {
    return "Pad 6 (metallic)";
  }
  if( inst == 31 )
  {
    return "Distortion Guitar";
  }
  if( inst == 95 )
  {
    return "Pad 7 (halo)";
  }
  if( inst == 32 )
  {
    return "Guitar harmonics";
  }
  if( inst == 96 )
  {
    return "Pad 8 (sweep)";
  }
  if( inst == 33 )
  {
    return "Acoustic Bass";
  }
  if( inst == 97 )
  {
    return "FX 1 (rain)";
  }
  if( inst == 34 )
  {
    return "Electric Bass (finger)";
  }
  if( inst == 98 )
  {
    return "FX 2 (soundtrack)";
  }
  if( inst == 35 )
  {
    return "Electric Bass (pick)";
  }
  if( inst == 99 )
  {
    return "FX 3 (crystal)";
  }
  if( inst == 36 )
  {
    return "Fretless Bass";
  }
  if( inst == 100.)
  {
    return "FX 4 (atmosphere)";
  }
  if( inst == 37 )
  {
    return "Slap Bass 1";
  }
  if( inst == 101.)
  {
    return "FX 5 (brightness)";
  }
  if( inst == 38 )
  {
    return "Slap Bass 2";
  }
  if( inst == 102.)
  {
    return "FX 6 (goblins)";
  }
  if( inst == 39 )
  {
    return "Synth Bass 1";
  }
  if( inst == 103.)
  {
    return "FX 7 (echoes)";
  }
  if( inst == 40 )
  {
    return "Synth Bass 2";
  }
  if( inst == 104.)
  {
    return "FX 8 (sci-fi)";
  }
  if( inst == 41 )
  {
    return "Violin";
  }
  if( inst == 105.)
  {
    return "Sitar";
  }
  if( inst == 42 )
  {
    return "Viola";
  }
  if( inst == 106.)
  {
    return "Banjo";
  }
  if( inst == 43 )
  {
    return "Cello";
  }
  if( inst == 107.)
  {
    return "Shamisen";
  }
  if( inst == 44 )
  {
    return "Contrabass";
  }
  if( inst == 108.)
  {
    return "Koto";
  }
  if( inst == 45 )
  {
    return "Tremolo Strings";
  }
  if( inst == 109.)
  {
    return "Kalimba";
  }
  if( inst == 46 )
  {
    return "Pizzicato Strings";
  }
  if( inst == 110.)
  {
    return "Bag pipe";
  }
  if( inst == 47 )
  {
    return "Orchestral Harp";
  }
  if( inst == 111.)
  {
    return "Fiddle";
  }
  if( inst == 48 )
  {
    return "Timpani";
  }
  if( inst == 112.)
  {
    return "Shanai";
  }
  if( inst == 49 )
  {
    return "String Ensemble 1";
  }
  if( inst == 113.)
  {
    return "Tinkle Bell";
  }
  if( inst == 50 )
  {
    return "String Ensemble 2";
  }
  if( inst == 114.)
  {
    return "Agogo";
  }
  if( inst == 51 )
  {
    return "SynthStrings 1";
  }
  if( inst == 115.)
  {
    return "Steel Drums";
  }
  if( inst == 52 )
  {
    return "SynthStrings 2";
  }
  if( inst == 116.)
  {
    return "Woodblock";
  }
  if( inst == 53 )
  {
    return "Choir Aahs";
  }
  if( inst == 117.)
  {
    return "Taiko Drum";
  }
  if( inst == 54 )
  {
    return "Voice Oohs";
  }
  if( inst == 118.)
  {
    return "Melodic Tom";
  }
  if( inst == 55 )
  {
    return "Synth Voice";
  }
  if( inst == 119.)
  {
    return "Synth Drum";
  }
  if( inst == 56 )
  {
    return "Orchestra Hit";
  }
  if( inst == 120.)
  {
    return "Reverse Cymbal";
  }
  if( inst == 57 )
  {
    return "Trumpet";
  }
  if( inst == 121.)
  {
    return "Guitar Fret Noise";
  }
  if( inst == 58 )
  {
    return "Trombone";
  }
  if( inst == 122.)
  {
    return "Breath Noise";
  }
  if( inst == 59 )
  {
    return "Tuba";
  }
  if( inst == 123.)
  {
    return "Seashore";
  }
  if( inst == 60 )
  {
    return "Muted Trumpet";
  }
  if( inst == 124.)
  {
    return "Bird Tweet";
  }
  if( inst == 61 )
  {
    return "French Horn";
  }
  if( inst == 125.)
  {
    return "Telephone Ring";
  }
  if( inst == 62 )
  {
    return "Brass Section";
  }
  if( inst == 126.)
  {
    return "Helicopter";
  }
  if( inst == 63 )
  {
    return "SynthBrass 1";
  }
  if( inst == 127.)
  {
    return "Applause";
  }
  if( inst == 64 )
  {
    return "SynthBrass 2";
  }
  if( inst == 128.)
  {
    return "Gunshot";
  }

  return "";
}
