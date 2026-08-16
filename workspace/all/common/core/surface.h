// core/surface.h — RAII owner for SDL_Surface (C++, header-only).
//
// SDL_Surface is a C resource freed with SDL_FreeSurface(). Scattering manual
// SDL_FreeSurface() calls through the launcher is exactly the ownership
// bookkeeping that leaks on an early return and double-frees when two variables
// alias the same surface — the class of bug Phase A's audit kept turning up.
//
// core::SurfacePtr is a std::unique_ptr specialization that frees the surface
// when it goes out of scope, moves ownership explicitly, and (via .get())
// hands a raw SDL_Surface* to the many C APIs — SDL_BlitSurface, the GFX_*
// helpers, IMG/TTF calls — that still take one.
//
// Usage:
//   core::SurfacePtr img{IMG_Load(path)};        // adopt a freshly-made surface
//   if (!img) return;                            // NULL-safe, like the raw ptr
//   SDL_BlitSurface(img.get(), NULL, dst, &r);   // hand the raw ptr to C APIs
//   thumb = std::move(img);                      // transfer ownership, no double free
//   // freed automatically at scope exit
//
// Ownership mirrors unique_ptr: exactly one owner, copy deleted, move transfers.
// Use .release() to hand ownership to a C API that will free it itself, and
// .reset(p) to replace the managed surface (frees the old one first).
//
// This header forward-declares SDL_Surface / SDL_FreeSurface so core/ stays
// clear of the SDL1-vs-SDL2 include tangle (see common/sdl.h); every real use
// site already includes SDL through api.h, which supplies the complete type the
// deleter needs at the point of destruction.
//
// Usable from any C++ translation unit. Include as: #include "core/surface.h"

#ifndef NEXTUI_CORE_SURFACE_H
#define NEXTUI_CORE_SURFACE_H

#include <memory>

struct SDL_Surface;
extern "C" void SDL_FreeSurface(SDL_Surface* surface);

namespace core {

// Deleter that returns an owned SDL_Surface to SDL. unique_ptr only invokes it
// on a non-null pointer, so no null guard is needed here.
struct SurfaceDeleter {
	void operator()(SDL_Surface* s) const { SDL_FreeSurface(s); }
};

// Move-only owning handle for an SDL_Surface. Frees on destruction.
using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

} // namespace core

#endif // NEXTUI_CORE_SURFACE_H
