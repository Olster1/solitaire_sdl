#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#include "./transform.cpp"
#include <stdio.h>
#include "./sound.cpp"

enum GameMode {
    NONE_MODE,
    EASY_MODE,
    HARD_MODE
};

enum CardType {
    CARD_DIAMONDS,
    CARD_HEARTS,
    CARD_CLUBS,
    CARD_SPADES,
    CARD_NONE,
};

struct CardAnimation {
    //NOTE: 0 to 1 
    float tValue;

    float2 startA;
    float2 startB;


    bool isMovement;
    bool isFlipAnimation; //NOTE: If card is turning over

    bool isValid;
};


struct Card {
    int number;
    CardType type;
    bool isTurnedOver;

    CardAnimation animation;

    bool isSameCard(Card c, Card c1) {

        bool result = c.number == c1.number && c.type == c1.type;

        return result;

    }
};


#include "./interaction.cpp"

struct CardPillar {
    int cardCount;
    Card cards[52];
};
struct CardFoundation {
    CardType type;
    int cardCount;
    Card cards[52];
};

struct GameState {
    bool inited;
    float dt;
    float screenWidth;
    float aspectRatio_y_over_x;

    float rot;

    float zoomFactor;

    CardInteractionAnimation cardAnimation;

    Interaction currentInteraction;

    //NOTE: Whether easy or hard mode -> determines whether 1 or 3 draw cards.
    GameMode gameMode;

    int packCount;
    Card pack[52];

    int showingCount;
    Card showing[52];

    CardPillar pillars[7];
    CardFoundation foundations[4];


    SDL_Texture *cardImages[52];
    SDL_Texture *cardTemplate; 
    SDL_Texture *backOfCard; 

    SDL_Texture *easyImage; 
    SDL_Texture *hardImage; 
    SDL_Texture *zoomOutImage; 
    SDL_Texture *zoomInImage; 

    float2 mouseP_screenSpace;
    float2 mouseP_01;
    MouseKeyState mouseLeftBtn;

    WavFile cardFlipSound[2];

    SDL_AudioSpec audioSpec;

};


TransformX updateCardAnimation(GameState *gameState, Card *card, TransformX T) {

    if(card && card->animation.isValid) {
        //NOTE: Update the animation

        float speedFactor = 3.0f;

        card->animation.tValue += gameState->dt*speedFactor;

        if(card->animation.tValue > 1.0f) {
            card->animation.tValue = 1.0f;
        }

        if(card->animation.isMovement) {
            T.pos.x = lerp(card->animation.startA.x, card->animation.startB.x, make_lerpTValue(card->animation.tValue));
            T.pos.y = lerp(card->animation.startA.y, card->animation.startB.y, make_lerpTValue(card->animation.tValue));
        }

        if(card->animation.isFlipAnimation) {
            T.rotationY = lerp(PI32, 0, make_lerpTValue(card->animation.tValue));
        }
        
        if(card->animation.tValue >= 1.0f) {
            card->animation.isValid = false;
        }
    }

    return T;
}

SDL_Texture *getCardImage(GameState *gameState, Card *c) {
    int suitFactor = 0;

    if(c->type == CARD_HEARTS) {
        suitFactor = 1;
    } else if(c->type == CARD_CLUBS) {
        suitFactor = 2;
    } else if(c->type == CARD_SPADES) {
        suitFactor = 3;
    }

    int index = (c->number - 1) + (13 * suitFactor);

    assert(index >= 0 && index < 52);

    return gameState->cardImages[index];

}

void drawPillar(CardPillar *p, GameState *gameState, float x, float y, float2 mouseWorldP, int i, float16 screenT, SDL_Renderer *renderer) {

    for(int j = -1; j < p->cardCount; ++j) {
        TransformX T;
        T.pos = make_float3(x, y, 0); 
        T.scale = make_float3(50, 100, 1); 
        T.rotationY = 0;

        SDL_Texture *img = 0;
        bool drawCard = true;

        if(j == -1) {
            //NOTE: Draw the underneath
            img = gameState->cardTemplate;
        } else {
            Card *c = &p->cards[j];
            img = getCardImage(gameState, c);

            if(gameState->currentInteraction.isValid) {
                for(int m = 0; m < gameState->currentInteraction.cardCount; ++m) {
                    if(c->isSameCard(gameState->currentInteraction.c[m], *c)) {
                        //NOTE: don't draw card
                        drawCard = false;
                        break;
                    }
                }
            }

            y -= 30;

            if(!c->isTurnedOver) {
                img = gameState->backOfCard;
            }

            T = updateCardAnimation(gameState, c, T);

            if(T.rotationY > 0.5f*PI32) {
                img = gameState->backOfCard;
            }

        }

        if(drawCard) {

            
            SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

            //NOTE: Draw the image
            SDL_RenderCopy(renderer, img, NULL, &texr);
        }
    }
}

