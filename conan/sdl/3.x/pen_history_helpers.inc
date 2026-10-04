/* InfiniPaint Windows pen history state. Included at file scope in SDL.
 * SDL retains its original zlib license; these additions are GPL-3.0-or-later.
 */
typedef struct IP_PenHistoryState {
    POINTER_PEN_INFO recent[4096];
    UINT32 size, next, pointer;
    HANDLE device;
    Uint64 qpc_origin, ticks_origin;
} IP_PenHistoryState;

static void SDLCALL IP_FreePenHistory(void *userdata, void *value)
{
    (void)userdata;
    SDL_free(value);
}

static bool IP_SamePenReport(const POINTER_PEN_INFO *a, const POINTER_PEN_INFO *b)
{
    const POINTER_INFO *x = &a->pointerInfo, *y = &b->pointerInfo;
    /* Do not compare padding or historyCount (an acquisition envelope).
     * Equal timestamps alone are NOT duplicate evidence. */
    return x->pointerId == y->pointerId && x->sourceDevice == y->sourceDevice &&
        x->frameId == y->frameId && x->PerformanceCount == y->PerformanceCount &&
        x->dwTime == y->dwTime && x->pointerFlags == y->pointerFlags &&
        x->ButtonChangeType == y->ButtonChangeType &&
        x->ptHimetricLocationRaw.x == y->ptHimetricLocationRaw.x &&
        x->ptHimetricLocationRaw.y == y->ptHimetricLocationRaw.y &&
        x->ptPixelLocationRaw.x == y->ptPixelLocationRaw.x &&
        x->ptPixelLocationRaw.y == y->ptPixelLocationRaw.y &&
        a->penFlags == b->penFlags && a->penMask == b->penMask &&
        a->pressure == b->pressure && a->rotation == b->rotation &&
        a->tiltX == b->tiltX && a->tiltY == b->tiltY;
}

static bool IP_SeenPenReport(IP_PenHistoryState *state, const POINTER_PEN_INFO *sample)
{
    if (!state) return false;
    for (UINT32 i = 0; i < state->size; ++i) {
        if (IP_SamePenReport(&state->recent[i], sample)) return true;
    }
    state->recent[state->next] = *sample;
    state->next = (state->next + 1) % 4096;
    if (state->size < 4096) ++state->size;
    return false;
}
