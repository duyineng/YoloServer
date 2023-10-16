/*****************************************************************************
*!
*! FILE NAME  : base64.c
*!
*! DESCRIPTION: Base 64 encode/decode
*!              Copied from the Axis UPnP implementation
*!
*! FUNCTIONS  : base64_encode
*! (EXTERNAL)   base64_decode
*!
*!
*! ---------------------------------------------------------------------------
*! (C) Copyright 2001, 2004, Axis Communications AB, LUND, SWEDEN
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/
//#include "stdafx.h"
#include "base64.h"
#include <assert.h>

const char* Base64_Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/****************** CONSTANT AND MACRO SECTION ******************************/

/****************** TYPE DEFINITION SECTION *********************************/

/****************** LOCAL FUNCTION DECLARATION SECTION **********************/

/****************** GLOBAL VARIABLE DECLARATION SECTION *********************/

/****************** LOCAL VARIABLE DECLARATION SECTION **********************/

/****************** FUNCTION DEFINITION SECTION *****************************/


/*****************************************************************************
*#
*# FUNCTION NAME: base64_encode
*#
*# PARAMETERS   : unsigned char *dest - where to write the encoding
*#                int dest_len - the length of dest
*#                unsigned char *src - the test to encode
*#                int src_len - the length of src
*#
*# RETURNS      : The number of octets written to dest, or -1 if the
*#                encoding would not fit.
*#
*# SIDE EFFECTS : None
*#
*# DESCRIPTION  : Do base64 encoding (per rfc 1521) of 'src'.
*#
*#----------------------------------------------------------------------------
*# HISTORY
*# 
*# DATE         NAME               CHANGES
*# ----         ----               -------
*# Nov 22 2000  Henrik Eriksson    Initial version
*# Feb  7 2001  Henrik Eriksson    Fixed buffer overrun when src not multiple
*#                                 of 3.
*# 
*#***************************************************************************/
int base64_encode(unsigned char *dest,
                  int dest_len,
                  const unsigned char *src,
                  int src_len)
{
  unsigned char *wp = dest;
  int req_len;
  int i;
  int tmp;
  int cnt = 0;
  static char table[64] =
  { 
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
    's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', '+', '/'
  };
  

   /* Calculate the required length of the base64 encoded string */
   req_len = 4 * ((src_len + 2) / 3);
   tmp = (req_len / 72) * 74 + (req_len % 72); /* the output stream is written
                                                * in lines of 72 characters
                                                * followed by CRLF */
   req_len = tmp;
   if (req_len > dest_len)
   {
     return -1;
   }

   for (i = 0; i < src_len; i += 3)
   {
     switch (src_len - i)
     {
      case 1:
       *wp++ = table[src[i] >> 2];
       *wp++ = table[((src[i] & 0x03) << 4)];
       wp += 2;
      break;
      case 2:
       *wp++ = table[src[i] >> 2];
       *wp++ = table[((src[i] & 0x03) << 4) | (src[i+1] >> 4)];
       *wp++ = table[((src[i+1] & 0xf) << 2)];
       wp++;
       break;
      default:
       *wp++ = table[src[i] >> 2];
       *wp++ = table[((src[i] & 0x03) << 4) | (src[i+1] >> 4)];
       *wp++ = table[((src[i+1] & 0xf) << 2) | (src[i+2] >> 6)];
       *wp++ = table[src[i+2] & 0x3f];
       break;
     }
     cnt += 4;
     if (cnt == 72)
     {
       *wp++ = '\r';
       *wp++ = '\n';
       cnt = 0;
     }
   }
   
   if (i == src_len + 1)
   {
     *(wp - 1) = '=';
   }
   else if (i == src_len + 2)
   {
     *(wp - 1) = *(wp - 2) = '=';
   }
   return req_len;
}


