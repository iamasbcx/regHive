/***************************************************************************************
* AUTHOR : sudami [sudami@163.com]
* TIME   : 2009/02/05 
* MODULE : List_SubKey_and_Value.cpp 
*
* Description:
*   
*   ��ģ�鸺����ʾ������ "�Ӽ��б�" & "����"                     
*
***
* Copyright (c) 2008 - 2010 sudami.
* Freely distributable in source or binary for noncommercial purposes.
* TAKE IT EASY,JUST FOR FUN.
*
****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include <Shlobj.h>
#include <map>

#include "ntreg.h"
#include "InitHive.h"
#include "regedit.h"

#include "List_SubKey_and_Value.h"

//////////////////////////////////////////////////////////////////////////

extern HWND			ListW, TreeW	;
extern PHIVE_UNION	g_phive_union	;
extern struct hive	*g_pCurrentHive	;


/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                �оٸ�����һϵ��ֵ ListValue               +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


int 
GetUserName (
	IN char* szOutUserName
	)
{
	if ( szOutUserName == NULL ) { return 0 ; }
	*szOutUserName = 0;

	HANDLE hProcess = GetCurrentProcess();
	if(!hProcess) {
		return 0;
	}
	
	HANDLE hToken;
	if( !OpenProcessToken(hProcess, TOKEN_QUERY, &hToken) || !hToken ){
		CloseHandle(hProcess);
		return 0;
	}
	
	DWORD dwTemp = 0;
	char tagTokenInfoBuf[256] = {0};
	PTOKEN_USER tagTokenInfo = (PTOKEN_USER)tagTokenInfoBuf;
	if( !GetTokenInformation( hToken, TokenUser, tagTokenInfoBuf, sizeof(tagTokenInfoBuf),\
		&dwTemp ) ) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return 0;
	}
	
	typedef BOOL (WINAPI* PtrConvertSidToStringSid)(
		PSID Sid,
		LPTSTR* StringSid
		);
	
	
	PtrConvertSidToStringSid dwPtr = (PtrConvertSidToStringSid)GetProcAddress( 
		LoadLibrary("Advapi32.dll"), "ConvertSidToStringSidA" );
	
	LPTSTR MySid = NULL;
	dwPtr( tagTokenInfo->User.Sid, (LPTSTR*)&MySid );
	
	strcpy( szOutUserName, (char*)MySid );
//	printf("sudami's PC Name:\n%s\n", MySid);
//	getchar ();
	LocalFree( (HLOCAL)MySid );
	
	CloseHandle(hToken);
	CloseHandle(hProcess);
	
	return 0;
}


int
check_is_hive_open (
	IN struct _phive_union *pHive_nion,
	IN char *s,
	IN char *subKeyPath,
	IN int checkType // 0 ��ListValue; 1 ��ListSubKey
	)
{
	if ( s == NULL || *s == NULL ) { return ReturnType_ERRO ; }
	if( NULL != subKeyPath ) { *subKeyPath = 0 ; }
	
	if ( (0 == stricmp( s, "HKEY_LOCAL_MACHINE" ))
		|| (0 == stricmp( s, "HKEY_USERS" ))
		|| (0 != strstr( s, "HKEY_CURRENT_CONFIG" ))
		|| (0 != strstr( s, "HKEY_LOCAL_MACHINE\\HARDWARE" ))
	   )
	{
		// �����򿪵���һ�������,û��hive��֮��Ӧ
		return ReturnType_IsRootKey ;
	}


	if ( (0 != strstr( s, "HKEY_CLASSES_ROOT" ))
		|| (0 != strstr( s, "HKEY_LOCAL_MACHINE\\SOFTWARE" ))
	   )
	{
		// HKEY_LOCAL_MACHINE\SOFTWARE\Classes �� HKEY_CLASSES_ROOT ������
		// �Ȱ����������hive�ĺ���ʹ�õ��Ӽ�ȫ·��. eg. \\360Safe
		if ( 0 != strstr( s, "HKEY_CLASSES_ROOT" ) ) {
			char* ptr = strchr( s, '\\' );
			if ( ptr++ ) {
				sprintf( subKeyPath, "\\Classes\\%s", ptr );
			} else {
				strcpy( subKeyPath, "\\Classes" );
			}

		} else if( 0 != strstr( s, "HKEY_LOCAL_MACHINE\\SOFTWARE" ) ) {
			char* ptr = strchr( s, '\\' );
			if ( ptr++ )
			{
				ptr = strchr( ptr, '\\' );
				if ( ptr ) {
					sprintf( subKeyPath, "%s", ptr );
				}
			}
		}

		if ( pHive_nion->pHive_HKLM_SOFTWARE ) 
		{
			// �����Ѵ򿪴�hive
			g_pCurrentHive = pHive_nion->pHive_HKLM_SOFTWARE ;
			return ReturnType_OK ;
		} 
		
		pHive_nion->pHive_HKLM_SOFTWARE = My_openHive( HIVE_SOFTWARE, HMODE_RW );
		g_pCurrentHive = pHive_nion->pHive_HKLM_SOFTWARE ;
		if ( NULL == g_pCurrentHive ) { return ReturnType_ERRO ; }
		return ReturnType_OK ;
	}
	

	if ( 0 != strstr( s, "HKEY_CURRENT_USER" ) )
	{
		// ����Ҫ�о� HKEY_CURRENT_USER\\xx �ļ�ֵ.��Ҫ�ȴ�
		// HKEY_USERS\S-1-5-21-1214440339-1078145449-1343024091-500
		// ��ֱ�Ӵ򿪵�ǰ�û���CLSID��Ӧ��Hive����

		// �Ȱ����������hive�ĺ���ʹ�õ��Ӽ�ȫ·��. eg. \\360Safe
		char* ptr = strchr( s, '\\' );
		if ( ptr ) {
			sprintf( subKeyPath, "%s", ptr );
		}

		char szCurrentUserName[256] = "";
		char szTmp[256] = "";
		GetUserName( szCurrentUserName );
		
		if ( pHive_nion->pHive_HKCU && 0 != strstr( pHive_nion->pHive_HKCU->filename, szCurrentUserName ) ) 
		{
			// �����Ѵ򿪴�hive
			g_pCurrentHive = pHive_nion->pHive_HKCU ;
			return ReturnType_OK ;
		}

		sprintf( szTmp, "\\REGISTRY\\USER\\%s", szCurrentUserName ); 
		pHive_nion->pHive_HKCU = My_openHive( szTmp, HMODE_RW );
		g_pCurrentHive = pHive_nion->pHive_HKCU ;
		if ( NULL == g_pCurrentHive ) { return ReturnType_ERRO ; }
		return ReturnType_OK ;
	}


	if ( 0 != strstr( s, "HKEY_LOCAL_MACHINE\\SAM" ) )
	{
		// �Ȱ����������hive�ĺ���ʹ�õ��Ӽ�ȫ·��. eg. \\360Safe
		char* ptr = strchr( s, '\\' );
		if ( ptr++ )
		{
			ptr = strchr( ptr, '\\' );
			if ( ptr ) {
				sprintf( subKeyPath, "%s", ptr );
			}
		}

		if ( pHive_nion->pHive_HKLM_SAM && 0 != strstr( pHive_nion->pHive_HKLM_SAM->filename, HIVE_SAM ) ) 
		{
			// �����Ѵ򿪴�hive
			g_pCurrentHive = pHive_nion->pHive_HKLM_SAM ;
			return ReturnType_OK ;
		}
		
		pHive_nion->pHive_HKLM_SAM = My_openHive( HIVE_SAM, HMODE_RW );
		g_pCurrentHive = pHive_nion->pHive_HKLM_SAM ;
		if ( NULL == g_pCurrentHive ) { return ReturnType_ERRO ; }
		return ReturnType_OK ;
	}


	if ( 0 != strstr( s, "HKEY_LOCAL_MACHINE\\SECURITY" ) )
	{
		// �Ȱ����������hive�ĺ���ʹ�õ��Ӽ�ȫ·��. eg. \\360Safe
		char* ptr = strchr( s, '\\' );
		if ( ptr++ )
		{
			ptr = strchr( ptr, '\\' );
			if ( ptr ) {
				sprintf( subKeyPath, "%s", ptr );
			}
		}
		
		if ( pHive_nion->pHive_HKLM_SECURITY && 0 != strstr( pHive_nion->pHive_HKLM_SECURITY->filename, HIVE_SECURITY ) ) 
		{
			// �����Ѵ򿪴�hive
			g_pCurrentHive = pHive_nion->pHive_HKLM_SECURITY ;
			return ReturnType_OK ;
		}
		
		pHive_nion->pHive_HKLM_SECURITY = My_openHive( HIVE_SECURITY, HMODE_RW );
		g_pCurrentHive = pHive_nion->pHive_HKLM_SECURITY ;
		if ( NULL == g_pCurrentHive ) { return ReturnType_ERRO ; }
		return ReturnType_OK ;
	}

	
	if ( 0 != strstr( s, "HKEY_LOCAL_MACHINE\\SYSTEM" )
	//	|| (0 != strstr( s, "HKEY_CURRENT_CONFIG" ))
	   )
	{
/*++
  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Hardware Profiles\Current �� HKEY_CURRENT_CONFIG ������
��Ҫ�� HIVE_SYSTEM,���������Ӧ���Ӽ�ȫ·��
--*/
		// �Ȱ����������hive�ĺ���ʹ�õ��Ӽ�ȫ·��. eg. \\360Safe
		char* ptr = strchr( s, '\\' );

		if( 0 != strstr( s, "HKEY_CURRENT_CONFIG" ) ) {
			// �������hive�ļ���û��,Ҳ��һ��Link. ֻ���ó���API������
// 			if ( ptr ) {
// 				sprintf( subKeyPath, "\\CurrentControlSet\\Hardware Profiles\\Current%s", ptr );
// 			} else {
// 				strcpy( subKeyPath, "\\CurrentControlSet\\Hardware Profiles\\Current" );
// 			}
			
		} else {
			if ( ptr++ )
			{
				ptr = strchr( ptr, '\\' );
				if ( ptr ) {
					sprintf( subKeyPath, "%s", ptr );
				}
			}
		}

		if ( pHive_nion->pHive_HKLM_SYSTEM && 0 != strstr( pHive_nion->pHive_HKLM_SYSTEM->filename, HIVE_SYSTEM ) ) 
		{
			// �����Ѵ򿪴�hive
			g_pCurrentHive = pHive_nion->pHive_HKLM_SYSTEM ;
			return ReturnType_OK ;
		}
		
		pHive_nion->pHive_HKLM_SYSTEM = My_openHive( HIVE_SYSTEM, HMODE_RW );
		g_pCurrentHive = pHive_nion->pHive_HKLM_SYSTEM ;
		if ( NULL == g_pCurrentHive ) { return ReturnType_ERRO ; }
		return ReturnType_OK ;
	}
	
	if ( 0 != strstr( s, "HKEY_USERS" ) )
	{
		// �Ȱ����������hive�ĺ���ʹ�õ��Ӽ�ȫ·��. eg. \\360Safe
		char* ptrXX = strchr( s, '\\' );
		if ( ptrXX++ )
		{
			ptrXX = strchr( ptrXX, '\\' );
			if ( ptrXX ) {
				sprintf( subKeyPath, "%s", ptrXX );
			}
		}

		// ����Ҫ�о� HKEY_USERS\\xx �ļ�ֵ.
		char* ptr = strchr( s, '\\' ) ;
		if ( NULL == ptr )
		{
			// �����Ǵ򿪵�HKEY_USERS�������
			return ReturnType_IsRootKey ;
		}

		ptr++ ;
		char* ptrTmp = strchr( ptr, '\\' );
		if ( ptrTmp )
		{
			int nlength = (int)( ptrTmp - ptr );
			*( ptr + nlength ) = '\0';
		}

		char szTmp[256] = "";
		sprintf( szTmp, "\\REGISTRY\\USER\\%s", ptr ); 

		int nxx = 0;
		for( nxx = 0; pHive_nion->pHive_OTHER_USER[nxx]; nxx++ ) 
		{
			if ( 0 == stricmp( pHive_nion->pHive_OTHER_USER[nxx]->filename, szTmp ) )
			{
				// Ҫ�򿪵ļ�ֵ��hive�Ѿ���map����,ֱ��ȡ������
				g_pCurrentHive = pHive_nion->pHive_OTHER_USER[nxx] ;
				return ReturnType_OK ;
			}
		}
		
		// û���ҵ�,��Ҫmapһ��hive���ڴ�
		for( nxx = 0; ; nxx++ ) 
		{
			// �ҵ�һ���յĽṹ
			if ( NULL == pHive_nion->pHive_OTHER_USER[nxx] ) { break; }
			if ( 10 == nxx ) { return ReturnType_ERRO ; }
		}

		pHive_nion->pHive_OTHER_USER[nxx] = My_openHive( szTmp, HMODE_RW );
		g_pCurrentHive = pHive_nion->pHive_OTHER_USER[nxx] ;
		if ( NULL == g_pCurrentHive ) { return ReturnType_ERRO ; }

		return ReturnType_OK ;
	}

	if ( 0 == strstr( s, "\\" ) ) { return ReturnType_IsRootKey ; }
	return ReturnType_ERRO ;
}


