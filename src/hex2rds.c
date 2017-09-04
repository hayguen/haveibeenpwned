/*
 * hex2rds
 *
 * Copyright 2017 Hayati Ayguen.  Distributed under the terms
 * of the GNU General Public License (GPL)
 *
 * hex2rds converts text files with hexadecimal (hash) lines to raw data set (rds) files
 *
 * Usage: see below at usage()
 *
 * Author:  Hayati Ayguen
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBGOUT  0

/* returns length in number of hexadecimal digits - might be odd! */
static
int hashLen( const char * s )
{
  int len = 0;
  int idx = 0;
  while ( s[idx] )
  {
    if ( isxdigit(s[idx]) )
      ++len;
    else if ( isblank(s[idx]) )
      ;  /* ignore space/tab */
    else if ( isspace(s[idx]) )
      break;
    else
    {
      len = -1; /* error */
      break;
    }
    ++idx;
  }
#if 0 && DBGOUT
  if ( len <= 0 || (len & 1) )
    fprintf(stderr, "line '%s' has hashLen %d\n", s, len);
#endif
  return len;
}


/* returns number of converted bytes */
static
int convertHash( const char * s, size_t maxBinLen, unsigned char * bin )
{
  int xlen = 0;
  int blen = 0;  /* 0 length is no error - simply empty line */
  int idx = 0;
  int v = 0;
  while ( s[idx] )
  {
    if ( isxdigit(s[idx]) )
    {
      char c = s[idx];
      v = v << 4;
      if ( c >= '0' && c <= '9' )
        v = v | ( c - '0' );
      else if ( c >= 'A' && c <= 'F' )
        v = v | ( 10 + c - 'A' );
      else if ( c >= 'a' && c <= 'f' )
        v = v | ( 10 + c - 'a' );
      ++xlen;
      if ( !(xlen & 1) )
      {
        if ( blen >= maxBinLen )
        {
          blen = -1; /* error */
          break;
        }
        bin[ blen++ ] = (unsigned char)(v);
        v = 0;
      }
    }
    else if ( isblank(s[idx]) )
      ;  /* ignore space/tab */
    else if ( isspace(s[idx]) )
      break;
    else
    {
      blen = -1; /* error */
      break;
    }
    ++idx;
  }
  if ( xlen & 1 )
  {
#if 0 && DBGOUT
    fprintf(stderr, "line '%s' has %d odd number of hexadecimals!\n", s, xlen);
#endif
    blen = -1; /* error */
  }
#if 0 && DBGOUT
  if ( blen != (int)maxBinLen )
    fprintf(stderr, "line '%s' has binLen %d\n", s, blen);
#endif
  return blen;
}

static
void usage() {
  fputs("Usage: hex2rds [-h | --help] [-n <rawSize>] [-i <input>] [-o <output>]\n",stderr);
  fputs("  hex2rds converts text files with hexadecimal (hash) codes to raw data set (rds) files\n",stderr);
  fputs("  every input line must have same even length! spaces are ignored.\n",stderr);
  fputs("  -h | --help   print usage\n",stderr);
  fputs("  -n <rawSize>  tell expected raw length in bytes, e.g. 20 for SHA1\n",stderr);
  fputs("                by default, the 1st line's length will be used\n",stderr);
  fputs("  -i <input>    use input from file. default: stdin\n",stderr);
  fputs("  -o <output>   output to file. default: stdout\n",stderr);
}