/*****************************************************************************
*#
*# FUNCTION NAME: base64_decode
*#
*# PARAMETERS   : unsigned char *dest - where to write the decoded bytestream
*#                int dest_len - the length of dest
*#                unsigned char *src - the base64 encoded data
*#                int src_len - the length of src
*#
*# RETURNS      : The number of octets written to dest, or -1 if truncated
*#
*# SIDE EFFECTS : None
*#
*# DESCRIPTION  : Base 64 decode the encoded text in 'src'. Write the result
*#                (not zere terminated) to 'dest'.
*#
*#----------------------------------------------------------------------------
*# HISTORY
*# 
*# DATE         NAME               CHANGES
*# ----         ----               -------
*# Nov 22 2000  Henrik Eriksson    Initial version
*# 
*#***************************************************************************/
int base64_decode(unsigned char *dest,
                  int dest_len,
                  const unsigned char *src,
                  int src_len)
{
  static signed char table[] =
  {
    -1 /* 000;  */, -1 /* 001;  */, -1 /* 002;  */, -1 /* 003;  */, 
    -1 /* 004;  */, -1 /* 005;  */, -1 /* 006;  */, -1 /* 007;  */, 
    -1 /* 008;  */, -1 /* 009;  */, -1 /* 010;  */, -1 /* 011;  */, 
    -1 /* 012;  */, -1 /* 013;  */, -1 /* 014;  */, -1 /* 015;  */, 
    -1 /* 016;  */, -1 /* 017;  */, -1 /* 018;  */, -1 /* 019;  */, 
    -1 /* 020;  */, -1 /* 021;  */, -1 /* 022;  */, -1 /* 023;  */, 
    -1 /* 024;  */, -1 /* 025;  */, -1 /* 026;  */, -1 /* 027;  */, 
    -1 /* 028;  */, -1 /* 029;  */, -1 /* 030;  */, -1 /* 031;  */, 
    -1 /* 032;  */, -1 /* 033;! */, -1 /* 034;" */, -1 /* 035;# */, 
    -1 /* 036;$ */, -1 /* 037;% */, -1 /* 038;& */, -1 /* 039;' */, 
    -1 /* 040;( */, -1 /* 041;) */, -1 /* 042;* */, 62 /* 043;+ */, 
    -1 /* 044;, */, -1 /* 045;- */, -1 /* 046;. */, 63 /* 047;/ */, 
    52 /* 048;0 */, 53 /* 049;1 */, 54 /* 050;2 */, 55 /* 051;3 */, 
    56 /* 052;4 */, 57 /* 053;5 */, 58 /* 054;6 */, 59 /* 055;7 */, 
    60 /* 056;8 */, 61 /* 057;9 */, -1 /* 058;: */, -1 /* 059;; */, 
    -1 /* 060;< */, -1 /* 061;= */, -1 /* 062;> */, -1 /* 063;? */, 
    -1 /* 064;@ */, 0  /* 065;A */, 1  /* 066;B */, 2  /* 067;C */, 
    3  /* 068;D */, 4  /* 069;E */, 5  /* 070;F */, 6  /* 071;G */, 
    7  /* 072;H */, 8  /* 073;I */, 9  /* 074;J */, 10 /* 075;K */, 
    11 /* 076;L */, 12 /* 077;M */, 13 /* 078;N */, 14 /* 079;O */, 
    15 /* 080;P */, 16 /* 081;Q */, 17 /* 082;R */, 18 /* 083;S */, 
    19 /* 084;T */, 20 /* 085;U */, 21 /* 086;V */, 22 /* 087;W */, 
    23 /* 088;X */, 24 /* 089;Y */, 25 /* 090;Z */, -1 /* 091;[ */, 
    -1 /* 092;\ */, -1 /* 093;] */, -1 /* 094;^ */, -1 /* 095;_ */, 
    -1 /* 096;` */, 26 /* 097;a */, 27 /* 098;b */, 28 /* 099;c */, 
    29 /* 100;d */, 30 /* 101;e */, 31 /* 102;f */, 32 /* 103;g */, 
    33 /* 104;h */, 34 /* 105;i */, 35 /* 106;j */, 36 /* 107;k */, 
    37 /* 108;l */, 38 /* 109;m */, 39 /* 110;n */, 40 /* 111;o */, 
    41 /* 112;p */, 42 /* 113;q */, 43 /* 114;r */, 44 /* 115;s */, 
    45 /* 116;t */, 46 /* 117;u */, 47 /* 118;v */, 48 /* 119;w */, 
    49 /* 120;x */, 50 /* 121;y */, 51 /* 122;z */, -1 /* 123;{ */, 
    -1 /* 124;| */, -1 /* 125;} */, -1 /* 126;~ */, -1 /* 127;  */
  };
  int           s_ix            = 0;
  int           d_ix            = 0;
  char          dec_buf[4];
  int           dec_cnt         = 0;
  int           stop            = 0;

  while ((s_ix < src_len) && !stop)
  {
    /* Compare both upper and lower limit of src[s_ix] to handle platforms
     * where char is unsigned */
    if ((src[s_ix] <= 127) && (table[src[s_ix]] != -1))
    {
      /* It's a base 64 char, decode */
      dec_buf[dec_cnt++] = table[src[s_ix]];
    }
    else if (src[s_ix] == '=')
    {
      /* pad char, should we really assume end of data (which we know should
       * be when s_ix == src_len) ? */
      stop = 1;
    }
    else
    {
      /* skip it */
    }    
    
    ++s_ix;
    
    if ((dec_cnt == 4) || stop || (s_ix == src_len))
    {
      unsigned char      bits[3];
      int                i           = dec_cnt;
      int                bytes       = (dec_cnt * 3) / 4;
      
      switch (dec_cnt)
      {
       case 1:
        dec_buf[i++] = 0;
        /* flowthrough */
       case 2:
        dec_buf[i++] = 0;
        /* flowthrough */
       case 3:
        dec_buf[i++] = 0;
        /* flowthrough */
       case 4:
        /* decode this group */
        
        if ((d_ix + bytes) > dest_len)
        {
          /* result truncated */
          return -1;
        }
        bits[0] = (unsigned char)(((dec_buf[0] << 2) & 0xfc) |  /* 6 - all, to 6 hi */
          ((dec_buf[1] >> 4) & 0x03));           /* 2 - hi, to 2 low */
        bits[1] = (unsigned char)(((dec_buf[1] << 4) & 0xf0) |  /* 4 - low, to 4 hi */
          ((dec_buf[2] >> 2) & 0x0f));           /* 4 - hi, to 4 low */
        bits[2] = (unsigned char)(((dec_buf[2] << 6) & 0xc0) |  /* 2 - low, to 2 hi */
          (dec_buf[3] & 0x3f));                  /* 6 - all, to 6 low */
        break;
       case 0:
        return d_ix;
        break;
       default:
        assert(0);
      }
      i = 0;
      while (bytes > 0)
      {
        dest[d_ix++] = bits[i++];
        --bytes;
      }
      dec_cnt = 0;
    }
  }
  
  return d_ix;
}


/********************* END OF FILE base64.c **********************************/
