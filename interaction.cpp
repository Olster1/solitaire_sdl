
enum CardLocation {
    PILLAR, 
    FOUNDATION,
    DRAW_PILE

};
struct Interaction {
    int cardCount;
    Card c[52];
    float2 grabOffset;

    CardLocation location;
    int locationIndex;

    bool isValid;

    
};