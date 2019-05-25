/***************************************************************************************
* AUTHOR : sudami [sudami@163.com]
* TIME   : 2009/01/17 
* MODULE : ntreg.c - NT Registry Hive access library
*
* Description:
*   
*   ����ע���Hive                      
*
***
* Copyright (c) 2008 - 2010 sudami.
* Freely distributable in source or binary for noncommercial purposes.
* TAKE IT EASY,JUST FOR FUN.
*
****************************************************************************************/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>

#include "ntreg.h"

//////////////////////////////////////////////////////////////////////////


#define DOCORE 0

char *g_value_types[REG_MAX+1] = {
  "REG_NONE", "REG_SZ", "REG_EXPAND_SZ", "REG_BINARY", "REG_DWORD",       /* 0 - 4 */
  "REG_DWORD_BIG_ENDIAN", "REG_LINK",                                     /* 5 - 6 */
  "REG_MULTI_SZ", "REG_RESOUCE_LIST", "REG_FULL_RES_DESC", "REG_RES_REQ", /* 7 - 10 */
};


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                ������ -- �������ģ�鹦��               +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


void bzero(void *s,int n)
{
	memset(s,0,n);
}


int strncasecmp(const char *s1, const char *s2, size_t n)
{
	return _strnicmp( s1, s2, n );
}


