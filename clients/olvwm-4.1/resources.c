/*
 *      (c) Copyright 1990 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *      file for terms of the license.
 */

#ifdef IDENT
#ident "@(#)resources.c	1.9 olvwm version 03/02/00"
#endif

/*
 * Based on
#ident "@(#)resources.c	26.75	93/06/28 SMI"
 *
 */

#ifdef SYSV
#include <sys/types.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>

#include "i18n.h"
#include <olgx/olgx.h>

#include "olwm.h"
#include "ollocale.h"
#include "defaults.h"
#include "globals.h"
#include "resources.h"
#include "win.h"
#include "olcursor.h"
#include "events.h"
#include "mem.h"
#include "menu.h"
#include "virtual.h"
#include "error.h"
#include "winframe.h"
#include "evbind.h"
#include "usermenu.h"
#include "slots.h"

/* static data */

static Bool     updateWorkspaceBackground;
static Bool     forceKeyRegrab;

static Bool matchWorkspaceStyle(char *value, WorkspaceStyle *ret);
static Bool matchFocusKeyword(char *value, Bool *ret);
static Bool matchBeepKeyword(char *value, BeepStatus *ret);
static Bool matchIconPlace(char *value, IconPreference *ret);
static Bool matchMouselessKeyword(char *str, MouselessMode *ret);
static Bool parseKeySpec(Display *dpy, char *str, unsigned int *modmask, KeyCode *keycode);
static Bool cvtBoolean(Display *dpy, ResourceItem *item, char *string, void *addr);
#ifdef OW_I18N_L4
static Bool cvtFontSet(Display *dpy, ResourceItem *item, char *string, void *addr);
#endif
static Bool cvtFont(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtCursorFont(Display *dpy, ResourceItem *item, char *string, void *addr);
#ifdef OW_I18N_L4
static Bool cvtWString(Display *dpy, ResourceItem *item, char *string, void *addr);
#endif
static Bool cvtString(Display *dpy, ResourceItem *item, char *string, void *addr);
#ifdef NOT
static Bool cvtFloat(Display *dpy, ResourceItem *item, char *string, void *addr);
#endif
static Bool cvtInteger(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtWorkspaceStyle(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtClickTimeout(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtFocusStyle(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtBeepStatus(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtMouseless(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtIconLocation(Display *dpy, ResourceItem *item, char *string, void *addr);
static Bool cvtKey(Display *dpy, ResourceItem *item, char *string, void *addr);
static void buildStringList(char *str, List **pplist);
static void *freeStringList(char *str, void *junk);
static Bool cvtStringList(Display *dpy, ResourceItem *item, char *string, void *addr);
#ifdef OW_I18N_L3
static Bool cvtOLLC(Display *dpy, ResourceItem *item, char *string, void *addr);
#endif
static void updString(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updStringList(Display *dpy, ResourceItem *item, List **cur, List **new);
static void updWorkspaceStyle(Display *dpy, ResourceItem *item, WorkspaceStyle *cur, WorkspaceStyle *new);
static void updWorkspace(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updWindow(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updForeground(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updBackground(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updBorder(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updSync(Display *dpy, ResourceItem *item, Bool *cur, Bool *new);
#ifdef OW_I18N_L4
static void updTitleFont(Display *dpy, ResourceItem *item, XFontSetInfo *cur, XFontSetInfo *new);
static void updTextFont(Display *dpy, ResourceItem *item, XFontSetInfo *cur, XFontSetInfo *new);
static void updButtonFont(Display *dpy, ResourceItem *item, XFontSetInfo *cur, XFontSetInfo *new);
static void updIconFont(Display *dpy, ResourceItem *item, XFontSetInfo *cur, XFontSetInfo *new);
#else
static void updTitleFont(Display *dpy, ResourceItem *item, XFontStruct **cur, XFontStruct **new);
static void updTextFont(Display *dpy, ResourceItem *item, XFontStruct **cur, XFontStruct **new);
static void updButtonFont(Display *dpy, ResourceItem *item, XFontStruct **cur, XFontStruct **new);
static void updIconFont(Display *dpy, ResourceItem *item, XFontStruct **cur, XFontStruct **new);
#endif
static void updGlyphFont(Display *dpy, ResourceItem *item, XFontStruct **cur, XFontStruct **new);
static void updIconLocation(Display *dpy, ResourceItem *item, IconPreference *cur, IconPreference *new);
static void updMouseless(Display *dpy, ResourceItem *item, MouselessMode *cur, MouselessMode *new);
static void updMenuAccelerators(Display *dpy, ResourceItem *item, Bool *cur, Bool *new);
static void updWindowCacheSize(Display *dpy, ResourceItem *item, int *cur, int *new);
static void *unconfigureFocus(Client *cli);
static void *reconfigureFocus(Client *cli);
#ifdef OW_I18N_L3
static void setOLLCPosix(void);
static void GRVLCInit(void);
#endif 
static void setVirtualScreenAttribute(Display *dpy, FuncPtr f); 
static void updVirtualFgColor(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualBgColor(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualFontColor(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualGridColor(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updInputFocusColor(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualFont(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualGeometry(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualMap(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualMapColor(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualDesktop(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualIconGeometry(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualScale(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updVirtualDrawSticky(Display *dpy, ResourceItem *item, char **cur, char **new);
static Bool cvtGridEnum(Display *dpy, ResourceItem *item, char *value, VirtualGridType *ret);
static Bool cvtSortType(Display *dpy, ResourceItem *item, char *value, SortType *ret);
static Bool cvtImageType(Display *dpy, ResourceItem *item, char *value, ImageType *ret);
#ifdef NOT
static Bool cvtNoop(Display *dpy, ResourceItem *item, char *value, void *ret);
#endif
static void updCursors(Display *dpy, ResourceItem *item, char **cur, char **new);
static void updIconSlots(Display *dpy, ResourceItem *item, char **cur, char **new);

/* values for flags field */
 
#define RI_IMMUTABLE    (1<<0)      /* cannot be updated once initialized */
#define RI_LOCALE_DEP   (1<<1)      /* is locale dependent */
#define RI_LAST_ITEM    (1<<2)      /* this is the last item in the table */


/* values for ScanResourceItemTable()'s flags argument */

#define SR_IMMUTABLE    (1<<0)      /* scan immutable items as well */
#define SR_USE_DEFAULT  (1<<1)      /* If probe missed, use default */
#define SR_UPDATING     (1<<2)      /* update resource DB */

/*
 * Locale Item Table.  This table contains resource items pertaining to locale
 * information.  It is separate from the Main Item Table (below), because
 * locale announcement may affect which files are read to obtain resources,
 * and also because some items in the main table may require an additional
 * locale-specific resource database probe.  No items in the locale item
 * table should require such a probe, i.e. no items in this table should have
 * the RI_LOCALE_DEP flag set.
 */

ResourceItem LocaleItemTable[] = {
 
#ifdef OW_I18N_L3
{   "basicLocale",              "BasicLocale",          NULL,
     &(GRV.lc_basic),            cvtOLLC,                NULL,
     0L },
{   "displayLang",              "DisplayLang",          NULL,
     &(GRV.lc_dlang),            cvtOLLC,                NULL,
     0L },
{   "inputLang",                "InputLang",            NULL,
     &(GRV.lc_ilang),            cvtOLLC,                NULL,
     0L },
{   "numeric",                  "Numeric",              NULL,
     &(GRV.lc_numeric),          cvtOLLC,                NULL,
     0L },
{   "dateFormat",               "DateFormat",           NULL,
     &(GRV.lc_datefmt),          cvtOLLC,                NULL,
     0L },
#endif /* OW_I18N_L3 */

/* NOTE: the following item must always be the last. */

{   NULL,                       NULL,                   NULL,
    NULL,                       NULL,                   NULL,
    RI_LAST_ITEM }
};

/*
 * Main Item Table.  Contains resource items corresponding to all global
 * variables, except those pertaining to locale information.
 */

ResourceItem MainItemTable[] = {

{   "titleFont",		"TitleFont",
    "-b&h-lucida-bold-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.TitleFontSetInfo),	cvtFontSet,		updTitleFont,
#else
    &(GRV.TitleFontInfo),	cvtFont,		updTitleFont,
#endif
    RI_LOCALE_DEP },
{   "textFont",			"TextFont",
    "-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.TextFontSetInfo),	cvtFontSet,		updTextFont,
#else
    &(GRV.TextFontInfo),	cvtFont,		updTextFont,
#endif
    RI_LOCALE_DEP },
{   "buttonFont",		"ButtonFont",
    "-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.ButtonFontSetInfo),	cvtFontSet,		updButtonFont,
#else
    &(GRV.ButtonFontInfo),	cvtFont,		updButtonFont,
#endif
    RI_LOCALE_DEP },
{   "iconFont",			"IconFont",
    "-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.IconFontSetInfo),	cvtFontSet,		updIconFont,
#else
    &(GRV.IconFontInfo),	cvtFont,		updIconFont,
#endif
    RI_LOCALE_DEP },
{   "glyphFont",		"GlyphFont",
    "-sun-open look glyph-*-*-*-*-*-120-*-*-*-*-*-*",
    &(GRV.GlyphFontInfo),	cvtFont,		updGlyphFont,
    RI_LOCALE_DEP },
{   "cursorFont",		"CursorFont",
    "-sun-open look cursor-*-*-*-*-*-120-*-*-*-*-*-*",
    &(GRV.BasicPointer),	cvtCursorFont,		NULL,
    0L },
{   "foreground",		"Foreground",		"#000000",
    &(GRV.ForegroundColor),	cvtString,		updForeground,
    0L },
{   "background",		"Background",		"#ffffff",
    &(GRV.BackgroundColor),	cvtString,		updBackground,
    0L },
{   "reverseVideo",		"ReverseVideo",		"False",
    &(GRV.ReverseVideo),	cvtBoolean,		NULL,
    0L },
{   "borderColor",		"BorderColor",		"#000000",
    &(GRV.BorderColor),		cvtString,		updBorder,
    0L },
{   "windowColor",		"WindowColor",		"#cccccc",
    &(GRV.WindowColor),		cvtString,		updWindow,
    0L },
{   "workspaceStyle",		"WorkspaceStyle",	"paintcolor",
    &(GRV.WorkspaceStyle),	cvtWorkspaceStyle,	updWorkspaceStyle,
    0L },
{   "workspaceColor",		"WorkspaceColor",	"#40a0c0",
    &(GRV.WorkspaceColor),	cvtString,		updWorkspace,
    0L },
{   "workspaceBitmapFile",	"WorkspaceBitmapFile",	"gray",
    &(GRV.WorkspaceBitmapFile),	cvtString,		updWorkspace,
    0L },
{   "workspaceBitmapFg",	"WorkspaceBitmapFg",	"#000000",
    &(GRV.WorkspaceBitmapFg),	cvtString,		updWorkspace,
    0L },
{   "workspaceBitmapBg",	"WorkspaceBitmapBg",	"#ffffff",
    &(GRV.WorkspaceBitmapBg),	cvtString,		updWorkspace,
    0L },
{   "paintWorkspace",		"PaintWorkspace",	"True",
    &(GRV.PaintWorkspace),	cvtBoolean,		NULL,
    0L },
{   "pointerWorkspace",		"PointerWorkspace",	"True",
    &(GRV.PointerWorkspace),	cvtBoolean,		NULL,
    0L },
{   "use3D",			"Use3D",		"True", 
    &(GRV.F3dUsed),		cvtBoolean,		NULL,
    0L },
{   "setInput",			"SetInput",		"Select",
    &(GRV.FocusFollowsMouse),	cvtFocusStyle,		UpdFocusStyle,
    0L },
{   "defaultTitle",		"DefaultTitle",		"No Name", 
#ifdef OW_I18N_L4
    &(GRV.DefaultWinName),	cvtWString,		updString,
#else
    &(GRV.DefaultWinName),	cvtString,		updString,
#endif
    0L },
{   "flashFrequency",		"FlashFrequency",	"100000", 
    &(GRV.FlashTime),		cvtInteger,		NULL,
    0L },
{   "flashTime",		"FlashTime",		"100000",
    &(GRV.FlashTime),		cvtInteger,		NULL,
    0L },
{   "iconLocation",		"IconLocation", 	"bottom",
    &(GRV.IconPlacement),	cvtIconLocation,	updIconLocation,
    0L },
{   "focusLenience",		"FocusLenience", 	"False",
    &(GRV.FocusLenience),	cvtBoolean,		NULL,
    0L },
{   "dragWindow",		"DragWindow", 		"False",
    &(GRV.DragWindow),		cvtBoolean,		NULL,
    0L },
{   "autoRaise",		"AutoRaise",		"False",
    &(GRV.AutoRaise),		cvtBoolean,		NULL,
    0L },
{   "autoRaiseDelay",		"AutoRaiseDelay",	"0",
    &(GRV.AutoRaiseDelay),	cvtInteger,		NULL,
    0L },
{   "dragRightDistance",	"DragRightDistance",	"100",
    &(GRV.DragRightDistance),	cvtInteger,		NULL,
    0L },
{   "moveThreshold",		"MoveThreshold",	"5",
    &(GRV.MoveThreshold),	cvtInteger,		NULL,
    0L },
{   "dragThreshold",		"DragThreshold",	"5",
    &(GRV.MoveThreshold),	cvtInteger,		NULL,
    0L },
{   "clickMoveThreshold",	"ClickMoveThreshold",	"5",
    &(GRV.ClickMoveThreshold),	cvtInteger,		NULL,
    0L },
{   "multiClickTimeout",	"MultiClickTimeout",	"5",
    &(GRV.DoubleClickTime),	cvtClickTimeout,	NULL,
    0L },
{   "frontKey",			"FrontKey",		"Any L5",
    &(GRV.FrontKey),		cvtKey,			NULL,
    0L },
{   "helpKey",			"HelpKey",		"Help",
    &(GRV.HelpKey),		cvtKey,			NULL,
    0L },
{   "openKey",			"OpenKey",		"Any L7",
    &(GRV.OpenKey),		cvtKey,			NULL,
    0L },
{   "confirmKey",		"ConfirmKey",		"Return",
    &(GRV.ConfirmKey),		cvtKey,			NULL,
    0L },
{   "printOrphans",		"PrintOrphans",		"False", 
    &(GRV.PrintOrphans),	cvtBoolean,		NULL,
    0L },
{   "printAll",			"PrintAll",		"False", 
    &(GRV.PrintAll),		cvtBoolean,		NULL,
    0L },
{   "synchronize",		"Synchronize",		"False", 
    &(GRV.Synchronize),		cvtBoolean,		updSync,
    0L },
{   "snapToGrid",		"SnapToGrid",		"False",
    &(GRV.FSnapToGrid),		cvtBoolean,		NULL,
    0L },
{   "saveWorkspaceTimeout",	"SaveWorkspaceTimeout", "30",
    &(GRV.SaveWorkspaceTimeout), cvtInteger,		NULL,
    0L },
{   "saveWorkspaceCmd",		"SaveWorkspaceCmd",	
    "owplaces -silent -multi -local -script -tw -output $HOME/.openwin-init",
    &(GRV.SaveWorkspaceCmd),	cvtString,		NULL,
    0L },
{   "popupJumpCursor",		"PopupJumpCursor",	"True",
    &(GRV.PopupJumpCursor),	cvtBoolean,		NULL,
    0L },
{   "cancelKey",		"CancelKey",		"Escape",
    &(GRV.CancelKey),		cvtKey,			NULL,
    0L },
{   "colorLockKey",		"ColorLockKey",		"Control L2",
    &(GRV.ColorLockKey),	cvtKey,			NULL,
    0L },
{   "colorUnlockKey",		"ColorUnlockKey",	"Control L4",
    &(GRV.ColorUnlockKey),	cvtKey,			NULL,
    0L },
{   "colorFocusLocked",		"ColorFocusLocked",	"False",
    &(GRV.ColorLocked),	cvtBoolean,		NULL,
    0L },
{   "edgeMoveThreshold",	"EdgeMoveThreshold", 	"10",
    &(GRV.EdgeThreshold),	cvtInteger,		NULL,
    0L },
{   "rubberBandThickness",	"RubberBandThickness",	"2",
    &(GRV.RubberBandThickness),	cvtInteger,		NULL,
    0L },
{   "beep",			"Beep",			"always",
    &(GRV.Beep),		cvtBeepStatus,		NULL,
    0L },
{   "pPositionCompat",		"PPositionCompat",	"false",
    &(GRV.PPositionCompat),	cvtBoolean,		NULL,
    0L },
{   "minimalDecor",		"MinimalDecor",		"",
    &(GRV.Minimals),		cvtStringList,		updStringList,
    0L },
{   "use3DFrames",		"Use3DFrames",		"False", 
    &(GRV.F3dFrames),		cvtBoolean,		NULL,
    0L },
{   "use3DResize",		"Use3DResize",		"True",
    &(GRV.F3dResize),		cvtBoolean,		NULL,
    0L },
{   "refreshRecursively",	"RefreshRecursively",	"True",
    &(GRV.RefreshRecursively),	cvtBoolean,		NULL,
    0L },
{   "mouseChordTimeout",	"MouseChordTimeout",	"100",
    &(GRV.MouseChordTimeout),	cvtInteger,		NULL,
    0L },
{   "mouseChordMenu",		"MouseChordMenu",	"False",
    &(GRV.MouseChordMenu),	cvtBoolean,		NULL,
    0L },
{   "singleScreen",		"SingleScreen",		"False",
    &(GRV.SingleScreen),	cvtBoolean,		NULL,
    0L },
{   "autoReReadMenuFile",        "AutoReReadMenuFile",  "True",
    &(GRV.AutoReReadMenuFile),  cvtBoolean,		NULL,
    0L },
{   "keepTransientsAbove",	"KeepTransientsAbove",	"False",
    &(GRV.KeepTransientsAbove),	cvtBoolean,		NULL,
    0L },
{   "transientsSaveUnder",	"TransientsSaveUnder",	"False",
    &(GRV.TransientsSaveUnder),	cvtBoolean,		NULL,
    0L },
{   "transientsTitled",		"TransientsTitled",	"True",
    &(GRV.TransientsTitled),	cvtBoolean,		NULL,
    0L },
{   "selectWindows",		"SelectWindows",	"True",
    &(GRV.SelectWindows),	cvtBoolean,		NULL,
    0L },
{   "showMoveGeometry",		"ShowMoveGeometry",	"False",
    &(GRV.ShowMoveGeometry),	cvtBoolean,		NULL,
    0L },
{   "showResizeGeometry",	"ShowResizeGeometry",	"False",
    &(GRV.ShowResizeGeometry),	cvtBoolean,		NULL,
    0L },
{   "invertFocusHighlighting",	"InvertFocusHighlighting", "False",
    &(GRV.InvertFocusHighlighting), cvtBoolean,		NULL,
    0L },
{   "runSlaveProcess",		"RunSlaveProcess",	"True",
    &(GRV.RunSlaveProcess),	cvtBoolean,		NULL,
    0L },
{   "selectToggleStacking",	"SelectToggleStacking","False",
    &(GRV.SelectToggleStacking),cvtBoolean,		NULL,
    0L },
{   "flashCount",		"FlashCount",		"6",
    &(GRV.FlashCount),		cvtInteger,		NULL,
    0L },
{   "defaultIconImage",		"DefaultIconImage",	NULL,
    &(GRV.DefaultIconImage),	cvtString,		NULL,
    0L },
{   "defaultIconMask",		"DefaultIconMask",	NULL,
    &(GRV.DefaultIconMask),	cvtString,		NULL,
    0L },
{   "serverGrabs",		"ServerGrabs",		"True",
    &(GRV.ServerGrabs),		cvtBoolean,		NULL,
    0L },
{   "iconFlashCount",		"IconFlashCount",	"3",
    &(GRV.IconFlashCount),	cvtInteger,		NULL,
    0L },
{   "selectDisplaysMenu",	"SelectDisplaysMenu",	"False",
    &(GRV.SelectDisplaysMenu),	cvtBoolean,		NULL,
    0L },
{   "selectionFuzz",		"SelectionFuzz",	"1",
    &(GRV.SelectionFuzz),	cvtInteger,		NULL,
    0L },
{   "autoInputFocus",		"AutoInputFocus",	"False",
    &(GRV.AutoInputFocus),	cvtBoolean,		NULL,
    0L },
{   "autoColorFocus",		"AutoColorFocus",	"False",
    &(GRV.AutoColorFocus),	cvtBoolean,		NULL,
    0L },
{   "colorTracksInputFocus",	"ColorTracksInputFocus","False",
    &(GRV.ColorTracksInputFocus),cvtBoolean,		NULL,
    0L },
{   "iconFlashOnTime",		"IconFlashOnTime",	"20000",
    &(GRV.IconFlashOnTime),	cvtInteger,		NULL,
    0L },
{   "iconFlashOffTime",		"IconFlashOffTime",	"1",
    &(GRV.IconFlashOffTime),	cvtInteger,		NULL,
    0L },
{   "keyboardCommands",		"KeyboardCommands",	"Basic",
    &(GRV.Mouseless),		cvtMouseless,		updMouseless,
    0L },
{   "raiseOnActivate",		"RaiseOnActivate",	"True",
    &(GRV.RaiseOnActivate),	cvtBoolean,		NULL,
    0L },
{   "restackWhenWithdraw",	"RestackWhenWithdraw",	"True",
    &(GRV.RestackWhenWithdraw),	cvtBoolean,		NULL,
    0L },
{   "boldFontEmulation",	"BoldFontEmulation",	"False",
    &(GRV.BoldFontEmulation),	cvtBoolean,		NULL,
    RI_LOCALE_DEP },
{   "raiseOnMove",		"RaiseOnMove",		"False",
    &(GRV.RaiseOnMove),		cvtBoolean,		NULL,
    0L },
{   "raiseOnResize",		"RaiseOnResize",	"False",
    &(GRV.RaiseOnResize),	cvtBoolean,		NULL,
    0L },
{   "startDSDM",		"StartDSDM",		"True",
    &(GRV.StartDSDM),		cvtBoolean,		NULL,
    0L },
{   "printWarnings",		"PrintWarnings",	"False",
    &(GRV.PrintWarnings),	cvtBoolean,		NULL,
    0L },
{   "windowCacheSize",		"WindowCacheSize",	"500",
    &(GRV.WindowCacheSize),	cvtInteger,		updWindowCacheSize,
    0L },
{   "menuAccelerators",		"MenuAccelerators",	"True",
    &(GRV.MenuAccelerators),	cvtBoolean,		updMenuAccelerators,
    0L },
{   "windowMenuAccelerators",	"WindowMenuAccelerators", "True",
    &(GRV.WindowMenuAccelerators), cvtBoolean,		updMenuAccelerators,
    0L },
#ifdef OW_I18N_L3
{   "characterSet",		"CharacterSet",		ISO_LATIN_1,
    &(GRV.CharacterSet),	cvtString,		NULL,
    RI_LOCALE_DEP },
#endif

/* Resources for the virtual desktop */

{   "virtualDesktop",		"VirtualDesktop", 	"3x2",
    &(GRV.VirtualDesktop), 	cvtString,		updVirtualDesktop,
    0L },
{   "pannerScale",		"PannerScale", 		"16",
    &(GRV.VDMScale), 		cvtInteger,		updVirtualScale,
    0L },
{   "allowMoveIntoDesktop",	"AllowMoveIntoDesktop", "True",
    &(GRV.AllowMoveIntoDesktop),cvtBoolean,		NULL,
    0L },
{   "allowArrowInRoot",		"AllowArrowInRoot", 	"True",
    &(GRV.ArrowInRoot),		cvtBoolean,		NULL,
    0L },
{   "virtualGeometry",		"VirtualGeometry", 	"",
    &(GRV.VirtualGeometry),	cvtString,		updVirtualGeometry,
    0L },
{   "virtualFont",		"VirtualFont", 		"5x8",
    &(GRV.VirtualFontName),	cvtString,		updVirtualFont,
    0L },
{   "virtualBackgroundMap",	"VirtualBackgroundMap", NULL,
    &(GRV.VirtualBackgroundMap),cvtString,		updVirtualMap,
    0L },
{   "virtualBackgroundColor",	"VirtualBackgroundColor", NULL,
    &(GRV.VirtualBackgroundColor),cvtString,		updVirtualBgColor,
    0L },
{   "virtualPixmapColor",	"VirtualPixmapColor", 	NULL,
    &(GRV.VirtualPixmapColor),cvtString,		updVirtualMapColor,
    0L },
{   "virtualIconGeometry",	"VirtualIconGeometry", 	"",
    &(GRV.VirtualIconGeometry),cvtString,		updVirtualIconGeometry,
    0L },
{   "virtualForegroundColor",	"VirtualForegroundColor", NULL,
    &(GRV.VirtualForegroundColor),cvtString,		updVirtualFgColor,
    0L },
{   "virtualFontColor",		"VirtualFontColor", 	NULL,
    &(GRV.VirtualFontColor),	cvtString,		updVirtualFontColor,
    0L },
{   "virtualIconic",		"VirtualIconic", 	"False",
    &(GRV.VirtualIconic),	cvtBoolean,		NULL,
    0L },
{   "virtualSticky",		"VirtualSticky", 	"",
    &(GRV.StickyList),		cvtStringList,		NULL,
    0L },
{   "relativePosition",		"RelativePosition", 	"True",
    &(GRV.UseRelativePosition),	cvtBoolean,		NULL,
    0L },
{   "grabVirtualKeys",		"GrabVirtualKeys", 	"True",
    &(GRV.GrabVirtualKeys),	cvtBoolean,		NULL,
    0L },
{   "virtualGrid",		"VirtualGrid", 		"Visible",
    &(GRV.VirtualGrid),		cvtGridEnum,		NULL,
    0L },
{   "virtualGridColor",		"VirtualGridColor", 	"Black",
    &(GRV.VirtualGridColor),	cvtString,		updVirtualGridColor,
    0L },
{   "virtualRaiseVDM",		"VirtualRaiseVDM", 	"False",
    &(GRV.VirtualRaiseVDM),	cvtBoolean,		NULL,
    0L },
{   "stickyIcons",		"StickyIcons", 		"False",
    &(GRV.StickyIcons),		cvtBoolean,		NULL,
    0L },
{   "stickyIconScreen",		"StickyIconScreen", 	"False",
    &(GRV.StickyIconScreen),	cvtBoolean,		NULL,
    0L },
{   "virtualMoveGroup",		"VirtualMoveGroup", 	"True",
    &(GRV.VirtualMoveGroups),	cvtBoolean,		NULL,
    0L },
{   "virtualReRead",		"VirtualReRead", 	"True",
    &(GRV.VirtualReRead),	cvtBoolean,		NULL,
    0L },
{   "syntheticEvents",		"SyntheticEvents", 	"False",
    &(GRV.SyntheticEvents),	cvtBoolean,		NULL,
    0L },
{   "allowSyntheticEvents",	"AllowSyntheticEvents", "False",
    &(GRV.AllowSyntheticEvents),cvtBoolean,		NULL,
    0L },
{   "noVirtualKey",		"NoVirtualKey", 	"",
    &(GRV.NoVirtualKey),	cvtStringList,		NULL,
    0L },
{   "noVirtualLKey",		"NoVirtualLKey", 	"",
    &(GRV.NoVirtualLKey),	cvtStringList,		NULL,
    0L },
{   "noVirtualFKey",		"NoVirtualFKey", 	"",
    &(GRV.NoVirtualFKey),	cvtStringList,		NULL,
    0L },
{   "noVirtualRKey",		"NoVirtualRKey", 	"",
    &(GRV.NoVirtualRKey),	cvtStringList,		NULL,
    0L },
{   "virtualDrawSticky",	"VirtualDrawSticky", 	"True",
    &(GRV.VirtualDrawSticky),	cvtBoolean,		updVirtualDrawSticky,
    0L },
{   "parentScreenPopup",	"ParentScreenPopup", 	"True",
    &(GRV.ParentScreenPopup),	cvtBoolean,		NULL,
    0L },
{   "autoShowRootMenu",		"AutoShowRootMenu", 	"False",
    &(GRV.AutoShowRootMenu),	cvtBoolean,		NULL,
    0L },
{   "autoRootMenuX",		"AutoRootMenuX", 	"0",
    &(GRV.AutoRootMenuX),	cvtInteger,		NULL,
    0L },
{   "autoRootMenuY",		"AutoRootMenuY", 	"0",
    &(GRV.AutoRootMenuY),	cvtInteger,		NULL,
    0L },
{   "inputFocusColor",		"InputFocusColor", 	NULL,
    &(GRV.InputFocusColor),	cvtString,		updInputFocusColor,
    0L },
{   "fullSizeZoomX",		"FullSizeZoomX", 	"False",
    &(GRV.FullSizeZoomX),	cvtBoolean,		NULL,
    0L },
{   "noDecor",			"NoDecor", 		"",
    &(GRV.NoDecors),		cvtStringList,		NULL,
    0L },
{   "resizeMoveGeometry",	"ResizeMoveGeometry", 	"0+0",
    &(GRV.ResizePosition),	cvtString,		NULL,
    0L },
{   "useImages",		"UseImages", 		"UseVDM",
    &(GRV.UseImageMenu),	cvtImageType,		NULL,
    0L },
{   "sortMenuType",		"SortMenuType", 	"Alphabetic",
    &(GRV.VirtualMenuSort),	cvtSortType,		NULL,
    0L },
{   "sortDirType",		"SortDirType", 		"Alphabetic",
    &(GRV.VirtualDirSort),	cvtSortType,		NULL,
    0L },
{   "freeIconSlots",		"FreeIconSlots", 	"False",
    &(GRV.FreeIconSlots),	cvtBoolean,		NULL,
    0L },
{   "iconGridHeight",		"IconGridHeight", 	"13",
    &(GRV.IconGridHeight),	cvtInteger,		updIconSlots,
    0L },
{   "iconGridWidth",		"IconGridWidth", 	"13",
    &(GRV.IconGridWidth),	cvtInteger,		updIconSlots,
    0L },
{   "uniqueIconSlots",		"UniqueIconSlots", 	"False",
    &(GRV.UniqueIconSlots),	cvtBoolean,		NULL,
    0L },
{   "cursorSpecialResize",	"CursorSpecialResize", 	"False",
    &(GRV.SpecialResizePointerData),cvtString,		updCursors,
    0L },
{   "cursorBasic",		"CursorBasic",	  	"OLC_basic",
    &(GRV.BasicPointerData),  	cvtString,	      	updCursors,
    0L },
{   "cursorMove",	     	"CursorMove",	   	"OLC_basic",
    &(GRV.MovePointerData),   	cvtString,	      	updCursors,
    0L },
{   "cursorBusy",	     	"CursorBusy",	   	"OLC_busy",
    &(GRV.BusyPointerData),   	cvtString,	      	updCursors,
    0L },
{   "cursorIcon",	     	"CursorIcon",	   	"OLC_basic",
    &(GRV.IconPointerData),   	cvtString,	      	updCursors,
    0L },
{   "cursorResize",	   	"CursorResize",	 	"OLC_beye",
    &(GRV.ResizePointerData), 	cvtString,	      	updCursors,
    0L },
{   "cursorMenu",	     	"CursorMenu",	   	"OLC_basic",
    &(GRV.MenuPointerData),   	cvtString,	      	updCursors,
    0L },
{   "cursorQuestion",	 	"CursorQuestion",       "OLC_basic",
    &(GRV.QuestionPointerData), cvtString,      	updCursors,
    0L },
{   "cursorTarget",	   	"CursorTarget",	 	"OLC_basic",
    &(GRV.TargetPointerData), 	cvtString,	      	updCursors,
    0L },
{   "cursorPan",	      	"CursorPan",	    	"OLC_basic",
    &(GRV.PanPointerData),    	cvtString,	      	updCursors,
    0L },
{   "cursorCloseUp",	  	"CursorCloseUp",	"OLC_basic",
    &(GRV.CloseUpPointerData),	cvtString,      	updCursors,
    0L },
{   "cursorCloseDown",		"CursorCloseDown",      "OLC_basic",
    &(GRV.CloseDownPointerData),cvtString,	    	updCursors,
    0L },
{   "maxMapColors",		"MaxMapColors",         "200",
    &(GRV.MaxMapColors),	cvtInteger,	    	NULL,
    0L },


/* NOTE: the following item must always be the last. */

{   NULL,			NULL,			NULL,
    NULL,			NULL,			NULL,
    RI_LAST_ITEM }
};


/* ===== Utilities ======================================================== */


/*
 * Copy a string, converting it to lower case.
 */
void
strnlower(dest, src, n)
    char *dest;
    char *src;
    int  n;
{
    char *p;

    strncpy(dest, src, n);
    dest[n-1] = '\0';		/* force null termination */

    for (p = dest; *p; ++p)
	if (isupper(*p))
	    *p = tolower(*p);
}


#define BSIZE 100

/*
 * Determine whether value matches pattern, irrespective of case.
 * This routine is necessary because not all systems have strcasecmp().
 */
Bool
MatchString(value, pattern)
    char *value;
    char *pattern;
{
    char buf[BSIZE];

    strnlower(buf, value, BSIZE);
    return (0 == strcmp(buf, pattern));
}


/*
 * Match any of the following booleans: yes, no, 1, 0, on, off, t, nil, 
 * true, false.  Pass back the boolean matched in ret, and return True.  
 * Otherwise, return False.  Matches are case-insensitive.
 */
Bool
matchBool(value, ret)
    char *value;
    Bool *ret;
{
    char buf[BSIZE];

    strnlower(buf, value, BSIZE);

    if (0 == strcmp(buf, "yes") ||
	0 == strcmp(buf, "on") ||
	0 == strcmp(buf, "t") ||
	0 == strcmp(buf, "true") ||
	0 == strcmp(buf, "1"))
    {
	*ret = True;
	return True;
    }

    if (0 == strcmp(buf, "no") ||
	0 == strcmp(buf, "off") ||
	0 == strcmp(buf, "nil") ||
	0 == strcmp(buf, "false") ||
	0 == strcmp(buf, "0"))
    {
	*ret = False;
	return True;
    }

    return False;
}


/*
 * BoolString() - return Bool based on string, returning the default value if 
 * the string can't be converted.
 */
Bool
BoolString(s, dflt)
	char	*s;
	Bool	dflt;
{
	Bool	b;

	if (matchBool(s,&b))
	    return b;
	else
	    return dflt;
}

/*
 * Match any of the WorkspaceStyle keywords: paintcolor, tilebitmap, none
 * Pass back the WorkspaceStyle value by reference, and return True, if
 * a match was found; otherwise return False and do not disturb the
 * passed value.
 */
static Bool
matchWorkspaceStyle(value, ret)
	char	   *value;
	WorkspaceStyle *ret;
{
	if (MatchString(value,"paintcolor"))
	{
	    *ret = WkspColor;
	    return True;
	}
	if (MatchString(value,"tilebitmap"))
	{
	    *ret = WkspPixmap;
	    return True;
	}
	if (MatchString(value,"default"))
	{
	    *ret = WkspDefault;
	    return True;
	}
	return False;
}

/*
 * Match any of the following input focus keywords: followmouse, follow, f, 
 * select, s, click, clicktotype, c.  Pass back True for focusfollows or 
 * False for clicktotype in ret (since FocusFollowsMouse is the global
 * corresponding to this resource), and return True.  
 * Otherwise, return False.
 */
static Bool
matchFocusKeyword(value, ret)
    char *value;
    Bool *ret;
{
    char buf[BSIZE];

    strnlower(buf, value, BSIZE);

    if (0 == strcmp(buf, "followmouse") ||
	0 == strcmp(buf, "follow") ||
	0 == strcmp(buf, "f"))
    {
	*ret = True;
	return True;
    }

    if (0 == strcmp(buf, "select") ||
	0 == strcmp(buf, "click") ||
	0 == strcmp(buf, "clicktotype") ||
	0 == strcmp(buf, "c") ||
	0 == strcmp(buf, "s"))
    {
	*ret = False;
	return True;
    }

    return False;
}


/*
 * Match any of the three possible beep keywords:  always, never, or notices.
 * Pass back the BeepStatus value by reference, and return True, if
 * a match was found; otherwise return False and do not disturb the
 * passed value.
 */
static Bool
matchBeepKeyword(value, ret)
    char *value;
    BeepStatus *ret;
{
	if (MatchString(value,"always"))
	{
	    *ret = BeepAlways;
	    return True;
	}
	if (MatchString(value,"never"))
	{
	    *ret = BeepNever;
	    return True;
	}
	if (MatchString(value,"notices"))
	{
	    *ret = BeepNotices;
	    return True;
	}
	return False;
}


/*
 * Match an icon placement keyword.  Store matched value in ret and return 
 * True, or return False if no match occurred.
 */
static Bool
matchIconPlace( value, ret )
char		*value;
IconPreference	*ret;
{
	if (MatchString(value, "top"))
	{
		*ret = AlongTop;
		return True;
	}
	if (MatchString(value, "bottom"))
	{
		*ret = AlongBottom;
		return True;
	}
	if (MatchString(value, "right"))
	{
		*ret = AlongRight;
		return True;
	}
	if (MatchString(value, "left"))
	{
		*ret = AlongLeft;
		return True;
	}
	if (MatchString(value, "top-lr"))
	{
		*ret = AlongTop;
		return True;
	}
	if (MatchString(value, "top-rl"))
	{
		*ret = AlongTopRL;
		return True;
	}
	if (MatchString(value, "bottom-lr"))
	{
		*ret = AlongBottom;
		return True;
	}
	if (MatchString(value, "bottom-rl"))
	{
		*ret = AlongBottomRL;
		return True;
	}
	if (MatchString(value, "right-tb"))
	{
		*ret = AlongRight;
		return True;
	}
	if (MatchString(value, "right-bt"))
	{
		*ret = AlongRightBT;
		return True;
	}
	if (MatchString(value, "left-tb"))
	{
		*ret = AlongLeft;
		return True;
	}
	if (MatchString(value, "left-bt"))
	{
		*ret = AlongLeftBT;
		return True;
	}

	return False;
}


static Bool
matchMouselessKeyword(str, ret)
    char *str;
    MouselessMode *ret;
{
    if (0 == strcmp(str, "SunView1")) {
	*ret = KbdSunView;
	return True;
    } else if (0 == strcmp(str, "Basic")) {
	*ret = KbdBasic;
	return True;
    } else if (0 == strcmp(str, "Full")) {
	*ret = KbdFull;
	return True;
    }
    return False;
}


/*
 * Parse a key specification of the form
 *
 * [modifier ...] keysym
 *
 * For example, "Control Shift F7".  Returns True if a valid keyspec was
 * parsed, otherwise False.  The modifier mask is returned in modmask, and the
 * keycode is returned in keycode.
 */
static Bool
parseKeySpec(dpy, str, modmask, keycode)
    Display *dpy;
    char *str;
    unsigned int *modmask;
    KeyCode *keycode;
{
    char line[100];
    char *word;
    int kc, m;
    int mask = 0;
    int code = 0;
    KeySym ks;

    strcpy(line, str);
    word = strtok(line, " \t");
    if (word == NULL)
	return False;

    while (word != NULL) {
	ks = XStringToKeysym(word);
	if (ks == NoSymbol) {
	    if (strcmp(word, "Any") == 0) {
		mask = AnyModifier;
		word = strtok(NULL, " \t");
		continue;
	    } else if (strcmp(word, "Shift") == 0)
		ks = XK_Shift_L;
	    else if (strcmp(word, "Control") == 0)
		ks = XK_Control_L;
	    else if (strcmp(word, "Meta") == 0)
		ks = XK_Meta_L;
	    else if (strcmp(word, "Alt") == 0)
		ks = XK_Alt_L;
	    else if (strcmp(word, "Super") == 0)
		ks = XK_Super_L;
	    else if (strcmp(word, "Hyper") == 0)
		ks = XK_Hyper_L;
	    else
		return False;
	}
	    
	kc = XKeysymToKeycode(dpy, ks);
	if (kc == 0)
	    return False;

	m = FindModifierMask(kc);
	if (m == 0) {
	    code = kc;
	    break;
	}
	mask |= m;
	word = strtok(NULL, " \t");
    }

    if (code == 0)
	return False;

    *keycode = code;
    *modmask = mask;
    return True;
}

/* ===== Converters ======================================================= */


/*
 * static Bool cvtWhatever(dpy, item, string, addr)
 *
 * The job of the converter is to take a string and convert it into the value
 * appropriate for storage into a global variable.  If the conversion is
 * successful, the value is stored at addr and True is returned.  Otherwise,
 * False is returned.  NOTE: the converted global variable shouldn't have any
 * pointers into the resource database.  If it's necessary to keep a handle on
 * this data, the converter should allocate memory and make a copy.  See also
 * the note about memory allocation in the comment at the top of the updaters
 * section, below.
 */


static Bool
/* ARGSUSED */
cvtBoolean(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchBool(string, (Bool *)addr);
}

#ifdef OW_I18N_L4
static Bool
cvtFontSet(dpy, item, string, addr)
    Display         *dpy;
    ResourceItem    *item;
    char            *string;
    void            *addr;
{
    XFontSetInfo    *dest = addr;
    XFontSet        info;
    char            *locale;

    /* XXX - is this right? the locale may not have been set up properly */
    locale = setlocale(LC_CTYPE, NULL);
    info = loadQueryFontSet(dpy, string, locale);
    if (info == NULL)
	return False;

    dest->fs = info;
    dest->fsx = XExtentsOfFontSet(info);

    return True;
}
#endif

static Bool
/* ARGSUSED */
cvtFont(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    XFontStruct	    **dest = addr;
    XFontStruct	    *info;
    
    info = XLoadQueryFont(dpy, string);

#if 0
    if (info == NULL)
	return False;
#else
    if (info == NULL) {
       /* ++roman: Unfortunately olvwm crashes if it has NULL pointers to
        * fonts :-( So try to load a default font if the requested failed. */
       fprintf( stderr, "failed to load font `%s' -- using `fixed' instead\n",
                string );
       info = XLoadQueryFont(dpy, "fixed" );
       if (!info) {
           fprintf( stderr, "failed to load `fixed' too -- expect a crash\n" );
           return False;
       }
    }
#endif

    *dest = info;
    return True;
}


/*
 * cvtCursorFont -- set up ALL cursors from cursor font specified.
 *
 * NOTE that CursorColor and Bg1Color must be set before the cursors!
 *
 * Notice that six cursors are set up (and stored in six separate GRV
 * elements) from this single resource.  REMIND: this is kind of bogus.  
 * Ideally, all six cursors would have fonts and character indexes specifiable 
 * independently.  Further, addr isn't used; GRV is stored directly.
 *
 * REMIND: this appears to have a resource leak, in that cursorFont is loaded 
 * but never unloaded.
 *
 * This function became obsolete in version 3.2 of olvwm, when all cursors
 * started to be defined in initCursor in cursor.c
 */
static Bool
/* ARGSUSED */
cvtCursorFont(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
#ifdef not
    Font	    cursorFont;
    int		    ii;
    Cursor	    *tmpVariable;
    unsigned int    tmpFontIndex;
    unsigned int    defaultIndex;
    XColor	    foreColor, backColor;
    
    cursorFont = XLoadFont(dpy, string);

    /*
     * REMIND: the following doesn't make any sense.  XLoadFont() simply 
     * allocates an ID, sends the LoadFont requst, and returns the ID.  There 
     * is no error indication in the return value from XLoadFont().  This 
     * needs to be fixed.  Perhaps using XLoadQueryFont() would be the right 
     * thing.
     */

    if (cursorFont == NULL)
	return False;

    /*
     * REMIND: in the future, we will probably want to set up some scheme for 
     * customizing cursor colors.  For now, use black and white.
     */

    foreColor.red = foreColor.green = foreColor.blue = 0;	/* black */
    backColor.red = backColor.green = backColor.blue = 65535;	/* white */

    for (ii = 0; ii < NUM_CURSORS; ++ii) {

	switch (ii) {

	case BASICPTR:
	    tmpVariable = &GRV.BasicPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_left_ptr;
	    break;

	case MOVEPTR:
	    tmpVariable = &GRV.MovePointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_left_ptr;
	    break;

	case BUSYPTR:
	    tmpVariable = &GRV.BusyPointer;
	    tmpFontIndex = OLC_busy;
	    defaultIndex = XC_watch;
	    break;

	case ICONPTR:
	    tmpVariable = &GRV.IconPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_left_ptr;
	    break;

	case RESIZEPTR:
	    tmpVariable = &GRV.ResizePointer;
	    tmpFontIndex = OLC_beye;
	    defaultIndex = XC_tcross;
	    break;

	case MENUPTR:
	    tmpVariable = &GRV.MenuPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_sb_right_arrow;
	    break;

	case QUESTIONPTR:
	    tmpVariable = &GRV.QuestionPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_question_arrow;
	    break;

	case TARGETPTR:
	    tmpVariable = &GRV.TargetPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_circle;
	    break;

	case PANPTR:
	    tmpVariable = &GRV.PanPointer;
	    tmpFontIndex = OLC_panning;
	    defaultIndex = XC_sb_v_double_arrow;
	    break;
	}

	if (cursorFont == 0 ||
	    0 == (*tmpVariable = XCreateGlyphCursor(dpy, cursorFont,
			cursorFont, tmpFontIndex, tmpFontIndex+1, 
			&foreColor, &backColor)))
	{
	    /* use default */
	    *tmpVariable = XCreateFontCursor( dpy, defaultIndex );
#ifdef LATER
	    XRecolorCursor(dpy, tmpVariable, &foreColor, &backColor);
#endif
	}
    }
#endif
    return True;
}

#ifdef OW_I18N_L4
/*
 * Converting a string simply means making a copy of it.
 */
static Bool
cvtWString(dpy, item, string, addr)
    Display         *dpy;
    ResourceItem    *item;
    char            *string;
    void            *addr;
{
    wchar_t **str = addr;

    if (string == NULL)
	return False;

    *str = mbstowcsdup(string);
    return True;
}
#endif

/*
 * Converting a string simply means making a copy of it.
 */
static Bool
/* ARGSUSED */
cvtString(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    char **str = addr;

    if (string == NULL)
	return False;

    *str = MemNewString(string);
    return True;
}

#ifdef NOT
static Bool
/* ARGSUSED */
cvtFloat(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return (1 == sscanf(string, "%f", (float *)addr));
}
#endif


/*
 * Convert an integer.  Note that %i converts from decimal, octal, and 
 * hexadecimal representations.
 */
static Bool
/* ARGSUSED */
cvtInteger(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return (1 == sscanf(string, "%i", (int *)addr));
}

/*
 * Convert a string representing WorkspaceStyle
 */
static Bool
cvtWorkspaceStyle(dpy, item, string, addr)
    Display         *dpy;
    ResourceItem    *item;
    char            *string;
    void            *addr;
{
    return matchWorkspaceStyle(string, (WorkspaceStyle *)addr);
}

/*
 * Convert a string representing tenths of a second into milliseconds.
 */
static Bool
/* ARGSUSED */
cvtClickTimeout(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    int intval;
    int *dest = addr;

    if (1 != sscanf(string, "%d", &intval))
	return False;

    intval *= 100;			/* convert to milliseconds */

    /*
     * It's nearly impossible for typical mouse hardware to generate two
     * clicks in less than 100ms.  We special-case this and make the minimum
     * timeout value be 150ms.
     */
    if (intval < 150)
	intval = 150;

    *dest = intval;
    return True;
}


static Bool
/* ARGSUSED */
cvtFocusStyle(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchFocusKeyword(string, (Bool *)addr);
}


static Bool
/* ARGSUSED */
cvtBeepStatus(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchBeepKeyword(string, (BeepStatus *)addr);
}


static Bool
/* ARGSUSED */
cvtMouseless(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchMouselessKeyword(string, (MouselessMode *)addr);
}


static Bool
/* ARGSUSED */
cvtIconLocation(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchIconPlace(string, (IconPreference *)addr);
}


/*
 * Convert a key specification.  REMIND: this needs to be reconciled with the 
 * key specification stuff in evbind.c.
 */
static Bool
/* ARGSUSED */
cvtKey(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    KeySpec	    *keyspec = addr;
    unsigned int    modmask;
    KeyCode	    keycode;

    if (!parseKeySpec(dpy, string, &modmask, &keycode))
	return False;

    keyspec->modmask = modmask;
    keyspec->keycode = keycode;
    return True;
}


/*
 * buildStringList -- parse a string into words and build a linked list of 
 * them.
 */
static void
buildStringList(str, pplist)
char *str;
List **pplist;
{
    char *swork, *swork2;
    List *l = NULL_LIST;

    swork2 = swork = MemNewString(str);

    while ((swork2 = strtok(swork2, " \t")) != NULL) {
	l = ListCons(MemNewString(swork2),l);
	swork2 = NULL;
    }
    MemFree(swork);
    *pplist = l;
}


static void *
/* ARGSUSED */
freeStringList(str,junk)
char *str;
void *junk;
{
	MemFree(str);
	return NULL;
}


static Bool
/* ARGSUSED */
cvtStringList(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    List **dest = addr;
    List *newl = NULL_LIST;

    buildStringList(string, &newl);
    *dest = newl;
    return True;
}


#ifdef OW_I18N_L3

/*
 * REMIND: somewhat strange.  This function always returns True, so the
 * default value in the Resource Table is never used.  Further, this function 
 * handles both the conversion and update functions itself.
 */
static Bool
/* ARGSUSED */
cvtOLLC(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    OLLCItem	    *ollcitem = addr;
    char	    *newlocale;

#ifdef DEBUG
    fprintf(stderr, "cvtOLLC: locale#%d, newlocale %s, curlocale %s\n",
	    ollcitem->posixCategory, string,
	    ollcitem->locale ? ollcitem->locale : "(null)");
#endif

    /* don't need to do anything if the new locale is the same as the old */

    if ((string == NULL && ollcitem->locale == NULL) ||
        (string != NULL && ollcitem->locale != NULL &&
	 0 == strcmp(string, ollcitem->locale)))
    {
	return True;
    }

    /* they differ; update the locale */

    if (string == NULL)
	newlocale = NULL;
    else
	newlocale = MemNewString(string);

    if (ollcitem->locale != NULL)
	MemFree(ollcitem->locale);

    ollcitem->locale = newlocale;

#ifdef DEBUG
    fprintf(stderr, "cvtOLLC: locale#%d -> %s\n",
	    ollcitem->posixCategory,
	    ollcitem->locale ? ollcitem->locale : "(null)");
#endif

    return True;
}

#endif	/* OW_I18N_L3 */

#if defined (DEBUG) && defined (OW_I18N_L3)
void dump_locale()
{
    fprintf(stderr, "  -> %5.5s %5.5s %5.5s %5.5s %5.5s\n",
	    "basic", "dlang", "ilang", "numeric", "date");
    fprintf(stderr, "  -> %5.5s %5.5s %5.5s %5.5s %5.5s\n",
	    GRV.lc_basic.locale ? GRV.lc_basic.locale : "(null)",
	    GRV.lc_dlang.locale ? GRV.lc_dlang.locale : "(null)",
	    GRV.lc_ilang.locale ? GRV.lc_ilang.locale : "(null)",
	    GRV.lc_numeric.locale ? GRV.lc_numeric.locale : "(null)",
	    GRV.lc_datefmt.locale ? GRV.lc_datefmt.locale : "(null)");
}
#endif


/* ===== Updaters ========================================================= */


/*
 * static void updWhatever(dpy, item, cur, new);
 *
 * The job of the updater is to compare the current value and newly converted
 * values, and update the current value if they differ.  It is responsible
 * for all changes in global state, such as grabbing and ungrabbing keys.  
 * NOTE: if the converter has allocated memory, the updater must free it 
 * appropriately.  Since the updater is called with old and new values, 
 * exactly one of them should be freed by the updater, otherwise a memory leak 
 * will result.
 */

static void
/* ARGSUSED */
updString(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
}


static void
/* ARGSUSED */
updStringList(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    List	    **cur, **new;
{
    ListApply(*cur, freeStringList, NULL);
    ListDestroy(*cur);
    *cur = *new;
}

static void
/* ARGSUSED */
updWorkspaceStyle(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    WorkspaceStyle  *cur, *new;
{
    *cur = *new;
    updateWorkspaceBackground = True;
}

static void
/* ARGSUSED */
updWorkspace(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    updateWorkspaceBackground = True;
}


static void
/* ARGSUSED */
updWindow(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetWindowColor(dpy);
}


static void
/* ARGSUSED */
updForeground(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetForegroundColor(dpy);
}

static void
/* ARGSUSED */
updBackground(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetBackgroundColor(dpy);
}

static void
/* ARGSUSED */
updBorder(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetBorderColor(dpy);
}


static void
/* ARGSUSED */
updSync(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    Bool	    *cur, *new;
{
    if (*cur != *new) {
	(void) XSynchronize(dpy, *new);
	*cur = *new;
    }
}


static void
/* ARGSUSED */
updTitleFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
#ifdef OW_I18N_L4
    XFontSetInfo    *cur, *new;
#else
    XFontStruct     **cur, **new;
#endif
{
#ifdef OW_I18N_L4
    freeFontSet(dpy, cur->fs);
#else
    XFree((char *) *cur);
#endif
    *cur = *new;
    SetTitleFont(dpy);
}


static void
/* ARGSUSED */
updTextFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
#ifdef OW_I18N_L4
    XFontSetInfo    *cur, *new;
#else
    XFontStruct     **cur, **new;
#endif
{
#ifdef OW_I18N_L4
    freeFontSet(dpy, cur->fs);
#else
    XFree((char *) *cur);
#endif
    *cur = *new;
    SetTextFont(dpy);
}


static void
/* ARGSUSED */
updButtonFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
#ifdef OW_I18N_L4
    XFontSetInfo    *cur, *new;
#else
    XFontStruct     **cur, **new;
#endif
{
#ifdef OW_I18N_L4
    freeFontSet(dpy, cur->fs);
#else
    XFree((char *) *cur);
#endif
    *cur = *new;
    SetButtonFont(dpy);
}


static void
/* ARGSUSED */
updIconFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
#ifdef OW_I18N_L4
    XFontSetInfo    *cur, *new;
#else
    XFontStruct     **cur, **new;
#endif
{
#ifdef OW_I18N_L4
    freeFontSet(dpy, cur->fs);
#else
    XFree((char *) *cur);
#endif
    *cur = *new;
    SetIconFont(dpy);
}


static void
/* ARGSUSED */
updGlyphFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    XFontStruct	    **cur, **new;
{
    XFree((char *) *cur);
    *cur = *new;
    SetGlyphFont(dpy);
}


static void
/* ARGSUSED */
updIconLocation(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    IconPreference  *cur, *new;
{
    if (*cur != *new) {
	*cur = *new;
	SetIconLocation(dpy);
    }
}


static void
/* ARGSUSED */
updMouseless(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    MouselessMode   *cur, *new;
{
    if (*cur != *new) {
	*cur = *new;
	forceKeyRegrab = True;
    }
}

static void
updMenuAccelerators(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    Bool            *cur, *new;
{
    if (*cur != *new) {
        *cur = *new;
        forceKeyRegrab = True;
    }
}

static void
updWindowCacheSize(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    int             *cur, *new;
{
    if (*cur != *new) {
        *cur = *new;
        ScreenUpdateWinCacheSize(dpy);
    }
}

/*
 * unconfigureFocus
 *
 * Tell a client to remove any grabs it may have set up according to the focus 
 * mode.  If this client is the focus, tell it to draw in its unfocused state.
 */
static void *
unconfigureFocus(cli)
    Client *cli;
{
    if (cli->framewin == NULL)
	return NULL;
    FrameSetupGrabs(cli, cli->framewin->core.self, False);
    if (cli->isFocus) {
	cli->isFocus = False;
	WinCallDraw((WinGeneric *)cli->framewin);
	cli->isFocus = True;
    }
    return NULL;
}


/*
 * reconfigureFocus
 *
 * Tell a client to restore any grabs it may need for the new focus mode.  If 
 * this client is the focus, tell it to draw using the proper highlighting for 
 * the new focus mode.
 */
static void *
reconfigureFocus(cli)
    Client *cli;
{
    if (cli->framewin == NULL)
	return NULL;
    FrameSetupGrabs(cli, cli->framewin->core.self, True);
    if (cli->isFocus) {
	WinCallDraw((WinGeneric *)cli->framewin);
    }
    return NULL;
}


/*
 * UpdFocusStyle -- change the focus style on the fly
 *
 * If focus style needs updating, call unconfigureFocus on every client.  This
 * will clear grabs and highlighting and such while the old focus mode is
 * still in effect.  Update the global value, and then call reconfigureFocus
 * on every client to set up stuff for the new focus mode.
 *
 * REMIND: This function is global because it's called from FlipFocusFunc in
 * services.c.  This call passes NULL for item.  This needs to be cleaned up.
 */
void
/* ARGSUSED */
UpdFocusStyle(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    Bool	    *cur, *new;
{
    if (*cur != *new) {
	ListApply(ActiveClientList, unconfigureFocus, 0);
	*cur = *new;
	ListApply(ActiveClientList, reconfigureFocus, 0);
    }
}


/* ===== Global Functions ================================================= */

/*
 * datum -- temporary destination for converted values.  On resource updates,
 * values are converted into this area instead of directly into the global
 * variable.  This allows the update function to compare the converted values
 * to see whether an update is really necessary.  This union should be as
 * large as the largest field in GRV.
 */

static union {
    int             intval;
    void            *pointer;
    KeySpec         keyspec;
#ifdef OW_I18N_L3
    OLLCItem        ollcitem;
#ifdef OW_I18N_L4
    XFontSetInfo    fontsetinfo;
#endif
#endif /* OW_I18N_L3 */
} datum;

/*
 * ScanResourceItemTable.  Scans a resource item table, probing the
 * resource database for each item.  Ignores any immutable items in
 * the table.  If SR_IMMUTABLE is True, a hit will cause the table
 * entry's immutable flag to be set.  If the probe missed and
 * SR_USE_DEFAULT is True, the default value will be converted.
 *
 * If SR_UPDATING is True, then we are updating the resource database
 * instead of initializing it.  This causes a couple of differences.
 * If an updater function is given for the item, conversions are done
 * into a piece of temporary storage and then the updater function is
 * called to update the real global variable from this temporary
 * storage.
 */
 
void
ScanResourceItemTable(dpy, table, rdb, oldlocale, flags)
    Display *dpy;
    ResourceItem *table;
    XrmDatabase rdb;
    char *oldlocale;
    unsigned long flags;
{
    XrmRepresentation type;
    XrmValue value;
    XrmValue oldvalue;
    XrmQuark classes[4];
    XrmQuark instances[4];
    ResourceItem *item;
    Bool hit;
 
#ifdef OW_I18N_L3
    XrmQuark localeQ;
    XrmQuark oldlocaleQ;

    if (GRV.lc_basic.locale != NULL)
	localeQ = XrmStringToQuark(GRV.lc_basic.locale);
    else
	localeQ = NULLQUARK;

    if (oldlocale != NULL)
	oldlocaleQ = XrmStringToQuark(oldlocale);
#endif

    classes[0] = OpenWinQ;
    instances[0] = TopInstanceQ;

    for (item = table; !(item->flags & RI_LAST_ITEM); ++item) {

	/* never update an immutable item */
	if ((item->flags & RI_IMMUTABLE) && (flags & SR_UPDATING))
	    continue;

	classes[1] = item->classQ;
	instances[1] = item->instanceQ;
	hit = False;

#ifdef OW_I18N_L3
	if (item->flags & RI_LOCALE_DEP) {
	    classes[2] = instances[2] = localeQ;
	    classes[3] = instances[3] = NULLQUARK;
	    hit = XrmQGetResource(rdb, instances, classes, &type, &value);
	    if (!hit) {
		classes[0] = OlwmQ;
	    	hit = XrmQGetResource(rdb, instances, classes, &type, &value);
		classes[0] = OpenWinQ;
	    }
	}
#endif

	if (!hit) {
	    classes[2] = instances[2] = NULLQUARK;
	    hit = XrmQGetResource(rdb, instances, classes, &type, &value);
	    if (!hit) {
		classes[0] = OlwmQ;
	    	hit = XrmQGetResource(rdb, instances, classes, &type, &value);
		classes[0] = OpenWinQ;
	    }
	}

	if (flags & SR_UPDATING) {

	    /* ignore if not found */
	    if (!hit)
		continue;

	    /* ignore if old and new values are the same */
#ifdef OW_I18N_L3
	    if ((item->flags & RI_LOCALE_DEP) && (oldlocale != NULL))
		classes[2] = instances[2] = oldlocaleQ;
#endif
	    if (XrmQGetResource(OlwmDB, instances, classes, &type, &oldvalue)		&& 0 == strcmp((char *)value.addr, (char *)oldvalue.addr))
	    {
		continue;
	    }
#ifdef OW_I18N_L3
	    if ((item->flags & RI_LOCALE_DEP) && (oldlocale != NULL))
		classes[2] = instances[2] = localeQ;
#endif

	    if (item->updater == NULL) {
		(void) (*item->converter)(dpy, item, (char *)value.addr,
					  item->addr);
	    } else {
		(void) memset((char *) &datum, 0, sizeof(datum));
		if ((*item->converter)(dpy, item, (char *)value.addr, &datum))
		    (*item->updater)(dpy, item, item->addr, &datum);
	    }
	} else {
	    if (hit && (*item->converter)(dpy, item, value.addr, item->addr)) {
		if (flags & SR_IMMUTABLE)
		    item->flags |= RI_IMMUTABLE;
	    } else {
		if (flags & SR_USE_DEFAULT)
		    (void) (*item->converter)(dpy, item, item->defaultString,					      item->addr);
	    }
	}
    }    
}

/*
 * InitGlobals.  Zero out all global variables.  Run through resource tables,
 * interning their quarks.  Called once at startup time.  Destroys cmdDB.
 */
void
InitGlobals(dpy, cmdDB)
    Display *dpy;
    XrmDatabase cmdDB;
{
    ResourceItem *item;
    XrmDatabase userDB;

    (void) memset((char *) &GRV, 0, sizeof(GRV));
		 
    /* Run through the tables and intern the quarks. */
 
    for (item = LocaleItemTable; !(item->flags & RI_LAST_ITEM); ++item) {
	item->classQ    = XrmStringToQuark(item->class);
	item->instanceQ = XrmStringToQuark(item->instance);
    }
 
    for (item = MainItemTable; !(item->flags & RI_LAST_ITEM); ++item) {
	item->classQ    = XrmStringToQuark(item->class);
	item->instanceQ = XrmStringToQuark(item->instance);
    }
 
#ifdef OW_I18N_L3
    GRVLCInit();
#endif
 
    userDB = GetUserDefaults(dpy);
 
    ScanResourceItemTable(dpy, LocaleItemTable, cmdDB, (char *) NULL, (unsigned long) SR_IMMUTABLE);
    ScanResourceItemTable(dpy, LocaleItemTable, userDB, (char *) NULL, (unsigned long) 0);

#ifdef OW_I18N_L3
    setOLLCPosix();
    EffectOLLC(dpy, True, NULL, NULL);
#endif

    ScanResourceItemTable(dpy, MainItemTable, cmdDB, (char *) NULL, (unsigned long) SR_IMMUTABLE);

    OlwmDB = GetAppDefaults();
    XrmMergeDatabases(userDB, &OlwmDB);
    XrmMergeDatabases(cmdDB, &OlwmDB);

    ScanResourceItemTable(dpy, MainItemTable, OlwmDB, (char *) NULL, (unsigned long) SR_USE_DEFAULT);

    /*
     * Special case for glyph font: if we couldn't find a valid glyph font,
     * it's a fatal error.
     */  
    if (GRV.GlyphFontInfo == NULL)
	ErrorGeneral(GetString("can't open glyph font"));
	/*NOTREACHED*/
}

/*
 * UpdateGlobals -- handle updates to the server's resource database.  Called
 * every time the server's RESOURCE_MANAGER property changes.  Refetches the
 * user's database and the app-defaults database and merges them, and then
 * replaces the global database with this new one.  This loses the resources
 * that corresponded to the command-line arguments, but that should be OK
 * since we should have set them to be immutable at startup time.
 */
void
UpdateGlobals(dpy)
    Display		*dpy;
{
    XrmDatabase         userDB;
    XrmDatabase         newDB;
    Bool                dlangChanged = False;
#ifdef OW_I18N_L3
    char                oldBasicLocale[MAXNAMELEN + 1];
    char                oldDisplayLang[MAXNAMELEN + 1];
#endif
    updateWorkspaceBackground = False;
    forceKeyRegrab = False;
 
    userDB = GetUserDefaults(dpy);
 
    ScanResourceItemTable(dpy, LocaleItemTable, userDB, (char *) NULL, (unsigned long) SR_UPDATING);
 
#ifdef OW_I18N_L3
    EffectOLLC(dpy, False, oldBasicLocale, oldDisplayLang);
#endif /* OW_I18N_L3 */
 
    /*
     * This re-fetches the app-defaults file every time the user database
     * changes.  This may be necessary if the locale changes.  It may also be
     * necessary if a resource disappears from the user's database.  In this
     * case, we will want the value to revert to a value in the app-defaults
     * file, a value that had been overridden before.
     */
    newDB = GetAppDefaults();
    XrmMergeDatabases(userDB, &newDB);
 
#ifdef OW_I18N_L3
    ScanResourceItemTable(dpy, MainItemTable, newDB,
			  oldBasicLocale, (unsigned long) SR_UPDATING);
 
    if (strcmp(GRV.lc_dlang.locale, oldDisplayLang) != 0)
	dlangChanged = True;
 
#else
    ScanResourceItemTable(dpy, MainItemTable, newDB, NULL, SR_UPDATING);
#endif
 
    if (updateWorkspaceBackground)
	SetWorkspaceBackground(dpy);

    if (dlangChanged || UpdateBindings(dpy, newDB, forceKeyRegrab))
	ReInitAllUserMenus(dpy);

    XrmDestroyDatabase(OlwmDB);
    OlwmDB = newDB;
}

/* ===== Internationalization ============================================= */
 
#ifdef OW_I18N_L3
 
/*
 * setOLLCPosix
 *
 * For each locale category setting that's NULL, fetch its current POSIX
 * setting and store it into GRV.
 */
static void
setOLLCPosix()
{
    OLLCItem *ollci;
    OLLCItem *last = &GRV.LC[OLLC_LC_MAX];


    (void) setlocale(LC_ALL, "");
#ifdef DEBUG
    fprintf(stderr, "Just bfore OLLCPosix\n");
    dump_locale();
#endif
    for (ollci = GRV.LC; ollci < last; ollci++) {
	if (ollci->locale == NULL && ollci->posixCategory >= 0)
	    ollci->locale =
		MemNewString(setlocale(ollci->posixCategory, NULL));
    }
#ifdef DEBUG
    dump_locale();
#endif
}

/*
 * GRVLCInit
 *
 * For each OPEN LOOK locale category, fill in its corresponding POSIX locale
 * category identifier.  Note: this is not intended to be a complete mapping.
 */
static void
GRVLCInit()
{
    GRV.lc_basic.posixCategory          =  LC_CTYPE;
    GRV.lc_basic.envName                = "LC_CTYPE";
    GRV.lc_dlang.posixCategory          =  LC_MESSAGES;
    GRV.lc_dlang.envName                = "LC_MESSAGES";
    GRV.lc_ilang.posixCategory          = -1;
    GRV.lc_ilang.envName                = NULL;
    GRV.lc_numeric.posixCategory        =  LC_NUMERIC;
    GRV.lc_numeric.envName              = "LC_NUMERIC";
    GRV.lc_datefmt.posixCategory        =  LC_TIME;
    GRV.lc_datefmt.envName              = "LC_TIME";
}

/*
 * EffectOLLC
 *
 * Apply restrictions to locale category combinations and then effect locale
 * changes as necessary (using setlocale()).  Restrictions are as follows:
 *
 * + The basic locale can be changed from C to a non-C locale.  However, once
 * in a non-C locale, it can never be changed again.  If Olwm is in a non-C
 * locale, it can support only applications in that locale and applications in
 * the C locale.  It cannot support applications in multiple non-C locales, as
 * that might require Olwm to switch between locales from window to window,
 * which it can't do.
 *
 * + If the first or initial time through, then ignore the above restriction.
 * This is because the locale may be set to a non-C  locale in olwm.c/main()
 * using an environment variable.  Here we're using the locale setting from
 * the resources which override the environment.  So the first time through
 * set the locale using the resource setting.
 *
 * + If the basic locale is C, all other locale categories must be C.  If the
 * basic locale is non-C, the other categories must either be C or must match
 * the basic locale.
 *
 * REMIND: need to check return values from setlocale().
 */
void
EffectOLLC(dpy, initial, oldBasicLocale, oldDisplayLang)
    Display *dpy;
    Bool     initial;
    char    *oldBasicLocale;
    char    *oldDisplayLang;
{
    OLLCItem *ollci;
    char *basic, *new, *cur;
    Bool basic_updated = False;
    Bool sticky_locale;

#ifdef DEBUG
    fprintf(stderr, "Before calling EffectOLLC\n");
    dump_locale();
#endif
    /*
     * Apply restrictions to the basic locale if current locale is not
     * sticky locale (sticky locale is defined to be locale which uses
     * none iso latin1 as characterset), updating if necessary.
     * Ensure that GRV.lc_basic matches reality.
     *   
     * Note: update using LC_ALL in order to get the POSIX locale
     * categories that aren't covered by the OPEN LOOK locale
     * categories.  This forces us to update all the other locale
     * categories, even if they otherwise wouldn't need to be updated.
     */  
    if (initial || strcmp(GRV.CharacterSet, ISO_LATIN_1) == 0)
	sticky_locale = False;
    else
	sticky_locale = True;

    basic = MemNewString(setlocale(LC_CTYPE, NULL));
    if (oldBasicLocale != NULL)
	(void) strcpy(oldBasicLocale, basic);
    if (oldDisplayLang != NULL)
	(void) strcpy(oldDisplayLang, setlocale(LC_MESSAGES, NULL));
    if (initial || ! sticky_locale ||
	(strcmp(basic, "C") == 0 && strcmp(GRV.lc_basic.locale, "C") != 0) )
    {
#ifdef DEBUG
	fprintf(stderr, "Basic Locale -> %s\n", GRV.lc_basic.locale);
#endif
	setlocale(LC_ALL, GRV.lc_basic.locale);
	basic_updated = True;
#ifdef OW_I18N_L4
	/*
	 * Check with Xlib to see basiclocale/LC_CTYPE is supported or
	 * not.
	 */
	if (! XSupportsLocale()) {
	    /*
	     * Assumption: "C" locale is always supported by the Xlib.
	     */
	    (void) fprintf(stderr, "%s: Supplied locale (%s) is not supported by Xlib - defaulting to C\n",
			   ProgramName, GRV.lc_basic.locale);
	    (void) setlocale(LC_ALL, "C");
	    if (strcmp(basic, "C") == 0)
		basic_updated = False;
	    MemFree(GRV.lc_basic.locale);
	    GRV.lc_basic.locale = MemNewString("C");
	    MemFree(GRV.lc_dlang.locale);
	    GRV.lc_dlang.locale = MemNewString("C");
	}
	if (! XSetLocaleModifiers(""))
	    (void) fprintf(stderr, "%s: Error in setting locale modifier to Xlib\n",
			   ProgramName);
#endif
	MemFree(basic);
	basic = MemNewString(GRV.lc_basic.locale);
    } else if (strcmp(basic, GRV.lc_basic.locale) != 0) {
	MemFree(GRV.lc_basic.locale);
	GRV.lc_basic.locale = MemNewString(basic);
    }

    /*
     * Run through the other locale categories, applying the restrictions, and
     * updating if necessary.  Skip categories that have no corresponding
     * Posix locale category.  As before, make sure the value in GRV matches
     * the actual current setting.
     */  
    for (ollci = GRV.LC + 1; ollci < &GRV.LC[OLLC_LC_MAX]; ++ollci) {

	if (ollci->posixCategory < 0)
	    continue;

	if (sticky_locale) {
	    if (strcmp(basic, "C") != 0
		&& strcmp(ollci->locale, "C") != 0)
	    {
		new = basic;
	    } else {
		new = "C";
	    }
	} else
	    new = ollci->locale;

	cur = setlocale(ollci->posixCategory, NULL);
	if (basic_updated || strcmp(cur, new) != 0) {
#ifdef DEBUG
	    fprintf(stderr, "locale#%d -> %s\n", ollci->posixCategory, new);
#endif
	    setlocale(ollci->posixCategory, new);
	}

	if (strcmp(ollci->locale, new) != 0) {
	    MemFree(ollci->locale);
	    ollci->locale = MemNewString(new);
	}
    }    

    MemFree(basic);
#ifdef DEBUG
    dump_locale();
#endif
}

#endif /* OW_I18N_L3 */

/*
 * ReInitAllUserMenus -- Reinitalize the user menus for each screen
 */
void
ReInitAllUserMenus(dpy)
    Display	 *dpy;
{
ScreenInfo      *si;
extern List	*ScreenInfoList;
List	    *l = ScreenInfoList;
 
    for (si = ListEnum(&l); si; si = ListEnum(&l))
	ReInitUserMenu(dpy, si, True);
}
 
/*
 * setVirtualScreenAttribute - set the given attribute for each vdm
 */
static void
setVirtualScreenAttribute(dpy, f) 
    Display	 *dpy;
    FuncPtr	    f;
{
ScreenInfo      *si;
extern List	*ScreenInfoList;
List	    *l = ScreenInfoList;
			   
    for (si = ListEnum(&l); si; si = ListEnum(&l)) {
	(*f)(dpy,si);
	RedrawVDM(si->vdm);
    }
}

static void
/* ARGSUSED */
updVirtualFgColor(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) SetScreenVirtualForegroundColor);
}

static void
/* ARGSUSED */
updVirtualBgColor(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) SetScreenVirtualBackgroundColor);
}

static void
/* ARGSUSED */
updVirtualFontColor(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) SetScreenVirtualFontColor);
}

static void
/* ARGSUSED */
updVirtualGridColor(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) SetScreenVirtualGridColor);
}

static void
/* ARGSUSED */
updInputFocusColor(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) SetScreenInputFocusColor);
}

static void
/* ARGSUSED */
updVirtualFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualFont);
}

static void
/* ARGSUSED */
updVirtualGeometry(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualGeometry);
}

static void
/* ARGSUSED */
updVirtualMap(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualMap);
}

static void
/* ARGSUSED */
updVirtualMapColor(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) SetScreenVirtualPixmapColor);
}

static void
/* ARGSUSED */
updVirtualDesktop(dpy, item, cur, new)
    Display		*dpy;
    ResourceItem	*item;
    char		**cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualDesktop);
}

static void
/* ARGSUSED */
updVirtualIconGeometry(dpy, item, cur, new)
    Display		*dpy;
    ResourceItem	*item;
    char		**cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualIconGeometry);
}

static void
/* ARGSUSED */
updVirtualScale(dpy, item, cur, new)
    Display		*dpy;
    ResourceItem	*item;
    char		**cur, **new;
{
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualScale);
}

static void
/* ARGSUSED */
updVirtualDrawSticky(dpy, item, cur, new)
    Display		*dpy;
    ResourceItem	*item;
    char		**cur, **new;
{
    *cur = *new;
    setVirtualScreenAttribute(dpy, (FuncPtr) UpdateScreenVirtualDrawSticky);
}

static Bool
/* ARGSUSED */
cvtGridEnum(dpy, item, value, ret)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *value;
    VirtualGridType *ret;
{
	if (MatchString(value,"none"))
	{
	    *ret = GridNone;
	    return True;
	}
	if (MatchString(value,"invisible"))
	{
	    *ret = GridInvisible;
	    return True;
	}
	if (MatchString(value,"visible"))
	{
	    *ret = GridVisible;
	    return True;
	}
	return False;
}

static Bool
/* ARGSUSED */
cvtSortType(dpy, item, value, ret)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *value;
    SortType 	    *ret;
{
	if (MatchString(value,"alphabetic"))
	{
	    *ret = SortAlpha;
	    return True;
	}
	if (MatchString(value,"youngest"))
	{
	    *ret = SortYounger;
	    return True;
	}
	if (MatchString(value,"allalphabetic"))
	{
	    *ret = SortAlphaAll;
	    return True;
	}
	if (MatchString(value,"allyoungest"))
	{
	    *ret = SortYoungerAll;
	    return True;
	}
	return False;
}

static Bool
/* ARGSUSED */
cvtImageType(dpy, item, value, ret)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *value;
    ImageType 	    *ret;
{
	if (MatchString(value,"useall"))
	{
	    *ret = UseAll;
	    return True;
	}
	if (MatchString(value,"usenone"))
	{
	    *ret = UseNone;
	    return True;
	}
	if (MatchString(value,"usevdm"))
	{
	    *ret = UseVDM;
	    return True;
	}
	return False;
}

#ifdef NOT
static Bool
/* ARGSUSED */
cvtNoop(dpy, item, value, ret)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *value;
    void 	    *ret;
{
    return False;
}
#endif

static void
/* ARGSUSED */
updCursors(dpy, item, cur, new)
    Display		*dpy;
    ResourceItem	*item;
    char		**cur, **new;
{
    /* free the new string, as the cursors never update */
    MemFree(*new);
}

static void
updIconSlots(dpy, item, cur, new)
    Display		*dpy;
    ResourceItem	*item;
    char		**cur, **new;
{
    ScreenInfo      *si;
    extern List	    *ScreenInfoList;
    List	    *l = ScreenInfoList;
			   
    *cur = *new;

    for (si = ListEnum(&l); si; si = ListEnum(&l)) {
	SlotSetLocations(dpy,si->iconGrid);
    }
}
