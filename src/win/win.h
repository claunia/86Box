/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		The Emulator's Windows core.
 *
 * NOTE		This should be named 'plat.h' and then include any 
 *		Windows-specific header files needed, to keep them
 *		out of the main code.
 *
 * Version:	@(#)win.h	1.0.4	2017/10/07
 *
 * Authors:	Sarah Walker, <http://pcem-emulator.co.uk/>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *		Copyright 2008-2017 Sarah Walker.
 *		Copyright 2016,2017 Miran Grca.
 *		Copyright 2017 Fred N. van Kempen.
 */
#ifndef BOX_WIN_H
# define BOX_WIN_H

# ifndef NO_UNICODE
#  define UNICODE
# endif
# define BITMAP WINDOWS_BITMAP
# if 0
#  ifdef _WIN32_WINNT
#   undef _WIN32_WINNT
#   define _WIN32_WINNT 0x0501
#  endif
# endif
# include <windows.h>
# include "resource.h"
# undef BITMAP


/* Class names and such. */
#define CLASS_NAME		L"86BoxMainWnd"
#define MENU_NAME		L"MainMenu"
#define ACCEL_NAME		L"MainAccel"
#define SUB_CLASS_NAME		L"86BoxSubWnd"
#define SB_CLASS_NAME		L"86BoxStatusBar"
#define SB_MENU_NAME		L"StatusBarMenu"
#define RENDER_NAME		L"RenderWindow"

/* Application-specific window messages. */
#define WM_RESETD3D		WM_USER
#define WM_LEAVEFULLSCREEN	WM_USER+1
#define WM_SAVESETTINGS		0x8888


extern HINSTANCE	hinstance;
extern HWND		hwndMain;
extern HICON		hIcon[512];
extern int		status_is_open;
extern int		mousecapture;
extern LCID		dwLanguage;

extern char		openfilestring[260];
extern WCHAR		wopenfilestring[260];

extern int		pause;


#ifdef __cplusplus
extern "C" {
#endif

extern HICON	LoadIconEx(PCTSTR pszIconName);
extern BOOL	DirectoryExists(LPCTSTR szPath);

extern void	leave_fullscreen(void);
extern void	get_executable_name(wchar_t *s, int size);
extern void	set_window_title(wchar_t *s);

extern void	win_language_set(void);
extern void	win_language_update(void);
extern void	win_language_check(void);

extern LPTSTR	win_get_string(int i);
extern LPTSTR	win_get_string_from_string(char *str);

extern void	startblit(void);
extern void	endblit(void);


#ifdef EMU_DEVICE_H
extern void	deviceconfig_open(HWND hwnd, device_t *device);
#endif
extern void	joystickconfig_open(HWND hwnd, int joy_nr, int type);

extern int	getfile(HWND hwnd, char *f, char *fn);
extern int	getsfile(HWND hwnd, char *f, char *fn);

extern void	win_settings_open(HWND hwnd);

extern void	hard_disk_add_open(HWND hwnd, int is_existing);
extern int	hard_disk_was_added(void);

extern void	get_registry_key_map(void);
extern void	process_raw_input(LPARAM lParam, int infocus);

extern void	cdrom_init_host_drives(void);
extern void	cdrom_close(uint8_t id);


/* Functions in win_about.c: */
extern void	AboutDialogCreate(HWND hwnd);


/* Functions in win_status.c: */
extern HWND	hwndStatus;
extern void	StatusWindowCreate(HWND hwnd);


/* Functions in win_stbar.c: */
#define SB_ICON_WIDTH	24
#define SB_FLOPPY       0x00
#define SB_CDROM        0x10
#define SB_RDISK        0x20
#define SB_HDD          0x40
#define SB_NETWORK      0x50
#define SB_TEXT         0x60

extern HWND	hwndSBAR;
extern void	StatusBarCreate(HWND hwndParent, int idStatus, HINSTANCE hInst);
extern int	fdd_type_to_icon(int type);
extern int	StatusBarFindPart(int tag);
extern void	StatusBarUpdatePanes(void);
extern void	StatusBarUpdateTip(int meaning);
extern void	StatusBarUpdateIcon(int tag, int val);
extern void	StatusBarUpdateIconState(int tag, int active);
extern void	StatusBarCheckMenuItem(int tag, int id, int chk);
extern void	StatusBarEnableMenuItem(int tag, int id, int val);
extern void	StatusBarSetTextW(wchar_t *wstr);
extern void	StatusBarSetText(char *str);


/* Functions in win_dialog.c: */
extern int	msgbox_reset(HWND hwndParent);
extern int	msgbox_reset_yn(HWND hwndParent);
extern int	msgbox_question(HWND hwndParent, int i);
extern void	msgbox_info(HWND hwndParent, int i);
extern void	msgbox_info_wstr(HWND hwndParent, WCHAR *wstr);
extern void	msgbox_error(HWND hwndParent, int i);
extern void	msgbox_error_wstr(HWND hwndParent, WCHAR *wstr);
extern void	msgbox_fatal(HWND hwndParent, char *string);
extern void	msgbox_critical(HWND hwndParent, int i);

extern int	file_dlg_w(HWND hwnd, WCHAR *f, WCHAR *fn, int save);
extern int	file_dlg(HWND hwnd, WCHAR *f, char *fn, int save);
extern int	file_dlg_mb(HWND hwnd, char *f, char *fn, int save);
extern int	file_dlg_w_st(HWND hwnd, int i, WCHAR *fn, int save);
extern int	file_dlg_st(HWND hwnd, int i, char *fn, int save);

extern wchar_t	*BrowseFolder(wchar_t *saved_path, wchar_t *title);


#ifdef __cplusplus
}
#endif


#endif	/*BOX_WIN_H*/
