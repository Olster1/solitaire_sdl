struct TransformX {
    float3 pos; 
    float3 scale;
    float rotationY;
};

float normalizedQuad[4] = { -0.5f, -0.5f, 0.5f, 0.5f};

SDL_Rect getModelToScreenSpace(TransformX T, float16 viewToClipSpace, float2 windowScale) {
    float16 i = float16_identity();
    float16 posT = float16_identity();

    
    i = float16_scale(i, T.scale);
    i = float16_multiply(float16_angle_aroundY(T.rotationY), i);
    i = float16_set_pos(i, T.pos);
    i = float16_multiply(viewToClipSpace, i);

    float4 pos[2];
    int index = 0;

    for(int j = 0; j < 4; j += 2) {
        float x = normalizedQuad[j];
        float y = normalizedQuad[j + 1];

        float4 p = make_float4(x, y, 0, 1);
        
        p = float16_transform(i, p);
        
        
        //NOTE: Blow out to the screen size now - since the clip matrix
        p.x = windowScale.x*((p.x / 2.0f) + 0.5f);
        p.y = windowScale.y*((p.y / 2.0f) + 0.5f);

        assert(index < 2);
        pos[index] = p;
        index++;
    }


    //NOTE: Now make the final rect to send to the SDL renderer
    SDL_Rect result;

    result.w = pos[1].x - pos[0].x;
    result.h = pos[1].y - pos[0].y;
    
    result.x = pos[0].x;
    result.y = windowScale.y - (result.h + pos[0].y); //NOTE: This is a final conversion since the SDL renderer is top corner positive down

    return result;
}