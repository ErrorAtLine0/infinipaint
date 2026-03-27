#pragma once
#include "../Elements/ScrollArea.hpp"

namespace GUIStuff { namespace ElementHelpers {

struct ScrollBarManyEntriesOptions {
    float entryHeight;
    size_t entryCount;
    bool clipHorizontal = false;
    std::function<void(size_t elementIndex)> elementContent;
    std::function<void(const ScrollArea::InnerContentParameters&)> innerContentExtraCallback;
};

void scroll_area_many_entries(GUIManager& gui, const char* id, const ScrollBarManyEntriesOptions& options);

}}
