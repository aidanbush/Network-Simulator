#include <vector>
#include <stdexcept>

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

Tilecoder::Tilecoder(int numDimensions, vector<pair<double, double>> dimensionRanges,
        vector<vector<int>> tilecodingPatterns, vector<int> tilesPerDimTiling,
        vector<int> numTilings) {
    if (int(dimensionRanges.size()) != numDimensions) {
        throw runtime_error("Number of dimensions and ranges don't match");
    }

    if (tilecodingPatterns.size() != tilesPerDimTiling.size() || tilecodingPatterns.size() != numTilings.size()) {
        throw runtime_error("Number of patterns, tiles per, and tilings don't match");
    }

    // TODO check values in patterns

    for (auto pattern: tilecodingPatterns) {
        for (auto dim: pattern) {
            if (dim < 0 || dim >= numDimensions) {
                throw runtime_error("Invalid dimension in tilcoder pattern");
            }
        }
    }

    this->numDimensions = numDimensions;
    this->ranges = dimensionRanges;
    this->tilecodingPatterns = tilecodingPatterns;
    this->tilesPerDimTiling = tilesPerDimTiling;
    this->numTilings = numTilings;

    // calculate tileWidths, and offsets
    this->tileWidths.resize(tilecodingPatterns.size());
    this->offsets.resize(tilecodingPatterns.size());

    for (int i = 0; i < int(tilecodingPatterns.size()); i++) {
        this->tileWidths[i] = 1 / double(tilesPerDimTiling[i] - 1);
        this->offsets[i] = tileWidths[i] / numTilings[i];
    }

    calculateNumTiles();

    calculateNumTotalTilings();
}

void Tilecoder::calculateNumTiles() {
    int tiles = 0;
    for (int i = 0; i < int(tilecodingPatterns.size()); i++) {
        vector<int> pattern = tilecodingPatterns[i];

        int curTiles = calculatePatternTiles(pattern, tilesPerDimTiling[i], numTilings[i]);

        numPatternTiles.push_back(curTiles);
        tiles += curTiles;
    }

    numTotalTiles = tiles;
}

int Tilecoder::calculatePatternTiles(vector<int> pattern, int tilesPerDimTiling, int numTilings) {
    int size = 1;

    for (int i = 0; i < int(pattern.size()); i++) {
        size *= tilesPerDimTiling;
    }

    size *= numTilings;

    return size;
}

void Tilecoder::calculateNumTotalTilings() {
    for (auto tilings: numTilings) {
        numTotalTilings += tilings;
    }
}

int Tilecoder::getNumTiles() {
    return numTotalTiles;
}

int Tilecoder::getNumTotalTilings() {
    return numTotalTilings;
}

vector<int> Tilecoder::tilecodeSingle(vector<double> state, int tilesPerDim, int numTilings, double tileWidth,
        double offset) {
    vector<int> tiles = vector<int>(numTilings);

    for (int t = 0; t < int(tiles.size()); t++) {
        int tile = 0;
        int dimensionOffset = 1;

        for (int d = 0; d < int(state.size()); d++) {
            tile += int((state[d] + offset * t) / tileWidth) * dimensionOffset;
            dimensionOffset *= tilesPerDim;
        }

        // tiling offset
        tile += t * dimensionOffset;

        tiles[t] = tile;
    }

    return tiles;
}

vector<int> Tilecoder::tilecode(vector<double> state) {
    // check number of dimensions
    if (int(state.size()) != numDimensions) {
        throw runtime_error("Number of state dimensions doesn't match expected");
    }

    // normalize
    for (int i = 0; i < numDimensions; i++) {
        state[i] = (state[i] - ranges[i].first) / (ranges[i].second - ranges[i].first);
    }

    vector<int> tiles;

    int patternOffset = 0;

    // tilecode for every pattern
    for (int i = 0; i < int(tilecodingPatterns.size()); i++) {
        vector<int> pattern = tilecodingPatterns[i];
        vector<double> currentState;

        for (int j = 0; j < int(pattern.size()); j++) {
            currentState.push_back(state[pattern[j]]);
        }

        vector<int> curTiles = tilecodeSingle(currentState, tilesPerDimTiling[i], numTilings[i], tileWidths[i],
                offsets[i]);

        // add tiles with offset
        for (int i = 0; i < int(curTiles.size()); i++) {
            tiles.push_back(curTiles[i] + patternOffset);
        }

        patternOffset += numPatternTiles[i];
    }

    return tiles;
}