void loadCardImages(GameState *gameState, SDL_Renderer *renderer) {
    gameState->cardTemplate = IMG_LoadTexture(renderer, "./cards/card-back2.png");
    gameState->backOfCard = IMG_LoadTexture(renderer, "./cards/card-back1.png");

    gameState->easyImage = IMG_LoadTexture(renderer, "./images/easy.png");
    gameState->hardImage = IMG_LoadTexture(renderer, "./images/hard.png");
    gameState->zoomInImage = IMG_LoadTexture(renderer, "./images/zoomIn.png");
    gameState->zoomOutImage = IMG_LoadTexture(renderer, "./images/zoomOut.png");

    for(int i = 0; i < 52; i++) {
         char *type = "";
        if(i < 13) {
            type = "diamonds";
        } else if(i < 13*2) {
            type = "hearts";
        } else if(i < 13*3) {
            type = "clubs";
        } else if(i < 13*4) {
            type = "spades";
        }

        int length = snprintf(NULL, 0, "./cards/card-%s-%d.png", type, (i % 13) + 1) + 1; //NOTE: Plus one for null-terminator
        // printf("%d\n", length);

        char *fileName = (char *)malloc(length);

        snprintf(fileName, length, "./cards/card-%s-%d.png", type, (i % 13) + 1);

        // printf("%s\n", fileName);

        gameState->cardImages[i] = IMG_LoadTexture(renderer, fileName);

        free(fileName);
    }
}



void addToPillar(GameState *gameState, int pillarIndex, Card c) {
    CardPillar *p =  &gameState->pillars[pillarIndex];
    p->cards[p->cardCount++] = c;

}

void beginFlipAnimation(GameState *gameState, Card *card) {
     card->animation.tValue = 0;

    card->animation.isMovement = false;
    card->animation.isFlipAnimation = true; 

    card->animation.isValid = true;

    playSound(&gameState->cardFlipSound[1]);
}


void removeCardFromLocation(GameState *gameState) {
    if(gameState->currentInteraction.location == PILLAR) {
        CardPillar *pillar = &gameState->pillars[gameState->currentInteraction.locationIndex];
        --pillar->cardCount;

        if(pillar->cardCount > 0) {
            pillar->cards[pillar->cardCount - 1].isTurnedOver = true;
            
            beginFlipAnimation(gameState, &pillar->cards[pillar->cardCount - 1]);
        }

    } else if(gameState->currentInteraction.location == FOUNDATION) {
        CardFoundation *foundation = &gameState->foundations[gameState->currentInteraction.locationIndex];

        assert(foundation->cardCount > 0);
        if(foundation->cardCount > 0) {
            foundation->cardCount--;
        }
        

    } else if(gameState->currentInteraction.location == DRAW_PILE) {
        
        for(int i = gameState->showingCount - 1; i < gameState->currentInteraction.locationIndex; --i) {
            assert(i >= 0);
            gameState->showing[i - 1] = gameState->showing[i];
        }
        gameState->showingCount--;
        
    }
}

void loadDeck(GameState *gameState) {
    for(int i = 0; i < 52; i++) {
         CardType type = CARD_DIAMONDS;
        if(i < 13) {
            type = CARD_DIAMONDS;
        } else if(i < 13*2) {
            type = CARD_HEARTS;
        } else if(i < 13*3) {
            type = CARD_CLUBS;
        } else if(i < 13*4) {
            type = CARD_SPADES;
        }
        Card c;
        c.type = type;
        c.isTurnedOver = false;
        c.number = (i % 13) + 1;
        gameState->pack[i] = c;
    }

    //NOTE: Shuffle the deck
    for(int h = 0; h < 5; h++) {
        for (int i = 0; i < 52; i++) 
        {
            int pos = (int)((float)((float)(rand()) / (float)RAND_MAX) * 52);
            
            assert(pos >= 0 && pos < 52);
            Card t = gameState->pack[pos];
            gameState->pack[pos] = gameState->pack[i];
            gameState->pack[i] = t;
        }
    }
}

