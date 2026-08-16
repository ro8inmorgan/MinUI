// Desktop self-test for core/surface.h. Standalone — no SDL link needed:
//   g++ -std=c++17 -I../../ surface_test.cpp -o surface_test && ./surface_test
// Exits non-zero on the first failed check.
//
// We supply our own complete SDL_Surface plus a counting SDL_FreeSurface stub
// so the RAII semantics (single free, move transfer, reset, release) can be
// checked without pulling in real SDL.

struct SDL_Surface { int id; };

static int free_calls = 0;
extern "C" void SDL_FreeSurface(SDL_Surface* s) {
	if (s) { free_calls++; delete s; }
}

#include "core/surface.h"

#include <cstdio>
#include <utility>

static int failures = 0;
#define CHECK(cond) do { \
	if (!(cond)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } \
} while (0)

static SDL_Surface* make() { return new SDL_Surface{0}; }

int main() {
	// scope exit frees exactly once
	{
		free_calls = 0;
		{ core::SurfacePtr s{make()}; CHECK(free_calls == 0); }
		CHECK(free_calls == 1);
	}
	// get() exposes the raw pointer without giving up ownership
	{
		free_calls = 0;
		SDL_Surface* raw = make();
		core::SurfacePtr s{raw};
		CHECK(s.get() == raw);
		CHECK(static_cast<bool>(s));
		CHECK(free_calls == 0);
	} // freed here
	CHECK(free_calls == 1);
	// move transfers ownership — source empties, no double free
	{
		free_calls = 0;
		core::SurfacePtr a{make()};
		core::SurfacePtr b{std::move(a)};
		CHECK(a.get() == nullptr);
		CHECK(!a);
		CHECK(b.get() != nullptr);
		CHECK(free_calls == 0);
	} // only b frees
	CHECK(free_calls == 1);
	// move-assignment frees the old target before taking the new surface
	{
		free_calls = 0;
		core::SurfacePtr a{make()};
		core::SurfacePtr b{make()};
		b = std::move(a);      // b's original surface freed here
		CHECK(free_calls == 1);
	} // b (now holding a's surface) frees
	CHECK(free_calls == 2);
	// reset(p) frees the old surface and adopts the new one
	{
		free_calls = 0;
		core::SurfacePtr s{make()};
		s.reset(make());
		CHECK(free_calls == 1);
		s.reset();             // explicit empty
		CHECK(free_calls == 2);
		CHECK(!s);
	}
	CHECK(free_calls == 2);    // already empty, nothing more freed at scope exit
	// release() yields the pointer without freeing; caller owns it now
	{
		free_calls = 0;
		core::SurfacePtr s{make()};
		SDL_Surface* raw = s.release();
		CHECK(!s);
		CHECK(free_calls == 0);
		SDL_FreeSurface(raw);  // manual free
		CHECK(free_calls == 1);
	}
	// a default (null) handle frees nothing
	{
		free_calls = 0;
		{ core::SurfacePtr s; CHECK(!s); }
		CHECK(free_calls == 0);
	}

	if (failures == 0) printf("core/surface.h: all checks passed\n");
	return failures == 0 ? 0 : 1;
}
