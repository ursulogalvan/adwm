/* See COPYING file for copyright and license details. */

#include "adwm.h"
#include "ewmh.h"
#include "layout.h"
#include "tags.h"
#include "actions.h"
#include "config.h"
#include "icons.h"
#include "draw.h"
#include "image.h"
#if 1
#include "ximage.h"
#else
#include "xlib.h"
#endif
#include "render.h" /* verification */

#if defined RENDER
void
render_removepixmap(AdwmPixmap *p)
{
	if (p->pixmap.pict) {
		XRenderFreePicture(dpy, p->pixmap.pict);
		p->pixmap.pict = None;
	}
	if (p->bitmap.pict) {
		XRenderFreePicture(dpy, p->bitmap.pict);
		p->bitmap.pict = None;
	}
}

Bool
render_createbitmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h)
{
	XImage *xdraw = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage *bi, **bis;
	AdwmPixmap *px;
	unsigned i, j, d = ds->depth, th = ds->style.titleheight;
	unsigned long pamask = 0;
	Pixmap pixmap;
	Picture pict;
	XRenderPictureAttributes pa = { 0, };

	if (!(xdraw = XGetImage(dpy, icon, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", icon, w, h);
		return (False);
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		XDestroyImage(xdraw);
		return (False);
	}
	if (mask && !(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		XDestroyImage(xdraw);
		XDestroyImage(xmask);
		return (False);
	}
	if (ximage && !(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
		EPRINTF("could not allocate ximage data %ux%ux%u\n", w, h, d);
		XDestroyImage(xdraw);
		XDestroyImage(xmask);
		XDestroyImage(ximage);
		return (False);
	}
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			if (XGetPixel(xdraw, i, j)) {
				XPutPixel(ximage, i, j, 0xffffffff);
			} else {
				XPutPixel(ximage, i, j, 0xff000000);
			}
			if (xmask && j < xmask->height && i < xmask->width && XGetPixel(xmask, i, j)) {
				XPutPixel(ximage, i, j,
					  XGetPixel(ximage, i, j) | 0xff000000);
			} else {
				XPutPixel(ximage, i, j,
					  XGetPixel(ximage, i, j) & 0x00ffffff);
			}
		}
	}
	XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
	pixmap = XCreatePixmap(dpy, ds->drawable, w, h, d);
	XPutImage(dpy, pixmap, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);
	XDestroyImage(ximage);

	pa.repeat = RepeatNone;
	pamask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	pamask |= CPPolyEdge;
	pa.component_alpha = True;
	pamask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, pixmap, ds->format, pamask, &pa);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		bi = *bis;
		px = &bi->px;
		px->x = px->y = px->b = 0;
		px->d = d;
		px->w = w;
		px->h = h;
		if (h > th) {
			XDouble scale = (XDouble) th / (XDouble) h;
			XTransform trans = {
				{ { XDoubleToFixed(scale), 0, 0 },
				  { 0, XDoubleToFixed(scale), 0 },
				  { 0, 0, XDoubleToFixed(1.0)   } }
			};
			XRenderSetPictureTransform(dpy, pict, &trans);
			XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
			px->w = w * scale;
			px->h = h * scale;
		}
		if (px->bitmap.pict) {
			XRenderFreePicture(dpy, px->bitmap.pict);
			px->bitmap.pict = None;
		}
		px->bitmap.pict = pict;
		bi->present = True;
	}
	return (True);
}

