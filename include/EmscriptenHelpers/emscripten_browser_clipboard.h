#pragma once

#include <string>
#include <emscripten.h>

#define _EM_JS_INLINE(ret, c_name, js_name, params, code)                          \
  extern "C" {                                                                     \
  ret c_name params EM_IMPORT(js_name);                                            \
  EMSCRIPTEN_KEEPALIVE                                                             \
  __attribute__((section("em_js"), aligned(1))) inline char __em_js__##js_name[] = \
    #params "<::>" code;                                                           \
  }

#define EM_JS_INLINE(ret, name, params, ...) _EM_JS_INLINE(ret, name, name, params, #__VA_ARGS__)

namespace emscripten_browser_clipboard {

/////////////////////////////////// Interface //////////////////////////////////

using paste_image_handler = void(*)(std::string, void*);
using paste_handler = void(*)(std::string&&, void*);
using copy_handler = char const*(*)(void*);

inline void paste_event(paste_handler callback, void *callback_data = nullptr);
inline void paste_async(paste_handler callback, void *callback_data = nullptr);
inline void copy(copy_handler callback, void *callback_data = nullptr);
inline void copy(std::string const &content);

///////////////////////////////// Implementation ///////////////////////////////

namespace detail {

EM_JS_INLINE(void, paste_event_js, (paste_handler callback, void *callback_data), {
  document.addEventListener('paste', (event) => {
    Module["ccall"]('emscripten_browser_clipboard_detail_paste_return', 'number', ['string', 'number', 'number'], [event.clipboardData.getData('text/plain'), callback, callback_data]);
  });
});

EM_JS_INLINE(void, paste_async_js, (paste_handler callback, void *callback_data), {
  /// Register the given callback to handle paste events. Callback data pointer is passed through to the callback.
  /// Paste handler callback signature is:
  ///   void my_handler(std::string const &paste_data, void *callback_data = nullptr);
  navigator.clipboard.readText()
    .then(text => {
      Module["ccall"]('emscripten_browser_clipboard_detail_paste_return', 'number', ['string', 'number', 'number'], [text, callback, callback_data]);
    })
    .catch(err => {
      console.error('Failed to read clipboard contents: ', err);
    });
});

EM_JS_INLINE(void, paste_async_image_js, (paste_image_handler callback, void *callback_data), {
  /// Register the given callback to handle paste events. Callback data pointer is passed through to the callback.
  /// Paste handler callback signature is:
  ///   void my_handler(std::string const &paste_data, void *callback_data = nullptr);
  navigator.clipboard.read()
    .then(clipboardContents => {
      const imageMimetypesSupported = ["image/png", "image/jpeg", "image/gif", "image/webp", "image/svg+xml"];
      for (const item of clipboardContents) {
        for(const mimeType of imageMimetypesSupported) {
          if(item.types.includes(mimeType)) {
            item.getType(mimeType).then(blob => {
              const file_reader = new FileReader();
              file_reader.onload = (event) => {
                const uint8Arr = new Uint8Array(event.target.result);
                const data_ptr = Module["_malloc"](uint8Arr.length);
                const data_on_heap = new Uint8Array(Module["HEAPU8"].buffer, data_ptr, uint8Arr.length);
                data_on_heap.set(uint8Arr);
                Module["ccall"]('emscripten_browser_clipboard_detail_paste_image_return', 'number', ['number', 'number', 'number', 'number'], [data_on_heap.byteOffset, uint8Arr.length, callback, callback_data]);
                Module["_free"](data_ptr);
              };
              file_reader.readAsArrayBuffer(blob);
            })
            .catch(err => {
              console.error('Failed to get blob from clipboard item: ', err);
            });
            return;
          }
        }
      }
      console.log('Could not find image mimetype in clipboard');
    })
    .catch(err => {
      console.error('Failed to read clipboard contents: ', err);
    });
});

EM_JS_INLINE(void, copy_js, (copy_handler callback, void *callback_data), {
  /// Register the given callback to handle copy events. Callback data pointer is passed through to the callback.
  /// Copy handler callback signature is:
  ///   char const *my_handler(void *callback_data = nullptr);
  document.addEventListener('copy', (event) => {
    const content_ptr = Module["ccall"]('emscripten_browser_clipboard_detail_copy_return', 'number', ['number', 'number'], [callback, callback_data]);
    event.clipboardData.setData('text/plain', UTF8ToString(content_ptr));
    event.preventDefault();
  });
});

EM_JS_INLINE(void, copy_async_js, (char const *content_ptr), {
  /// Attempt to copy the provided text asynchronously
  navigator.clipboard.writeText(UTF8ToString(content_ptr));
});

} // namespace detail

inline void paste_event(paste_handler callback, void *callback_data) {
  /// C++ wrapper for javascript paste call
  detail::paste_event_js(callback, callback_data);
}

inline void paste_async(paste_handler callback, void *callback_data) {
  /// C++ wrapper for javascript paste call
  detail::paste_async_js(callback, callback_data);
}

inline void paste_async_image(paste_image_handler callback, void *callback_data) {
  /// C++ wrapper for javascript paste call
  detail::paste_async_image_js(callback, callback_data);
}

inline void copy(copy_handler callback, void *callback_data) {
  /// C++ wrapper for javascript copy call
  detail::copy_js(callback, callback_data);
}

inline void copy(std::string const &content) {
  /// C++ wrapper for javascript copy call
  detail::copy_async_js(content.c_str());
}

namespace detail {

extern "C" {

EMSCRIPTEN_KEEPALIVE inline int emscripten_browser_clipboard_detail_paste_return(char const *paste_data, paste_handler callback, void *callback_data);

EMSCRIPTEN_KEEPALIVE inline int emscripten_browser_clipboard_detail_paste_return(char const *paste_data, paste_handler callback, void *callback_data) {
  /// Call paste callback - this function is called from javascript when the paste event occurs
  callback(paste_data, callback_data);
  return 1;
}

EMSCRIPTEN_KEEPALIVE inline int emscripten_browser_clipboard_detail_paste_image_return(char const *pasteData, int pasteSize, paste_image_handler callback, void *callback_data) {
  /// Call paste callback - this function is called from javascript when the paste event occurs
  callback(std::string(pasteData, pasteSize), callback_data);
  return 1;
}

EMSCRIPTEN_KEEPALIVE inline char const *emscripten_browser_clipboard_detail_copy_return(copy_handler callback, void *callback_data);

EMSCRIPTEN_KEEPALIVE inline char const *emscripten_browser_clipboard_detail_copy_return(copy_handler callback, void *callback_data) {
  /// Call paste callback - this function is called from javascript when the paste event occurs
  return callback(callback_data);
}

}

} // namespace detail

} // namespace emscripten_browser_clipboard
