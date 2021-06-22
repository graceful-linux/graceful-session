#include "num-lock.h"
#include <string.h>
#include <stdlib.h>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

/* the XKB stuff is based on code created by Oswald Buddenhagen <ossi@kde.org> */

static unsigned int xkb_mask_modifier(Display* /*dpy*/, XkbDescPtr xkb, const char *name )
{
    int i = 0;
    if( !xkb || !xkb->names )
        return 0;
    for( i = 0; i < XkbNumVirtualMods; i++ ) {
        char* modStr = XGetAtomName( xkb->dpy, xkb->names->vmods[i] );
        if( modStr != nullptr && strcmp(name, modStr) == 0 ) {
            unsigned int mask = 0;
            XkbVirtualModsToReal( xkb, 1 << i, &mask );
            return mask;
        }
    }
    return 0;
}

static unsigned int xkb_numlock_mask(Display* dpy)
{
    XkbDescPtr xkb = nullptr;
    if(( xkb = XkbGetKeyboard( dpy, XkbAllComponentsMask, XkbUseCoreKbd )) != nullptr ) {
        unsigned int mask = xkb_mask_modifier( dpy, xkb, "NumLock" );
        XkbFreeKeyboard( xkb, 0, True );
        return mask;
    }
    return 0;
}

static int xkb_set_on(Display* dpy)
{
    unsigned int mask = 0;
    mask = xkb_numlock_mask(dpy);
    if( mask == 0 )
        return 0;
    XkbLockModifiers ( dpy, XkbUseCoreKbd, mask, mask);
    return 1;
}

void enableNumlock()
{
    // this currently only works for X11
    if(QX11Info::isPlatformX11())
        xkb_set_on(QX11Info::display());
}