Bool
render_createpixmapicon(AScreen *ds, Client *c, Pixmap icon, Pixmap mask, unsigned w, unsigned h, unsigned d)
{
	XImage *xdraw = NULL, *xmask = NULL, *ximage = NULL;
	ButtonImage *bi, **bis;
	AdwmPixmap *px;
	unsigned i, j, th = ds->style.titleheight;
	unsigned long pamask = 0, pixel;
	Pixmap draw;
	Picture pict;
	XRenderPictureAttributes pa = { 0, };

	if (!(xdraw = XGetImage(dpy, icon, 0, 0, w, h, AllPlanes, ZPixmap))) {
		EPRINTF("could not get pixmap 0x%lx %ux%u\n", icon, w, h);
		return (False);
	}
	if (mask && !(xmask = XGetImage(dpy, mask, 0, 0, w, h, 0x1, XYPixmap))) {
		EPRINTF("could not get bitmap 0x%lx %ux%u\n", mask, w, h);
		return (False);
	}
	if (mask && !(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		XDestroyImage(xdraw);
		XDestroyImage(xmask);
		return (False);
	}
	if (ximage && !(ximage->data = emallocz(ximage->bytes_per_line * ximage->height))) {
		EPRINTF("could not allocate ximage data %ux%ux%u\n", w, h, d);
		XDestroyImage(xdraw);
		XDestroyImage(xmask);
		XDestroyImage(ximage);
		return (False);
	}
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			pixel = XGetPixel(xdraw, i, j);
			if (xmask && j < xmask->height && i < xmask->width && XGetPixel(xmask, i, j)) {
				XPutPixel(ximage, i, j, pixel | 0xff000000);
			} else {
				XPutPixel(ximage, i, j, pixel & 0x00ffffff);
			}
		}
	}
	XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
	draw = XCreatePixmap(dpy, ds->drawable, w, h, ds->depth);
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);
	XDestroyImage(ximage);

	pa.repeat = RepeatNone;
	pamask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	pamask |= CPPolyEdge;
	pa.component_alpha = True;
	pamask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, draw, ds->format, pamask, &pa);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		bi = *bis;
		px = &bi->px;
		px->x = px->y = px->b = 0;
		px->d = ds->depth;
		px->w = w;
		px->h = h;
		if (h > th) {
			/* get XRender to scale the image for us */
			XDouble scale = (XDouble) th / (XDouble) h;
			XTransform trans = {
				{ { XDoubleToFixed(scale), 0, 0 },
				  { 0, XDoubleToFixed(scale), 0 },
				  { 0, 0, XDoubleToFixed(1.0)   } }
			};
			XRenderSetPictureTransform(dpy, pict, &trans);
			XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
			px->w = w * scale;
			px->h = h * scale;
		}
		if (px->pixmap.pict) {
			XRenderFreePicture(dpy, px->pixmap.pict);
			px->pixmap.pict = None;
		}
		px->pixmap.pict = pict;
		bi->present = True;
	}
	return (True);
}

Bool
render_createdataicon(AScreen *ds, Client *c, unsigned w, unsigned h, long *data)
{
	XImage *ximage = NULL;
	ButtonImage *bi, **bis;
	AdwmPixmap *px;
	unsigned i, j, d = ds->depth, th = ds->style.titleheight;
	unsigned long mask = 0;
	long *p;
	Pixmap draw;
	Picture pict;
	XRenderPictureAttributes pa = { 0, };

	if (!(ximage = XCreateImage(dpy, ds->visual, d, ZPixmap, 0, NULL, w, h, 8, 0))) {
		EPRINTF("could not create ximage %ux%ux%u\n", w, h, d);
		return (False);
	}
	for (p = data, j = 0; j < h; j++)
		for (i = 0; i < w; i++, p++)
			XPutPixel(ximage, i, j, *p);

	draw = XCreatePixmap(dpy, ds->drawable, w, h, d);
	XPutImage(dpy, draw, ds->dc.gc, ximage, 0, 0, 0, 0, w, h);
	XDestroyImage(ximage);

	pa.repeat = RepeatNone;
	mask |= CPRepeat;
	pa.poly_edge = PolyEdgeSmooth;
	mask |= CPPolyEdge;
	pa.component_alpha = True;
	mask |= CPComponentAlpha;
	pict = XRenderCreatePicture(dpy, draw, ds->format, mask, &pa);

	for (bis = getbuttons(c); bis && *bis; bis++) {
		bi = *bis;
		px = &bi->px;
		px->x = px->y = px->b = 0;
		px->d = d;
		px->w = w;
		px->h = h;
		if (h > th) {
			/* get XRender to scale the image for us */
			XDouble scale = (XDouble) th / (XDouble) h;
			XTransform trans = {
				{ { XDoubleToFixed(scale), 0, 0 },
				  { 0, XDoubleToFixed(scale), 0 },
				  { 0, 0, XDoubleToFixed(1.0)   } }
			};
			XRenderSetPictureTransform(dpy, pict, &trans);
			XRenderSetPictureFilter(dpy, pict, FilterBilinear, NULL, 0);
			px->w = w * scale;
			px->h = h * scale;
		}
		if (px->pixmap.pict) {
			XRenderFreePicture(dpy, px->pixmap.pict);
			px->pixmap.pict = None;
		}
		px->pixmap.pict = pict;
		bi->present = True;
	}
	return (True);
}

