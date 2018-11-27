#include "SDL.h"
#include "SDL_syswm.h"
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <string>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <Fcntl.h>

#ifdef _WIN32
#include <windows.h>
#endif

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font *gFont = NULL;

struct LTexture
{
    SDL_Texture* texture;
    int width, height;
};

LTexture gTextTexture;

const int DOT_WIDTH = 20;
const int DOT_HEIGHT = 20;
const int DOT_VEL = 10;

struct Dot
{
    int posX, posY;
    int velX, velY;
};

LTexture dotTexture;
Dot dot;

void dot_move(Dot& dot)
{
    dot.posX += dot.velX;
    
    if(dot.posX < 0 || dot.posX + DOT_WIDTH > SCREEN_WIDTH) 
    {
        dot.posX -= dot.velX;
    }
    
    dot.posY += dot.velY;
    
    if(dot.posY < 0 || dot.posY + DOT_HEIGHT > SCREEN_HEIGHT) 
    {
        dot.posY -= dot.velY;
    }
}

const int BUTTON_WIDTH = 300;
const int BUTTON_HEIGHT = 200;
const int TOTAL_BUTTONS = 4;

enum LButtonState
{
    BUTTON_SPRITE_MOUSE_OUT = 0,
    BUTTON_SPRITE_MOUSE_OVER_MOTION = 1,
    BUTTON_SPRITE_MOUSE_DOWN = 2,
    BUTTON_SPRITE_MOUSE_UP = 3,
    
    BUTTON_SPRITE_TOTAL = 4
};

struct LButton
{
    SDL_Point position;
    LButtonState currentState;
};

SDL_Rect gSpriteClips[TOTAL_BUTTONS];
LTexture gButtonSpriteSheetTexture;
LButton gButtons[TOTAL_BUTTONS];

void button_set_positions(LButton& button, int x, int y)
{
    button.position.x = x;
    button.position.y = y;
}

void button_handleEvent(LButton& button, SDL_Event* e)
{
    if(e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP)
    {
        int x, y;
        SDL_GetMouseState(&x, &y);
        
        bool inside = true;
        
        if(x < button.position.x)
        {
            inside = false;
        }
        else if(x > button.position.x + BUTTON_WIDTH)
        {
            inside = false;
        }
        else if(y < button.position.y)
        {
            inside = false;
        }
        else if(y > button.position.y + BUTTON_HEIGHT)
        {
            inside = false;
        }
        
        if(inside)
        {
           switch(e-> type)
           {
               case SDL_MOUSEMOTION:
               button.currentState = BUTTON_SPRITE_MOUSE_OVER_MOTION;
               break;
               
               case SDL_MOUSEBUTTONDOWN:
               button.currentState = BUTTON_SPRITE_MOUSE_DOWN;
               break;
               
               case SDL_MOUSEBUTTONUP:
               button.currentState = BUTTON_SPRITE_MOUSE_UP;
               break;
           }
        }
        else
        {
            button.currentState = BUTTON_SPRITE_MOUSE_OUT; 
        }
    }
}

SDL_Surface* create_surface_from_file(std::string path)
{
    SDL_Surface* optimizedSurface = NULL;
    
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if(loadedSurface == NULL)
    {
        printf("Unable to load image %s! SDL Error: %s\n", path.c_str(), IMG_GetError());
    }
    else
    {
        // Convert the surface to the same format as the screen
        // Apparently this process would occur when blitting, so I believe we're just caching it
        optimizedSurface = SDL_ConvertSurface(loadedSurface, gScreenSurface->format, NULL);
        if(optimizedSurface == NULL)
        {
            printf("Unable to load optimized surface %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
        }
        
        SDL_FreeSurface(loadedSurface);
    }
    
    return optimizedSurface;
}

SDL_Texture* create_texture_from_file(std::string path, int& width, int& height)
{
    SDL_Texture* newTexture = NULL;
    
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if(loadedSurface == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
    }
    else
    {
        SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF)); // pixels that are cyan become transparent
        
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        if(newTexture == NULL)
        {
            printf("Unable to create texture from %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
        }
        else
        {
            width = loadedSurface->w;
            height = loadedSurface->h;
        }
        
        SDL_FreeSurface(loadedSurface);
    }
    
    return newTexture;
}

#ifdef _SDL_TTF_H
SDL_Texture* create_texture_from_text(std::string textureText, int& width, int& height, SDL_Color textColor)
{
    SDL_Texture* result = NULL;
    
    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);
    if(textSurface == NULL)
    {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
    }
    else
    {
        result = SDL_CreateTextureFromSurface(gRenderer, textSurface);
        if(result == NULL)
        {
            printf("Unable to create texture from renderered text! SDL_ttf Error: %s\n", TTF_GetError());
        }
        else
        {
            width = textSurface->w;
            height = textSurface->h;
        }
        
        SDL_FreeSurface(textSurface);
    }
    
    return result;
}
#endif

// NOTE(chris) deviating from the tutorial because i don't think we need a class for this
void render_texture_at_pos(LTexture& texture, int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE)
{
    SDL_Rect renderQuad = {x, y, texture.width, texture.height};
    
    if(clip != NULL)
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    
    SDL_RenderCopyEx(gRenderer, texture.texture, clip, &renderQuad, angle, center, flip);
}