int main(int argc, char *argv[])
{
  FILE * inp = stdin;
  FILE * out = stdout;
  size_t rawSize = 0;
  int printUsage = 0;
  int ret = 0;  /* default: no error */
  int i = 0;
  unsigned converted = 0;

  char * lineBuf = NULL;
  unsigned char * binBuf = NULL;

  while (1)
  {

    for ( i = 1; i < argc; ++i )
    {
      if ( !strcmp(argv[i], "-n") && i+1 < argc )
      {
        int tmp = atoi( argv[i+1] );
        if (tmp <= 0)
        {
          fprintf(stderr, "error: rawSize (value for '-n' = '%s') must be > 0 !\n", argv[i+1]);
          ret = 10;
          break;
        }
        rawSize = tmp;
        ++i;
      }
      else if ( !strcmp(argv[i], "-i") && i+1 < argc )
      {
        inp = fopen( argv[i+1], "r" );
        if (!inp)
        {
          fprintf(stderr, "error: input file '%s' could not be opened!\n", argv[i+1]);
          ret = 10;
          break;
        }
        ++i;
      }
      else if ( !strcmp(argv[i], "-o") && i+1 < argc )
      {
        out = fopen( argv[i+1], "wb" );
        if (!out)
        {
          fprintf(stderr, "error: output file '%s' could not be opened!\n", argv[i+1]);
          ret = 10;
          break;
        }
        ++i;
      }
      else if ( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") )
      {
        printUsage = 1;
      }
      else
      {
        fprintf(stderr, "error: ignoring option '%s'!\n", argv[i]);
        printUsage = 1;
      }
    }

    if (ret)
      break;

    if (printUsage)
    {
      usage();
      ret = 1;
      break;
    }

    /* assume up to 3 characters (2 nibbles + 1 space) per byte and some extra spaces */
    const int bufLen = ( rawSize > 0 ) ? (rawSize * 3 + 16) : 1024;
    free(lineBuf);
    lineBuf = malloc( bufLen * sizeof(char) );
    if (!lineBuf)
    {
      fprintf(stderr, "error allocating line buffer of %u bytes!\n", (unsigned)(bufLen*sizeof(char)) );
      ret = 10;
      break;
    }
    if ( rawSize > 0 )
    {
      free(binBuf);
      binBuf = (unsigned char*)malloc( rawSize * sizeof(unsigned char) );
      if (!binBuf)
      {
        fprintf(stderr, "error allocating binary buffer of %u bytes!\n", (unsigned)(rawSize*sizeof(unsigned char)) );
        ret = 10;
        break;
      }
    }
    int lineNo = 0;

    while ( !ret && !feof(inp))
    {
      char * r = fgets(lineBuf, bufLen, inp);
      if (r)
      {
        ++lineNo;

        if ( !rawSize )
        {
          const int len = hashLen(lineBuf);
          if (len <= 0 || (len & 1))
          {
            fprintf(stderr, "error: hexadecimal length of 1st line (=%d) must be even!\n", len);
            ret = 10;
            break;
          }
          rawSize = len / 2;
          fprintf(stderr, "info: using rawSize %d from 1st line with hexLen %d\n", (int)rawSize, len);
          free(binBuf);
          binBuf = (unsigned char*)malloc( rawSize * sizeof(unsigned char) );
          if (!binBuf)
          {
            fprintf(stderr, "error allocating binary buffer of %u bytes!\n", (unsigned)(rawSize*sizeof(unsigned char)) );
            ret = 10;
            break;
          }
        }

        int bLen = convertHash(lineBuf, rawSize, binBuf);
        if ( ! bLen )
        {
          /* empty line */
        }
        else if ( bLen < 0 || (size_t)bLen != rawSize )
        {
          fprintf(stderr, "warning: hexadecimal length %d of line %d does not match expected value of %d! skipping line '%s'\n", bLen, lineNo, (int)rawSize, lineBuf);
        }
        else
        {
          size_t w = fwrite( binBuf, rawSize, 1, out );
          if ( w != 1 )
          {
            fprintf(stderr, "error writing binary for line %d to output!\n", lineNo);
            ret = 10;
            break;
          }
          ++converted;
        }
      }
    }

    break;
  }

  free(lineBuf);
  free(binBuf);

  if ( converted )
    fprintf(stderr, "successfully converted %u hexadecimal lines.\n", converted);

  if ( inp != stdin )
    fclose(inp);
  if ( out != stdout )
    fclose(out);

  return ret;
}

