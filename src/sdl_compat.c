#include "system.h"

#undef SDL_UpdateWindowSurface
#undef SDL_UpdateWindowSurfaceRects

static SDL_Window *fd_window = NULL;
static SDL_Surface *fd_framebuffer = NULL;
static SDL_Surface *fd_window_surface = NULL;
static SDL_Rect fd_present_dst = {0, 0, 0, 0};
static int fd_logical_w = 0;
static int fd_logical_h = 0;

static bool FD_RefreshPresentationState(void);
static void FD_UpdatePresentationRect(void);
static void FD_MapLogicalRectToPhysical(const SDL_Rect *logical, SDL_Rect *physical);
static bool FD_Present(const SDL_Rect *rects, int numrects);

static bool
FD_RefreshPresentationState(void)
{
	if (fd_window == NULL) {
		return false;
	}

	fd_window_surface = SDL_GetWindowSurface(fd_window);
	if (fd_window_surface == NULL) {
		return false;
	}

	if (fd_logical_w <= 0 || fd_logical_h <= 0) {
		return SDL_SetError("Invalid logical framebuffer size");
	}

	if (fd_framebuffer == NULL ||
	    fd_framebuffer->w != fd_logical_w ||
	    fd_framebuffer->h != fd_logical_h ||
	    fd_framebuffer->format != fd_window_surface->format) {
		SDL_Surface *new_fb;

		new_fb = SDL_CreateSurface(fd_logical_w, fd_logical_h, fd_window_surface->format);
		if (new_fb == NULL) {
			return false;
		}

		if (fd_framebuffer != NULL) {
			SDL_BlitSurface(fd_framebuffer, NULL, new_fb, NULL);
			SDL_DestroySurface(fd_framebuffer);
		}

		fd_framebuffer = new_fb;
	}

	FD_UpdatePresentationRect();
	return true;
}

static void
FD_UpdatePresentationRect(void)
{
	int scaled_w, scaled_h;
	double scale_x, scale_y, scale;

	fd_present_dst.x = 0;
	fd_present_dst.y = 0;
	fd_present_dst.w = 0;
	fd_present_dst.h = 0;

	if (fd_window_surface == NULL || fd_logical_w <= 0 || fd_logical_h <= 0) {
		return;
	}

	scale_x = (double)fd_window_surface->w / (double)fd_logical_w;
	scale_y = (double)fd_window_surface->h / (double)fd_logical_h;
	scale = (scale_x < scale_y) ? scale_x : scale_y;
	if (scale <= 0.0) {
		return;
	}

	scaled_w = (int)((double)fd_logical_w * scale + 0.5);
	scaled_h = (int)((double)fd_logical_h * scale + 0.5);
	if (scaled_w < 1) scaled_w = 1;
	if (scaled_h < 1) scaled_h = 1;

	fd_present_dst.w = scaled_w;
	fd_present_dst.h = scaled_h;
	fd_present_dst.x = (fd_window_surface->w - scaled_w) / 2;
	fd_present_dst.y = (fd_window_surface->h - scaled_h) / 2;
}

static void
FD_MapLogicalRectToPhysical(const SDL_Rect *logical, SDL_Rect *physical)
{
	int logical_x2, logical_y2;
	int physical_x2, physical_y2;

	if (physical == NULL) {
		return;
	}

	if (logical == NULL || fd_logical_w <= 0 || fd_logical_h <= 0 ||
	    fd_present_dst.w <= 0 || fd_present_dst.h <= 0) {
		*physical = fd_present_dst;
		return;
	}

	logical_x2 = logical->x + logical->w;
	logical_y2 = logical->y + logical->h;

	physical->x = fd_present_dst.x + (logical->x * fd_present_dst.w) / fd_logical_w;
	physical->y = fd_present_dst.y + (logical->y * fd_present_dst.h) / fd_logical_h;
	physical_x2 = fd_present_dst.x + (logical_x2 * fd_present_dst.w + fd_logical_w - 1) / fd_logical_w;
	physical_y2 = fd_present_dst.y + (logical_y2 * fd_present_dst.h + fd_logical_h - 1) / fd_logical_h;

	if (physical->x < fd_present_dst.x) physical->x = fd_present_dst.x;
	if (physical->y < fd_present_dst.y) physical->y = fd_present_dst.y;
	if (physical_x2 > fd_present_dst.x + fd_present_dst.w) physical_x2 = fd_present_dst.x + fd_present_dst.w;
	if (physical_y2 > fd_present_dst.y + fd_present_dst.h) physical_y2 = fd_present_dst.y + fd_present_dst.h;

	physical->w = physical_x2 - physical->x;
	physical->h = physical_y2 - physical->y;
}