void drawPlayingField(GameState *gameState) {
    int cardCount = 1;
    for(int i = 0; i < arrayCount(gameState->pillars); i++) {
        for(int j = 0; j < cardCount; ++j) {
            Card c = gameState->pack[--gameState->packCount];
            
            if(j == cardCount - 1) {
                c.isTurnedOver = true;
            }
            addToPillar(gameState, i, c);
        }
        cardCount++;

    }
}

bool cardIsOppositeColor(Card c1, Card c2) {
    bool result = false;
    if(c1.type == CARD_DIAMONDS || c1.type == CARD_HEARTS) {
        result = (c2.type == CARD_CLUBS || c2.type == CARD_SPADES);
    } else if(c1.type == CARD_CLUBS || c1.type == CARD_SPADES) {
        result = (c2.type == CARD_DIAMONDS || c2.type == CARD_HEARTS);
    }
    return result;

}

void updateGame(GameState *gameState, SDL_Renderer *renderer) {
    if(!gameState->inited) {
        loadCardImages(gameState, renderer);
        loadDeck(gameState);

        for(int i = 0; i < arrayCount(gameState->foundations); ++i) {
            gameState->foundations[i].type = CARD_NONE;
        }

        gameState->packCount = 52;
        gameState->showingCount = 0;

        gameState->gameMode = NONE_MODE;
        gameState->zoomFactor = 1;

        drawPlayingField(gameState);

        loadWavFile(&gameState->cardFlipSound[0], "./sounds/cardFlip.wav", &gameState->audioSpec);
        loadWavFile(&gameState->cardFlipSound[1], "./sounds/cardFlip1.wav", &gameState->audioSpec);

        gameState->inited = true;
    }

    //NOTE: For debug testing card rotation
    // gameState->rot += gameState->dt;

    float fauxWidth = 800*gameState->zoomFactor;
    float fauxHeight = fauxWidth*gameState->aspectRatio_y_over_x;
    float16 screenT = make_ortho_matrix_bottom_left_corner(fauxWidth, fauxHeight, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);

     float2 mouseWorldP = make_float2(gameState->mouseP_01.x*fauxWidth, gameState->mouseP_01.y*fauxWidth*gameState->aspectRatio_y_over_x);
     float startX = 100;

    if(gameState->gameMode == NONE_MODE) {
        float offsetX = 0.1f*fauxWidth;
        float x =  (fauxWidth * 0.5f) - offsetX;
        float y = fauxHeight / 2.0f;
        

        SDL_Texture *images[2] = {gameState->easyImage, gameState->hardImage};

        for(int i = 0; i < arrayCount(images); ++i) {
            TransformX T;
            T.pos = make_float3(x, y, 0); 
            T.scale = make_float3(100, 100, 1); 
            T.rotationY = 0;

            SDL_Texture *img = images[i];

            SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

            //NOTE: Draw the image
            SDL_RenderCopy(renderer, img, NULL, &texr);

            x += 2*offsetX;

            Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));

            if(in_rect2f_bounds(bounds, mouseWorldP) && gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) { 

                if(i == 0) {
                    gameState->gameMode = EASY_MODE;
                } else if(i == 1) {
                    gameState->gameMode = HARD_MODE;
                }
            }
        }
        
        return;
    }
   
    {
     //NOTE: Draw the deck 

        float x = startX;
        float y = fauxHeight - 100;

        {
            TransformX T;
            T.pos = make_float3(x, y, 0); 
            T.scale = make_float3(50, 100, 1); 
            T.rotationY = 0;

            SDL_Texture *img = gameState->backOfCard;

            if(gameState->packCount <= 0) {
                img = gameState->cardTemplate;
            }
            
            if(!gameState->currentInteraction.isValid) {
                //NOTE: Check if they clicked on it
                Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));

                if(in_rect2f_bounds(bounds, mouseWorldP) && gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
                        playSound(&gameState->cardFlipSound[1]);
                    if(gameState->packCount > 0) {
                        //NOTE: Get some cards out
                        int cardsToTake = 3;
                        if(gameState->packCount < cardsToTake) {
                            cardsToTake = gameState->packCount;
                        }
                        
                        for(int i = 0; i < cardsToTake; ++i) {
                            Card c = gameState->pack[--gameState->packCount];
                            assert(gameState->showingCount < arrayCount(gameState->showing));
                            
                            gameState->showing[gameState->showingCount++] = c;
                        }
                    } else {
                        //NOTE: Put the other ones back on
                        for(int i = 0; i < gameState->showingCount; ++i) {
                            gameState->pack[i] = gameState->showing[gameState->showingCount - i - 1];
                        }
                        gameState->packCount = gameState->showingCount;
                        gameState->showingCount = 0;
                    }
                }
            } 
            SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

            //NOTE: Draw the image
            SDL_RenderCopy(renderer, img, NULL, &texr);
        }

        x += 70;
        
        if(gameState->showingCount > 0) {
            int count = (gameState->gameMode == EASY_MODE) ? 1 : MathMin(gameState->showingCount, 3);
            
            x += 40*count - 40;
            for(int i = count - 1; i >= 0; --i) {
                int locationIndex = (gameState->showingCount - 1) - i;
                Card *c = &gameState->showing[locationIndex];

                TransformX T;
                T.pos = make_float3(x, y, 0); 
                T.scale = make_float3(50, 100, 1); 
                T.rotationY = 0;
                bool shouldDraw = true;

                
                //NOTE: Check if user put the picked up from the draw pile
                if(!gameState->currentInteraction.isValid) {
                    Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));
                    
                    if(i == 0 && gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED && in_rect2f_bounds(bounds, mouseWorldP)) {
                        
                        gameState->currentInteraction.c[0] = *c;
                        gameState->currentInteraction.cardCount = 1;
                        gameState->currentInteraction.c[0].isTurnedOver = true;

                        gameState->currentInteraction.grabOffset = minus_float2(make_float2(T.pos.x, T.pos.y), mouseWorldP);
                        
                        gameState->currentInteraction.location = DRAW_PILE;
                        gameState->currentInteraction.locationIndex = locationIndex;
                        
                        gameState->currentInteraction.isValid = true;

                        gameState->cardAnimation.startB = make_float2(T.pos.x, T.pos.y);
                    } 
                } else if(c->isSameCard(*c, gameState->currentInteraction.c[0])) {
                    //NOTE: Has grabbed this card off the draw pile
                    shouldDraw = false;
                }

                if(shouldDraw) {
                    SDL_Texture *img = img = getCardImage(gameState, c);
                    SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

                    //NOTE: Draw the image
                    SDL_RenderCopy(renderer, img, NULL, &texr);
                }
                x -= 40;
            }
        }


         //NOTE: Draw the card foundations
        {
            float x = 370;

            for(int i = 0; i < arrayCount(gameState->foundations); ++i) {
                CardFoundation* p = &gameState->foundations[i];

                TransformX T;
                T.pos = make_float3(x, y, 0); 
                T.scale = make_float3(50, 100, 1); 
                T.rotationY = 0;

                SDL_Texture *img;

                Card *card = &p->cards[p->cardCount - 1];
                bool getCard = p->cardCount > 0;

                if(gameState->currentInteraction.isValid && card->isSameCard(gameState->currentInteraction.c[0], *card)) {
                    if((p->cardCount - 1) > 0) {
                        card = &p->cards[p->cardCount - 2];
                    } else {
                        getCard = false;
                    }

                }

                if(getCard) {
                    //NOTE: Draw the last Card
                    img = getCardImage(gameState, card);

                } else {
                    //NOTE: Draw a blank card
                    img = gameState->cardTemplate;
                }


                SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

                //NOTE: Draw the image
                SDL_RenderCopy(renderer, img, NULL, &texr);

                x += 70;
                
                CardType type = CARD_NONE;
                if(p->type != CARD_NONE) {
                    type = p->type;
                }

                //NOTE: Check if user put the card on the foundation
                if(gameState->currentInteraction.isValid) {
                    Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));
                    
                    if(in_rect2f_bounds(bounds, mouseWorldP)) {
                        if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED && gameState->currentInteraction.cardCount == 1) {
                            if((gameState->currentInteraction.c[0].type == type || type == CARD_NONE) && (gameState->currentInteraction.c[0].number == 1 || gameState->currentInteraction.c[0].number == (card->number + 1))) {
                                //NOTE: Put on the foundation
                                p->cards[p->cardCount++] = gameState->currentInteraction.c[0];

                                //NOTE: Take off the where it was
                                removeCardFromLocation(gameState);

                                p->type = gameState->currentInteraction.c[0].type;
                                
                                gameState->currentInteraction.isValid = false;
                            }
                        }
                    } 
                } else if(p->cardCount > 0) {
                    //NOTE: Check if they clicked on it
                    Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));

                    if(in_rect2f_bounds(bounds, mouseWorldP) && gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
                        //NOTE: Pick up a card
                        gameState->currentInteraction.c[0] = *card;
                        gameState->currentInteraction.cardCount = 1;
                        gameState->currentInteraction.grabOffset = minus_float2(make_float2(T.pos.x, T.pos.y), mouseWorldP);
                        
                        gameState->currentInteraction.location = FOUNDATION;
                        gameState->currentInteraction.locationIndex = i;

                        gameState->cardAnimation.startB = make_float2(T.pos.x, T.pos.y); 
                        
                        gameState->currentInteraction.isValid = true;
                    }
                }
            }
        }
    }
    
    //NOTE: Draw the card pillars
    {
        float x = startX;
        float cardLayoutY = fauxHeight - 300;
        float y = cardLayoutY;
        for(int i = 0; i < arrayCount(gameState->pillars); ++i) {
            CardPillar* p = &gameState->pillars[i];
            
            y = cardLayoutY - (p->cardCount * 30) + 30;

            //NOTE: Update the grab action
            for(int j = p->cardCount - 1; j >= -1; --j) {
                TransformX T;

                if(j == -1) {
                    y -= 30;
                }

                T.pos = make_float3(x, y, 0); 
                T.scale = make_float3(50, 100, 1); 
                T.rotationY = 0;

            
                if(!gameState->currentInteraction.isValid) {
                    //NOTE: Check if they clicked on it
                    Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));

                    Card *card = &p->cards[j];

                    if(in_rect2f_bounds(bounds, mouseWorldP) && gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED && card->isTurnedOver) {
                        gameState->currentInteraction.cardCount = 0;
                        for(int k = j; k < p->cardCount; ++k) {
                            //NOTE: Pick up a card
                            gameState->currentInteraction.c[gameState->currentInteraction.cardCount++] = p->cards[k];
                        }
                        
                        gameState->currentInteraction.grabOffset = minus_float2(make_float2(T.pos.x, T.pos.y), mouseWorldP);
                        
                        gameState->currentInteraction.location = PILLAR;
                        gameState->currentInteraction.locationIndex = i;

                        gameState->cardAnimation.startB = make_float2(T.pos.x, T.pos.y); 
                        
                        gameState->currentInteraction.isValid = true;
                    }
                } else {
                    Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));
                    
                    //NOTE: Check if user put the card on a pillar
                    if(in_rect2f_bounds(bounds, mouseWorldP)) {
                        if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED && gameState->currentInteraction.locationIndex != i && j == (p->cardCount - 1)) {
                            Card lastCard;
                            if(p->cardCount > 0) {
                                lastCard = p->cards[p->cardCount - 1];
                            }

                            //NOTE: Check if the card you're trying to put on a pillar is opposite color AND card number - 1
                            
                            if(p->cardCount == 0 || !lastCard.isTurnedOver || (cardIsOppositeColor(gameState->currentInteraction.c[0], lastCard) && (gameState->currentInteraction.c[0].number + 1) == lastCard.number)) 
                            {
                                 for(int m = 0; m < gameState->currentInteraction.cardCount; ++m) {
                                    //NOTE: Put on the pillar
                                    p->cards[p->cardCount++] = gameState->currentInteraction.c[m];

                                    //NOTE: Take off the where it was
                                    removeCardFromLocation(gameState);
                                }
                                
                                gameState->currentInteraction.isValid = false;
                            }
                        }
                    }
                }

                y += 30;
            }

            //NOTE: Update release action
            
            //NOTE: Reset the draw y location
            y = cardLayoutY;
            
            drawPillar(p, gameState, x, y, mouseWorldP, i, screenT, renderer);
            
            y = cardLayoutY;
            x += 70;
        }
    }

    float2 cardXY = plus_float2(mouseWorldP, gameState->currentInteraction.grabOffset);
    bool endAnimation = false;

    if(gameState->cardAnimation.isValid) {

        float speedFactor = 3.0f;

        gameState->cardAnimation.tValue += gameState->dt*speedFactor;

        if(gameState->cardAnimation.tValue > 1.0f) {
            gameState->cardAnimation.tValue = 1.0f;
        }

        cardXY.x = lerp(gameState->cardAnimation.startA.x, gameState->cardAnimation.startB.x, make_lerpTValue(gameState->cardAnimation.tValue));
        cardXY.y = lerp(gameState->cardAnimation.startA.y, gameState->cardAnimation.startB.y, make_lerpTValue(gameState->cardAnimation.tValue));
        
        
        if(gameState->cardAnimation.tValue >= 1.0f) {
            endAnimation = true;
        }
    }

    if(gameState->currentInteraction.isValid) {
        float y = 0;
        for(int i = 0; i < gameState->currentInteraction.cardCount; ++i) {
            SDL_Texture *img = getCardImage(gameState, &gameState->currentInteraction.c[i]);

            TransformX T;
            T.pos = make_float3(0, 0, 0); 
            T.scale = make_float3(50, 100, 1); 
            T.rotationY = 0;

            
            T.pos.x = cardXY.x;
            T.pos.y = cardXY.y + y;

            SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

            //NOTE: Draw the image
            SDL_RenderCopy(renderer, img, NULL, &texr);

            y -= 30;
        }
    }

    // if(gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) {
    //     playSound(&gameState->cardFlipSound[1]);
    // }

    if(endAnimation) {
        gameState->cardAnimation.isValid = false;
        gameState->currentInteraction.isValid = false;
        gameState->currentInteraction.cardCount = 0;
    }

    if(gameState->mouseLeftBtn == MOUSE_BUTTON_RELEASED && gameState->currentInteraction.isValid && !gameState->cardAnimation.isValid) {
        //NOTE: Start animation
        gameState->cardAnimation.isValid = true;
        gameState->cardAnimation.interaction = gameState->currentInteraction;

        gameState->cardAnimation.tValue = 0;

        gameState->cardAnimation.startA = cardXY;
        //NOTE: Set when we grabbed it 
        // gameState->cardAnimation.startB;
    }

    if(gameState->gameMode != NONE_MODE) {

        float fauxWidth = 800;
        float fauxHeight = fauxWidth*gameState->aspectRatio_y_over_x;
        float16 screenT = make_ortho_matrix_bottom_left_corner(fauxWidth, fauxHeight, MATH_3D_NEAR_CLIP_PlANE, MATH_3D_FAR_CLIP_PlANE);

        float2 mouseWorldP = make_float2(gameState->mouseP_01.x*fauxWidth, gameState->mouseP_01.y*fauxWidth*gameState->aspectRatio_y_over_x);


        float size = 50;
        
        float x =  fauxWidth - 2*size;
        float y = fauxHeight - size;
        
        
        SDL_Texture *images[2] = {gameState->zoomInImage, gameState->zoomOutImage};

        for(int i = 0; i < arrayCount(images); ++i) {
            TransformX T;
            T.pos = make_float3(x, y, 0); 
            T.scale = make_float3(size, size, 1); 
            T.rotationY = 0;

            SDL_Texture *img = images[i];

            SDL_Rect texr = getModelToScreenSpace(T, screenT, make_float2(gameState->screenWidth, gameState->screenWidth*gameState->aspectRatio_y_over_x));

            //NOTE: Draw the image
            SDL_RenderCopy(renderer, img, NULL, &texr);

            x += size;

            Rect2f bounds = make_rect2f_center_dim(make_float2(T.pos.x, T.pos.y), make_float2(T.scale.x, T.scale.y));

            if(in_rect2f_bounds(bounds, mouseWorldP) && gameState->mouseLeftBtn == MOUSE_BUTTON_PRESSED) { 

                float increment = 0.2f;
                if(i == 0) {
                    gameState->zoomFactor -= increment;
                } else if(i == 1) {
                    gameState->zoomFactor += increment;
                }
            }
        }
        
        return;
    }

   
}