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

bool check_collision(SDL_Rect a, SDL_Rect b); // TODO(chris) just shoving this here so i don't have to move functions around... need to cleanup

struct LWindow
{
    SDL_Window* window;
    
    int width;
    int height;
    
    bool mouseFocus;
    bool keyboardFocus;
    bool fullScreen;
    bool minimized;
};

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

LWindow gWindow;

SDL_Surface* gScreenSurface = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font *gFont = NULL;

struct LTexture
{
    SDL_Texture* texture;
    int width, height;
};

LTexture gTextTexture;

const int MOVE_VEL = 10;

struct Transform
{
    int posX, posY;
    int velX, velY;
    
    SDL_Rect collider;
};

Transform paddle;
SDL_Rect wall;

void transform_move(Transform& transform, SDL_Rect& wall)
{
    transform.posX += transform.velX;
    transform.collider.x = transform.posX;
    
    if(transform.posX < 0 || transform.posX + transform.collider.w > gWindow.width || check_collision(transform.collider, wall))
    {
        transform.posX -= transform.velX;
        transform.collider.x = transform.posX;
    }
    
    transform.posY += transform.velY;
    transform.collider.y = transform.posY;
    
    if(transform.posY < 0 || transform.posY + transform.collider.h > gWindow.height || check_collision(transform.collider, wall))
    {
        transform.posY -= transform.velY;
        transform.collider.y = transform.posY;
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

void button_handle_event(LButton& button, SDL_Event* e)
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

void window_handle_event(LWindow& window, SDL_Event& e)
{
    if(e.type == SDL_WINDOWEVENT)
    {
        bool updateCaption = false;
        
        switch(e.window.event)
        {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            window.width = e.window.data1;
            window.height = e.window.data2;
            SDL_RenderPresent(gRenderer);
            break;
            
            case SDL_WINDOWEVENT_EXPOSED: // The window was obscured in some way and is now no longer obscured
            SDL_RenderPresent(gRenderer);
            break;
            
            case SDL_WINDOWEVENT_ENTER:
            window.mouseFocus = true;
            updateCaption = true;
            break;
            
            case SDL_WINDOWEVENT_LEAVE:
            window.mouseFocus = false;
            updateCaption = true;
            break;
            
            case SDL_WINDOWEVENT_FOCUS_GAINED:
            window.keyboardFocus = true;
            updateCaption = true;
            break;
            
            case SDL_WINDOWEVENT_FOCUS_LOST:
            window.keyboardFocus = false;
            updateCaption = true;
            break;
            
            case SDL_WINDOWEVENT_MINIMIZED:
            window.minimized = true;
            break;
            
            case SDL_WINDOWEVENT_MAXIMIZED:
            window.minimized = false;
            break;
            
            case SDL_WINDOWEVENT_RESTORED:
            window.minimized = false;
            break;
        }
        
        if(updateCaption)
        {
            std::stringstream caption;
            caption << "Breakout - MouseFocus: " << ((window.mouseFocus) ? "On" : "Off") << " KeyboardFocus: " << ((window.keyboardFocus) ? "On" : "Off");
            SDL_SetWindowTitle(window.window, caption.str().c_str());
        }
    }
    else if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)
    {
        if(window.fullScreen)
        {
            SDL_SetWindowFullscreen(window.window, SDL_FALSE);
            window.fullScreen = false;
        }
        else
        {
            SDL_SetWindowFullscreen(window.window, SDL_TRUE);
            window.fullScreen = true;
            window.minimized = false;
        }
    }
}

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

bool check_collision(SDL_Rect a, SDL_Rect b)
{
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;
    
    leftA = a.x;
    rightA = a.x + a.w;
    topA = a.y;
    bottomA = a.y + a.h;
    
    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;
    
    // Detect if any sides of A are outside of B
    if(bottomA <= topB) return false;
    if(topA >= bottomB) return false;
    if(rightA <= leftB) return false;
    if(leftA >= rightB) return false;
    
    return true;
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
            gWindow.window = SDL_CreateWindow("Breakout", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
            gWindow.width = SCREEN_WIDTH;
            gWindow.height = SCREEN_HEIGHT;
            if(gWindow.window == NULL)
            {
                printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
                success = false;
            }
            else
            {
                gRenderer = SDL_CreateRenderer(gWindow.window, -1, SDL_RENDERER_ACCELERATED);
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
        
        gButtonSpriteSheetTexture.texture = create_texture_from_file("button.png", gButtonSpriteSheetTexture.width, gButtonSpriteSheetTexture.height);
        
        for(int i = 0; i < BUTTON_SPRITE_TOTAL; ++i)
        {
            gSpriteClips[i].x = 0;
            gSpriteClips[i].y = i * 200;
            gSpriteClips[i].w = BUTTON_WIDTH;
            gSpriteClips[i].h = BUTTON_HEIGHT;
        }
        
        button_set_positions(gButtons[0], 0, 0);
        button_set_positions(gButtons[1], gWindow.width - BUTTON_WIDTH, 0);
        button_set_positions(gButtons[2], 0, gWindow.height - BUTTON_HEIGHT);
        button_set_positions(gButtons[3], gWindow.width - BUTTON_WIDTH, gWindow.height - BUTTON_HEIGHT);
    }
    
    bool quit = false;
    SDL_Event e;
    
    SDL_Color textColor = {0, 0, 0, 255};
    std::stringstream timeText;
    
    int countedFrames = 0;
    Uint32 appTimer = SDL_GetTicks();
    Uint32 frameTimer;
    
    paddle.collider.w = 100;
    paddle.collider.h = 40;
    
    paddle.posX = gWindow.width / 2;
    paddle.posY = gWindow.height - 30;
    
    // TODO(chris) dot and wall are both just globals... consider making them local
    wall.x = 300;
    wall.y = 40;
    wall.w = 40;
    wall.h = 400;
    
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
            
            window_handle_event(gWindow, e);
            
            for(int i = 0; i < TOTAL_BUTTONS; ++i)
            {
                button_handle_event(gButtons[i], &e);
            }
            
            paddle.velX = 0;
            paddle.velY = 0;
            if(e.type == SDL_KEYDOWN)
            {
                switch(e.key.keysym.sym)
                {
                    // case SDLK_UP: paddle.velY = -MOVE_VEL; break;
                    // case SDLK_DOWN: paddle.velY = MOVE_VEL; break;
                    case SDLK_LEFT: paddle.velX = -MOVE_VEL; break;
                    case SDLK_RIGHT: paddle.velX = MOVE_VEL; break;
                }
            }
        }
        
        if(!gWindow.minimized)
        {
            transform_move(paddle, wall);
            
            float averageFPS = countedFrames / ((SDL_GetTicks() - appTimer) / 1000.0f);
            
            if(averageFPS > 2000000) averageFPS = 0;
            
            timeText.str("");
            timeText << "FPS: " << averageFPS;
            
            gTextTexture.texture = create_texture_from_text(timeText.str().c_str(), gTextTexture.width, gTextTexture.height, textColor);
            
            SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderClear(gRenderer); 
            
            render_texture_at_pos(gTextTexture, 0, 0);
            render_texture_at_pos(gButtonSpriteSheetTexture, gButtons[3].position.x, gButtons[3].position.y, &gSpriteClips[gButtons[3].currentState]);
            
            SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderDrawRect(gRenderer, &wall);
            
            SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderFillRect(gRenderer, &paddle.collider);
            
            SDL_RenderPresent(gRenderer);
            ++countedFrames;
            
            // Wait until we reach 60 FPS (in case the frame completes early)
            int frameTicks = SDL_GetTicks() - frameTimer;
            if(frameTicks < SCREEN_TICKS_PER_FRAME)
            {
                SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
            }
        }
    }

    { // close
        SDL_DestroyTexture(gTextTexture.texture);
        SDL_DestroyTexture(gButtonSpriteSheetTexture.texture);
    
        TTF_CloseFont(gFont);
        gFont = NULL;
        
        SDL_DestroyRenderer(gRenderer);
        SDL_DestroyWindow(gWindow.window);
        gWindow.window = NULL;
        gRenderer = NULL;
        
        IMG_Quit();
        SDL_Quit();
    }
    
    return 0;
}