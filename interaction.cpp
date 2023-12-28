
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

struct CardInteractionAnimation {
    Interaction interaction;

    //NOTE: 0 to 1 
    float tValue;

    float2 startA;
    float2 startB;

    bool isValid;
};