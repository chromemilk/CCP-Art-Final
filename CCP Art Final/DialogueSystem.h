#pragma once

#include "GameDataTypes.h"

#include <algorithm>
#include <utility>
#include <vector>

class DialogueSystem
{
public:
    void start( std::vector<DialogueLine> lines ) {
        queue = std::move( lines );
        index = 0;
        lineTimer = queue.empty() ? 0.0f : std::max( 0.1f, queue[ 0 ].duration );
        active = !queue.empty();
    }

    void update( float dt ) {
        if (!active || queue.empty()) return;

        lineTimer -= dt;
        while (lineTimer <= 0.0f && active)
        {
            ++index;
            if (index >= queue.size())
            {
                clear();
                return;
            }
            lineTimer += std::max( 0.1f, queue[ index ].duration );
        }
    }

    bool isActive() const {
        return active;
    }

    const std::string &currentText() const {
        static const std::string kEmpty;
        if (!active || queue.empty() || index >= queue.size()) return kEmpty;
        return queue[ index ].text;
    }

    void clear() {
        queue.clear();
        index = 0;
        lineTimer = 0.0f;
        active = false;
    }

private:
    std::vector<DialogueLine> queue;
    size_t index = 0;
    float lineTimer = 0.0f;
    bool active = false;
};
