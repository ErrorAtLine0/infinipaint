/* InfiniPaint modification of SDL's Windows pen message handler.
 * Included inside WIN_WindowProc, replacing the WM_POINTER pen case.
 * Keep the SDL source's original license notice when distributing SDL.
 */
    case WM_POINTERDOWN:
    case WM_POINTERUP:
    case WM_POINTERUPDATE: {
        const UINT32 pointerid = GET_POINTERID_WPARAM(wParam);
        POINTER_INPUT_TYPE pointer_type = PT_POINTER;
        if (!data->videodata->GetPointerType || !data->videodata->GetPointerType(pointerid, &pointer_type)) {
            break;
        } else if ((msg == WM_POINTERUPDATE) && (pointer_type == PT_MOUSE)) {
            data->last_pointer_update = lParam;
            returnCode = 0;
            break;
        } else if (pointer_type != PT_PEN) {
            break;
        }
        const SDL_PenID pen = SDL_FindPenByHandle((void *)(size_t)1);
        POINTER_PEN_INFO latest;
        if (!pen || !data->videodata->GetPointerPenInfo || !data->videodata->GetPointerPenInfo(pointerid, &latest)) break;

        typedef BOOL (WINAPI *PenHistoryFn)(UINT32, UINT32 *, POINTER_PEN_INFO *);
        PenHistoryFn history_fn = (PenHistoryFn)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetPointerPenInfoHistory");
        UINT32 count = 0;
        POINTER_PEN_INFO *history = NULL;
        if (history_fn && history_fn(pointerid, &count, NULL) && count && count <= 4096) {
            for (unsigned attempt = 0; attempt < 3; ++attempt) {
                const UINT32 capacity = count;
                history = (POINTER_PEN_INFO *)SDL_malloc(capacity * sizeof(*history));
                if (!history) break;
                const bool ok = history_fn(pointerid, &count, history) != FALSE;
                if (ok && count && count <= capacity) break;
                const DWORD error = GetLastError();
                SDL_free(history);
                history = NULL;
                if ((!ok && error != ERROR_INSUFFICIENT_BUFFER) || count <= capacity || count > 4096) break;
            }
        }
        if (!history) count = 1;

        const Uint64 now = SDL_GetTicksNS();
        LARGE_INTEGER qpc_now, qpc_frequency;
        const bool have_qpc = QueryPerformanceCounter(&qpc_now) && QueryPerformanceFrequency(&qpc_frequency) && qpc_frequency.QuadPart > 0;
        SDL_Window *window = data->window;
        const SDL_PropertiesID props = SDL_GetWindowProperties(window);
        IP_PenHistoryState *state = (IP_PenHistoryState *)SDL_GetPointerProperty(props, "infinipaint.pen.history", NULL);
        if (!state) {
            state = (IP_PenHistoryState *)SDL_calloc(1, sizeof(*state));
            if (state && !SDL_SetPointerPropertyWithCleanup(props, "infinipaint.pen.history", state, IP_FreePenHistory, NULL)) {
                /* SDL invokes the supplied cleanup even on failure. */
                state = NULL;
            }
        }
        if (state && (state->pointer != pointerid || state->device != latest.pointerInfo.sourceDevice)) {
            // SDL's Windows backend represents all native pens by one SDL id.
            SDL_SendPenTouch(now, pen, window, false, false);
            state->pointer = pointerid;
            state->device = latest.pointerInfo.sourceDevice;
            state->size = state->next = 0;
        }
        if (state && !state->qpc_origin && have_qpc) {
            state->qpc_origin = (Uint64)qpc_now.QuadPart;
            state->ticks_origin = now;
        }
        POINT origin = {0, 0};
        if (!ClientToScreen(data->hwnd, &origin)) {
            SDL_SendPenTouch(now, pen, window, false, false);
            SDL_free(history);
            returnCode = 0;
            break;
        }
        RECT device, display;
        const bool mapped = data->videodata->GetPointerDeviceRects &&
            data->videodata->GetPointerDeviceRects(latest.pointerInfo.sourceDevice, &device, &display) &&
            device.right > device.left && device.bottom > device.top &&
            display.right > display.left && display.bottom > display.top;

        // Windows returns newest first; SDL must receive oldest first.
        for (UINT32 i = count; i > 0; --i) {
            const POINTER_PEN_INFO *sample = history ? &history[i - 1] : &latest;
            const POINTER_INFO *info = &sample->pointerInfo;
            if (IP_SeenPenReport(state, sample)) continue;
            // Stable calibration retains report intervals even when a device's
            // report clock is slightly ahead of receipt. No per-report switching.
            // Timestamp 1 marks unusable timing; repeated times split filter runs.
            Uint64 timestamp = 1;
            if (state && have_qpc && info->PerformanceCount) {
                const double delta = ((double)info->PerformanceCount - (double)state->qpc_origin) /
                    (double)qpc_frequency.QuadPart;
                const double mapped_time = (double)state->ticks_origin + delta * 1e9;
                if (mapped_time > 1 && SDL_fabs(mapped_time-(double)now) < 60e9)
                    timestamp = (Uint64)mapped_time;
            }
            float x = (float)(info->ptPixelLocationRaw.x - origin.x);
            float y = (float)(info->ptPixelLocationRaw.y - origin.y);
            if (mapped) {
                // Preserve fractional coordinates and non-zero mapping origins.
                // These are Windows reports, not electrical sensor ground truth.
                x = (float)(display.left + ((double)info->ptHimetricLocationRaw.x - device.left) *
                    (display.right - display.left) / (device.right - device.left) - origin.x);
                y = (float)(display.top + ((double)info->ptHimetricLocationRaw.y - device.top) *
                    (display.bottom - display.top) / (device.bottom - device.top) - origin.y);
            }
            const bool touching = (info->pointerFlags & POINTER_FLAG_INCONTACT) &&
                !(info->pointerFlags & (POINTER_FLAG_CANCELED | POINTER_FLAG_UP));
            const bool eraser = (sample->penFlags & PEN_FLAG_INVERTED) != 0;
            // Release BEFORE moving to the first hover/lift position.
            if (!touching) SDL_SendPenTouch(timestamp, pen, window, eraser, false);
            // Cache pressure before motion. Never apply the next sample's
            // pressure by changing the preceding point from an axis event.
            if (touching)
                SDL_SendPenAxis(timestamp, pen, window, SDL_PEN_AXIS_PRESSURE,
                    (sample->penMask & PEN_MASK_PRESSURE) ? sample->pressure / 1024.0f : 1.0f);
            if (!(info->pointerFlags & POINTER_FLAG_CANCELED)) {
                SDL_SetBooleanProperty(props, "infinipaint.pen.contact_report", touching);
                SDL_SendPenMotion(timestamp, pen, window, x, y);
                SDL_SetBooleanProperty(props, "infinipaint.pen.contact_report", false);
            }
            if (touching) SDL_SendPenTouch(timestamp, pen, window, eraser, true);
            if (!touching && (sample->penMask & PEN_MASK_PRESSURE))
                SDL_SendPenAxis(timestamp, pen, window, SDL_PEN_AXIS_PRESSURE, sample->pressure / 1024.0f);
            SDL_SendPenButton(timestamp, pen, window, 1, (sample->penFlags & PEN_FLAG_BARREL) != 0);
            SDL_SendPenButton(timestamp, pen, window, 2, (sample->penFlags & PEN_FLAG_ERASER) != 0);
            if (sample->penMask & PEN_MASK_ROTATION)
                SDL_SendPenAxis(timestamp, pen, window, SDL_PEN_AXIS_ROTATION, (float)sample->rotation);
            if (sample->penMask & PEN_MASK_TILT_X)
                SDL_SendPenAxis(timestamp, pen, window, SDL_PEN_AXIS_XTILT, (float)sample->tiltX);
            if (sample->penMask & PEN_MASK_TILT_Y)
                SDL_SendPenAxis(timestamp, pen, window, SDL_PEN_AXIS_YTILT, (float)sample->tiltY);
        }
        SDL_free(history);
        returnCode = 0;
    } break;
