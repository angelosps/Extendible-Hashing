#include "common.h"

int next_file_idx() {
    for(int i = 0; i < MAX_OPEN_FILES; ++i)
        if(allFiles[i].hasFile == false)
            return i;
    return -1;
}