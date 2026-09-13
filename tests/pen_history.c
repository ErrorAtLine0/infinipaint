#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include "../conan/sdl/3.x/pen_history_helpers.inc"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "Failed line %d\n", __LINE__); return 1; } } while(0)
int main(void) {
    IP_PenHistoryState *s = SDL_calloc(1, sizeof(*s));
    CHECK(s);
    POINTER_PEN_INFO a = {0};
    a.pointerInfo.pointerId = 2;
    a.pointerInfo.PerformanceCount = 50;
    a.pointerInfo.pointerFlags = POINTER_FLAG_INCONTACT;
    CHECK(!IP_SeenPenReport(s, &a));
    CHECK(IP_SeenPenReport(s, &a));
    a.pressure = 100; /* same timestamp, distinct pressure */
    CHECK(!IP_SeenPenReport(s, &a));
    a.pointerInfo.ptHimetricLocationRaw.x = 3;
    CHECK(!IP_SeenPenReport(s, &a));
    a.pointerInfo.pointerFlags = POINTER_FLAG_UP;
    CHECK(!IP_SeenPenReport(s, &a));
    a.pointerInfo.historyCount = 19;
    CHECK(IP_SeenPenReport(s, &a)); /* acquisition envelope is not new data */
    for (unsigned i=0; i<9000; ++i) {
        a.pointerInfo.PerformanceCount = 100+i;
        CHECK(!IP_SeenPenReport(s, &a));
    }
    CHECK(s->size == 4096);
    CHECK(IP_SeenPenReport(s, &a));
    IP_FreePenHistory(NULL, s);
    /* The cleanup callback is invoked by SDL on a failed property assignment. */
    s = SDL_calloc(1, sizeof(*s));
    CHECK(s);
    CHECK(!SDL_SetPointerPropertyWithCleanup(0, "test", s, IP_FreePenHistory, NULL));
    puts("Pen history checks passed");
    return 0;
}
