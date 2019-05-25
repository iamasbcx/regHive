#ifndef __AUTOMGM_H__
#define __AUTOMGM_H__
#pragma once

/// Simple useful classes for dealing with allocated character strings
struct achar 
{
	char *c;
	DWORD s, l;

	operator char*() { return c;} // ��ʽת���� char* ����
	operator unsigned char*() { return (unsigned char*)c;} // ��ʽת���� unsigned char* ����
	operator const char*() const { return c;} // ��ʽת���� const char* ����
	operator const unsigned char*() const // ��ʽת���� const unsigned char* ����
	{ 
		return (unsigned char*)c ;
	} 
	
	achar() { c = 0; s = 0; l = 0; } // ���캯��
	
	achar( DWORD n ) // ���캯��. ����һ�� n��С���ڴ�
	{
		c = (char*) malloc( s = ++n );
		l = 0; 
	}
	
	achar( const achar &a ) // ���캯��. ���Ʒݽṹ��
	{ 
		s = a.s, l = a.l ;
		c = a.c ? (char*) malloc(s) : 0 ; 
		c && memcpy( c, a.c, l + 1 ) ;
	}
	
	void operator =(const achar &a) // ��������ж�����. ��������һ���ַ���ָ����и�ֵ
	{ 
		s = a.s, l = a.l; 
		c = a.c ? (char*) realloc(c, s) : (free(c),0) ; 
		c && memcpy(c, a.c, l + 1) ; 
	}
	
	achar( const char *ss ) // ��ֵ�ַ������ṹ����,��Ҫresizeһ��
	{ 
		c = 0; 
		if (ss) {
			strcpy( resize( l = strlen(ss) ), ss );
		} else {
			(s = l = 0); 
		}
	}
	
	~achar() { free(c); } // ��������
	
	char *resize(DWORD n) 
	{ 
		c = (char*) realloc( c, s = ++n );
		return c; 
	}

	char *resize()
	{ 
		return l >= s? resize(l) : c;
	}

	void checklen() { l = c? strlen(c) : 0; } // �õ��ַ����ĳ���
	DWORD asize() const { return s; }
	DWORD size() const { return l; }

	// ���ַ���ת��ΪСд
	void strlwr() 
	{ 
		for( char *s = c, *e = c + l; s < e; s++ ) {
			*s = tolower(*s) ;
		}
	}

	UINT GetDlgItemText( HWND hwnd, int nIDDlgItem );
	UINT GetDlgItemTextUnCEsc( HWND hwnd, int nIDDlgItem );
	LONG QueryValue( HKEY hk, const char *name, DWORD &type );
};



struct fchar 
{
	char *c;
	fchar() : c(0) {} // ���캯��. ��ʼ��
	explicit fchar(void *v) : c((char*)v) {} // ���캯��. ��ֵ
	fchar(fchar &f) : c(f.c) { f.c = 0; } // ���캯��. �ṹ�帳ֵ

	operator char*&() { return c;}
	operator unsigned char*&() { return (unsigned char*&)c;}
	char &operator *() { return *c; }
	operator const char*() const { return c;}
	operator const unsigned char*() const { return (unsigned char*)c;}
	//    operator bool() const { return c != 0; }
	~fchar() { free(c); }
private:
	//  fchar(const fchar &f);
};


struct auto_close_handle
 {
	HANDLE h;
	auto_close_handle(HANDLE H) : h(H) {}
	~auto_close_handle() { CloseHandle(h); }
};

struct auto_close_hkey {
	HKEY hk;
	auto_close_hkey(HKEY HK) : hk(HK) {}
	~auto_close_hkey() { if (hk != (HKEY)INVALID_HANDLE_VALUE) RegCloseKey(hk); }
};

#endif //__AUTOMGM_H__
