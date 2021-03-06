#ifndef __REGSAVLD_H__
#define __REGSAVLD_H__

BOOL CALLBACK DialogLDH(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK DialogSVK(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK DialogLDK(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK DialogRplK(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

int LoadDump8bit(const char *fname);

struct load_hive_dialog_data {
  const char *root_key_name;
  achar fname, subkey_name;
};

struct save_key_dialog_data {
  achar key_name;
  achar fname;
};

struct load_key_dialog_data {
  achar key_name;
  achar fname;
  bool force, nolazy, refresh, volatil;
};

#ifndef REG_FORCE_RESTORE
#define REG_FORCE_RESTORE (0x00000008L)
#endif

struct replace_key_dialog_data {
  achar key_name;
  achar fname_new, fname_old;
};

#endif //__REGSAVLD_H__