Bool
render_createpngicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create PNG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
render_createsvgicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create SVG icon %s\n", file);
	/* for now */
	return (False);
}

Bool
render_createxpmicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create XPM icon %s\n", file);
	/* for now */
	return (False);
}

Bool
render_createxbmicon(AScreen *ds, Client *c, const char *file)
{
	EPRINTF("would use RENDER to create XBM icon %s\n", file);
	/* for now */
	return (False);
}

#ifdef DAMAGE
Bool
render_drawdamage(Client *c, XDamageNotifyEvent * ev)
{
	return (False);
}
#endif

int
render_drawbutton(AScreen *ds, Client *c, ElementType type, XftColor *col, int x)
{
	ElementClient *ec = &c->element[type];
	XftColor *fg;
	Geometry g = { 0, };
	ButtonImage *bi;
	AdwmPixmap *px;
	int th;

	if (!(bi = buttonimage(ds, c, type)) || !bi->present) {
		XPRINTF("button %d has no button image\n", type);
		return 0;
	}
	px = &bi->px;

	th = ds->dc.h;
	if (ds->style.outline)
		th--;

	/* geometry of the container */
	g.x = x;
	g.y = 0;
	g.w = max(th, px->w);
	g.h = th;

	/* element client geometry used to detect element under pointer */
	ec->eg.x = g.x + (g.w - px->w) / 2;
	ec->eg.y = g.y + (g.h - px->h) / 2;
	ec->eg.w = px->w;
	ec->eg.h = px->h;

	fg = ec->pressed ? &col[ColFG] : &col[ColButton];

	if (px->pixmap.pict) {
		Picture dst = XftDrawPicture(ds->dc.draw.xft);

		XRenderComposite(dpy, PictOpOver, px->pixmap.pict, None, dst,
				0, 0, 0, 0, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
	} else
	if (px->bitmap.pict) {
		Picture dst = XftDrawPicture(ds->dc.draw.xft);
		Picture fill = XRenderCreateSolidFill(dpy,  &fg->color);

		XRenderComposite(dpy, PictOpOver, fill, px->bitmap.pict, dst,
				0, 0, 0, 0, ec->eg.x, ec->eg.y, ec->eg.w, ec->eg.h);
		XRenderFreePicture(dpy, fill);
	}
	XPRINTF("button %d has no pixmap or bitmap\n", type);
	return 0;
}

int
render_drawtext(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw,
	 XftColor *col, int hilite, int x, int y, int mw)
{
	int w, h;
	char buf[256];
	unsigned int len, olen;
	int gap;
	XftFont *font = ds->style.font[hilite];
	struct _FontInfo *info = &ds->dc.font[hilite];
	XftColor *fcol = ds->style.color.hue[hilite];
	int drop = ds->style.drop[hilite];

	if (!text)
		return 0;
	olen = len = strlen(text);
	w = 0;
	if (len >= sizeof buf)
		len = sizeof buf - 1;
	memcpy(buf, text, len);
	buf[len] = 0;
	h = ds->style.titleheight;
	y = ds->dc.h / 2 + info->ascent / 2 - 1 - ds->style.outline;
	gap = info->height / 2;
	x += gap;
	/* shorten text if necessary */
	while (len && (w = textnw(ds, buf, len, hilite)) > mw) {
		buf[--len] = 0;
	}
	if (len < olen) {
		if (len > 1)
			buf[len - 1] = '.';
		if (len > 2)
			buf[len - 2] = '.';
		if (len > 3)
			buf[len - 3] = '.';
	}
	if (w > mw)
		return 0;	/* too long */
	while (x <= 0)
		x = ds->dc.x++;

	Picture dst = XftDrawPicture(xftdraw);
	Picture src = XftDrawSrcPicture(xftdraw, &fcol[ColFG]);
	Picture shd = XftDrawSrcPicture(xftdraw, &fcol[ColShadow]);

	if (dst && src) {
		XRenderFillRectangle(dpy, PictOpSrc, dst, &col[ColBG].color, x - gap, 0,
				     w + gap * 2, h);
		if (drop)
			XftTextRenderUtf8(dpy, PictOpOver, shd, font, dst, 0, 0, x + drop,
					  y + drop, (FcChar8 *) buf, len);
		XftTextRenderUtf8(dpy, PictOpOver, src, font, dst, 0, 0, x + drop,
				  y + drop, (FcChar8 *) buf, len);
	}
	return w + gap * 2;
}

int
render_drawsep(AScreen *ds, const char *text, Drawable drawable, XftDraw *xftdraw, XftColor *col, int hilite, int x, int y, int mw)
{
	return ximage_drawsep(ds, text, drawable, xftdraw, col, hilite, x, y, mw);
}

void
render_drawdockapp(AScreen *ds, Client *c)
{
	int status;
	unsigned long pixel;

	pixel = getpixel(ds, c, ColBG);
	/* to avoid clearing window, initiallly set to norm[ColBG] */
	// XSetWindowBackground(dpy, c->frame, pixel);
	// XClearArea(dpy, c->frame, 0, 0, 0, 0, True);

	ds->dc.x = ds->dc.y = 0;
	if (!(ds->dc.w = c->c.w))
		return;
	if (!(ds->dc.h = c->c.h))
		return;
	XSetForeground(dpy, ds->gc, pixel);
	XSetLineAttributes(dpy, ds->gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->gc, FillSolid);
	status = XFillRectangle(dpy, c->frame, ds->gc,
			ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	CPRINTF(c, "Filled dockapp frame %dx%d+%d+%d\n", ds->dc.w, ds->dc.h, ds->dc.x,
		ds->dc.y);
	/* note that ParentRelative dockapps need the background set to the foregroudn */
	XSetWindowBackground(dpy, c->frame, pixel);
	/* following are quite disruptive - many dockapps ignore expose events and simply
	   update on timer */
	// XClearWindow(dpy, c->icon ? : c->win);
	// XClearArea(dpy, c->icon ? : c->win, 0, 0, 0, 0, True);
}

void
render_drawnormal(AScreen *ds, Client *c)
{
	size_t i;
	int status;

	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.titleheight;
	if (ds->dc.draw.w < ds->dc.w) {
		XFreePixmap(dpy, ds->dc.draw.pixmap);
		ds->dc.draw.w = ds->dc.w;
		XPRINTF(__CFMTS(c) "creating title pixmap %dx%dx%d\n", __CARGS(c),
				ds->dc.w, ds->dc.draw.h, ds->depth);
		ds->dc.draw.pixmap = XCreatePixmap(dpy, ds->root, ds->dc.w,
						   ds->dc.draw.h, ds->depth);
		XftDrawChange(ds->dc.draw.xft, ds->dc.draw.pixmap);
	}
	XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBG));
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc,
				ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	/* Don't know about this... */
	if (ds->dc.w < textw(ds, c->name, gethilite(c))) {
		ds->dc.w -= elementw(ds, c, CloseBtn);
		render_drawtext(ds, c->name, ds->dc.draw.pixmap, ds->dc.draw.xft,
				gethues(ds, c), gethilite(c), ds->dc.x, ds->dc.y, ds->dc.w);
		render_drawbutton(ds, c, CloseBtn, gethues(ds, c), ds->dc.w);
		goto end;
	}
	/* Left */
	ds->dc.x += (ds->style.spacing > ds->style.border) ?
	    ds->style.spacing - ds->style.border : 0;
	for (i = 0; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x +=
		    drawelement(ds, ds->style.titlelayout[i], ds->dc.x, AlignLeft, c);
		ds->dc.x += ds->style.spacing;
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Center */
	ds->dc.x = ds->dc.w / 2;
	for (i++; i < strlen(ds->style.titlelayout); i++) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]) / 2;
		ds->dc.x += drawelement(ds, ds->style.titlelayout[i], 0, AlignCenter, c);
	}
	if (i == strlen(ds->style.titlelayout) || ds->dc.x >= ds->dc.w)
		goto end;
	/* Right */
	ds->dc.x = ds->dc.w;
	ds->dc.x -= (ds->style.spacing > ds->style.border) ?
	    ds->style.spacing - ds->style.border : 0;
	for (i = strlen(ds->style.titlelayout); i--;) {
		if (ds->style.titlelayout[i] == ' ' || ds->style.titlelayout[i] == '-')
			break;
		ds->dc.x -= elementw(ds, c, ds->style.titlelayout[i]);
		drawelement(ds, ds->style.titlelayout[i], 0, AlignRight, c);
		ds->dc.x -= ds->style.spacing;
	}
      end:
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBorder));
		XPRINTF(__CFMTS(c) "drawing line from +%d+%d to +%d+%d\n", __CARGS(c),
				0, ds->dc.h - 1, ds->dc.w, ds->dc.h - 1);
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, ds->dc.h - 1, ds->dc.w,
			  ds->dc.h - 1);
	}
	if (c->title) {
		XPRINTF(__CFMTS(c) "copying title pixmap to %dx%d+%d+%d to +%d+%d\n",
			__CARGS(c), c->c.w, ds->dc.h, 0, 0, 0, 0);
		XCopyArea(dpy, ds->dc.draw.pixmap, c->title, ds->dc.gc, 0, 0, c->c.w,
			  ds->dc.h, 0, 0);
	}
	ds->dc.x = ds->dc.y = 0;
	ds->dc.w = c->c.w;
	ds->dc.h = ds->style.gripsheight;
	XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBG));
	XSetLineAttributes(dpy, ds->dc.gc, ds->style.border, LineSolid, CapNotLast,
			   JoinMiter);
	XSetFillStyle(dpy, ds->dc.gc, FillSolid);
	status = XFillRectangle(dpy, ds->dc.draw.pixmap, ds->dc.gc,
				ds->dc.x, ds->dc.y, ds->dc.w, ds->dc.h);
	if (!status)
		XPRINTF("Could not fill rectangle, error %d\n", status);
	if (ds->style.outline) {
		XSetForeground(dpy, ds->dc.gc, getpixel(ds, c, ColBorder));
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, 0, 0, ds->dc.w, 0);
		/* needs to be adjusted to do ds->style.gripswidth instead */
		ds->dc.x = ds->dc.w / 2 - ds->dc.w / 5;
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, 0, ds->dc.x,
			  ds->dc.h);
		ds->dc.x = ds->dc.w / 2 + ds->dc.w / 5;
		XDrawLine(dpy, ds->dc.draw.pixmap, ds->dc.gc, ds->dc.x, 0, ds->dc.x,
			  ds->dc.h);
	}
	if (c->grips)
		XCopyArea(dpy, ds->dc.draw.pixmap, c->grips, ds->dc.gc, 0, 0, c->c.w,
			  ds->dc.h, 0, 0);
}