char 
*str_dup (
	IN const char *str
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �����ַ�����һ����������ڴ���
    
Arguments:
  str - ���������ַ���ָ��

Return Value:
  ����Ʒ�ĵ�ַ   

--*/
{
    char *str_new;

    if (!str) { return 0; }

    CREATE( str_new, char, strlen(str) + 1 ); // Ϊ����ַ�������һ�����ڴ�
    strcpy( str_new, str );

    return str_new;
}


int 
GetUserInputInfo (
	IN char *prmpt, 
	IN OUT char *ibuf, 
	IN int maxlen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ��ȡ�û���CMD���������Ϣ,�����䳤��. ��������
    
Arguments:
  prmpt - ��ʾ��Ϣ
  ibuf - ����/���������,��stdin�л�ȡ
  maxlen - ���뻺������С 

Return Value:
  �û�������Ϣ�ĳ���    

--*/
{
	printf( "%s", prmpt );  
	fgets( ibuf, maxlen+1, stdin ); // �ȴ��û�����
	ibuf[strlen(ibuf)-1] = 0; 
	
	return ( strlen(ibuf) );
}


void 
hexprint (
	IN char *s, 
	IN unsigned char *bytes, 
	IN int len
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ��ӡ��Ϣ 
    
Arguments:
  s - ��ʾ��Ϣ
  bytes - ������
  len - ����������

--*/
{
   int i;

   printf( "%s", s );
   for (i = 0; i < len; i++) {
      printf( "%02x ", bytes[i] );
   }

   printf("\n");
}


void 
hexdump (
	IN char *hbuf, 
	IN int start, 
	IN int stop, 
	IN int ascii
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ��ʽ����ӡ,ÿ�д�ӡ16��16���Ƶ��ֽ��� �Լ���ascii��  
    
Arguments:
  hbuf - ������
  start - �������������ʼλ��
  stop - �������������ֹλ��
  ascii - �Ƿ������Ӧ��ascii��

--*/
{
   char c ;
   int diff, i ;
   
   while (start < stop ) 
   {
      diff = stop - start ;
      if (diff > 16) { diff = 16; }
      
      printf( ":%05X  ", start );	
      for (i = 0; i < diff; i++) // ��ӡ��ַ %02X
	  {	
		  c = (char)*( hbuf + start + i );
	      printf( "%02X ", (unsigned char)c );
      }

      if (ascii)
	  {
		  for (i = diff; i < 16; i++) { printf("   "); }
		  for (i = 0; i < diff; i++) // ��ӡ��ַ�е����� %c
		  {
			  c = (char)*(hbuf+start+i); 
			  printf("%c", isprint(c) ? c : '.'); 
		  }
/*++
  ԭ�ͣ�int isprint(int c);
  �÷���#include <ctype.h>
  ���ܣ��ж��ַ�c�Ƿ�Ϊ�ɴ�ӡ�ַ�(���ո�)
��˵������cΪ�ɴ�ӡ�ַ�(0x20-0x7e)ʱ,���ط���ֵ,���򷵻��㡣
--*/	  
      }

      printf( "\n" );
      start += 16;
   }

   return ;
}


int 
find_in_buf (
	IN char *buf, 
	IN char *what,
	IN int sz, 
	IN int len, 
	IN int start
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
    
Arguments:
  buf - ������
  what - Ҫ���ҵ�����
  sz - Ҫ���ҵ��ַ�����
  len - ���ҵķ�Χ����
  start - ��ʼ����λ��

Return Value:
  
--*/
{
	int i;
	
	for (; start < sz; start++) 
	{
		for (i = 0; i < len; i++) 
		{
			if (*(buf+start+i) != *(what+i)) { break; }
		}

		if (i == len) { return(start); }
	}
	
	return ( 0 );
}


int 
get_int (
	IN char *array 
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �ӻ������ж�ȡһ��4�ֽ�����.�õ�page�е�ǰλ����,�ռ���������: 
  1. ����ֵN<0 ��ʾ�Ѿ�������N�ֽڴ�С�Ŀռ�
  2. ����ֵN>0 ��ʾ�������ֽڴ�С�Ŀռ�
    
--*/
{
	return (
		(array[0]&0xff) + ((array[1]<<8)&0xff00) +
		((array[2]<<16)&0xff0000) +
		((array[3]<<24)&0xff000000)
		);
}


PVOID
change_to_ansi (
	IN char* szName,
	IN DWORD nNameLen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/02/02 

Routine Description:
  �����������ַ���(����ʲô��ʽ),һ��ת����ansi.
  �м� �����߱����ͷ��ڴ�
  
Arguments:
  szOutBuffer - 
  szName - ��������Ҫ��ת�����ַ���
  nNameLen - �ַ�������

Return Value:
  ת�����ansi�ַ���ָ��

--*/
{
	// �������Ϸ���
	int OutBufferlen = 4096 ;
	char* szOutBuffer = NULL;
	
	szOutBuffer = (char *)malloc( OutBufferlen );
	if ( NULL == szOutBuffer || NULL == szName ) { return NULL ; }

	// �鿴�Ƿ�Ϊ�����ַ���
	memset( szOutBuffer, 0, sizeof( szOutBuffer ) );
	if ( TRUE == string_is_unicode( szName, nNameLen ) )
	{
		WCharToMByte (
			(LPCWSTR) szName, 
			szOutBuffer, 
			OutBufferlen,
			nNameLen );
		
	} else if ( TRUE == string_is_widechar( szName, nNameLen ) ) {

		// �Ƿ��ǿ��ַ�. ����: "73 00 64 00 66 00"  s.d.f.ÿ�μ��һ��0
		int i = 0;
		cheap_uni2ascii( (char *)szName, szOutBuffer, nNameLen );

	} else {

		strncpy( szOutBuffer, szName, nNameLen );
		*(szOutBuffer + nNameLen) = '\0' ;
	}
				
	return szOutBuffer ;
}


BOOL 
string_is_unicode (
	IN PCHAR strKeyName, 
	IN ULONG nLen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Arguments:
  strKeyName - Hive����
  nLen - �ļ�����(ע��Hive�ļ��д�ŵĳ�����ռ���ֽ�����)

Return Value:
  ��Unicode�ַ���TRUE,����FALSE

--*/
{
	BOOL bResult = FALSE;
	ULONG nIndex;
	
	// �Ƿ��������ַ���
	for(nIndex = 0; nIndex < nLen; nIndex++)
	{
		if(
			'\0' != strKeyName[nIndex] // û�н���,      (0x20~0x7e)
			&& (0 != (strKeyName[nIndex] & 0x80) || 0 == isprint(strKeyName[nIndex]))
		  )
		{
			bResult = TRUE;
			break;
		}
	}
	
	return bResult;
}



BOOL 
string_is_widechar (
	IN PCHAR strKeyName, 
	IN ULONG nLen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Arguments:
  strKeyName - Hive����
  nLen - �ļ�����(ע��Hive�ļ��д�ŵĳ�����ռ���ֽ�����)

Return Value:
  ��widechar�ַ���TRUE,����FALSE

--*/
{
	BOOL bResult = FALSE;
	ULONG nIndex, number;
	
	number = 0 ;
	for( nIndex = 0; nIndex < nLen; nIndex++ )
	{
		nIndex++ ;
		if( '\0' == strKeyName[nIndex] && nIndex == nLen )
		{
			// ���ַ���ĩβ��,�˳�
			break ;
		}

		if( '\0' == strKeyName[nIndex] )
		{
			++number ;
			if ( number == nLen/2 )
			{
				bResult = TRUE;
				break;
			}
		}
	}
	
	return bResult;
}


/*++

  ʹ�÷���Ҳ�ܼ�,ʾ������:
	wchar_t wText[10] = {L"����ʾ��"};
	char sText[20]= {0};
	WCharToMByte(wText,sText,sizeof(sText)/sizeof(sText[0]));
	MByteToWChar(sText,wText,sizeof(wText)/sizeof(wText[0]));

--*/

//-------------------------------------------------------------------------------------
//Description:
// This function maps a character string to a wide-character (Unicode) string
//
//Parameters:
// lpcszStr: [in] Pointer to the character string to be converted 
// lpwszStr: [out] Pointer to a buffer that receives the translated string. 
// dwSize: [in] Size of the buffer
//
//Return Values:
// TRUE: Succeed
// FALSE: Failed
// 
//Example:
// MByteToWChar(szA,szW,sizeof(szW)/sizeof(szW[0]));
//---------------------------------------------------------------------------------------
BOOL 
MByteToWChar (
	IN LPCSTR lpcszStr, 
	OUT LPWSTR lpwszStr, 
	IN DWORD dwSize
	)
{
    DWORD dwMinSize;
    dwMinSize = MultiByteToWideChar( CP_ACP, 0, lpcszStr, -1, NULL, 0 );
	
    if(dwSize < dwMinSize) { return FALSE; }
	
    MultiByteToWideChar( CP_ACP, 0, lpcszStr, -1, lpwszStr, dwMinSize ); 
    return TRUE;
}


//-------------------------------------------------------------------------------------
//Description:
// This function maps a wide-character string to a new character string
//
//Parameters:
// lpcwszStr: [in] Pointer to the character string to be converted 
// lpOutBuffer: [out] Pointer to a buffer that receives the translated string. 
// dwSize: [in] Size of the buffer
//
//Return Values:
// TRUE: Succeed
// FALSE: Failed
// 
//Example:
// MByteToWChar(szW,szA,sizeof(szA)/sizeof(szA[0]));
//---------------------------------------------------------------------------------------
BOOL 
WCharToMByte (
	IN LPCWSTR lpcwszStr, 
	OUT LPSTR lpOutBuffer, 
	IN DWORD OutBufferlen,
	IN DWORD nNameLength
	)
{
	DWORD n, dwMinSize;

	dwMinSize = WideCharToMultiByte( CP_OEMCP, 0, lpcwszStr, -1, NULL, 0, NULL, FALSE );
	if( OutBufferlen < dwMinSize ) { return FALSE; }

	WideCharToMultiByte( CP_OEMCP, 0, lpcwszStr, -1, lpOutBuffer, OutBufferlen, NULL, FALSE );

	n = nNameLength;
	if ( n > 0 ) {
		*( lpOutBuffer + n ) = '\0' ;
	}
	
	return TRUE;
}



void 
cheap_uni2ascii (
	IN char *src,
	OUT char *dest,
	IN int l
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Quick and dirty UNICODE to std. ascii; Uniocdeת����Ansi
  
Arguments:
  src - ָ��unicode��
  dest - ascii��
  l - unicode���ַ����ֽ���

--*/
{
   for (; l > 0; l -= 2) 
   {
      *dest = *src ;
      dest++ ; 
	  src += 2 ;
   }

   *dest = 0;
   return ;
}


void 
cheap_ascii2uni (
	IN char *src, 
	OUT char *dest,
	IN int l
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Ansiת����Uniocde
  
Arguments:
  src - ascii���ַ���
  dest - unicode���ַ���
  l - src�ĳ���

--*/
{
   for (; l > 0; l--) 
   {
      *dest++ = *src++;
      *dest++ = 0;
   }

   return ;
}


void 
skipspace (
	IN char **c
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �����ո�
  
Arguments:
  c - �ַ���

--*/
{
   while( **c == ' ' ) { (*c)++; }

   return ;
}


int 
gethex (
	IN char **c
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ���ַ�������ȡһ������,����֮.
    
Arguments:
  c - �ַ���

--*/
{
   int value;
   
   skipspace( c );
   
   if (!(**c)) { return ( 0 ); }

   sscanf( *c, "%x", &value );

   while( **c != ' ' && (**c)) { (*c)++; }

   return ( value );
}
   

int 
gethexorstr (
	IN char **c, 
	OUT char *wb
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Get a string of HEX bytes (space separated),  
  or if first char is ' get an ASCII string instead.

Arguments:
  c -  
  wb -  

Return Value:
  
--*/
{
	int l = 0;
	
	skipspace(c);
	
	if ( **c == '\'') 
	{
		(*c)++;
		while ( **c ) 
		{
			*(wb++) = *((*c)++);
			l++;
		}
	} 
	else
	{
		do 
		{
			*(wb++) = gethex(c);
			l++;
			skipspace(c);
		} while ( **c );
	}
	
	return(l);
}


int 
debugit (
	IN char *buf, 
	IN int sz
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Simple buffer debugger, returns 1 if buffer dirty/edited

Arguments:
  buf -  
  sz -  

Return Value:
  
--*/
{
	char inbuf[100], whatbuf[100], *bp ;
	int dirty=0,to,from,l,i,j,wlen,cofs = 0;
	
	return 0;
//	printf("Buffer debugger. '?' for help.\n");
	while (1)
	{
		l = GetUserInputInfo( ".", inbuf, 90 );
		bp = inbuf;
		
		skipspace(&bp);
		
		if (l > 0 && *bp) 
		{
			switch(*bp) 
			{
			case 'd' :
				bp++;
				if (*bp) 
				{
					from = gethex(&bp);
					to   = gethex(&bp);
				}
				else {
					from = cofs; to = 0;
				}

				if (to == 0) { to = from + 0x100; }
				if (to > sz) { to = sz; }

				hexdump( buf, from, to, 1 );
				cofs = to;
				break;

			case 'a' :
				bp++;
				if (*bp) {
					from = gethex(&bp);
					to   = gethex(&bp);
				} else {
					from = cofs; to = 0;
				}

				if (to == 0) { to = from + 0x100; }
				if (to > sz) { to = sz; }

				hexdump( buf, from, to, 0 );
				cofs = to;
				break;
#if 0
			case 'k' :
				bp++;
				if (*bp) {
					from = gethex(&bp);
				} else {
					from = cofs;
				}
				if (to > sz) to = sz;
				parse_block(from,1);
				cofs = to;
				break;
#endif
#if 0
			case 'l' :
				bp++;
				if (*bp) {
					from = gethex(&bp);
				} else {
					from = cofs;
				}
				if (to > sz) to = sz;
				nk_ls(from+4,0);
				cofs = to;
				break;
#endif
			case 'q':
				return(0);
				break;

			case 's':
				if (!dirty) printf("Buffer has not changed, no need to write..\n");
				return(dirty);
				break;

			case 'h':
				bp++;
				if (*bp == 'a') {
					from = 0;
					to = sz;
					bp++;
				}
				else 
				{
					from = gethex(&bp);
					to   = gethex(&bp);
				}
				wlen = gethexorstr(&bp,whatbuf);
				if (to > sz) 
					to = sz;
				printf("from: %x, to: %x, wlen: %d\n",from,to,wlen);
				for (i = from; i < to; i++) 
				{
					for (j = 0; j < wlen; j++)
					{
						if ( *(buf+i+j) != *(whatbuf+j))
							break;
					}
					if (j == wlen) 
						printf("%06x ",i);
				}
				printf("\n");
				break;

			case ':':
				bp++;
				if (!*bp) break;
				from = gethex(&bp);
				wlen = gethexorstr(&bp,whatbuf);
				
				printf("from: %x, wlen: %d\n",from,wlen);
				
				memcpy(buf+from,whatbuf,wlen);
				dirty = 1;
				break;
#if 0
			case 'p':
				j = 0;
				if (*(++bp) != 0)
				{
					from = gethex(&bp);
				}
				if (*(++bp) != 0) 
				{
					j = gethex(&bp);
				}
				printf("from: %x, rid: %x\n",from,j);
				seek_n_destroy(from,j,500,0);
				break;
#endif
			case '?':
				printf("d [<from>] [<to>] - dump buffer within range\n");
				printf("a [<from>] [<to>] - same as d, but without ascii-part (for cut'n'paste)\n");
				printf(": <offset> <hexbyte> [<hexbyte> ...] - change bytes\n");
				printf("h <from> <to> <hexbyte> [<hexbyte> ...] - hunt (search) for bytes\n");
				printf("ha <hexbyte> [<hexbyte] - Hunt all (whole buffer)\n");
				printf("s - save & quit\n");
				printf("q - quit (no save)\n");
				printf("  instead of <hexbyte> etc. you may give 'string to enter/search a string\n");
				break;

			default:
				printf("?\n");
				break;
		 }
      }
   }
}

/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                      һЩ��Ҫ����                         +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


#define D_OFFS(number) ( (int *)&(key->number)-(int *)hdesc->buffer-vofs )

void 
parse_nk (
	IN struct hive *hdesc,
	IN int vofs, 
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ����CM_KEY_NODE�ṹ��[����ӡ��Ϣ]

Arguments:
  vofs - offset into struct (after size linkage)

--*/
{
	
	struct nk_key *key;
	int i;
	
	key = (struct nk_key *)( hdesc->buffer + vofs );
	printf( "%04x   type              = 0x%02x %s\n", D_OFFS(type), key->type, (key->type == KEY_ROOT ? "ROOT_KEY" : "") );
	printf( "%04x   timestamp skipped\n", D_OFFS(timestamp) );
	printf( "%04x   parent key offset = 0x%0lx\n", D_OFFS(ofs_parent), key->ofs_parent );
	printf( "%04x   number of subkeys = %ld\n", D_OFFS(no_subkeys), key->no_subkeys );
	printf( "%04x   lf-record offset  = 0x%0lx\n", D_OFFS(ofs_lf), key->ofs_lf );
	printf( "%04x   number of values  = %ld\n", D_OFFS(no_values), key->no_values );
	printf( "%04x   val-list offset   = 0x%0lx\n", D_OFFS(ofs_vallist), key->ofs_vallist );
	printf( "%04x   sk-record offset  = 0x%0lx\n", D_OFFS(ofs_sk), key->ofs_sk );
	printf( "%04x   classname offset  = 0x%0lx\n", D_OFFS(ofs_classnam), key->ofs_classnam );
	printf( "%04x   *unused?*         = 0x%0lx\n", D_OFFS(dummy4), key->dummy4 );
	printf( "%04x   name length       = %d\n", D_OFFS(len_name), key->len_name );
	printf( "%04x   classname length  = %d\n", D_OFFS(len_classnam), key->len_classnam );
	
	printf( "%04x   Key name: <", D_OFFS(keyname) );
	for(i = 0; i < key->len_name; i++) { putchar( key->keyname[i] ); }
	printf( ">\n\n" );
	
	return ;
}


void 
parse_vk (
	IN struct hive *hdesc,
	IN int vofs, 
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ����_CM_KEY_VALUE�ṹ��[����ӡ��Ϣ]

Arguments:
  vofs - offset into struct (after size linkage)

--*/
{
	struct vk_key *key;
	int i;
	
	key = (struct vk_key *)( hdesc->buffer + vofs );
	printf( "%04x   name length       = %d (0x%0x)\n", D_OFFS(len_name), key->len_name, key->len_name  );
	printf( "%04x   length of data    = %ld (0x%0lx)\n", D_OFFS(len_data), key->len_data, key->len_data  );
	printf( "%04x   data offset       = 0x%0lx\n", D_OFFS(ofs_data), key->ofs_data );
	printf( "%04x   value type        = 0x%0lx  %s\n", D_OFFS(val_type), key->val_type, (key->val_type <= REG_MAX ? g_value_types[key->val_type] : "(unknown)") ) ;
	printf( "%04x   flag              = 0x%0x\n", D_OFFS(flag), key->flag );
	printf( "%04x   *unused?*         = 0x%0x\n", D_OFFS(dummy1), key->dummy1 );
	
	printf( "%04x   Key name: <",D_OFFS(keyname) );
	for(i = 0; i < key->len_name; i++) { putchar( key->keyname[i] ); }
	printf( ">\n\n" );
	
	return ;
}


void 
parse_sk (
	IN struct hive *hdesc,
	IN int vofs,
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ����sk�ṹ��[����ӡ��Ϣ] Gee, this is the security info. Who cares? *evil grin*

Arguments:
  vofs - offset into struct (after size linkage)

--*/
{
	struct sk_key *key;
	
	key = (struct sk_key *)( hdesc->buffer + vofs );
	printf( "%04x   *unused?*         = %d\n", D_OFFS(dummy1), key->dummy1 );
	printf( "%04x   Offset to prev sk = 0x%0lx\n", D_OFFS(ofs_prevsk), key->ofs_prevsk );
	printf( "%04x   Offset to next sk = 0x%0lx\n", D_OFFS(ofs_nextsk), key->ofs_nextsk );
	printf( "%04x   Usage counter     = %ld (0x%0lx)\n", D_OFFS(no_usage), key->no_usage, key->no_usage );
	printf( "%04x   Security data len = %ld (0x%0lx)\n\n", D_OFFS(len_sk), key->len_sk, key->len_sk );

	return ;
}


void 
parse_lf (
	IN struct hive *hdesc,
	IN int vofs, 
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Parse the lf datablock (>4.0 'nk' offsets lookuptable)

Arguments:
  vofs - offset into struct (after size linkage)

--*/
{
	struct lf_key *key;
	int i;
	
	key = (struct lf_key *)( hdesc->buffer + vofs );
	printf("%04x   number of keys    = %d\n", D_OFFS(no_keys), key->no_keys  );
	
	for(i = 0; i < key->no_keys; i++)
	{
		printf(
			"%04x      %3d   Offset: 0x%0lx  - <%c%c%c%c>\n", 
			D_OFFS(hash[i].ofs_nk), i,
			key->hash[i].ofs_nk,
			key->hash[i].name[0],
			key->hash[i].name[1],
			key->hash[i].name[2],
			key->hash[i].name[3] 
			);
	}
	
	printf("\n");
	return ;
}


void 
parse_lh (
	IN struct hive *hdesc,
	IN int vofs, 
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Parse the lh datablock (WinXP offsets lookuptable)
  The hash is most likely a base 37 conversion of the name string

Arguments:
  vofs - offset into struct (after size linkage)
  
--*/
{
	struct lf_key *key;
	int i;
		
	key = (struct lf_key *)( hdesc->buffer + vofs );
	printf("%04x   number of keys    = %d\n", D_OFFS(no_keys), key->no_keys  );
	
	for(i = 0; i < key->no_keys; i++) 
	{
		printf(
			"%04x      %3d   Offset: 0x%0lx  - <hash: %08lx>\n", 
			D_OFFS(lh_hash[i].ofs_nk), i,
			key->lh_hash[i].ofs_nk,
			key->lh_hash[i].hash 
			);
	}
	
	printf( "\n" );
	return ;
}


void 
parse_li (
	IN struct hive *hdesc,
	IN int vofs, 
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Parse the li datablock (3.x 'nk' offsets list)

Arguments:
  vofs - offset into struct (after size linkage)

Return Value:
  
--*/
{
	struct li_key *key;
	int i;
	
/*#define D_OFFS(o) ( (int *)&(key->o)-(int *)hdesc->buffer-vofs )*/
	key = (struct li_key *)( hdesc->buffer + vofs );
	printf("%04x   number of keys    = %d\n", D_OFFS(no_keys), key->no_keys  );
	
	for(i = 0; i < key->no_keys; i++) 
	{
		printf(
			"%04x      %3d   Offset: 0x%0lx\n", 
			D_OFFS(hash[i].ofs_nk), i,
			key->hash[i].ofs_nk
			);
	}

	printf( "\n" );
	return ;
}


void 
parse_ri (
	IN struct hive *hdesc,
	IN int vofs, 
	IN int blen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Parse the ri subindex-datablock(Used to list li/lf/lh's when ~>500keys)

Arguments:
  vofs - offset into struct (after size linkage)

Return Value:
  
--*/
{
	struct ri_key *key;
	int i;

// #define D_OFFS(o) ( (int *)&(key->o)-(int *)hdesc->buffer-vofs )
	key = (struct ri_key *)( hdesc->buffer + vofs );
	printf("%04x   number of subindices = %d\n", D_OFFS(no_lis), key->no_lis  );
	
	for(i = 0; i < key->no_lis; i++) 
	{
		printf(
			"%04x      %3d   Offset: 0x%0lx\n", 
			D_OFFS(hash[i].ofs_li), i,
			key->hash[i].ofs_li
			);
	}

	printf( "\n" );
	return ;
}


int 
parse_block (
	IN struct hive *hdesc,
	IN int vofs,
	IN int verbose
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ����һ��BIN

Arguments:
  vofs - offset into struct (after size linkage)

--*/
{
	unsigned short id;
	int seglen;
	
	seglen = get_int( hdesc->buffer + vofs );  
	
	if (seglen == 0) 
	{
		printf("Whoops! FATAL! Zero data block size!\n");
		debugit(hdesc->buffer,hdesc->size);
		return ( 0 );
	}
	
	if (seglen < 0) {
		seglen = -seglen ;
		hdesc->usetot += seglen ;
		hdesc->useblk++ ;
	} else {
		hdesc->unusetot += seglen ;
		hdesc->unuseblk++ ;

		bzero( hdesc->buffer + vofs + 4, seglen - 4 );
	}
	
	vofs += 4 ;
	id = ( *(hdesc->buffer + vofs) << 8 ) + *( hdesc->buffer + vofs + 1 );
	
	if (verbose) 
	{
		switch (id) 
		{
		case CM_KEY_NodeKey: /* nk */
			parse_nk( hdesc, vofs, seglen );
			break;
		case CM_KEY_ValueKey: /* vk */
			parse_vk( hdesc, vofs, seglen );
			break;
		case CM_KEY_FAST_LEAF: /* lf */
			parse_lf( hdesc, vofs, seglen );
			break;
		case CM_KEY_HASH_LEAF: /* lh */
			parse_lh( hdesc, vofs, seglen );
			break;
		case CM_KEY_IIII_LEAF: /* li */
			parse_li( hdesc, vofs, seglen );
			break;
		case CM_KEY_sk_SecurityKey: /* sk */
			parse_sk( hdesc, vofs, seglen );
			break;
		case CM_KEY_ri_BigRoot: /* ri */
			parse_ri( hdesc, vofs, seglen );
			break;
		default:
			printf("value data, or not handeled yet!\n");
			break;
		}
	}

	return ( seglen );
}


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                  ���ע����ֵ���                       +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


int
find_page_start (
	IN struct hive *hdesc, 
	int vofs
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �ҵ����ƫ��vofs��Ӧ��BIN��λ��

   _________________
  |    RegHeader    |
  |_____0x1000______|
  |       BIN	    |
  |_____0x1000______|
  |		  BIN       |
  |_____0x1000______|  <--vofs
  |     ... ...     |
  
Arguments:
  vofs - offset into struct (after size linkage)

Return Value:
  ��ǰƫ�ƶ�Ӧ��BIN����ʼ��ַ

--*/
{
	int r, prev ;
	struct hbin_page *h ;

	r = 0x1000;
	while ( r < hdesc->size ) 
	{
		prev = r;
		h = (struct hbin_page *)( hdesc->buffer + r );
		if (h->id != HBIN) { return(0); }
		if (h->ofs_next == 0) 
		{
			printf("find_page_start: zero len or ofs_next found in page at 0x%x\n",r);
			return ( 0 );
		}

		r += h->ofs_next;
		if (r > vofs) { return (prev); }
	}

	return ( 0 );
}


int 
find_free_blk (
	IN struct hive *hdesc, 
	IN int pofs, 
	IN int size
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �ڵ�ǰ���BIN���ҵ���϶

Arguments:
  size - ��Ҫ�Ŀ�϶��С
  pofs - ��ǰָ��BIN��ƫ��

Return Value:
  offset to free block, or 0 for error

--*/
{
	int vofs = pofs + 0x20 ;
	int seglen ;
	struct hbin_page *p ;
	
	p = (struct hbin_page *)(hdesc->buffer + pofs);
	
	// ������ǰBIN
	while ( vofs-pofs < (p->ofs_next - HBIN_ENDFILL) ) 
	{
		seglen = get_int( hdesc->buffer + vofs );  
		
		if (seglen == 0)
		{
			printf("find_free_blk: FATAL! Zero data block size! (not registry or corrupt file?)\n");
			debugit( hdesc->buffer, hdesc->size );
			return (0);
		}
		
		if (seglen < 0) {
			seglen = -seglen;
		} 
		else
		{
			if (seglen >= size) 
			{
			//	printf("find_free_blk: found size %d block at 0x%x\n",seglen,vofs);
				return (vofs);
			}
		}

		vofs += seglen;
	}

	return (0);
}


int 
find_free (
	IN struct hive *hdesc,
	IN int size
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �ӿ�ʼ0x1000��,��ʼ����ÿ��BIN,ֱ���ҵ����ʵÿ�϶.
  ע����ֺܶ�4KB��page��,��ÿ��page���治һ����������,
  ����Ӽ�ֵʱ,���Դ���Ѱ�Һ��ʴ�С�ÿռ�,���з�������

Arguments:
  size - ��Ҫ�Ŀ�϶��С

Return Value:
  offset to free block, 0 if not found or error

--*/
{
	int r, blk ;
	struct hbin_page *h ;
	
	// 8�ֽڶ���
	if (size & 7) { size += (8 - (size & 7)); }
	
	r = 0x1000;
	while (r < hdesc->size) // ��������Hive
	{
		h = (struct hbin_page *)( hdesc->buffer + r );
		if (h->id != HBIN) { return (0); }
		if (h->ofs_next == 0) { return(0); } // ���ҵ�ͷ���˶�û�ҵ�,ֻ�ܷ���0

		blk = find_free_blk( hdesc, r, size );
		if (blk) return (blk);
		r += h->ofs_next;
	}

	return (0);
}


int 
alloc_block (
	IN struct hive *hdesc, 
	IN int ofs, 
	IN int size
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  This function WILL CHANGE THE HIVE (change block linkage) if it succeeds.

Arguments:
  ofs - 
  size - ��Ҫ�Ŀ�϶��С

Return Value:
  0 - failed, else pointer to allocated block.

--*/
{
	int pofs = 0;
	int blk = 0;
	int trail, trailsize, oldsz;
	
	if (hdesc->state & HMODE_NOALLOC)
	{
		printf("alloc_block: ERROR: Hive %s is in no allocation safe mode,"
			"new space not allocated. Operation will fail!\n", hdesc->filename);
		return (0);
	}
	
	size += 4 ;
	if (size & 7) size += (8 - (size & 7)); // 8�ֽڶ���
	
	// �ȼ�鵱ǰƫ�ƴ���Page���Ƿ����㹻��ʣ������ÿռ�
	if (ofs)
	{
		pofs = find_page_start( hdesc, ofs );
		blk = find_free_blk( hdesc, pofs, size );
	}
	
	// ��������Hive�ļ���Ѱ��
	if (!blk) {
		blk = find_free( hdesc, size );
	}
	
	if (!blk)
	{
		printf( "alloc_block: failed to alloc %d bytes!\n", size );
		return 0 ;
	}
	
	// Got the space
	oldsz = get_int( hdesc->buffer + blk );
#if 0
	printf("Block at         : %x\n", blk);
	printf("Old block size is: %x\n", oldsz);
	printf("New block size is: %x\n", size);
#endif
	trailsize = oldsz - size;
	
	if (trailsize == 4) 
	{
		trailsize = 0;
		size += 4;
	}
	
#if 1
	if (trailsize & 7) 
	{	// Trail must be 8 aligned
		trailsize -= (8 - (trailsize & 7));
		size += (8 - (trailsize & 7));
	}
	if (trailsize == 4) 
	{
		trailsize = 0;
		size += 4;
	}
#endif
	
#if 0
	printf("trail after comp: %x\n",trailsize);
	printf("size  after comp: %x\n",size);
#endif
	
	// Now change pointers on this to reflect new size
	*(int *)( (hdesc->buffer) + blk ) = -(size) ;
	/* If the fit was exact (unused block was same size as wee need)
	* there is no need for more, else make free block after end
	* of newly allocated one */
	
	hdesc->useblk++;
	hdesc->unuseblk--;
	hdesc->usetot += size;
	hdesc->unusetot -= size;
	
	if (trailsize) 
	{
		trail = blk + size;
		
		*(int *)((hdesc->buffer)+trail) = (int)trailsize;
		
		hdesc->useblk++;    // This will keep blockcount
		hdesc->unuseblk--;
		hdesc->usetot += 4; // But account for more linkage bytes
		hdesc->unusetot -= 4;
		
	}  

	// Clear the block data, makes it easier to debug
	bzero( (void *)(hdesc->buffer+blk+4), size-4 );
	
	hdesc->state |= HMODE_DIRTY;
	
	return (blk);
}


int 
free_block (
	IN struct hive *hdesc, 
	IN int blk
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Free a block in registry.Will CHANGE HIVE IF SUCCESSFUL (changes linkage)

Arguments:
  blk - offset of block, MUST POINT TO THE LINKAGE!

Return Value:
  returns bytes freed (incl linkage bytes) or 0 if fail

--*/
{
	int pofs,vofs,seglen,prev,next,nextsz,prevsz,size ;
	struct hbin_page *p;
	
	if (hdesc->state & HMODE_NOALLOC) 
	{
		printf("free_block: ERROR: Hive %s is in no allocation safe mode,"
			"space not freed. Operation will fail!\n", hdesc->filename);
		return (0);
	}
	
	size = get_int( hdesc->buffer + blk );
	if (size >= 0) 
	{
		printf("free_block: trying to free already free block!\n");
		return (0);
	}
	
	size = -size;
	
	// �ҵ��������Ӧ��BIN�ṹ��ʼ��ַ��ƫ��
	pofs = find_page_start( hdesc, blk );
	if (!pofs) { return (0); }

	p = (struct hbin_page *)( hdesc->buffer + pofs );
	vofs = pofs + 0x20; // ����BIN�ṹͷ
	
	prevsz = -32;
	
	if (vofs != blk) // Block is not at start of page?
	{	
		while (vofs-pofs < (p->ofs_next - HBIN_ENDFILL) ) 
		{
			seglen = get_int(hdesc->buffer+vofs);  
			
			if (seglen == 0) 
			{
				printf("free_block: EEEK! Zero data block size! (not registry or corrupt file?)\n");
				debugit(hdesc->buffer,hdesc->size);
				return (0);
			}
			
			if (seglen < 0) { seglen = -seglen; } 

			prev = vofs;
			vofs += seglen;
			if (vofs == blk) { break; }
		}
		
		if (vofs != blk) 
		{
			printf("free_block: ran off end of page!?!? Error in chains?\n");
			return (0);
		}
		
		prevsz = get_int( hdesc->buffer + prev );
	}
	
	// We also need details on next block (unless at end of page)
	next = blk + size;
	
	nextsz = 0;
	if (next-pofs < (p->ofs_next - HBIN_ENDFILL) ) { nextsz = get_int(hdesc->buffer+next); }
	
#if 0
	printf("offset prev : %x , blk: %x , next: %x\n",prev,blk,next);
	printf("size   prev : %x , blk: %x , next: %x\n",prevsz,size,nextsz);
#endif
	
	// Now check if next block is free, if so merge it with the one to be freed
	if ( nextsz > 0) 
	{
		size += nextsz;   // Swallow it in current block
		hdesc->useblk--;
		hdesc->usetot -= 4;
		hdesc->unusetot -= 4;
	}
	
	// Now free the block (possibly with ajusted size as above)
	bzero( (void *)(hdesc->buffer+blk), size);
	*(int *)((hdesc->buffer)+blk) = (int)size;
	hdesc->usetot -= size;
	hdesc->unusetot -= size;
	hdesc->unuseblk--;
	
	hdesc->state |= HMODE_DIRTY;
	
	// Check if previous block is also free, if so, merge..
	if (prevsz > 0) 
	{
		hdesc->usetot -= prevsz;
		hdesc->unusetot += prevsz;
		prevsz += size;
		// And swallow current..
		bzero( (void *)(hdesc->buffer+prev), prevsz);
		*(int *)((hdesc->buffer)+prev) = (int)prevsz;
		hdesc->useblk--;
		return(prevsz);
	}

	return (size);
}


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                ����/���� �Ӽ�,��ֵ ��ϵ�к���             +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


int 
ex_next_n (
	IN struct hive *hdesc, 
	IN int nkofs,
	IN int *count, 
	IN int *countri, 
	OUT struct ex_data *sptr
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  "directory scan", return next name/pointer of a subkey on each call
  caller must free the name-buffer (struct ex_data *name)
  ����һ������Ϊ360.����:
  +360
  | +-XGB
  |	| +-sudami
  |	| +-WangYu
  |	+-MJ
  ...
  
  �˺����Ǳ������Ӽ�,�� XGB �� MJ

Arguments:
  nkofs - ����360�������CM_KEY_NODE��ƫ��
  count - ����360ӵ�е��Ӽ�����(���������Ϊ2��)
  countri - �������������
  sptr - ���浱ǰ�Ӽ�����Ϣ

Return Value:
  -1 = error. 0 = end of key. 1 = more subkeys to scan

--*/
{
	struct nk_key *key, *newnkkey;
	int newnkofs;
	int len = 10 ;
	struct lf_key *lfkey;
	struct li_key *likey;
	struct ri_key *rikey;
	
	// �����Ϸ��Լ��
	if (!nkofs) { return (-1); }
	key = (struct nk_key *)( hdesc->buffer + nkofs );
	if ( key->id != 0x6b6e ) 
	{
		printf("ex_next error: Not a 'nk' node at 0x%0x\n", nkofs);
		return (-1);
	}
	
	lfkey = (struct lf_key *)( hdesc->buffer + key->ofs_lf + 0x1004 );
	rikey = (struct ri_key *)( hdesc->buffer + key->ofs_lf + 0x1004 );
	
	if ( 0x6972 == rikey->id ) // �Ǹ�������Root
	{   
		if ( *countri < 0 || *countri >= rikey->no_lis ) 
		{  // End of ri's
			return(0);
		}

		// Get the li of lf-struct that's current based on countri
		// ������Root -- Ri�ṹ�е�List������ָ��ÿ�����lf��li.���� Signature �ж�
		likey = (struct li_key *)( hdesc->buffer + rikey->hash[*countri].ofs_li + 0x1004 ) ;
		if (likey->id == 0x696c)
		{
			newnkofs = likey->hash[*count].ofs_nk + 0x1000;
		}
		else
		{
			lfkey = (struct lf_key *)( hdesc->buffer + rikey->hash[*countri].ofs_li + 0x1004 ) ;
			newnkofs = lfkey->hash[*count].ofs_nk + 0x1000;
		}
		
		// Check if current li/lf is exhausted
		if (*count >= likey->no_keys-1)
		{   // Last legal entry in li list
			(*countri)++;   // Bump up ri count so we take next ri entry next time
			(*count) = -1;  // Reset li traverse counter for next round, not used later here
		}
	} 
	else // ����,���ǳ�����Ri,����ͨ�����
	{
		if (key->no_subkeys <= 0 || *count >= key->no_subkeys) { return (0); }
		if (lfkey->id == 0x696c) 
		{   /* Is it 3.x 'li' instead? */
			likey = (struct li_key *)(hdesc->buffer + key->ofs_lf + 0x1004);
			newnkofs = likey->hash[*count].ofs_nk + 0x1000;
		}
		else 
		{
			newnkofs = lfkey->hash[*count].ofs_nk + 0x1000;
		}
	}
	
	sptr->nkoffs = newnkofs;
	newnkkey = (struct nk_key *)(hdesc->buffer + newnkofs + 4);
	sptr->nk = newnkkey;
	
	if (newnkkey->id != 0x6b6e) 
	{
		printf("ex_next: ERROR: not 'nk' node at 0x%0x\n",newnkofs);
		return (-1);
	}

	if (newnkkey->len_name <= 0)
	{
		printf("ex_next: nk at 0x%0x has no name!\n", newnkofs);
		sptr->name = (char *)HEAP_ALLOC( len ); // ��������,�ȷ�����ڴ�
	}
	else 
	{
		char *string ;
		len = newnkkey->len_name ;
		if (!sptr->name) 
		{
			printf("FATAL! ex_next: malloc() failed! Out of memory?\n");
			abort();
		}

		strncpy(sptr->name, newnkkey->keyname, newnkkey->len_name );
		sptr->name[newnkkey->len_name] = 0;

		// �鿴�Ƿ�Ϊ�����ַ���
		string = change_to_ansi( sptr->name, len );
		if ( string )
		{
			strncpy(sptr->name, string, newnkkey->len_name );
			sptr->name[newnkkey->len_name] = 0;
			FREE( string );
		}
	}

	(*count)++ ;
	return ( 1 );
}


int 
ex_next_v (
	IN struct hive *hdesc, 
	int nkofs, 
	int *count, 
	struct vex_data *sptr
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  "directory scan" for VALUES, return next name/pointer of a value on each call
  caller must free the name-buffer (struct ex_data *name)
  ����һ������Ϊ360.����:
  +360
  | +-XGB
  |	| +-sudami  360����������2���Լ���ֵ "REG_DWORD(����ΪTest)" �� "Ĭ��(����Ϊ360Safe)
  |	| +-WangYu    
  |	+-MJ
  ...

Arguments:
  nkofs - ����360�������CM_KEY_NODE��ƫ��
  count - ����360ӵ�е��Լ�ֵ������(���������Ϊ2��)
  sptr - ���浱ǰ�Ӽ�����Ϣ

Return Value:
  -1 = error. 0 = end of key. 1 = more subkeys to scan

--*/
{
	struct nk_key *key /* , *newnkkey */ ;
	int vkofs,vlistofs;
	int *vlistkey;
	struct vk_key *vkkey;
	char* string = NULL ;
	
	if (!nkofs) { return (-1); }
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e) { return (-1); }
	if (key->no_values <= 0 || *count >= key->no_values) { return(0); }
	
	vlistofs = key->ofs_vallist + 0x1004;
	vlistkey = (int *)(hdesc->buffer + vlistofs);
	
	vkofs = vlistkey[*count] + 0x1004;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	if (vkkey->id != 0x6b76)
	{
		printf("ex_next_v: hit non valuekey (vk) node during scan at offs 0x%0x\n",vkofs);
		return (-1);
	}
	
//	parse_vk( hdesc, vkofs, 4 );	
	sptr->vk = vkkey;
	sptr->vkoffs = vkofs;
	sptr->name = 0;
	sptr->size = (vkkey->len_data & 0x7fffffff);
	
	if (vkkey->len_name >0) 
	{
		int len = (int) (vkkey->len_name + 1) ;
		CREATE( sptr->name, char, vkkey->len_name + 1 );
		memcpy( sptr->name, vkkey->keyname, vkkey->len_name );
		sptr->name[vkkey->len_name] = 0;

		string = (char*) change_to_ansi( sptr->name, vkkey->len_name ); 
		if ( string ) {
			strncpy( sptr->name, string, len );
			FREE( string );
		}

	} else {
		sptr->name = str_dup("@"); // Ĭ��
	}
	
	sptr->type = vkkey->val_type;
	if (sptr->size) 
	{
		if (vkkey->val_type == REG_DWORD) 
		{
			if (vkkey->len_data & 0x80000000) 
			{
				sptr->val = (int)(vkkey->ofs_data);
			}
		}
	} 
	else if (vkkey->len_data == 0x80000000) 
	{ 
		// Data SIZE is 0, high bit set: special inline case, data is DWORD and in TYPE field!
		// Used a lot in SAM, and maybe in SECURITY I think 
		sptr->val = (int)(vkkey->val_type);
		sptr->size = 4;
		sptr->type = REG_DWORD;
	} 
	else 
	{
		sptr->val = 0;
		sptr->size = 0;
	}
	
	(*count)++;
	return ( *count <= key->no_values );
}


int 
vlist_find (
	IN struct hive *hdesc,
	IN int vlistofs, 
	IN int numval, 
	IN char *name
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ��nk�ṹ����ValueList������Ҷ�Ӧ��ValueName,�������

Arguments:
  vlistofs - Vk�ṹ����List�����ƫ��
  numval - �ø����Լ���ֵ������
  name - Ҫ���ҵ�ֵ������

Return Value:
  returns index into table or -1 if error

--*/
{
	struct vk_key *vkkey;
	int i, vkofs ;
	long *vlistkey ;
	
	vlistkey = (long *)(hdesc->buffer + vlistofs);
	
	for (i = 0; i < numval; i++) 
	{
		vkofs = vlistkey[i] + 0x1004;
		vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
		if (vkkey->len_name == 0 && *name == '@') 
		{	// Ĭ��
			return (i);
		}

		if ( 0 == strncmp( name, vkkey->keyname, strlen(name) ) )
		{
			return (i);
		}
	}

	return (-1);	
}


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                    ����·�� ��غ���                      +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--



int 
Get_nkofs_from_fullPath (
	IN struct hive *hdesc,
	IN char *ValuePath
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ����ָ������ȫ·��,�õ����Ӧ��nkofs

Arguments:
  ValuePath - ָ������ȫ·��

Return Value:
  -1 -- failed, others OK!

--*/
{
	int	nkofs = 0 ;
	char dummyPath[ABSPATHLEN+1] ;
	
	// �����Ϸ��Լ��
	if (!ValuePath || !*ValuePath) { return -1 ; }
	ZERO( dummyPath ) ;
	
	if ( '\\' != *ValuePath ) 
	{	// ·����ʼ����û��\\,Ĭ���Ǹ���,���Ǽ���ȥ
		_snprintf( dummyPath, MAX( ABSPATHLEN+1, sizeof(ValuePath) ) + 1, "%s%s", "\\", ValuePath ) ;
	} else {
		strncpy( dummyPath, ValuePath, MAX( ABSPATHLEN+1, sizeof(ValuePath) ) );
	}
	
	// ���Ҹ�����nkofs
	nkofs = trav_path( hdesc, 0, dummyPath, TRAV_PATH_WANT_NK );
	if ( !nkofs )
	{
		printf( "Get_nkofs_from_fullPath: Key <%s> not found\n", dummyPath );
		return -1 ;
	}

	return nkofs ;
}


int 
get_abs_path (
	IN struct hive *hdesc,
	IN int nkofs,
	OUT char *path, 
	IN int maxlen
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  traceback - trace nk's back to root,building path string as we go.
  ��, �ӵ�ǰ��ֵ����׷,�ó��������ļ�ֵ·��,��del_key_hasNoSubKey()�������� eg.

  +-360 ;����
  | +-MJ
  | +-XGB
  |	| +-sudami ;��ǰ��,��������õ�ȫ·��Ϊ:\\360\\XGB\\sudami
  |	| +-WangYu
  | |	...
  | ...
  ...	

Arguments:
  path - ������һ���ѷ���õ�Buffer��ָ��
  maxlen - ������Buffer����󳤶�

Return Value:
  ����ȫ·��:\\360\\XGB\\sudami�ĳ���

--*/
{
	struct nk_key *key;
	char tmp[ABSPATHLEN+1];
	
	maxlen = (maxlen < ABSPATHLEN ? maxlen : ABSPATHLEN);
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	
	if (key->id != 0x6b6e) 
	{
		printf("get_abs_path: Not a 'nk' node!\n");
		return (0);
	}
	
	// �ݹ�N�κ�,��������ɹ�����
	if (key->type == KEY_ROOT) { return (strlen(path)); }
	
	// tmp�б�������ϸ��ݹ�õ���·��,����ݹ�һ�κ�õ���·��Ϊ:
	// "\\sudami"
	strncpy( tmp, path, ABSPATHLEN - 1 );
	
	// Stop trace when string exhausted.��Buffer�Ŀռ䲻��,װ����·��ʱ
	// �޿��κ�,���ܷ���. �����6�� �ַ���"(...)"�ĳ���+һ��������'\0'
	if ( (int)(strlen(path) + key->len_name) >= maxlen-6) 
	{
		_snprintf( path, maxlen, "(...)%s", tmp );
		return (strlen(path));   
	}

	// �ؼ���2��. �ݹ�2�κ�,path�е����ݱ�Ϊ:
	// "\\XGB\\sudami"
	*path = '\\';
	memcpy( path+1, key->keyname, key->len_name );
	strncpy( path + key->len_name + 1, tmp, maxlen );

	// �����ݹ�
	return ( get_abs_path(hdesc, key->ofs_parent+0x1004, path, maxlen) ); 
}


int g_nIndex_xx = 0 ;

int 
trav_path (
	IN struct hive *hdesc,
	IN int vofs,
	IN char *path, 
	IN int type
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Recursevely follow 'nk'-nodes based on a path-string, returning offset of last 'nk' or 'vk'
  trav_path�����Ǵ�ģ�����Ҫ����,����һ��·��(eg:360\XGB\sudami),����һ����ʽ(eg:��NK/VK)
  �ݹ����,ÿ��ȡһ��\֮ǰ������,���бȽ��ж�.ֱ�������˶�Ӧ��(NK/VK)offset

Arguments:
  path - null-terminated pathname (relative to vofs, \ is separator)
  type - type to return 0=nk 1=vk

Return Value:
  offset to nk or vk (or NULL if not found)

--*/
{
	struct nk_key *key, *newnkkey;
	struct lf_key *lfkey;
	struct li_key *likey;
	struct ri_key *rikey;
	
	long *vlistkey;
	int newnkofs, plen, i, lfofs, vlistofs, adjust, r, ricnt, subs;
	char *buf;
	char part[ABSPATHLEN+1];
	char *partptr;
	
	if (!hdesc) return(0);
	buf = hdesc->buffer;
	
	if (*path == '\\' && *(path+1) != '\\') // �ַ��������ֽھ���\,�����Ǹ��� 
	{      
		path++;
		vofs = hdesc->rootofs+4;
	}
	
	key = (struct nk_key *)(buf + vofs);
	
	if (key->id != 0x6b6e) 
	{
		printf("trav_path: Error: Not a 'nk' node!\n");
		return(0);
	}
	
	// Find \ delimiter or end of string, copying to name part buffer as we go,
	partptr = part; // ÿ�ν�ȡб��\ǰ������,������ڴ��part��
	for(plen = 0; path[plen] && (path[plen] != '\\' || path[plen+1] == '\\'); plen++)
	{
		if (path[plen] == '\\' && path[plen+1] == '\\') 
		{ 
			plen++; // Skip one if double
		}
		*partptr++ = path[plen];
	}
	*partptr = '\0';
	
//	printf("Name component: <%s>\n",part);
	adjust = (path[plen] == '\\' ) ? 1 : 0;

	if (!plen) return(vofs-4);     // Path has no length - we're there!
	if ( (plen == 1) && (*path == '.')) 
	{	// Handle '.' current dir
		return ( trav_path( hdesc, vofs, path + plen + adjust, type ) );
	}

	if ( (plen == 2) && !strncmp("..",path,2) )
	{	
		newnkofs = key->ofs_parent + 0x1004 ; // �õ�����
		// Return parent (or only root if at the root)
		return (trav_path(hdesc, (key->type == KEY_ROOT ? vofs : newnkofs), path+plen+adjust, type));
	}
	
	// at last name of path, and we want vk, and the nk has values
	if (!path[plen] && type == 1 && key->no_values) 
	{   
		vlistofs = key->ofs_vallist + 0x1004;
		vlistkey = (long *)(buf + vlistofs);
		i = vlist_find(hdesc, vlistofs, key->no_values, part);
		if (i != -1) 
		{
			return(vlistkey[i] + 0x1000);
		}
	}
	
	if (key->no_subkeys > 0) // If it has subkeys, loop through the hash
	{    
		lfofs = key->ofs_lf + 0x1004;    // lf (hash) record
		lfkey = (struct lf_key *)(buf + lfofs);
		
		if (lfkey->id == 0x6972) // ri struct need special parsing.Prime loop state
		{ 
			rikey = (struct ri_key *)lfkey;
			ricnt = rikey->no_lis;
			r = 0;
			likey = (struct li_key *)( hdesc->buffer + rikey->hash[r].ofs_li + 0x1004 ) ;
			subs = likey->no_keys;
			if (likey->id != 0x696c) 
			{  // Bwah, not li anyway, XP uses lh usually which is actually smarter
				lfkey = (struct lf_key *)( hdesc->buffer + rikey->hash[r].ofs_li + 0x1004 ) ;
				likey = NULL;
			}
		} 
		else 
		{
			if (lfkey->id == 0x696c) {	// li?
				likey = (struct li_key *)(buf + lfofs);
			} else {
				likey = NULL;
			}

			ricnt = 0; 
			r = 0; 
			subs = key->no_subkeys;
		}
		
		do 
		{
			// �����Ӽ�,�ҵ����ȡ��������ƥ��ļ�
			for( g_nIndex_xx = 0; g_nIndex_xx < subs; g_nIndex_xx++ ) 
			{
				if (likey) 
					newnkofs = likey->hash[g_nIndex_xx].ofs_nk + 0x1004;
				else 
					newnkofs = lfkey->hash[g_nIndex_xx].ofs_nk + 0x1004;
				
				newnkkey = (struct nk_key *)(buf + newnkofs);
				if (newnkkey->id != 0x6b6e) 
				{
					printf("ERROR: not 'nk' node! (strange?)\n");
				} 
				else
				{
					if (newnkkey->len_name <= 0) 
					{
						printf("[No name]\n");
					} 
					else 
					{
						// �鿴�Ƿ�Ϊ�����ַ���
						char string[SZ_MAX+1] ;
						int len = newnkkey->len_name ;
						if ( TRUE == string_is_unicode( newnkkey->keyname, len ) )
						{
							WCharToMByte ( 
								(LPCWSTR)newnkkey->keyname,
								string,
								sizeof(string)/sizeof(string[0]), 
								len );

						} else {
							strncpy( string, newnkkey->keyname, len );
							*(string+len) = '\0' ;
						}

						if (!strncmp( part, string, plen)) 
						{
						//	printf("Key at 0x%0x matches! recursing!\n",newnkofs);
							return (trav_path(hdesc, newnkofs, path+plen+adjust, type));
						}
					}
				} /* if id OK */
			} /* hash loop */

			r++;
			if (ricnt && r < ricnt) 
			{
				newnkofs = rikey->hash[r].ofs_li;
				likey = (struct li_key *)( hdesc->buffer + newnkofs + 0x1004 ) ;
				subs = likey->no_keys;
				if (likey->id != 0x696c)
				{  // Bwah, not li anyway, XP uses lh usually which is actually smarter
					lfkey = (struct lf_key *)( hdesc->buffer + rikey->hash[r].ofs_li + 0x1004 ) ;
					likey = NULL;
				}
			}
		} while (r < ricnt && ricnt);
		
	}
	
	return (0);
}


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                 ��ʾ�о��Ӽ�/��ֵ ��غ���                +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


void 
getValueData (
	IN struct hive *hdesc,
	IN int nkofs, 
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �õ���ֵ������[����ӡ]

Arguments:
  path - ValueName. ��������

--*/
{    
	void *data ;
	int len, type ;
	char *string ;
	
	type = get_val_type(hdesc, nkofs, path);
	if (type == -1) 
	{
		printf("No such value <%s>\n",path);
		return ;
	}
	
	len = get_val_len(hdesc, nkofs, path);
	if (!len) 
	{
		printf("Value <%s> has zero length\n",path);
		return;
	}
	
	data = (void *)get_val_data(hdesc, nkofs, path, 0);
	if (!data) { return; }
	
	switch (type)
	{
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_MULTI_SZ:
		
// 		cheap_uni2ascii( (char *)data,tmpBuff,len );
// 		for (i = 0; i < (len>>1)-1; i++)
// 		{
// 			if (tmpBuff[i] == 0) 
// 				tmpBuff[i] = '\n';
// 			if (type == REG_SZ)
// 				break;
// 		}
//  		puts( tmpBuff );

		// ����ֵת����asni,����Ƚ�
		string = (char*) change_to_ansi( (char *)data, len ); 
		printf( "%s\n", string );

		break;
		
	case REG_DWORD:
		printf("0x%08x\n",*(unsigned short *)data);
		break;
	case REG_BINARY:
		hexdump((char *)data, 0, len, 1);

	default:
		return;
	}
}


// xx
void 
nk_ls (
	IN struct hive *hdesc, 
	IN char *path, 
	IN int vofs, 
	IN int type
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ������ǰ���ļ�ֵ���������Ӽ� [���ݹ�]

Arguments:
  path - Ϊ""Ĭ��,Ϊ"\\"��ʾ����,Ϊ��������ָ���ļ�(eg:360\\XGB\\sudami)

--*/
{
	struct nk_key *key;
	int nkofs;
	struct ex_data ex;
	struct vex_data vex;
	int count = 0, countri = 0;
	
	nkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_NK );
	if ( !nkofs ) 
	{
		printf("nk_ls: Key <%s> not found\n",path);
		return;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e)
	{
		printf("Error: Not a 'nk' node!\n");
		debugit( hdesc->buffer, hdesc->size );
	}
	
	printf("Node has %ld subkeys and %ld values",key->no_subkeys,key->no_values);
	if (key->len_classnam) { printf(", and class-data of %d bytes",key->len_classnam); }
	printf("\n");
	
	if (key->no_values) 
	{
		count = 0;
		printf("-------- SelfValue Lists --------\n");
		printf("offs        size      type   value name            [Data]\n");
		while ( (ex_next_v( hdesc, nkofs, &count, &vex ) > 0) ) 
		{
			printf (
				"[%6x] %6d  %-16s  <%s> %6s",
				vex.vkoffs,
				vex.size,
				(vex.type < REG_MAX ? g_value_types[vex.type] : "(unknown)"), 
				vex.name,
				"");
			
			if (vex.type == REG_DWORD) {
				printf(" %*d [0x%x]",25-strlen(vex.name),vex.val , vex.val);
			}
			
			getValueData( hdesc, nkofs, vex.name );
			FREE( vex.name );
		}
	}

	if (key->no_subkeys)
	{
		count = 0;
		printf("-------- SubKey Lists --------\n");
		printf("offs          key name\n");
		ex.name = (char *)HEAP_ALLOC( 0x1000 ); // ��������,�ȷ������ڴ�,��ÿ�α������������

		while( (ex_next_n( hdesc, nkofs, &count, &countri, &ex ) > 0) ) 
		{
			printf("[%6x]   %c  <%s>\n", ex.nkoffs, (ex.nk->len_classnam)?'*':' ',ex.name);	
		}

		HEAP_FREE( ex.name );
	}

	return ;
}


// xx
void 
nk_ls_depeth(
	IN struct hive *hdesc, 
	IN char *path, 
	IN int vofs,
	IN int type
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ������ǰ���ļ�ֵ���������Ӽ� [�ݹ�]

Arguments:
  path - Ϊ""Ĭ��,Ϊ"\\"��ʾ����,Ϊ��������ָ���ļ�(eg:360\\XGB\\sudami)

--*/
{
	struct nk_key *key;
	int nkofs;
	struct ex_data ex;
	struct vex_data vex;
	int count = 0, countri = 0, plen = 0 ;
	
	nkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_NK );
	
	if(!nkofs) 
	{
		printf("nk_ls: Key <%s> not found\n",path);
		return ;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	
	if (key->id != 0x6b6e)
	{
		printf("Error: Not a 'nk' node!\n");
		debugit( hdesc->buffer, hdesc->size );
		return ;
	}
	
	printf("Node has %ld subkeys and %ld values",key->no_subkeys,key->no_values);
	if (key->len_classnam) { printf(", and class-data of %d bytes",key->len_classnam); }
	printf("\n");
	
	// �ȴ�ӡ�Լ�ֵ����Ϣ
	if ( key->no_values ) 
	{
		count = 0;
		printf("-------- SelfValue Lists --------\n");
		printf("offs        size      type   value name            [Data]\n");
		while ( (ex_next_v( hdesc, nkofs, &count, &vex ) > 0) ) 
		{
			printf (
				"[%6x] %6d  %-16s  <%s> %6s",
				vex.vkoffs,
				vex.size,
				(vex.type < REG_MAX ? g_value_types[vex.type] : "(unknown)"), 
				vex.name,
				"");

			if (vex.type == REG_DWORD) {
				printf(" %*d [0x%x]",25-strlen(vex.name),vex.val , vex.val);
			}

			getValueData( hdesc, nkofs, vex.name );
			FREE( vex.name );
		}
	}

	// �ٴ�ӡ�Ӽ���Ϣ
	if ( key->no_subkeys )
	{
		count = 0;
		printf("-------- SubKey Lists --------\n");
		printf("offs          key name\n");
		ex.name = (char *)HEAP_ALLOC( 0x1000 ); // ��������,�ȷ������ڴ�,��ÿ�α������������

		while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) 
		{
			printf("[%6x]   %c  <%s>\n", ex.nkoffs, (ex.nk->len_classnam)?'*':' ',ex.name);
			//
			// �������Ӽ����¼�
			//
#if 0
			memset( tmpPath, 0, ABSPATHLEN - 1 );
			memcpy( tmpPath, path, ABSPATHLEN - 1 );
			if ( *path ) { strcat( tmpPath, "\\" ); }
			strcat( tmpPath, ex.name );
#endif
			nk_ls_depeth( hdesc, "", ex.nkoffs + 4, TRAV_PATH_WANT_NK );	
		}

		HEAP_FREE( ex.name );
	}

	return ;
}


int 
get_val_type (
	IN struct hive *hdesc, 
	IN int vofs, 
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ��ȡָ�����ļ�ֵ����. �������360��2���Լ���ֵ - Ĭ�� �� Test

Arguments:
  vofs - Ϊnk��(�����м�Ϊ360)��ƫ��
  path - ��ֵ����(�����б���ΪTest)

Return Value:
  ��ֵ����.eg:REG_SZ, REG_DWORD, ...

--*/
{
	struct vk_key *vkkey ;
	int vkofs ;
	
	vkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_VK );
	if (!vkofs) 
	{
		return -1;
	}
	vkofs +=4 ;
	vkkey = (struct vk_key *)( hdesc->buffer + vkofs );
#if 0
	if (vkkey->len_data & 0x80000000) return(REG_DWORD); /* Special case of INLINE storage */
#endif
	return ( vkkey->val_type );
}


int 
get_val_len (
	IN struct hive *hdesc,
	IN int vofs, 
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ��ȡָ�����ļ�ֵ����. �������360��2���Լ���ֵ - Ĭ�� �� Test

Arguments:
  vofs - Ϊnk��(�����м�Ϊ360)��ƫ��
  path - ��ֵ����(�����б���ΪTest)

Return Value:
  ��ֵ����

--*/
{
	struct vk_key *vkkey;
	int vkofs, len;

	vkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_VK );
	if (!vkofs) { return -1; }

	vkofs +=4 ;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	
	len = vkkey->len_data & 0x7fffffff;
	
	// Special inline case, return size of 4 (dword)
	// �������,ֻ�����λ��ֵ��, ��ü����ݾ�������
	if ( vkkey->len_data == 0x80000000 ) { len = 4; }
	
	return ( len );
}


void *
get_val_data (
	IN struct hive *hdesc,
	IN int vofs,
	IN char *path,
	IN int val_type
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Caller must keep track of value's length (call function above to get it)

Arguments:
  vofs - Ϊnk��(�����м�Ϊ360)��ƫ��
  path - ��ֵ����(�����б���ΪTest)
  val_type - ָ�����̵�ƥ������.��Ϊ0��ʾ��ָ��

Return Value:
  ָ���ֵ���ݵ�ָ��

--*/
{
	struct vk_key *vkkey ;
	int vkofs ;
	
	vkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_VK );
	if (!vkofs) { return NULL; }

	vkofs +=4;
	vkkey = (struct vk_key *)( hdesc->buffer + vkofs );
	
	if (vkkey->len_data == 0) { return NULL; }
	if (vkkey->len_data == 0x80000000) { // Special inline case (len = 0x80000000)
		return ( &vkkey->val_type );	 // Data (4 bytes?) in type field
	}    
	
	if (val_type && vkkey->val_type && (vkkey->val_type) != val_type)
	{
		printf( "Value <%s> is not of correct type!\n", path );
		return NULL;
	}
	
	// Negative len is inline, return ptr to offset-field which in
	// this case contains the data itself
	if (vkkey->len_data & 0x80000000) { return (&vkkey->ofs_data); }

	return ( hdesc->buffer + vkkey->ofs_data + 0x1004 );
}


struct keyval *
get_val2buf (
	IN struct hive *hdesc,
	IN struct keyval *kv,
	IN int vofs, 
	IN char *path,
	IN int type
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Get and copy key data (if any) to buffer.
  �����߱����ͷŴ˺���������ڴ� -- free( keyval )

Arguments:
  kv - �еĻ�����,û�еĻ��������ڴ���������.�����������
  vofs - Ϊnk��(�����м�Ϊ360)��ƫ��
  path - ��ֵ����(�����б���ΪTest)
  val_type - ָ�����̵�ƥ������.��Ϊ0��ʾ��ָ��

Return Value:
  ָ����Ҫ�鿴�ļ�ֵ������ ָ��

--*/
{
	int len ;
	struct keyval *kr ;
	void *keydataptr ;
	
	len = get_val_len( hdesc, vofs, path );
	if (len == -1) { return (NULL); }
	if (kv && (kv->len < len)) { return (NULL); }
	
	keydataptr = get_val_data( hdesc, vofs, path, type );
	
	// Allocate space for data + header, or use supplied buffer
	if (kv) {
		kr = kv;
	} else {
		ALLOC( (char*)kr, 1, len + sizeof(int) + 4 );
	}
	
	kr->len = len;
	memcpy( &(kr->data), keydataptr, len );
	
	return ( kr );
}


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+              ���� ��ֵ/�Ӽ�(���/ɾ��) ��غ���           +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


// xx
int 
get_dword (
	IN struct hive *hdesc,
	IN int vofs,
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �������,�ڲ�����get_val2buf����,�õ���ֵ����

Arguments:
  vofs - Ϊnk��(�����м�Ϊ360)��ƫ��
  path - ��ֵ����(�����б���ΪTest)

Return Value:
  ָ���ļ�ֵ����

--*/
{
	struct keyval *v ;
	int dword ;
	
	v = get_val2buf( hdesc, NULL, vofs, path, REG_DWORD );
	if (!v) 
	{	// well... -1 COULD BE THE STORED VALUE TOO
		return ( -1 ); // ˵���е���,�����㻹�Ƿ���-1. O__O"��
	}
	
	dword = (int)v->data ;
	
	FREE(v);
	
	return (dword);
}


int
fill_block (
	IN struct hive *hdesc,
	IN int ofs, 
	IN void *data, 
	IN int size
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Sanity checker when transferring data into a block
  ���յ�block��������� 

Arguments:
  data - ���ݿ�

Return Value:
  ����ֵ������ 

--*/
{
	int blksize;
	
	blksize = get_int( hdesc->buffer + ofs ); // �õ�block��Ŀ�������С
	blksize = -blksize;
	
#if 0
	printf("fill_block: ofs = %x - %x, size = %x, blksize = %x\n",ofs,ofs+size,size,blksize);
#endif
	/*  if (blksize < size || ( (ofs & 0xfffff000) != ((ofs+size) & 0xfffff000) )) { */
	if (blksize < size) 
	{
		printf("fill_block: ERROR: block to small for data: ofs = %x, size = %x, blksize = %x\n",ofs,size,blksize);
		debugit(hdesc->buffer,hdesc->size);
		abort();
	}
	
	memcpy( hdesc->buffer + ofs + 4, data, size );
	return ( 0 );
}


int 
free_val_data (
	IN struct hive *hdesc,
	IN int vofs,
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Free actual data of a value, and update value descriptor
  ��ռ�ֵValue���������

Arguments:
  vofs - Ϊvk����ƫ��
  path - ��ֵ����(�����б���ΪTest)

--*/
{
	struct vk_key *vkkey ;
	int vkofs, inl ;
	
	vkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_VK );
	if (!vkofs) { return 0; }

	vkofs +=4;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	
	inl = (vkkey->len_data & 0x80000000);
	if (!inl) { free_block( hdesc, vkkey->ofs_data + 0x1000 ); }

	vkkey->len_data = 0;
	vkkey->ofs_data = 0;
	
	return ( vkofs );
}


int 
alloc_val_data (
	IN struct hive *hdesc,
	IN int vofs, 
	IN char *path,
	IN int size
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Allocate data for value, realloc if it already contains stuff
  Ϊ��ֵValue����ռ�

Arguments:
  vofs - Ϊvk����ƫ��
  path - ��ֵ����(�����б���ΪTest)

Return Value:
  0 - error, >0 pointer to actual dataspace

--*/
{
	struct vk_key *vkkey;
	int vkofs, len;
	int datablk;
	
	vkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_VK );
	if (!vkofs) { return (0); }
	
	vkofs +=4;
	vkkey = (struct vk_key *)(hdesc->buffer + vkofs);
	
	// Allocate space for new data
	datablk = alloc_block( hdesc, vkofs, size );
	if (!datablk) { return (0); }
	
	len = vkkey->len_data & 0x7fffffff;
	
	// Then we dealloc if something was there before
	if (len) { free_val_data( hdesc, vofs, path ); }
	
	// Link in new datablock
	vkkey->ofs_data = datablk - 0x1000;
	vkkey->len_data = size;
	
	return ( datablk + 4 );
}


// xx
VOID
add_value_whatever (
	IN struct hive *hdesc,
	IN char *FatherPath,
	IN char *ValueName,
	IN int RegType,
	IN char *ValueContext
	) 
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ���һ���Ӽ�.ֻ��ָ�� �ü���ȫ·��/�Ӽ��� ����
  ����Ϊ����XGB���һ��ֵ.
    add_value_whatever( pHive, "\\XGB", "LD", REG_SZ );

Arguments:
  FatherPath - �ü���ȫ·��
  ValueName - Ҫ��ӵ�ֵ������(eg. LD)
  RegType - Ҫ��ӵ�ֵ������(eg. REG_SZ)
  ValueContext - ֵ������(eg. "30������" )

--*/
{
	struct keyvala *kr ;
	int r, nkofs, nCounts = 0 ;
	char szTmp[256] ;
	WCHAR szWtmp[256] ;

	nkofs = Get_nkofs_from_fullPath( hdesc, FatherPath );
	if ( -1 == nkofs ) { return ; }
	nkofs += 4;

	add_value( hdesc, nkofs, ValueName, RegType );

	///////////////////////////////////////////
	// ����Դ�
	if ( REG_DWORD == RegType )
	{
		put_dword( hdesc, nkofs, ValueName, ValueContext );
		return ;
	}

	ALLOC( (char*)kr, 1, sizeof(int) + sizeof(int) );
	
	strncpy( szTmp, ValueContext, 256 );
	while ( *ValueContext )
	{
		nCounts++;
		(&(*ValueContext))++ ;

		if ( 0 == *ValueContext ) { break ; }
	}

	// �ַ��͵�Ҫע��~~
	MByteToWChar( szTmp, szWtmp, 256 ); // Ҫ��ɿ��ַ�~
	kr->len = nCounts*2 ;
	kr->data = (PVOID)szWtmp;
	
	r = put_buf2val_sz( hdesc, kr, nkofs, ValueName, RegType );
	FREE(kr);

	return ;
}



// xx
struct vk_key *
add_value (
	IN struct hive *hdesc,
	IN int nkofs,
	IN char *name,
	IN int RegType
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Just add the metadata (empty value), to put data into it, use put_buf2val afterwards
  ����Ϊ�Ӽ����һ���µ�ֵ,ֻ������,������.Ҫ���������,���ŵ��� put_buf2val ��������

Arguments:
  nkofs - Ϊnk����ƫ��
  name - ��ֵ����(�����б���ΪTest)
  RegType - ע������� eg. REG_DWORD

Return Value:
  0 err, >0 offset to value metadata

--*/
{
	struct nk_key *nk ;
	int oldvlist = 0, newvlist, newvkofs ;
	struct vk_key *newvkkey ;
	char *blank="" ;
	
	if (!name || !*name) { return(NULL); }
	
	nk = (struct nk_key *)( hdesc->buffer + nkofs );
	if (nk->id != 0x6b6e)
	{
		printf("add_value: Key pointer not to 'nk' node!\n");
		return (NULL);
	}
	
	if (trav_path( hdesc, nkofs, name, TRAV_PATH_WANT_VK ))
	{
		printf("add_value: value %s already exists\n",name);
		return (NULL);
	}
	
	if (!strcmp(name,"@")) { name = blank; }
	if (nk->no_values) { oldvlist = nk->ofs_vallist; }
	
	newvlist = alloc_block( hdesc, nkofs, nk->no_values * 4 + 4 );
	if (!newvlist) 
	{
		printf("add_value: failed to allocate new value list!\n");
		return (NULL);
	}

	if (oldvlist) { // Copy old data if any
		memcpy(hdesc->buffer + newvlist + 4, hdesc->buffer + oldvlist + 0x1004, nk->no_values * 4 + 4);
	}
	
	// Allocate value descriptor including its name
	newvkofs = alloc_block( hdesc, newvlist, sizeof(struct vk_key) + strlen(name) );
	if (!newvkofs) 
	{
		printf("add_value: failed to allocate value descriptor\n");
		free_block( hdesc, newvlist );
		return (NULL);
	}
	
	// Success, now fill in the metadata
	newvkkey = (struct vk_key *)(hdesc->buffer + newvkofs + 4);
	
	// Add pointer in value list
	*(int *)(hdesc->buffer + newvlist + 4 + (nk->no_values * 4)) = newvkofs - 0x1000;
	
	// ��� ����µ� CM_KEY_VALUE �ṹ��
	newvkkey->id = 0x6b76;
	newvkkey->len_name = strlen(name);

	if (RegType == REG_DWORD || RegType == REG_DWORD_BIG_ENDIAN) {
		newvkkey->len_data = 0x80000004;  // Prime the DWORD inline stuff
	} else {
		newvkkey->len_data = 0x00000000;
	}

	newvkkey->ofs_data = 0; // ���������ֵ,��ֻ������,������.������û�������ݵ�ƫ��
	newvkkey->val_type = RegType;
	newvkkey->flag     = 1;   // Don't really know what this is
	newvkkey->dummy1   = 0;
	strcpy((char *)&newvkkey->keyname, name); 
	
	// ������list,���ͷ�oldlist
	nk->no_values++;
	nk->ofs_vallist = newvlist - 0x1000;
	if (oldvlist) { free_block( hdesc, oldvlist + 0x1000 ); }
	
	return ( newvkkey );
}


// xx
BOOL
add_key_whatever (
	IN struct hive *hdesc,
	IN char *FatherPath,
	IN char *SubKeyName
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ���һ���Ӽ�.ֻ��ָ�� �ü���ȫ·��/�Ӽ��� ����
  ����Ϊ����XGB���һ���Ӽ�LD.
    add_key_whatever( pHive, "\\XGB", "LD" );

Arguments:
  FatherPath - �ü���ȫ·��
  SubKeyName  - Ҫ��ӵ��Ӽ�������(eg. LD)

--*/
{
	int nkofs = Get_nkofs_from_fullPath( hdesc, FatherPath );
	if ( -1 == nkofs ) { return FALSE ; }
	nkofs += 4;

	if ( NULL == add_key( hdesc, nkofs, SubKeyName ) ) { 
		return FALSE ;
	}

	return TRUE ;
}


// xx
struct nk_key *
add_key (
	IN struct hive *hdesc,
	IN int nkofs,
	IN char *name
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Add a subkey to a key. ���һ���Ӽ�.����Ϊ����360���һ���Ӽ�XGB. eg:
  add_key( pHive, pHive->rootofs + 4, "LD" );  // ������360���һ���Ӽ�

Arguments:
  nkofs - Ϊnk����ƫ��
  name  - Ҫ��ӵ��Ӽ�������(eg. XGB)

Return Value:
  ptr to new keystruct, or NULL

--*/
{
	int slot, newlfofs = 0, oldlfofs = 0, newliofs = 0 ;
	int oldliofs = 0 ;
	int o, n, i, onkofs, newnkofs, cmp ;
	int rimax, rislot, riofs, namlen ;
	struct ri_key *ri = NULL ;
	struct lf_key *newlf = NULL, *oldlf ;
	struct li_key *newli = NULL, *oldli ;
	struct nk_key *key, *newnk, *onk ;
	long hash ;

	if ( !name && !*name )
	{
		printf("add_key: Invalid name\n");
		return (NULL);
	}
	
	key = (struct nk_key *)( hdesc->buffer + nkofs );
	if (key->id != 0x6b6e) 
	{
		printf("add_key: current ptr not 'nk'\n");
		return (NULL);
	}
	
	namlen = strlen( name );
	
	slot = -1;
	if ( key->no_subkeys ) 
	{
		oldlfofs = key->ofs_lf ;
		oldliofs = key->ofs_lf ;
		riofs	 = key->ofs_lf ;
		
		oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
		if (oldlf->id != 0x666c && oldlf->id != 0x686c && oldlf->id != 0x696c && oldlf->id != 0x6972) 
		{
			printf("add_key: index type not supported: 0x%04x\n",oldlf->id);
			return (NULL);
		}
		
		rimax = 0; ri = NULL; rislot = -1;
		if (oldlf->id == 0x6972) // �� 'ri' �ṹ
		{  
			ri = (struct ri_key *)( hdesc->buffer + riofs + 0x1004 );
			rimax = ri->no_lis - 1 ;

			oldlfofs = ri->hash[rislot+1].ofs_li ;
			oldliofs = ri->hash[rislot+1].ofs_li ;
		}
		
		do 
		{   // 'ri' loop, at least run once if no 'ri' deep index	
			if ( ri ) // Do next 'ri' slot
			{ 
				rislot++;
				oldliofs = ri->hash[rislot].ofs_li ;
				oldlfofs = ri->hash[rislot].ofs_li ;
				oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
				oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
			}
			
			oldli = (struct li_key *)(hdesc->buffer + oldliofs + 0x1004);
			oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
			
			slot = -1;
			if (oldli->id == 0x696c) // �� 'li' �ṹ
			{  
				FREE( newli ); // �ͷžɵ�����
				ALLOC( (char*)newli, 8 + 4 * oldli->no_keys + 4, 1 );
/*++
  ϸ˵�������Ĵ�С:
                                                                 ������4�ֽڵ�����,һ��ΪE8 FF FF FF
																 _____________________________________		
                           +-'lf'/'lh'�ṹ---+                  |+0x0 short id;						  |	 
						   |                  \                 |+0x2 short no_keys;                  | 
						   |				   } struct lf_key -+			                          |
						   |                  /                 |+0x4 struct lf_hash �� struct lh_hash|         
		(SubKey)		   |          +- 'lf'/'lh'�ṹ          |     �����СΪ8                     |
  CM_KEY_NODE.SubKeyLists -+-'ri'�ṹ-+						     -------------------------------------	
						   |		  +- 'li'�ṹ -+        
						   |             +---------+       
						   |              \	                ������4�ֽڵ�����,һ��ΪE8 FF FF FF
						   |               } struct li_key -+--------------------
						   |              /                 |+0x0 short id;      |
						   +-'li'�ṹ----+                  |+0x2 short no_keys; | 
                                                            |+0x4 struct li_hash | 
															|     �����СΪ4    |
															 --------------------
                      [ͼ.�Ӽ����ͼ�������ṹ.sudami]


  ���� 8 + 4 * oldli->no_keys + 4
	   __________________________
       |   |                    |_ ����һ���µĽṹ��.���С
	   |   |
       |   |_ li_key�ṹ��.�����СΪ4
	   |
	   |_  li_key�ṹ��֮ǰ��4�ֽ�����,�ټ�4�ֽ�(id �� no_keys ). ��8�ֽ�


--*/
				newli->no_keys = oldli->no_keys;
				newli->id = oldli->id;
				
				// Now copy old, checking where to insert (alphabetically)
				for ( o = 0, n = 0; o < oldli->no_keys; o++,n++ ) 
				{
					onkofs = oldli->hash[o].ofs_nk ;
					onk = (struct nk_key *)( onkofs + hdesc->buffer + 0x1004 );
					if (slot == -1) 
					{	
/*
  �������壺int strncasecmp( const char *s1, const char *s2, size_t n )
  ����˵����strncasecmp()�����Ƚϲ���s1��s2�ַ���ǰn���ַ����Ƚ�ʱ���Զ����Դ�Сд�Ĳ���
������ֵ��������s1��s2�ַ�����ͬ�򷵻�0; s1������s2�򷵻ش���0��ֵ; s1��С��s2�򷵻�С��0��ֵ 
*/
						// ���Ҫ��ӵ��Ӽ��Ƿ��Ѿ�����
						cmp = strncasecmp( name, onk->keyname, (namlen > onk->len_name) ? namlen : onk->len_name );
						if (!cmp) 
						{
							printf( "add_key: key %s already exists!\n", name );
							FREE( newli );
							return (NULL);
						}

						if ( cmp < 0) // û������~~
						{
							slot = o;
							rimax = rislot; // Cause end of 'ri' search, too
							n++;
						}
					}

					newli->hash[n].ofs_nk = oldli->hash[o].ofs_nk ;
				} // forѭ��

				if (slot == -1) { slot = oldli->no_keys; }
				
			} 
			else // �� 'lf' �� 'lh' �ṹ
			{ 
				
				oldlf = (struct lf_key *)(hdesc->buffer + oldlfofs + 0x1004);
				
				FREE( newlf ); // �ͷžɵ�����
				ALLOC( (char*)newlf, 8 + 8*oldlf->no_keys + 8, 1 );
				newlf->no_keys = oldlf->no_keys;
				newlf->id = oldlf->id;
				
				// Now copy old, checking where to insert (alphabetically)
				for (o = 0, n = 0; o < oldlf->no_keys; o++,n++) 
				{
					onkofs = oldlf->hash[o].ofs_nk;
					onk = (struct nk_key *)( onkofs + hdesc->buffer + 0x1004 );
					if (slot == -1) 
					{
						cmp = strncasecmp( name, onk->keyname, (namlen > onk->len_name) ? namlen : onk->len_name );
						if (!cmp) 
						{
							printf("add_key: key %s already exists!\n", name);
							FREE( newlf );
							return (NULL);
						}

						// �����Ӽ��ǰ���A-Z��˳�����е�,������Ҫ�ҵ��Ǹ��ڵ�
						if ( cmp < 0 ) 
						{
							// �ҵ���,����ʣ�µ�ҲҪѭ�����Ƶ��µ�Buffer��ȥ
							slot = o;
							rimax = rislot;  // ��������ͣѭ����.
							n++;
						}
						
						// ��������Ӽ��� ZZZZ,��ô�����ڶ��е����ͷ,���������,ԭ����û��
						// ����,���²����ʱ�������ѭ����~~~ ���Ե�һ����BUG
						if ( (-1 == slot) && (cmp > 0) && ((o+1) == oldlf->no_keys) )
						{
							// �Ѿ�Сѭ������, ������,˵��Ҫ������Ӽ���������������
							slot = o+1;		 // һ��Ҫ��1
							rimax = rislot;  // ��������ͣѭ����.
						}
						
					}

					newlf->hash[n].ofs_nk = oldlf->hash[o].ofs_nk;
					newlf->hash[n].name[0] = oldlf->hash[o].name[0];
					newlf->hash[n].name[1] = oldlf->hash[o].name[1];
					newlf->hash[n].name[2] = oldlf->hash[o].name[2];
					newlf->hash[n].name[3] = oldlf->hash[o].name[3];
				}

				if ( slot == -1 ) { slot = oldlf->no_keys; }

			} // li else check
		} while ( (rislot < rimax) );  // 'ri' wrapper loop
	} 
    else // Parent was empty, make new index block
	{    // �����ǿյ�,û���Ӽ�. �������һ���µĿ�
		ALLOC( (char*)newlf, 8 + 8, 1 );
		newlf->no_keys = 1;
		// Use ID (lf, lh or li) we fetched from root node, so we use same as rest of hive
		newlf->id = hdesc->nkindextype;
		slot = 0;
	}
	
	// Make and fill in new nk
	newnkofs = alloc_block( hdesc, nkofs, sizeof(struct nk_key) + strlen(name) );
	if (!newnkofs) 
	{
		printf("add_key: unable to allocate space for new key descriptor for %s!\n",name);
		FREE( newlf );
		FREE( newli );
		return (NULL);
	}
	
	newnk = (struct nk_key *)( hdesc->buffer + newnkofs + 4 );
	
	newnk->id            = 0x6b6e ;
	newnk->type          = KEY_NORMAL ;
	newnk->ofs_parent    = nkofs - 0x1004 ;
	newnk->no_subkeys    = 0 ;
	newnk->ofs_lf        = 0 ;
	newnk->no_values     = 0 ;
	newnk->ofs_vallist   = -1 ;
	newnk->ofs_sk        = key->ofs_sk ; // Get parents for now. 0 or -1 here crashes XP
	newnk->ofs_classnam  = -1 ;
	newnk->len_name      = strlen( name );
	newnk->len_classnam  = 0 ;
	strcpy( newnk->keyname, name );
	
	if ( newli ) // Handle li
	{  
		// And put its offset into parents index list
		newli->hash[slot].ofs_nk = newnkofs - 0x1000 ;
		newli->no_keys++ ;
		
		// Allocate space for our new li list and copy it into reg
		newliofs = alloc_block( hdesc, nkofs, 8 + 4 * newli->no_keys );
		if (!newliofs) 
		{
			printf("add_key: unable to allocate space for new index table for %s!\n", name);
			FREE( newli );
			free_block( hdesc, newnkofs );
			return (NULL);
		}
		//	memcpy(hdesc->buffer + newliofs + 4, newli, 8 + 4*newli->no_keys);
		fill_block( hdesc, newliofs, newli, 8 + 4 * newli->no_keys );
	} 
	else // lh or lf
	{  
	//	printf("add_key: lf/lh fill at slot: %d, rislot: %d\n",slot,rislot);
		// And put its offset into parents index list
		newlf->hash[slot].ofs_nk = newnkofs - 0x1000;
		newlf->no_keys++;
		if ( newlf->id == 0x666c )  // lf hash
		{        
			newlf->hash[slot].name[0] = 0;
			newlf->hash[slot].name[1] = 0;
			newlf->hash[slot].name[2] = 0;
			newlf->hash[slot].name[3] = 0;
			strncpy( newlf->hash[slot].name, name, 4 );
		} 
		else if (newlf->id == 0x686c) // lh. XP uses this. hashes whole name
		{  
			for ( i = 0, hash = 0; i < (int)strlen(name); i++ ) 
			{
				hash *= 37;
				hash += toupper(name[i]); // hashֵΪ��д
			}

			newlf->lh_hash[slot].hash = hash ;
		}
		
		// Allocate space for our new lf list and copy it into reg
		newlfofs = alloc_block( hdesc, nkofs, 8 + 8 * newlf->no_keys );
		if (!newlfofs)
		{
			printf("add_key: unable to allocate space for new index table for %s!\n",name);
			FREE( newlf );
			free_block( hdesc, newnkofs );
			return (NULL);
		}

		fill_block( hdesc, newlfofs, newlf, 8 + 8*newlf->no_keys );
		
	} // li else
	
	
	// Update parent, and free old lf list
	key->no_subkeys++ ;

	if (ri) { // ri index
		ri->hash[rislot].ofs_li = (newlf ? newlfofs : newliofs) - 0x1000 ;
	} else {  // Parent key
		key->ofs_lf = (newlf ? newlfofs : newliofs) - 0x1000 ;
	}
	
	if (newlf && oldlfofs) { free_block( hdesc, oldlfofs + 0x1000 ); }
	if (newli && oldliofs) { free_block( hdesc, oldliofs + 0x1000 ); }
	
	FREE( newlf );
	FREE( newli );
	return (newnk);
}



void 
del_vk (
	IN struct hive *hdesc,
	IN int vkofs
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Remove a vk-struct incl dataspace if any. Mostly for use by higher level stuff
  ���ܺ��� del_value �� del_allvalues����. �˺��������VK�е�����,�ͷ��ڴ�

Arguments:
  vkofs - offset to vk

--*/
{
	struct vk_key *vk ;
	
	vk = (struct vk_key *)( hdesc->buffer + vkofs );
	if (vk->id != 0x6b76)
	{
		printf("del_vk: Key pointer not to 'vk' node!\n");
		return ;
	}
	
	if ( !(vk->len_data & 0x80000000) && vk->ofs_data) 
	{
		// �����VK�ṹ������,������������һ��ƫ�ƴ�,�ͷŵ����Buffer
		free_block( hdesc, vk->ofs_data + 0x1000 );
	}
	
	free_block( hdesc, vkofs - 4 ); // �������ͷ�����VK�ṹ
	
	return ;
}


// xx
BOOL 
del_allValues_whatever (
	IN struct hive *hdesc,
	IN char *ValuePath
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ɾ��һ������ӵ�е���������. ֻ��ָ���ü���ȫ·������. eg.
    del_allValues_whatever( pHive, "\\XGB\\DBY\\LSB" );

Arguments:
  nkofs - Ϊnk����ƫ��

--*/
{
	int nkofs = Get_nkofs_from_fullPath( hdesc, ValuePath );
	if ( -1 == nkofs ) { return FALSE ; }
	nkofs += 4;

	// ��ʼ���ݸ�����nkofs,����ɾ������
	del_allvalues( hdesc, nkofs );

	return TRUE ;
}


// xx
void 
del_allvalues (
	IN struct hive *hdesc,
	IN int nkofs
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Delete all values from key (used in recursive delete)
  ɾ��һ������ӵ�е���������

Arguments:
  nkofs - Ϊnk����ƫ��

--*/
{
	int vlistofs, number, vkofs ;
	long *vlistkey ; // ָ���ֵ�����ָ��
	struct nk_key *nk ;
	
	// �Ϸ���У��
	nk = (struct nk_key *)(hdesc->buffer + nkofs);
	if (nk->id != 0x6b6e) 
	{
		printf("del_allvalues: Key pointer not to 'nk' node!\n");
		return;
	}
	
	if (!nk->no_values) { return; }
	
	// ��ʼ������еļ�ֵ
	vlistofs = nk->ofs_vallist + 0x1004;
	vlistkey = (long *)(hdesc->buffer + vlistofs);
	
	for (number = 0; number < nk->no_values; number++) 
	{
		vkofs = vlistkey[number] + 0x1004;
		del_vk( hdesc, vkofs );
	}
	
	// �ͷŴ洢VK list���������ڴ�.���¶�Ӧ��nk
	free_block( hdesc, vlistofs-4 );
	nk->ofs_vallist = -1 ;
	nk->no_values = 0 ;

	return ;
}


// xx
BOOL 
del_value_whatever (
	IN struct hive *hdesc,
	IN char *ValuePath,
	IN char *ValueName
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ɾ��һ������ָ��������, ֻ��ָ���ü���ȫ·��/ ��������. eg.
    del_value_whatever( pHive, "\\XGB\\DBY\\LSB", "@" );
  ��ɾ��LSB����ڵ��Ĭ��ֵ

Arguments:
  nkofs - Ϊnk����ƫ��
  name  - name of value to delete

Return Value:
  FALSE - failed; TRUE - OK

--*/
{
	int nkofs = Get_nkofs_from_fullPath( hdesc, ValuePath );
	if ( -1 == nkofs )
	{
		return FALSE ;
	}
	nkofs += 4;

	return del_value( hdesc, nkofs, ValueName ) ;
}


// xx
BOOL 
del_value (
	IN struct hive *hdesc,
	IN int nkofs,
	IN char *name
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Delete single value from key
  ɾ��һ������ָ��������

Arguments:
  nkofs - Ϊnk����ƫ��
  name  - name of value to delete

Return Value:
  FALSE - failed; TRUE - OK

--*/
{
	int vlistofs, slot, o, n, vkofs, newlistofs ;
	long *vlistkey, *tmplist, *newlistkey ;
	struct nk_key *nk ;
	char *blank = "" ; // Ĭ�ϼ�������
	
	if (!name || !*name) { return 1; }
	if ( 0 == strcmp(name,"@") ) { name = blank; }
	
	nk = (struct nk_key *)(hdesc->buffer + nkofs);
	if (nk->id != 0x6b6e) 
	{
		printf("del_value: Key pointer not to 'nk' node!\n");
		return FALSE;
	}
	
	if (!nk->no_values) 
	{
		printf("del_value: Key has no values!\n");
		return FALSE;
	}
	
	vlistofs = nk->ofs_vallist + 0x1004;
	vlistkey = (long *)(hdesc->buffer + vlistofs);
	
	slot = vlist_find( hdesc, vlistofs, nk->no_values, name );
	if (slot == -1) 
	{
		printf("del_value: value %s not found!\n",name);
		return FALSE;
	}
	
	// ɾ�� vk �� �����������
	vkofs = vlistkey[slot] + 0x1004;
	del_vk( hdesc, vkofs );
	
	// ���ƾ��������ݵ��·�����ڴ���. һ��Ҫ�ǵ��ͷ�
	CREATE( tmplist, long, nk->no_values );
	memcpy( tmplist, vlistkey, nk->no_values * sizeof(long) );
	
	free_block( hdesc, vlistofs - 4 );  // ɾ���ɵ�����
	nk->no_values-- ;
	
	if (nk->no_values) 
	{
		newlistofs = alloc_block( hdesc, vlistofs, nk->no_values * sizeof(long) );
		if (!newlistofs) 
		{
			printf("del_value: FATAL: Was not able to alloc new index list\n");
			FREE( tmplist ); // �ͷŸղ�������ڴ�
			return FALSE;
		}

		// Now copy over, omitting deleted entry
		newlistkey = (long *)(hdesc->buffer + newlistofs + 4);
		for (n = 0, o = 0; o < nk->no_values+1; o++, n++) 
		{
			if (o == slot) o++;
			newlistkey[n] = tmplist[o];
		}
		nk->ofs_vallist = newlistofs - 0x1000;

	} else {
		nk->ofs_vallist = -1;
	}

	FREE( tmplist ); // �ͷŸղ�������ڴ�
	return TRUE;
}


// xx
BOOL 
del_key_hasNoSubKey (
	IN struct hive *hdesc,
	IN int nkofs, 
	IN char *name
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Delete a subkey from a key. ɾ��һ�� "û���Ӽ�,û������" ���Ӽ�.
  ȱ���� -- ����2�������Ȼ�ȡ.����Ҫɾ���Ӽ��ĸ�����nk�ṹƫ��
  д�ĺ�Ư��,���ǵĺ���ȫ

  +-360 ;����
  | +-MJ
  | +-XGB
  |	| +-sudami ;��ǰ��,��������õ�ȫ·��Ϊ:\\360\\XGB\\sudami
  |	| +-WangYu
  | |	...
  | ...
  ...

  1. ����Ϊ����360ɾ��һ���Ӽ�MJ( MJ����û���Ӽ��� )
     Ӧ����������: del_key_hasNoSubKey( pHive, pHive->rootofs + 4, "MJ" );
  2. Ҫɾ��sudami����Ӽ�,���������֪������XGB��nk�ṹƫ��(eg. ��֪Ϊ XGBnkofs)
     Ӧ����������: del_key_hasNoSubKey( pHive, XGBnkofs, "sudami" );

Arguments:
  nkofs - Ϊnk����ƫ��
  name  - Ҫɾ�����Ӽ�������,Ҫ��ȫƥ��,�Ҵ�Сд����

Return Value:
  FALSE - failed; TRUE - OK

--*/
{
	int o, n, onkofs, del_nkofs ;
	int namlen, rimax, riofs, rislot ;
	int oldliofs = 0, newNumber_keys = 0, newriofs = 0 ;
	int slot_delIndex_in_SubKeyList = 0, newlfofs = 0, oldlfofs = 0;
	struct ri_key *ri, *newri = NULL ;
	struct lf_key *newlf = NULL, *oldlf = NULL ;
	struct li_key *newli = NULL, *oldli = NULL ;
	struct nk_key *key, *onk, *del_nk ;
	char fullpath[501] ;
	char *string = "" ; // xxxx
	
	// �õ�������nk,ȷ������ȷʵ���Ӽ�
	key = (struct nk_key *)( hdesc->buffer + nkofs );
	if (key->id != 0x6b6e) 
	{
		printf("del_key_hasNoSubKey: current ptr not nk\n");
		return FALSE ;
	}
	if (!key->no_subkeys)
	{
		printf("del_key_hasNoSubKey: FatherKey has no subkeys!\n");
		return FALSE ;
	}
	
	// OK,�Ϸ�.��ʼ�Ҹ�������Ҫɾ�����Ӽ�
	namlen = strlen( name ); // ������������,���ȼ�������ľ������.

	oldlfofs = key->ofs_lf ;
	oldliofs = key->ofs_lf ;
	
	oldlf = (struct lf_key *)( hdesc->buffer + oldlfofs + 0x1004 );
	if (oldlf->id != 0x666c && oldlf->id != 0x686c && oldlf->id != 0x696c && oldlf->id != 0x6972)  
	{
		printf("del_key_hasNoSubKey: index other than 'lf', 'li' or 'lh' not supported yet. 0x%04x\n",oldlf->id);
		return FALSE ;
	}
	
	rimax = 0; ri = NULL; riofs = 0; rislot = 0; slot_delIndex_in_SubKeyList = -1 ;
	
	if (oldlf->id == 0x6972) // �� 'ri' �ṹ
	{
		riofs = key->ofs_lf ;
		ri = (struct ri_key *)( hdesc->buffer + riofs + 0x1004 );
		rimax = ri->no_lis - 1 ;		

		oldliofs = ri->hash[rislot+1].ofs_li ; // ��2��ò��û����
		oldlfofs = ri->hash[rislot+1].ofs_li ;
	}
	
	do // 'ri' loop, ��ʹ���Ǵ�Root, Ҳ��ѭ��һ��,���Ǳ�Ҫ�� 
	{   
		if (ri) // ������Ǹ���Root,��ÿ��ѭ��ʱ,���ri�ṹ��ȡ��Ϣ
		{
			rislot++;
			oldliofs = ri->hash[rislot].ofs_li ;
			oldlfofs = ri->hash[rislot].ofs_li ;
			oldli = (struct li_key *)( hdesc->buffer + oldliofs + 0x1004 );
			oldlf = (struct lf_key *)( hdesc->buffer + oldlfofs + 0x1004 );
		}
		
		oldli = (struct li_key *)( hdesc->buffer + oldliofs + 0x1004 );
		oldlf = (struct lf_key *)( hdesc->buffer + oldlfofs + 0x1004 );	

		slot_delIndex_in_SubKeyList = -1 ;
		
		if (oldlf->id == 0x696c) //'li' handler
		{
			FREE( newli );
			ALLOC( (char*)newli, 8 + 4 * oldli->no_keys - 4, 1 ); 
			// �ȷ�����ʱ�ڴ�,�洢������Ϣ,�������ڴ˸���nk��Ӧ��block��
			// �ҵ��㹻�Ŀռ�,�������ʱ�ڴ����µ����ݿ�����ȥ

			newli->id		= oldli->id			 ;
			newli->no_keys	= oldli->no_keys - 1 ; 
			newNumber_keys	= newli->no_keys	 ;
			
			// �����ɵ��������ݵ��������Buffer��,�����������Ӽ�,�ҵ��Ǹ�Ҫɾ�����Ӽ�
			for (o = 0, n = 0; o < oldli->no_keys; o++, n++)
			{
				onkofs = oldli->hash[o].ofs_nk ;
				onk = (struct nk_key *)( onkofs + hdesc->buffer + 0x1004 );

				// ����ֵת����asni,����Ƚ�
				string = (char*) change_to_ansi( onk->keyname, onk->len_name ); 

				// �����if���ֻ��ִ��һ��,Ȼ��slot_delIndex_in_SubKeyList��ֵ�ͱ���.������-1��
				if ( slot_delIndex_in_SubKeyList == -1 
					&& onk->len_name == namlen 
					&& !strncmp( name, string, (onk->len_name > namlen) ? onk->len_name : namlen )
					) 
				{
					// �ҵ���Ҫɾ�����Ǹ��Ӽ�. slot_delIndex_in_SubKeyList ����������Ӽ���subkeyList�����е����
					slot_delIndex_in_SubKeyList = o		 ;
					del_nkofs					= onkofs ; 
					del_nk						= onk	 ;
					rimax						= rislot ; 
					// ��Ϊ���������һ��do ... while��ѭ��,�����������rislot < rimax
					// ����������þ���,��Ȼ�Ѿ��ҵ���Ҫɾ�����Ӽ���Ϣ,�Ͳ���ѭ����ȥ��.
					o++ ; // �Լӵ�Ŀ�����������������ɾ�����Ӽ�.���´������Ӧ��nkofs���µ�Buffer��
				}

				FREE( string ); // change_to_ansi()�������ú���Ҫ�ͷ���������ڴ�

				newli->hash[n].ofs_nk = oldli->hash[o].ofs_nk ; // ���𿽱�����Ϣ����Buffer��
			}
		} 
		else // 'lf' or 'lh' are similar
		{
			// ע��ͬ��
			FREE( newlf );
			ALLOC( (char*)newlf, 8 + 8 * oldlf->no_keys - 8, 1 );

			newlf->id		= oldlf->id			 ;
			newlf->no_keys	= oldlf->no_keys - 1 ; 
			newNumber_keys	= newlf->no_keys	 ;
			
			for (o = 0, n = 0; o < oldlf->no_keys; o++, n++) 
			{
				onkofs = oldlf->hash[o].ofs_nk ;
				onk = (struct nk_key *)( onkofs + hdesc->buffer + 0x1004 );

				string = (char*) change_to_ansi( onk->keyname, onk->len_name ); // ����ֵת����asni,����Ƚ�

				if (slot_delIndex_in_SubKeyList == -1 
					&& (onk->len_name == namlen) 
					&& !strncmp( name, string, namlen )
					) 
				{
					
					slot_delIndex_in_SubKeyList = o		 ;
					del_nkofs					= onkofs ; 
					del_nk						= onk	 ;
					rimax						= rislot ;
					o++ ;
				}

				FREE( string ); // change_to_ansi()�������ú���Ҫ�ͷ���������ڴ�

				// ��Ϊ���ݽṹ��ͬ,���Կ������������в�ͬ
				newlf->hash[n].ofs_nk  = oldlf->hash[o].ofs_nk  ;
				newlf->hash[n].name[0] = oldlf->hash[o].name[0] ;
				newlf->hash[n].name[1] = oldlf->hash[o].name[1] ;
				newlf->hash[n].name[2] = oldlf->hash[o].name[2] ;
				newlf->hash[n].name[3] = oldlf->hash[o].name[3] ;
			}
		} // else lh or lf
		
	} while (rislot < rimax);  // ri traverse loop
	
	if ( -1 == slot_delIndex_in_SubKeyList )
	{
		printf("del_key_hasNoSubKey: subkey %s not found!\n", name);
		FREE( newlf );
		FREE( newli );
		return FALSE ;
	}

	// OK! �ҵ���Ҫɾ���Ӽ���ȫ����Ϣ
	if ( del_nk->no_values ) 
	{ 
		del_allvalues( hdesc, del_nkofs) ; // ��Ҫɾ�����Ӽ�������,ɾ��֮
		del_nk->no_values = 0 ;
	}
	
	if ( del_nk->no_values || del_nk->no_subkeys ) 
	{
		// ȷ��Ҫɾ���Ӽ�û������,��û���Ӽ�. �����򷵻ش���
		printf("del_key_hasNoSubKey: subkey %s has subkeys or values. Not deleted.\n", name);
		FREE( newlf );
		FREE( newli );
		return FALSE ;
	}

	// ��ʼ�����ĸ���...
	if ( newNumber_keys && (newlf || newli) ) 
	{
		// newNumber_keys ����,����Ҫɾ�����Ӽ� ���� �ø��� Ψһ���Ӽ�
		newlfofs = alloc_block( hdesc, nkofs, 8 + (newlf ? 8 : 4) * newNumber_keys );
		if ( !newlfofs )
		{
			printf("del_key_hasNoSubKey: WARNING: unable to allocate space for new key descriptor for %s! Not deleted\n", name);
			FREE( newlf );
			return FALSE ;
		}
		
		// ������׼���õ� ��ʱ�ڴ���е� ������ ��䵽 ����Block��
		fill_block (
			hdesc, 
			newlfofs,
			( (void *)newlf ? (void *)newlf : (void *)newli ), 
			8 + (newlf ? 8 : 4) * newNumber_keys
			);
	} 
	else 
	{  
		// newNumber_keys ������,����Ҫɾ�����Ӽ� �� �ø��� Ψһ���Ӽ�
		newlfofs = 0xfff ;  // We subtract 0x1000 later
	}
	
	if (newlfofs < 0xfff) 
	{
		printf( "del_key_hasNoSubKey: ERROR: newlfofs = %x\n", newlfofs );
		debugit( hdesc->buffer, hdesc->size );
		return FALSE ;
	}
	
	// ��Ҫɾ�����Ӽ�ӵ��CLASS data,ͬ��Ҫfree��
	if ( del_nk->len_classnam ) {
		free_block( hdesc, del_nk->ofs_classnam + 0x1000 );
	}

	// Now it's safe to zap the nk And the old index list
	free_block( hdesc, del_nkofs + 0x1000 ); // �ͷŵ���ɾ���Ӽ������nk�ṹ
	free_block( hdesc, (oldlfofs ? oldlfofs : oldliofs) + 0x1000 ); // �ͷŵ������ɵ������
	
	key->no_subkeys-- ; // ���¸�����Ϣ
	
	if ( ri ) 
	{
		if ( newlfofs == 0xfff ) 
		{
			// �Ǵ�Root 'ri',��Ҫɾ�����Ӽ� �� �ø��� Ψһ���Ӽ� --> ��������Ĵ������
			*fullpath = 0 ;
			get_abs_path( hdesc, nkofs, fullpath, 480 );
			// �õ�������ȫ·�� eg. "360\\XGB"
			
		//	printf("del_key_hasNoSubKey: need to delete ri-entry! %x - %s\n", nkofs, fullpath );
			if ( ri->no_lis > 1 )
			{ 
				// We have subindiceblocks left? Delete from array
				ALLOC( (char*)newri, 8 + 4 * ri->no_lis - 4, 1 );
				newri->no_lis	= ri->no_lis - 1 ;
				newri->id		= ri->id		 ;

				for (o = 0, n = 0; o < ri->no_lis; o++,n++) 
				{
					if ( n == rislot ) { o++ ; }
					newri->hash[n].ofs_li = ri->hash[o].ofs_li ;
				}

				newriofs = alloc_block( hdesc, nkofs, 8 + newri->no_lis * 4 );
			//	printf("del_key_hasNoSubKey: alloc_block for ri-block returns: %x\n",newriofs);

				if ( !newriofs ) 
				{
					printf("del_key_hasNoSubKey: WARNING: unable to allocate space for ri-index for %s! Not deleted\n",name);
					FREE( newlf );
					FREE( newri );
					return FALSE ;
				}

				fill_block( hdesc, newriofs, newri, 8 + newri->no_lis * 4 );
				free_block( hdesc, riofs + 0x1000 ); // �ͷŵ������ɵ�ri�����

				key->ofs_lf = newriofs - 0x1000 ; // �ô��� �������Ӽ�ƫ�� ��ָ��ָ���µ����ri
				FREE( newri );
			} 
			else
			{
				// Last entry in ri was deleted, get rid of it, key is empty
			//	printf( "del_key_hasNoSubKey: last ri deleted for %x\n", nkofs );
				free_block( hdesc, riofs + 0x1000 );
				key->ofs_lf = -1 ;
			}
		}
		else
		{
			// ��Root 'ri',�� Ҫɾ�����Ӽ� �� �ø��� Ψһ���Ӽ� --> ��������Ĵ������
			ri->hash[rislot].ofs_li = newlfofs - 0x1000; 
		}
	}
	else
	{
		// ��Root 'ri',�� Ҫɾ�����Ӽ� �� �ø��� Ψһ���Ӽ� --> ��������Ĵ������
		key->ofs_lf = newlfofs - 0x1000;
	}
	
	return TRUE ;
}


// xx
BOOL 
del_key_whatever (
	IN struct hive *hdesc,
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Recursive delete keys. ɾ��ָ��·���ļ�. �ؼ���Ҫ�ҵ����
  ��ֵ��Ӧ�ĸ�����nkofs. ����Ҫɾ������XGB�µ�һ���Ӽ�DBY. 
    del_key_whatever( hdesc, "\\XGB\\DBY" ); 
  ����\\XGB�Ǹ���, DBY���ӽڵ�

Arguments:
  path  - Ҫɾ�����Ӽ���ȫ·��
  vofs - Ϊnk����ƫ��

--*/
{
	int	nkofs = 0, nfatherLen = 0 ;
	char dummyPath[ABSPATHLEN+1], szFatherPath[ABSPATHLEN+1] ;
	char *szSubNode_ptr, *szFatherPath_ptr ;

	// �����Ϸ��Լ��
	if (!path || !*path) { return FALSE ; }
	ZERO( dummyPath ); ZERO( szFatherPath );
	szFatherPath_ptr = szFatherPath ;

	if ( '\\' != *path ) 
	{	// ·����ʼ����û��\\,Ĭ���Ǹ���,���Ǽ���ȥ
		_snprintf( dummyPath, MAX( ABSPATHLEN+1, sizeof(path) ) + 1, "%s%s", "\\", path ) ;
	} else {
		strncpy( dummyPath, path, MAX( ABSPATHLEN+1, sizeof(path) ) );
	}
	
	// �ָ���������ӽڵ�
	szFatherPath_ptr = strrchr( dummyPath, '\\' );
	if ( szFatherPath_ptr )
	{
		nfatherLen = (int)( szFatherPath_ptr - dummyPath );
		if ( nfatherLen == 0 ) 
		{ 
			// ����ɾ���ľ��Ǹ����µ��Ӽ�
			szFatherPath_ptr++ ;
			del_key_father_s_subkey( hdesc, hdesc->rootofs + 4, szFatherPath_ptr );
			return TRUE ;
		}

		strncpy( szFatherPath, dummyPath, nfatherLen );
	}

	// ���Ҹ�����nkofs
	nkofs = trav_path( hdesc, 0, szFatherPath, TRAV_PATH_WANT_NK );
	if ( !nkofs )
	{
		printf( "del_key_whatever: Key <%s> not found\n", path );
		return FALSE ;
	}
	nkofs += 4;

	// �ҵ��ַ��� "\\XGB\\DBY" �е����һ�� "DBY"
	szSubNode_ptr = strrchr( dummyPath, '\\' );
	if ( ++szSubNode_ptr )
	{
		del_key_father_s_subkey( hdesc, nkofs, szSubNode_ptr );
	}

	return TRUE ;
}


// xx
void 
del_key_father_s_subkey (
	IN struct hive *hdesc,
	IN int vofs,
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  Recursive delete keys. ֻ��ɾ��������һ���Ӽ�(��������Ӽ����Ƿ��и�����Ӽ�)
  ����Ҫɾ������360�µ�һ���Ӽ�XGB. ��ǰ��������:
    *Path = "XGB"
    vofs = 360������� nk ��ƫ�� 

Arguments:
  path  - Ҫɾ�����Ӽ���ȫ·��
  vofs - Ϊnk����ƫ��

--*/
{
	struct ex_data	ex			;
	struct nk_key	*key		;
	int				nkofs = 0	;
	int count = 0, countri = 0  ;
	
	if (!path || !*path) { return; }
	
	nkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_NK );
	if ( !nkofs )
	{
		printf( "del_key_father_s_subkey: Key <%s> not found\n", path );
		return ;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e) 
	{
		printf("Error: Not a 'nk' node!\n");
		debugit( hdesc->buffer, hdesc->size );
	}
	
	// �����Ӽ������Լ����Ӽ�,�ݹ�ɾ��
	if ( key->no_subkeys ) 
	{
		ex.name = (char *)HEAP_ALLOC( 0x1000 ); // ��������,�ȷ������ڴ�,��ÿ�α������������

		while ( (ex_next_n( hdesc, nkofs, &count, &countri, &ex ) > 0) ) 
		{
#if 0
			printf("deleting SubKey Name: %s\n",ex.name);
#endif
			del_key_father_s_subkey( hdesc, nkofs, ex.name ); // �ݹ�ɾ�Ӽ�
			count = 0;   // ��2��ֵ����0����Ҫ,��Ϊɾ���ϸ��Ӽ���
			countri = 0; // �¸��Ӽ��ͱ���˵�һ��.
		}

		HEAP_FREE( ex.name );
	}
	
	// ��ʼɾ��...
	del_allvalues( hdesc, nkofs) ; // ɾ���˼�������
	del_key_hasNoSubKey( hdesc, key->ofs_parent+0x1004, path ); // ɾ����nk�ڵ�
	
	return ;
}
  

/* Get and copy keys CLASS-data (if any) to buffer
 * Returns a buffer with the data (first long is size). see ntreg.h
 * NOTE: caller must deallocate buffer! a simple free(keyval) will suffice.
 */
struct keyval *
get_class (
	IN struct hive *hdesc,
	IN int curnk,
	IN char *path
	)
{
  int clen = 0, dofs = 0, nkofs;
  struct nk_key *key;
  struct keyval *data;
  void *classdata;

  if (!path && !curnk) return(NULL);

  nkofs = trav_path( hdesc, curnk, path, TRAV_PATH_WANT_NK );

  if(!nkofs) 
  {
    printf("get_class: Key <%s> not found\n",path);
    return(NULL);
  }
  nkofs += 4;
  key = (struct nk_key *)(hdesc->buffer + nkofs);

  clen = key->len_classnam;
  if (!clen)
  {
    printf("get_class: Key has no class data.\n");
    return(NULL);
  }

  dofs = key->ofs_classnam;
  classdata = (void *)(hdesc->buffer + dofs + 0x1004);
  
#if 0
  printf("get_class: len_classnam = %d\n",clen);
  printf("get_class: ofs_classnam = 0x%x\n",dofs);
#endif

  ALLOC( (char*)data, sizeof(struct keyval) + clen, 1 );
  data->len = clen;
  memcpy(&data->data, classdata, clen);
  return(data);
}


/* Write to registry value.
 * If same size as existing, copy back in place to avoid changing too much
 * otherwise allocate new dataspace, then free the old
 * Thus enough space to hold both new and old data is needed
 * Pass inn buffer with data len as first DWORD (as routines above)
 * returns: 0 - error, len - OK (len of data)
 */

int 
put_buf2val (
	IN struct hive *hdesc,
	IN struct keyval *kv,
	IN int vofs,
	IN char *path, // ��ֵ������,����ȫ·��,���Ǹ��Լ�������
	IN int type
	)
{
	int l;
	void *keydataptr;
	
	if (!kv) return(0);
	l = get_val_len(hdesc, vofs, path);
	if (l == -1) return(0);  /* error */
	if (kv->len != l) 
	{  /* Realloc data block if not same size as existing */
		if (!alloc_val_data(hdesc, vofs, path, kv->len)) 
		{
			printf("put_buf2val: %s : alloc_val_data failed!\n",path);
			return(0);
		}
	}
	
	keydataptr = get_val_data(hdesc, vofs, path, type);
	if (!keydataptr) return(0); // error 
	
	memcpy(keydataptr, (CHAR*)kv->data, kv->len);
	
	hdesc->state |= HMODE_DIRTY;
	
	return(kv->len);
}


int 
put_buf2val_sz (
	IN struct hive *hdesc,
	IN struct keyvala *kv,
	IN int vofs,
	IN char *path, // ��ֵ������,����ȫ·��,���Ǹ��Լ�������
	IN int type
	)
{
	int l;
	void *keydataptr;
	
	if (!kv) return(0);
	l = get_val_len(hdesc, vofs, path);
	if (l == -1) return(0);  /* error */
	if (kv->len != l) 
	{  /* Realloc data block if not same size as existing */
		if (!alloc_val_data(hdesc, vofs, path, kv->len)) 
		{
			printf("put_buf2val: %s : alloc_val_data failed!\n",path);
			return(0);
		}
	}
	
	keydataptr = get_val_data(hdesc, vofs, path, type);
	if (!keydataptr) return(0); // error 
	
	memcpy(keydataptr, kv->data, kv->len);
	
	hdesc->state |= HMODE_DIRTY;
	
	return(kv->len);
}


/* And, yer basic DWORD write */

int 
put_dword (
	IN struct hive *hdesc, 
	IN int vofs, 
	IN char *path,
	IN char* szdword
	)
{
  struct keyval *kr;
  int r;

  ALLOC( (char*)kr, 1, sizeof(int) + sizeof(int) );
  
  kr->len = sizeof(int);
  (int)kr->data = (int)szdword ;

  r = put_buf2val(hdesc, kr, vofs, path, REG_DWORD);

  FREE(kr);

  return(r);
}


/* ================================================================ */

/* Hive control (load/save/close) etc */

void 
closeHive (
	IN struct hive *hdesc
	)
{

  printf("closing hive %s\n",hdesc->filename);
  if (hdesc->state & HMODE_OPEN)
  {
    _close(hdesc->filedesc);
  }
  FREE(hdesc->filename);
  FREE(hdesc->buffer);
  FREE(hdesc);

}

/* Write the hive back to disk (only if dirty & not readonly */
int 
writeHive (
	IN struct hive *hdesc
	)
{
  int len;

  if (hdesc->state & HMODE_RO)
	  return(0);
  if ( !(hdesc->state & HMODE_DIRTY))
	  return(0);

  if ( !(hdesc->state & HMODE_OPEN)) 
  { /* File has been closed */
    if (!(hdesc->filedesc = open(hdesc->filename,O_RDWR))) 
	{
      fprintf(stderr,"writeHive: open(%s) failed: %s, FILE NOT WRITTEN!\n",hdesc->filename,strerror(errno));
      return(1);
    }
    hdesc->state |= HMODE_OPEN;
  }  
  /* Seek back to begginning of file (in case it's already open) */
  _lseek(hdesc->filedesc, 0, SEEK_SET);

  len = write(hdesc->filedesc, hdesc->buffer, hdesc->size);
  if (len != hdesc->size)
  {
    fprintf(stderr,"writeHive: write of %s failed: %s.\n",hdesc->filename,strerror(errno));
    return(1);
  }

  hdesc->state &= (~HMODE_DIRTY);
  return(0);
}

struct hive *
openHive (
	IN char *filename, 
	IN int mode
	)
{

  struct hive *hdesc;
  int fmode,r,vofs;
  struct stat sbuf;
  unsigned long pofs;
  /* off_t l; */
  char *c;
  struct hbin_page *p;
  struct regf_header *hdr;
  struct nk_key *nk;
  struct ri_key *rikey;
  int verbose = (mode & HMODE_VERBOSE);

  CREATE(hdesc,struct hive,1);

  hdesc->filename = str_dup(filename);
  hdesc->state = 0;
  hdesc->size = 0;
  hdesc->buffer = NULL;

  if ( (mode & HMODE_RO) ) 
  {
    fmode = O_RDONLY;
  } 
  else 
  {
    fmode = O_RDWR;
  }
  hdesc->filedesc = open(hdesc->filename,fmode);
  if (hdesc->filedesc < 0)
  {
    fprintf(stderr,"openHive(%s) failed: %s, trying read-only\n",hdesc->filename,strerror(errno));
    fmode = O_RDONLY;
    mode |= HMODE_RO;
    hdesc->filedesc = open(hdesc->filename,fmode);
    if (hdesc->filedesc < 0) 
	{
      fprintf(stderr,"openHive(%s) in fallback RO-mode failed: %s\n",hdesc->filename,strerror(errno));
      closeHive(hdesc);
      return(NULL);
    }
  }


  if ( fstat(hdesc->filedesc,&sbuf) )
  {
    perror("stat()");
    exit(1);
  }

  hdesc->size = sbuf.st_size;
  hdesc->state = mode | HMODE_OPEN;
  /*  fprintf(stderr,"hiveOpen(%s) successful\n",hdesc->filename); */
  
  /* Read the whole file */

  ALLOC(hdesc->buffer,1,hdesc->size);

  r = read(hdesc->filedesc,hdesc->buffer,hdesc->size);
  if (r < hdesc->size)
  {
    fprintf(stderr,"Could not read file, got %d bytes while expecting %d\n",
	    r, hdesc->size);
    closeHive(hdesc);
    return(NULL);
  }

  /* Now run through file, tallying all pages */
  /* NOTE/KLUDGE: Assume first page starts at offset 0x1000 */

   pofs = 0x1000;

   hdr = (struct regf_header *)hdesc->buffer;
   if (hdr->id != 0x66676572) 
   {
     printf("openHive(%s): File does not seem to be a registry hive!\n",filename);
     return(hdesc);
   }
   printf("Hive's name (from header): <");
   for (c = hdr->name; *c && (c < hdr->name + 64); c += 2) putchar(*c);

   hdesc->rootofs = hdr->ofs_rootkey + 0x1000;
   printf(">\nROOT KEY at offset: 0x%06x * ",hdesc->rootofs);

   /* Cache the roots subkey index type (li,lf,lh) so we can use the correct
    * one when creating the first subkey in a key */
   
   nk = (struct nk_key *)(hdesc->buffer + hdesc->rootofs + 4);
   if (nk->id == 0x6b6e) 
   {
     rikey = (struct ri_key *)(hdesc->buffer + nk->ofs_lf + 0x1004);
     hdesc->nkindextype = rikey->id;
     if (hdesc->nkindextype == 0x6972) 
	 {  /* Gee, big root, must check indirectly */
       printf("load_hive: DEBUG: BIG ROOT!\n");
       rikey = (struct ri_key *)(hdesc->buffer + rikey->hash[0].ofs_li + 0x1004);
       hdesc->nkindextype = rikey->id;
     }
     if (hdesc->nkindextype != 0x666c &&
		 hdesc->nkindextype != 0x686c &&
		 hdesc->nkindextype != 0x696c) 
	 {
       hdesc->nkindextype = 0x666c;
     }

     printf("Subkey indexing type is: %04x <%c%c>\n",
	    hdesc->nkindextype,
	    hdesc->nkindextype & 0xff,
	    hdesc->nkindextype >> 8);
   } 
   else 
   {
     printf("load_hive: WARNING: ROOT key does not seem to be a key! (not type == nk)\n");
   }



   while ( (int)pofs < hdesc->size) 
   {
#ifdef LOAD_DEBUG
          if (verbose) hexdump(hdesc->buffer,pofs,pofs+0x20,1);
#endif
     p = (struct hbin_page *)(hdesc->buffer + pofs);
     if (p->id != 0x6E696268) 
	 {
       printf("Page at 0x%lx is not 'hbin', assuming file contains garbage at end\n",pofs);
       break;
     }
     hdesc->pages++;
#ifdef LOAD_DEBUG
     if (verbose) 
		 printf("\n###### Page at 0x%0lx has size 0x%0lx, next at 0x%0lx ######\n",pofs,p->len_page,p->ofs_next);
#endif
     if (p->ofs_next == 0) 
	 {
#ifdef LOAD_DEBUG
       if (verbose) printf("openhive debug: bailing out.. pagesize zero!\n");
#endif
       return(hdesc);
     }
#if 0
     if (p->len_page != p->ofs_next) 
	 {
#ifdef LOAD_DEBUG
       if (verbose) 
		   printf("openhive debug: len & ofs not same. HASTA!\n");
#endif
       exit(0);
     }
#endif


     vofs = pofs + 0x20; /* Skip page header */
#if 1
     while ( (int)(vofs-pofs) < (int)(p->ofs_next) ) 
	 {
       vofs += parse_block(hdesc,vofs,verbose);

     }
#endif
     pofs += p->ofs_next;
   }
   printf("\nFile size %d [%x] bytes, containing %d pages (+ 1 headerpage)\n",hdesc->size,hdesc->size, hdesc->pages);
   printf("Used for data: %d/%d blocks/bytes, unused: %d/%d blocks/bytes.\n",
	  hdesc->useblk,hdesc->usetot,hdesc->unuseblk,hdesc->unusetot);
  

   /* So, let's guess what kind of hive this is, based on keys in its root */

   hdesc->type = HTYPE_UNKNOWN;
   if (trav_path(hdesc, 0, "\\SAM", TRAV_PATH_WANT_NK)) 
	   hdesc->type = HTYPE_SAM;
   else if (trav_path(hdesc, 0, "\\ControlSet", TRAV_PATH_WANT_NK)) 
	   hdesc->type = HTYPE_SYSTEM;
   else if (trav_path(hdesc, 0, "\\Policy", TRAV_PATH_WANT_NK)) 
	   hdesc->type = HTYPE_SECURITY;
   else if (trav_path(hdesc, 0, "\\Microsoft", TRAV_PATH_WANT_NK)) 
	   hdesc->type = HTYPE_SOFTWARE;

  return(hdesc);

}


//////////////////////////////////////////////////////////////////////////
