#ifndef TILECODER_H
#define TILECODER_H

using namespace std;

class Tilecoder {
    public:
        Tilecoder(int numDimensions, vector<pair<double, double>> dimensionRanges,/* min, max */
            vector<vector<int>> tilecodingPatterns, vector<int> tilesPerDimTiling,
            vector<int> numTilings);

        vector<int> tilecode(vector<double> state);

        int getNumTiles();
        int getNumTilings();

    private:
        void calculateNumTiles();
        int calculatePatternTiles(vector<int> pattern, int tilesPerDimTiling, int numTilings);

        vector<int> tilecodeSingle(vector<double> state, int tilesPerDim, int numTilings, double tileWidth, double offset);

        int numDimensions;
        vector<pair<double, double>> ranges;
        vector<vector<int>> tilecodingPatterns;
        vector<int> tilesPerDimTiling;
        vector<int> numTilings;

        int numTotalTiles;
        int numTotalTilings;
        vector<int> numPatternTile;

        vector<double> tileWidths;
        vector<double> offsets;
};


#endif /* TILECODER_H */
