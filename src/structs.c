#include "structs.h"

void setQueueFamilyIndex(QueueFamilyIndex* qfi, uint32_t newIndex) {
    qfi->index = newIndex;
    qfi->set = true;
}

bool queueFamilyIndicesAreComplete(QueueFamilyIndices indices) {
    return indices.graphicsFamily.set && indices.presentFamily.set;
}