Bool
render_initpng(char *path, AdwmPixmap *px)
{
#ifdef LIBPNG
	Pixmap draw = None;
	Picture pict = None;
	XRenderPictureAttributes pa = { 0, };
	unsigned long pamask = 0;

	XImage *ximage;
	unsigned w, h, d = scr->depth;

	ximage = png_read_file_to_ximage(dpy, scr->visual, path);
	if (!ximage) {
		EPRINTF("could not read png file %s\n", path);
		goto error;
	}
	w = ximage->width;
	h = ximage->height;

	draw = XCreatePixmap(dpy, scr->drawable, w, h, d);
	if (!draw) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, scr->dc.gc, ximage, 0, 0, 0, 0, w, h);
	XDestroyImage(ximage);
	ximage = NULL;

	pa.repeat = RepeatNone;
	pa.poly_edge = PolyEdgeSmooth;
	pa.component_alpha = True;
	pamask = CPRepeat | CPPolyEdge | CPComponentAlpha;

	pict = XRenderCreatePicture(dpy, draw, scr->format, pamask, &pa);
	if (!pict) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->pixmap.pict) {
		XRenderFreePicture(dpy, px->pixmap.pict);
		px->pixmap.pict = None;
	}
	if (px->pixmap.draw) {
		XFreePixmap(dpy, px->pixmap.draw);
		if (px->pixmap.mask == px->pixmap.draw)
			px->pixmap.mask = None;
		px->pixmap.draw = None;
	}
	if (px->pixmap.mask) {
		XFreePixmap(dpy, px->pixmap.mask);
		px->pixmap.mask = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->pixmap.pict = pict;
	px->pixmap.draw = draw;
	px->pixmap.mask = None;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = w;
	px->h = h;
	if (px->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - scr->style.titleheight) / 2;
		px->h = scr->style.titleheight;
	}
	return True;