char*
GetStringEnd (
	IN char* ptr
	)
{
	do 
	{
		if ( '\0' == *ptr ) { break ; }
	} while ( ptr++ );

	return (char*)ptr ;
}


char* 
hexdump_mfc (
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
  �����߱����ͷ��ڴ�
    
Arguments:
  hbuf - ������
  start - �������������ʼλ��
  stop - �������������ֹλ��
  ascii - �Ƿ������Ӧ��ascii��

--*/
{
	char c ;
	int diff, i ;
	char* ptr = (char*) HEAP_ALLOC( 0x1000*4 ) ;
	char* ptrStart = ptr; 
	
	while (start < stop ) 
	{
		diff = stop - start ;
		if (diff > 16) { diff = 16; }
		
	//	sprintf( ptr, ":%05X  ", start );	ptr = GetStringEnd(ptr) ;
		for (i = 0; i < diff; i++) // ��ӡ��ַ %02X
		{	
			c = (char)*( hbuf + start + i );
			sprintf( ptr, "%02X ", (unsigned char)c ); ptr = GetStringEnd(ptr) ;
		}
		
		if (ascii)
		{
			for (i = diff; i < 16; i++) 
			{ 
				sprintf( ptr, "%s" ,"   "); ptr = GetStringEnd(ptr) ;
			}
			
			for (i = 0; i < diff; i++) // ��ӡ��ַ�е����� %c
			{
				c = (char)*(hbuf+start+i); 
				sprintf( ptr, "%c", isprint(c) ? c : '.'); ptr = GetStringEnd(ptr) ;
			}  
		}
		
		sprintf( ptr, "%s","\n" ); ptr = GetStringEnd(ptr) ;
		start += 16;
	}
	
	return (char* )ptrStart ;
}


char* 
getValueData_mfc (
	IN struct hive *hdesc,
	IN int nkofs, 
	IN char *path
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  �õ���ֵ������[����ӡ] "���������Ҫ�ͷ��ڴ�"

Arguments:
  path - ValueName. ��������

--*/
{    
	void *data ;
	int len, type ;
	char *string, *ptrNeedFree ;
	
	type = get_val_type(hdesc, nkofs, path);
	if (type == -1) 
	{
		printf("No such value <%s>\n",path);
		return NULL;
	}
	
	len = get_val_len(hdesc, nkofs, path);
	if (!len) 
	{
		printf("Value <%s> has zero length\n",path);
		return NULL;
	}
	
	data = (void *)get_val_data(hdesc, nkofs, path, 0);
	if (!data) { return NULL; }
	
	switch (type)
	{
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_MULTI_SZ:
		// ����ֵת����asni,����Ƚ�
		string = (char*) change_to_ansi( (char *)data, len ); 
		ptrNeedFree = (char*) HEAP_ALLOC( 0x1000*4 );
		strcpy( ptrNeedFree, string );
		return (char*)ptrNeedFree ;
		
	case REG_BINARY:
		ptrNeedFree = hexdump_mfc( (char *)data, 0, len, 1 );
		return ptrNeedFree;

	case REG_DWORD:

		ptrNeedFree = (char*) HEAP_ALLOC( 0x1000*4 );
		GetValueDataString( (char *)data, ptrNeedFree, len, type );
		return (char*)ptrNeedFree ;

	default:
		return NULL;
	}
}


void 
ListValues_hive_intenal (
	IN struct hive *hdesc, 
	IN char* szSubKeyPath,
	IN HWND ListW
	)
/*++

Author: sudami [sudami@163.com]
Time  : 2009/01/28 

Routine Description:
  ������ǰ���ļ�ֵ

Arguments:
  NormalRegPath - Ϊ""Ĭ��,Ϊ"\\"��ʾ����,Ϊ��������ָ���ļ�(eg:360\\XGB\\sudami)

--*/
{
	struct nk_key *key;
	int nkofs, I;
	struct vex_data vex;
	int count = 0 ;
	LVITEM item;
	char valb[2046] = "";
	
	nkofs = trav_path( hdesc, hdesc->rootofs + 4, szSubKeyPath, TRAV_PATH_WANT_NK );
	if ( !nkofs ) 
	{
		printf("nk_ls: Key <%s> not found\n",szSubKeyPath);
		return;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e)
	{
		printf("Error: Not a 'nk' node!\n");
		debugit( hdesc->buffer, hdesc->size );
	}

	ListView_DeleteAllItems( ListW );
	item.mask = LVIF_TEXT | LVIF_IMAGE /*| LVIF_PARAM*/, item.iItem=0, item.iSubItem=0;
	item.stateMask=0, item.lParam=0;
	
	printf("Node has %ld subkeys and %ld values",key->no_subkeys,key->no_values);
	if (key->len_classnam) { printf(", and class-data of %d bytes",key->len_classnam); }
	printf("\n");
	
	if (key->no_values) 
	{
		count = 0;
		printf("-------- SelfValue Lists --------\n");
		while ( (ex_next_v( hdesc, nkofs, &count, &vex ) > 0) ) 
		{
			item.pszText = vex.name; // �Ӽ�������
			item.iImage = ValueTypeIcon( vex.type );
			I = ListView_InsertItem( ListW, &item );
			if (I >= 0) // д���ֵ������
			{
				item.pszText = getValueData_mfc( hdesc, nkofs, vex.name ); // "���������Ҫ�ͷ��ڴ�"
				item.iItem=I;
				item.iSubItem=1;
				ListView_SetItem(ListW,&item);
				item.iSubItem=0;

				HEAP_FREE( item.pszText );
			}
			
			FREE( vex.name );
		}
	}

	return ;
}


int 
ListValues_hive (
	IN HWND hwnd,
	IN char *s
	)
{
	char szSubKeyPath[2048] = "" ;

	int nRtnType = check_is_hive_open( &g_phive_union, s, szSubKeyPath, 0 );
 	if ( ReturnType_OK != nRtnType ) { return 1; }
	OutputDebugString((char*)g_pCurrentHive->filename);

	// ͨ������hive��ö�ٸü�������ֵ
	ListValues_hive_intenal( g_pCurrentHive, szSubKeyPath, ListW );
	return 0;
}



/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                �оٸ�����һϵ�Ӽ� ListSubKey              +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--


int
ListSubKeys_hive (
	IN HTREEITEM hItem,
	TVINSERTSTRUCT tvins,
	IN char *s
	)
{
	char szSubKeyPath[2048] = "" ;

	int nRtnType = check_is_hive_open( &g_phive_union, s, szSubKeyPath, 1 );
	if ( ReturnType_OK != nRtnType ) { return nRtnType ; }
	OutputDebugString((char*)g_pCurrentHive->filename);

	// ͨ������hive��ö�ٸü��������Ӽ�
	nk_ls_ListSubKeys( g_pCurrentHive, szSubKeyPath, hItem, tvins );

	return ReturnType_OK ;
}


VOID
nk_ls_ListSubKeys (
	IN struct hive *hdesc, 
	IN char* szSubKeyPath,
	IN HTREEITEM hItem,
	IN TVINSERTSTRUCT tvins
	)
{
	char szTmpxx[512] ;
	struct nk_key *key ;
	struct ex_data ex ;
	int nkofs = 0, count = 0, countri = 0;
	HTREEITEM hdel = TreeView_GetChild( TreeW, hItem );

	szTmpxx[0] = 0 ;
	if ( hdel )
	{
		tvins.item.mask			= TVIF_TEXT ;
		tvins.item.pszText		= szTmpxx		;
		tvins.item.cchTextMax	= 512		;
		tvins.item.hItem		= hdel		;

		TreeView_GetItem( TreeW, &tvins.item );
	}
	
	tvins.hParent			= hItem ;
	tvins.hInsertAfter		= TVI_LAST ;
	tvins.item.mask			= TVIF_CHILDREN | TVIF_STATE | TVIF_TEXT ;
	tvins.item.state		= 0 ; 
	tvins.item.stateMask	= 0xFFFF ;
	tvins.item.cChildren	= 1 ;

	nkofs = trav_path( hdesc, hdesc->rootofs + 4, szSubKeyPath, TRAV_PATH_WANT_NK );
	if ( !nkofs ) 
	{
		printf("nk_ls: Key <%s> not found\n", szSubKeyPath);
		return;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e)
	{
		printf("Error: Not a 'nk' node!\n");
		debugit( hdesc->buffer, hdesc->size );
	}
		
	if (key->no_subkeys)
	{
		count = 0;	
		printf("-------- SubKey Lists --------\n");
		ex.name = (char *)HEAP_ALLOC( 0x1000 ); // ��������,�ȷ������ڴ�,��ÿ�α������������
	
		while( (ex_next_n( hdesc, nkofs, &count, &countri, &ex ) > 0) ) 
		{
			// �鿴���Ӽ��Ƿ�����Ӽ�,���л�ȡ�Ӽ�����
			tvins.item.cChildren = get_Key_s_subkey_Counts( 
				hdesc, "", ex.nkoffs + 4, TRAV_PATH_WANT_NK );

			// ���Ϸ���,������֮
			if ( strcmp( szTmpxx, ex.name ) ) {
				tvins.item.pszText		= ex.name ;
				TreeView_InsertItem( TreeW, &tvins );
			}
		}

		HEAP_FREE( ex.name );

		TreeView_SortChildren( TreeW, hItem, 0 );
	}

	return ;
}


int
get_Key_s_subkey_Counts (
	IN struct hive *hdesc, 
	IN char *path, 
	IN int vofs,
	IN int type
	)
{
	struct nk_key *key;
	int nkofs;
	int count = 0, countri = 0, plen = 0 ;
	
	nkofs = trav_path( hdesc, vofs, path, TRAV_PATH_WANT_NK );
	
	if(!nkofs) 
	{
		printf("nk_ls: Key <%s> not found\n",path);
		return 0;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	
	if (key->id != 0x6b6e)
	{
		printf("Error: Not a 'nk' node!\n");
		debugit( hdesc->buffer, hdesc->size );
		return 0;
	}
	
	printf("Node has %ld subkeys and %ld values",key->no_subkeys,key->no_values);

	return (int)key->no_subkeys ;
}

//////////////////////////////////////////////////////////////////////////