// NOTE(chris) ctrl + shift + b builds & runs! (see tasks.json)
int main(int argc, char* args[])
{
    { // Init
        bool success = true;
    
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            printf("SDL could not init! SDL_Error: %s\n", SDL_GetError());
            success = false;
        }
        else
        {
            // Window creation
            gWindow = SDL_CreateWindow("Breakout", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
            if(gWindow == NULL)
            {
                printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
                success = false;
            }
            else
            {
                gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
                if(gRenderer == NULL)
                {
                    printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                    success = false;
                }
                else
                {
                    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
                    
                    // SDL_image init
                    int imgFlags = IMG_INIT_PNG;
                    if(!(IMG_Init(imgFlags) & imgFlags))
                    {
                        printf("SDL_image could not initialize! SDL_image error: %s\n", IMG_GetError());
                        success = false;
                    }
                    
                    if(TTF_Init() == -1)
                    {
                        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
                        success = false;
                    }
                }
            }
        }
    }

    { // load media
        gFont = TTF_OpenFont("lazy.ttf", 20);
        if(gFont == NULL)
        {
            printf("Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
        }
        // else
        // {
        //     SDL_Color textColor = {0, 0, 0};
        //     gTextTexture.texture = create_texture_from_text("", gTextTexture.width, gTextTexture.height, textColor);
        // }
        
        dotTexture.texture = create_texture_from_file("dot.png", dotTexture.width, dotTexture.height);
        gButtonSpriteSheetTexture.texture = create_texture_from_file("button.png", gButtonSpriteSheetTexture.width, gButtonSpriteSheetTexture.height);
        
        for(int i = 0; i < BUTTON_SPRITE_TOTAL; ++i)
        {
            gSpriteClips[i].x = 0;
            gSpriteClips[i].y = i * 200;
            gSpriteClips[i].w = BUTTON_WIDTH;
            gSpriteClips[i].h = BUTTON_HEIGHT;
        }
        
        button_set_positions(gButtons[0], 0, 0);
        button_set_positions(gButtons[1], SCREEN_WIDTH - BUTTON_WIDTH, 0);
        button_set_positions(gButtons[2], 0, SCREEN_HEIGHT - BUTTON_HEIGHT);
        button_set_positions(gButtons[3], SCREEN_WIDTH - BUTTON_WIDTH, SCREEN_HEIGHT - BUTTON_HEIGHT);
    }
    
    bool quit = false;
    SDL_Event e;
    
    SDL_Color textColor = {0, 0, 0, 255};
    std::stringstream timeText;
    
    int countedFrames = 0;  
    Uint32 appTimer = SDL_GetTicks();
    Uint32 frameTimer;
    
    while(!quit)
    {
        frameTimer = SDL_GetTicks();
        
        while(SDL_PollEvent(&e) != 0)
        {
            if(e.type == SDL_QUIT)
            {
                quit = true;
            }
            else if(e.type == SDL_KEYDOWN)
            {
                switch(e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                    quit = true;
                    break;
                    
                    default:
                    break;
                }
            }
            
            for(int i = 0; i < TOTAL_BUTTONS; ++i)
            {
                button_handleEvent(gButtons[i], &e);
            }
            
            
            if(e.type == SDL_KEYDOWN && e.key.repeat == 0)
            {
                switch(e.key.keysym.sym)
                {
                    case SDLK_UP: dot.velY -= DOT_VEL; break;
                    case SDLK_DOWN: dot.velY += DOT_VEL; break;
                    case SDLK_LEFT: dot.velX -= DOT_VEL; break;
                    case SDLK_RIGHT: dot.velX += DOT_VEL; break;
                }
            }
            
            // NOTE(Chris) this is anther method of detecting input
            const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
            if(currentKeyStates[SDL_SCANCODE_UP])
            {
                printf("up!\n");
            }
            else if(currentKeyStates[SDL_SCANCODE_DOWN])
            {
                printf("down!\n");
            }
            else if(currentKeyStates[SDL_SCANCODE_LEFT])
            {
                printf("left!\n");
            }
            else if(currentKeyStates[SDL_SCANCODE_RIGHT])
            {
                printf("right!\n");
            }
        }
        
        dot_move(dot);
        
        float averageFPS = countedFrames / ((SDL_GetTicks() - appTimer) / 1000.0f);
        
        if(averageFPS > 2000000) averageFPS = 0;
        
        timeText.str("");
        timeText << "FPS: " << averageFPS;
        
        gTextTexture.texture = create_texture_from_text(timeText.str().c_str(), gTextTexture.width, gTextTexture.height, textColor);
        
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer); 
        
        render_texture_at_pos(gTextTexture, 0, 0);
        render_texture_at_pos(gButtonSpriteSheetTexture, gButtons[3].position.x, gButtons[3].position.y, &gSpriteClips[gButtons[3].currentState]);
        render_texture_at_pos(dotTexture, dot.posX, dot.posY);
        
        SDL_RenderPresent(gRenderer);
        ++countedFrames;
        
        // Wait until we reach 60 FPS (in case the frame completes early)
        int frameTicks = SDL_GetTicks() - frameTimer;
        if(frameTicks < SCREEN_TICKS_PER_FRAME)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    { // close
        SDL_DestroyTexture(gTextTexture.texture);
        SDL_DestroyTexture(gButtonSpriteSheetTexture.texture);
        SDL_DestroyTexture(dotTexture.texture);
    
        TTF_CloseFont(gFont);
        gFont = NULL;
        
        SDL_DestroyRenderer(gRenderer);
        SDL_DestroyWindow(gWindow);
        gWindow = NULL;
        gRenderer = NULL;
        
        IMG_Quit();
        SDL_Quit();
    }
    
    return 0;
}