error:
	if (ximage)
		XDestroyImage(ximage);
	if (draw)
		XFreePixmap(dpy, draw);
#endif
	return (False);
}

Bool
render_initsvg(char *path, AdwmPixmap *px)
{
	return (False);
}

Bool
render_initxpm(char *path, AdwmPixmap *px)
{
#ifdef XPM
	Pixmap draw = None;
	Picture pict = None;
	XRenderPictureAttributes pa = { 0, };
	unsigned long pamask = 0;

	XImage *xdraw = NULL, *xmask = NULL, *ximage = NULL;
	unsigned w, h, d = scr->depth;
	int status;

	{
		XpmAttributes xa = { 0, };

		xa.visual = scr->visual;
		xa.valuemask |= XpmVisual;
		xa.colormap = scr->colormap;
		xa.valuemask |= XpmColormap;
		xa.depth = scr->depth;
		xa.valuemask |= XpmDepth;

		status = XpmReadFileToImage(dpy, path, &xdraw, &xmask, &xa);
		if (status != XpmSuccess || !xdraw) {
			EPRINTF("could not load xpm file %s\n", path);
			goto error;
		}
		XpmFreeAttributes(&xa);
	}
	ximage = combine_pixmap_and_mask(dpy, scr->visual, xdraw, xmask);
	if (!ximage) {
		EPRINTF("could not combine images\n");
		goto error;
	}
	w = ximage->width;
	h = ximage->height;

	XDestroyImage(xdraw);
	xdraw = NULL;
	if (xmask) {
		XDestroyImage(xmask);
		xmask = NULL;
	}

	draw = XCreatePixmap(dpy, scr->drawable, w, h, d);
	if (!draw) {
		EPRINTF("could not create pixmap\n");
		goto error;
	}
	XPutImage(dpy, draw, scr->dc.gc, ximage, 0, 0, 0, 0, w, h);
	XDestroyImage(ximage);
	ximage = NULL;

	pa.repeat = RepeatNone;
	pa.poly_edge = PolyEdgeSmooth;
	pa.component_alpha = True;
	pamask = CPRepeat | CPPolyEdge | CPComponentAlpha;

	pict = XRenderCreatePicture(dpy, draw, scr->format, pamask, &pa);
	if (!pict) {
		EPRINTF("could not create picture\n");
		goto error;
	}
	if (px->pixmap.pict) {
		XRenderFreePicture(dpy, px->pixmap.pict);
		px->pixmap.pict = None;
	}
	if (px->pixmap.draw) {
		XFreePixmap(dpy, px->pixmap.draw);
		if (px->pixmap.mask == px->pixmap.draw)
			px->pixmap.mask = None;
		px->pixmap.draw = None;
	}
	if (px->pixmap.mask) {
		XFreePixmap(dpy, px->pixmap.mask);
		px->pixmap.mask = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->pixmap.pict = pict;
	px->pixmap.draw = draw;
	px->pixmap.mask = None;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = w;
	px->h = h;
	if (px->h > scr->style.titleheight) {
		/* read lower down into image to clip top and bottom by same amount */
		px->y += (px->h - scr->style.titleheight) / 2;
		px->h = scr->style.titleheight;
	}
	return True;
      error:
	if (xdraw)
		XDestroyImage(xdraw);
	if (xmask)
		XDestroyImage(xmask);
	if (ximage)
		XDestroyImage(ximage);
	if (draw)
		XFreePixmap(dpy, draw);
#endif				/* !XPM */
	return (False);
}

Bool
render_initxbm(char *path, AdwmPixmap *px)
{
	Pixmap draw;
	Picture pict;
	XRenderPictFormat *format;
	XRenderPictureAttributes pa = { 0, };
	unsigned long pamask = 0;
	int status;

	status = XReadBitmapFile(dpy, scr->root, path, &px->w, &px->h, &draw, &px->x, &px->y);
	if (status != BitmapSuccess || !draw) {
		EPRINTF("could not load xbm file %s\n", path);
		return False;
	}
	format = XRenderFindStandardFormat(dpy, PictStandardA1);

	pa.repeat = RepeatNone;
	pa.graphics_exposures = False;
	pa.poly_edge = PolyEdgeSharp;
	pa.component_alpha = True;
	pamask = CPRepeat|CPGraphicsExposure|CPPolyEdge|CPComponentAlpha;

	pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa);
	if (!pict) {
		XFreePixmap(dpy, draw);
		return False;
	}
	if (px->bitmap.pict) {
		XRenderFreePicture(dpy, px->bitmap.pict);
		px->bitmap.pict = None;
	}
	if (px->bitmap.draw) {
		XFreePixmap(dpy, px->bitmap.draw);
		if (px->bitmap.mask == px->pixmap.draw)
			px->bitmap.mask = None;
		px->bitmap.draw = None;
	}
	if (px->bitmap.mask) {
		XFreePixmap(dpy, px->bitmap.mask);
		px->bitmap.mask = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->bitmap.pict = pict;
	px->bitmap.draw = draw;
	px->bitmap.mask = None;
	px->file = path;
	px->x = px->y = px->b = 0;
	px->w = px->w;
	px->h = px->h;
	return (True);
}

Bool
render_initxbmdata(const unsigned char *bits, int w, int h, AdwmPixmap *px)
{
	Pixmap draw;
	Picture pict;
	XRenderPictFormat *format;
	XRenderPictureAttributes pa = { 0, };
	unsigned long pamask = 0;

	if (!(draw = XCreateBitmapFromData(dpy, scr->drawable, (char *)bits, w, h))) {
		EPRINTF("could not load xbm data\n");
		return False;
	}
	format = XRenderFindStandardFormat(dpy, PictStandardA1);

	pa.repeat = RepeatNone;
	pa.graphics_exposures = False;
	pa.poly_edge = PolyEdgeSharp;
	pa.component_alpha = True;
	pamask = CPRepeat|CPGraphicsExposure|CPPolyEdge|CPComponentAlpha;

	pict = XRenderCreatePicture(dpy, draw, format, pamask, &pa);
	if (!pict) {
		XFreePixmap(dpy, draw);
		return False;
	}
	if (px->bitmap.pict) {
		XRenderFreePicture(dpy, px->bitmap.pict);
		px->bitmap.pict = None;
	}
	if (px->bitmap.draw) {
		XFreePixmap(dpy, px->bitmap.draw);
		if (px->bitmap.mask == px->pixmap.draw)
			px->bitmap.mask = None;
		px->bitmap.draw = None;
	}
	if (px->bitmap.mask) {
		XFreePixmap(dpy, px->bitmap.mask);
		px->bitmap.mask = None;
	}
	if (px->file) {
		free(px->file);
		px->file = NULL;
	}
	px->bitmap.pict = pict;
	px->bitmap.draw = draw;
	px->bitmap.mask = None;
	px->x = px->y = px->b = 0;
	px->w = w;
	px->h = h;
	return (True);
}

#endif				/* defined RENDER */