static bool
FD_Present(const SDL_Rect *rects, int numrects)
{
	Uint32 black;
	SDL_Rect *mapped_rects = NULL;
	bool ok = true;
	int i, mapped_count = 0;

	if (!FD_RefreshPresentationState()) {
		return false;
	}

	black = SDL_MapRGB(fd_window_surface->format, 0, 0, 0);
	if (!SDL_FillSurfaceRect(fd_window_surface, NULL, black)) {
		return false;
	}

	if (fd_present_dst.w == fd_window_surface->w &&
	    fd_present_dst.h == fd_window_surface->h &&
	    fd_present_dst.x == 0 &&
	    fd_present_dst.y == 0) {
		ok = SDL_BlitSurface(fd_framebuffer, NULL, fd_window_surface, NULL);
	} else {
		ok = SDL_BlitSurfaceScaled(fd_framebuffer, NULL, fd_window_surface, &fd_present_dst, SDL_SCALEMODE_NEAREST);
	}

	if (!ok) {
		return false;
	}

	if (rects == NULL || numrects <= 0) {
		return SDL_UpdateWindowSurface(fd_window);
	}

	mapped_rects = SDL_malloc((size_t)numrects * sizeof(*mapped_rects));
	if (mapped_rects == NULL) {
		return SDL_SetError("Out of memory mapping window update rects");
	}

	for (i = 0; i < numrects; i++) {
		FD_MapLogicalRectToPhysical(&rects[i], &mapped_rects[mapped_count]);
		if (mapped_rects[mapped_count].w > 0 && mapped_rects[mapped_count].h > 0) {
			mapped_count++;
		}
	}

	if (mapped_count == 0) {
		ok = true;
	} else {
		ok = SDL_UpdateWindowSurfaceRects(fd_window, mapped_rects, mapped_count);
	}

	SDL_free(mapped_rects);
	return ok;
}

SDL_Surface *
FD_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
	bool want_fullscreen = ((flags & SDL_FULLSCREEN) != 0);

	(void)bpp;

	if (!fd_window) {
		fd_window = SDL_CreateWindow("Freedroid", width, height, 0);
		if (!fd_window)
			return NULL;
	}

	fd_logical_w = width;
	fd_logical_h = height;

	if (!SDL_SetWindowFullscreenMode(fd_window, NULL))
		return NULL;

	if (!want_fullscreen && !SDL_SetWindowSize(fd_window, width, height))
		return NULL;

	if (!SDL_SetWindowFullscreen(fd_window, want_fullscreen))
		return NULL;

	if (!SDL_SyncWindow(fd_window))
		return NULL;

	if (!FD_RefreshPresentationState())
		return NULL;

	if (!FD_Present(NULL, 0))
		return NULL;

	return fd_framebuffer;
}

SDL_Window *
FD_GetWindow(void)
{
	return fd_window;
}

void
FD_DestroyWindow(void)
{
	if (fd_framebuffer)
	{
		SDL_DestroySurface(fd_framebuffer);
		fd_framebuffer = NULL;
	}

	if (fd_window)
	{
		SDL_DestroyWindow(fd_window);
		fd_window = NULL;
	}

	fd_window_surface = NULL;
	fd_logical_w = 0;
	fd_logical_h = 0;
	fd_present_dst.x = 0;
	fd_present_dst.y = 0;
	fd_present_dst.w = 0;
	fd_present_dst.h = 0;
}

bool
FD_UpdateWindowSurface(SDL_Window *window)
{
	if (window != fd_window) {
		return SDL_SetError("FD_UpdateWindowSurface called with unexpected window");
	}

	return FD_Present(NULL, 0);
}

bool
FD_UpdateWindowSurfaceRects(SDL_Window *window, const SDL_Rect *rects, int numrects)
{
	if (window != fd_window) {
		return SDL_SetError("FD_UpdateWindowSurfaceRects called with unexpected window");
	}

	return FD_Present(rects, numrects);
}
