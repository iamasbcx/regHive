/***************************************************************************************
* AUTHOR : sudami [sudami@163.com]
* TIME   : 2009/02/06
* MODULE : Refresh_subtree.cpp 
*
* Description:
*   
*   ��ģ�鸺��ˢ�����οؼ�����������                     
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
#include "Refresh_subtree.h"

//////////////////////////////////////////////////////////////////////////

extern HWND			ListW, TreeW	;
extern PHIVE_UNION	g_phive_union	;
extern struct hive	*g_pCurrentHive	;

BOOL g_cantRefreshHive = FALSE ;
HKEY g_hk ;
char* g_szTmp_for_refresh = NULL;

/////////////////////////////////////////////////////////////////         --          --     
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//     --     -      -     -- 
//+                                                           +//     --      -   -       -- 
//+                  ˢ�����οؼ�����������	                  +//      --       -        --  
//+                                                           +//       -     sudami     -   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//        --            --    
/////////////////////////////////////////////////////////////////          --        --  
//                                                                           --    --
//		    																	--

BOOL 
RefreshSubtree_Total (
	IN HTREEITEM hfc, 
	IN HKEY hk,
	IN const char *kname
	) 
{
	char szxxname[4096] = "" ;

	g_hk = hk ;
	if ( (0 == stricmp( kname, "HKEY_CLASSES_ROOT" ))
		|| 0 == stricmp( kname, "HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes" )
	   )
	{
		//
		// ���ǵ�ˢ����ô���һ������,�ý���hive�ķ�ʽ��ʱ̫��,�ʻ��س���API�ķ���
		//
// 		g_pCurrentHive = g_phive_union.pHive_HKLM_SOFTWARE ;
// 		if ( !g_phive_union.pHive_HKLM_SOFTWARE ) { 
// 			g_phive_union.pHive_HKLM_SOFTWARE = My_openHive( HIVE_SOFTWARE, HMODE_RW );
// 		} 
// 	
// 		sprintf( szxxname, "%s", "HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes" );
// 		kname = szxxname;
// 
// 		RefreshSubtree_hive( hfc, szxxname );
// 		return g_cantRefreshHive ;

		RefreshSubtree_normal( hfc, hk, kname );
		return TRUE ;
	}

	//
	// �ǴӸ��� HKEY_LOCAL_MACHINE �� HKEY_USERS ��ˢ�µ�. �����ڴ汸������,
	//
	g_szTmp_for_refresh = (char *)HEAP_ALLOC( 0x1000 ); 

	RefreshSubtree_mix( hfc, hk, kname );

	HEAP_FREE( g_szTmp_for_refresh ); // �ͷ��ڴ�

	return g_cantRefreshHive ;
}


int 
RefreshSubtree_hive (
	IN HTREEITEM hti, 
	IN HKEY hk,
	IN const char *kname
	) 
{
	char buf[4096] ; //tmp?
	int n, knl, nkofs ;
	TVITEM tvi, tvi1;
	HTREEITEM hfc, hfcold;
	mystrhash_hive had_keys;
	char szSubKeyPath[2048] = "" ;

	knl = strlen( kname );
	hfc = TreeView_GetChild( TreeW, hti );

	// ��ͨ�����οؼ�,�õ��ɵ�Ԫ��,������hash����
	for( n = 0; hfc; n++ ) 
	{
		tvi.mask		= TVIF_HANDLE | TVIF_TEXT | TVIF_STATE ;
		tvi.hItem		= hfc ;
		tvi.stateMask	= TVIS_EXPANDED ;
		tvi.pszText		= buf ; 
		tvi.cchTextMax	= 4095 ;

		// ��ȡ��ǰ�Ӽ�������,������tvi.pszTextָ���buf��
		if ( !TreeView_GetItem(TreeW, &tvi) ) { ErrMsgDlgBox("RefreshSubtree"); return 1; }
		
		bool is_deleted = false;
		bool is_exp		= !!( tvi.state & TVIS_EXPANDED ) ;
		char *curname	= (char*)malloc( knl + strlen(buf) + 2 );
		strcpy( curname, kname );
		curname[knl] = '\\'; 
		strcpy( curname + knl + 1, buf );

		bool is_deleted_normalAPI = TRUE;
		HKEY sk ;
		
		// ������ͨAPI�򿪲���
		if ( RegOpenKeyEx( hk, buf, 0, 0, &sk ) == ERROR_SUCCESS 
			|| (RegOpenKeyEx( hk, buf, 0, KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE, &sk ) 
			== ERROR_SUCCESS)
		   ) 
		{
			// ��Ȼ��hive����������Bug,û���ҵ���2���Ӽ�,���ó���API�����ҵ���
			is_deleted_normalAPI = FALSE ;
			
			// ͬ�����뵽hash����ȥ
		}
		
		// ���ü��Ƿ����
		*szSubKeyPath = 0;

		//
		// ��ѽ,���� HKEY_USERS\\�µ��Ӽ�,�ú������޸Ĳ�����(curname)��ֵ. 
		// �������޸�ʱ���ֻ���������δ֪����,�����ڴ��ȱ���һ��.
		//
		memset( g_szTmp_for_refresh, 0, sizeof(g_szTmp_for_refresh) );
		strcpy( g_szTmp_for_refresh, curname ); // ����
		check_is_hive_open( &g_phive_union, curname, szSubKeyPath, 0 );
		nkofs = trav_path( g_pCurrentHive, g_pCurrentHive->rootofs + 4, szSubKeyPath, TRAV_PATH_WANT_NK );
		strcpy( curname, g_szTmp_for_refresh ); // �ָ�

		if ( !nkofs ) 
		{ 
			is_deleted = TRUE ; // û�ҵ������,�������Ѿ���ɾ����.���һ��

			//
			// ����ʱ����, �ڶ� HKEY_CURRENT_USER�����µ��Ӽ�����ˢ��ʱ, ��2���Ӽ�һֱ�Ҳ��� --
			// "SessionInformation" �� "Volatile Environment", ֻ��ˢ����������������,���ǵ�+
			// չ��,�����ҵ�����ȷ��ʾ��,Ŀǰ��Ȼ���������ԭ��. ��������ֻ���ó���API���ֲ�һ��.
			// ��,codeд��һ���㰡. sudami 2009/02/07
			// ԭ����2����������������,��ϵͳ�Դ���regedit.exe����Ҳ���ܽ�����������ɾ������
			//
			if ( FALSE == is_deleted_normalAPI )
			{
				is_deleted = FALSE ;
			}

		} else { // �ü���Ȼ����

			DWORD has_subkeys;
			if ( is_exp ) { // ���ü��Ѿ�չ��,��Ҫ�ݹ����
				has_subkeys = RefreshSubtree_hive( hfc, sk, curname );
			} else {

				// �鿴���Ӽ��Ƿ�����Ӽ�,���л�ȡ�Ӽ�����
				has_subkeys = get_Key_s_subkey_Counts ( 
									g_pCurrentHive, 
									szSubKeyPath, 
									g_pCurrentHive->rootofs + 4, 
									TRAV_PATH_WANT_NK );

				if ( 0 == has_subkeys )	{ has_subkeys = (DWORD)-1; }
			}

			if ( has_subkeys != (DWORD)-1 ) 
			{
				tvi1.mask		= TVIF_HANDLE | TVIF_CHILDREN ;
				tvi1.hItem		= hfc ;
				tvi1.cChildren	= has_subkeys != 0 ;
				TreeView_SetItem( TreeW, &tvi1 ); // �����Ӽ����Ӽ�,������
			}
		} // end -- if( nkofs )

		// ������Reg���, �ر�֮
		if ( FALSE == is_deleted_normalAPI ) { RegCloseKey(sk); }

		// �ҵ�ͷ���˶�û�ҵ����Buffer,�������µ�
		if ( (FALSE == is_deleted) &&  ( had_keys.find(buf) == had_keys.end() ) ) { 
			had_keys.insert( strdup(buf) ); // �Ѳ��ҵ���չ�����Ӽ������뵽hash����
		}
		
		free( curname );
		if ( is_deleted ) { hfcold = hfc ; }
		hfc = TreeView_GetNextSibling( TreeW, hfc );
		if (is_deleted) { TreeView_DeleteItem( TreeW, hfcold ); }
	}

	DWORD count_subkeys = had_keys.size();
	
	// ��ʼˢ��,������Ԫ��,������Ӽ������οؼ���
	check_is_hive_open( &g_phive_union, (char*)kname, szSubKeyPath, 0 );
	RefreshSubtree_hive_intenal ( 
		g_pCurrentHive, 
		hti, 
		(char*)szSubKeyPath, 
		(int*)&count_subkeys, 
		had_keys );

	// ��β����,����hash��
	mystrhash_hive::iterator i = had_keys.begin();
	while(i != had_keys.end()) {
		char *c = (*i);
		i++;
		delete []c;
	}
	had_keys.clear();

	return count_subkeys;
}


VOID
RefreshSubtree_hive_intenal (
	IN struct hive *hdesc, 
	IN HTREEITEM hti, 
	IN char* szFatherPath,
	IN int* count_subkeys,
	IN mystrhash_hive had_keys
	)
{
	int nkofs = 0, count = 0, countri = 0;
	char valb[2046] = "";
	char szSubKeyPath[2048] = "" ;
	TVINSERTSTRUCT tvins;
	struct nk_key *key ;
	struct ex_data ex;

	*szSubKeyPath = 0;
	nkofs = trav_path( hdesc, hdesc->rootofs + 4, szFatherPath, TRAV_PATH_WANT_NK );
	if ( !nkofs ) 
	{
		printf("nk_ls: Key <%s> not found\n",szFatherPath);
		return;
	}
	nkofs += 4;
	
	key = (struct nk_key *)(hdesc->buffer + nkofs);
	if (key->id != 0x6b6e)
	{
		printf("Error: Not a 'nk' node!\n");
		return;
	}

	if (key->no_subkeys)
	{
		count = 0;
		printf("-------- SubKey Lists --------\n");
		ex.name = (char *)HEAP_ALLOC( 0x1000 ); // ��������,�ȷ������ڴ�,��ÿ�α������������

		while( (ex_next_n( hdesc, nkofs, &count, &countri, &ex ) > 0) ) 
		{
			if ( count == 1 ) 
			{
				tvins.hParent		 = hti ;
				tvins.hInsertAfter	 = TVI_SORT ;
				tvins.item.mask		 = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE ;
				tvins.item.state	 = 0 ;
				tvins.item.stateMask = TVIS_EXPANDEDONCE ;
			}
			
			tvins.item.pszText   = ex.name ; // �Ӽ�������
			if ( had_keys.find( ex.name ) == had_keys.end() ) 
			{
				// �ھɵ�hash����û���ҵ�����Ӽ�������,�������²����.�Ͻ��ӵ����οؼ���
				DWORD has_subkeys = get_Key_s_subkey_Counts ( 
					hdesc, 
					"", 
					ex.nkoffs + 4, 
					TRAV_PATH_WANT_NK );
				
				tvins.item.cChildren = has_subkeys != 0;
				TreeView_InsertItem( TreeW, &tvins ); // ����֮~
				++(*count_subkeys);
			}
		}

		HEAP_FREE( ex.name );
	}

	return ;
}


//////////////////////////////////////////////////////////////////////////


typedef hash_set<char*, hash<char*>, str_equal_to> mystrhash_mix;

int 
RefreshSubtree_mix (
	IN HTREEITEM hti, 
	IN HKEY hk, 
	IN const char *kname 
	) 
{
	char buf[4096];//tmp?
	char szSubKeyPath[2048] = "" ;
	int n;
	int knl = strlen(kname);
	TVITEM tvi, tvi1;
	HTREEITEM hfc = TreeView_GetChild(TreeW, hti), hfcold;
	mystrhash_mix had_keys;
	for(n = 0; hfc; n++) 
	{
		HKEY sk;
		tvi.mask=TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
		tvi.hItem=hfc;
		tvi.stateMask = TVIS_EXPANDED;
		tvi.pszText = buf, tvi.cchTextMax = 4095;

		// ��ȡ��ǰ�Ӽ�������,������tvi.pszTextָ���buf��
		if (!TreeView_GetItem(TreeW, &tvi)) {ErrMsgDlgBox("RefreshSubtree"); return 1;}
		
		bool is_exp = !!(tvi.state & TVIS_EXPANDED);
		char *curname = (char*)malloc(knl + strlen(buf) + 2);
		strcpy(curname, kname); curname[knl] = '\\'; strcpy(curname + knl + 1, buf);
		
		bool is_deleted = false;

		if (RegOpenKeyEx(hk, buf, 0, KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE, &sk) == ERROR_SUCCESS) 
		{
			DWORD has_subkeys;
			if (is_exp) 
			{
				if ( (0 == stricmp( curname, "HKEY_LOCAL_MACHINE\\HARDWARE" )) 
					|| (0 == stricmp( curname, "HKEY_CURRENT_CONFIG\\Software" )) 
					|| (0 == stricmp( curname, "HKEY_CURRENT_CONFIG\\System" )) 
					) 
				{
					// �������,������APIö��ˢ��
					has_subkeys = RefreshSubtree_normal(hfc, sk, curname );

				} else {
					// OK! �Ѿ�ö�ٵ���ʵ�ʵ�hive��.�ý���hive�ķ�ʽˢ��֮
					has_subkeys = RefreshSubtree_hive( hfc, sk, curname );
				}
				
			} else {
				if (RegQueryInfoKey(sk, 0, 0, 0, &has_subkeys, 0, 0, 0, 0, 0, 0, 0))
					has_subkeys = (DWORD)-1;
			}
			RegCloseKey(sk);

			if (has_subkeys != (DWORD)-1) {
				tvi1.mask = TVIF_HANDLE | TVIF_CHILDREN;
				tvi1.hItem = hfc, tvi1.cChildren = has_subkeys != 0;
				TreeView_SetItem(TreeW, &tvi1); 
			}
			if (had_keys.find(buf) == had_keys.end()) {
				had_keys.insert(strdup(buf));
			}
		} else {
			if (RegOpenKeyEx(hk, buf, 0, 0, &sk) == ERROR_SUCCESS) { //??
				RegCloseKey(sk);
			} else {
				is_deleted = true;
			}
		}
		free(curname);
		if (is_deleted) hfcold = hfc;
		hfc = TreeView_GetNextSibling(TreeW, hfc);
		if (is_deleted) TreeView_DeleteItem(TreeW, hfcold);
	}

	DWORD count_subkeys = had_keys.size();
	DWORD sns = MAX_PATH + 1;
	char *sn = (char*)malloc(sns);
	FILETIME ft;
	TVINSERTSTRUCT tvins;

	// ��ͨ��ʽˢ��
	for(n = 0;; n++) 
	{
		sns = MAX_PATH + 1;
		LONG rv = RegEnumKeyEx(hk, n, sn, &sns, 0, 0,0, &ft);
		if (rv != 0) break;

		if (n == 0)
		{
			tvins.hParent = hti, tvins.hInsertAfter = TVI_SORT;
			tvins.item.mask =  TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
			tvins.item.state=0, tvins.item.stateMask=TVIS_EXPANDEDONCE;
			tvins.item.pszText = sn;
		}

		if (had_keys.find(sn) == had_keys.end()) 
		{
			HKEY sk;
			DWORD has_subkeys = 0;
			if (!RegOpenKeyEx(hk, sn, 0, KEY_EXECUTE, &sk)) {
				RegQueryInfoKey(sk, 0, 0, 0, &has_subkeys, 0, 0, 0, 0, 0, 0, 0);
				RegCloseKey(sk);
			}
			tvins.item.cChildren = has_subkeys != 0;
			TreeView_InsertItem(TreeW, &tvins);
			count_subkeys++;
		}
	}
	free(sn);
	
	// ������
	mystrhash_mix::iterator i = had_keys.begin();
	while(i != had_keys.end()) {
		char *c = (*i);
		i++;
		delete []c;
	}
	had_keys.clear();
	return count_subkeys;
}