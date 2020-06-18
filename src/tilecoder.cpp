#include <vector>

#include "tilecoder.h"

// All following defines can be overridden in tilecoderDefines.h, which can be modified for local testing
#if __has_include("tilecoderDefines.h")
#include "tilecoderDefines.h"
#else
#define NUM_TILINGS 10
#define NUM_TILES_PER_TILING 11
#define STATE_MIN 0.0
#define STATE_MAX 1.0
#endif //__has_include

using namespace std;

namespace Tilecoder {
    double tileWidth = (STATE_MAX - STATE_MIN)/(NUM_TILES_PER_TILING - 1);
    double offset = tileWidth/NUM_TILINGS;

    // Returns vector of indices which are active (set to 1)
    vector<int> tilecode(double state) {
        vector<int> indices = vector<int>(NUM_TILINGS);
        for (int i = 0; i < NUM_TILINGS; i++) {
            indices[i] = i*NUM_TILES_PER_TILING + (int)((state - STATE_MIN + i*offset)/tileWidth);
        }
        return indices;
    }

    int getNumTilings() {
        return NUM_TILINGS;
    }

    int getNumTiles() {
        return NUM_TILINGS*NUM_TILES_PER_TILING;
    }
}
