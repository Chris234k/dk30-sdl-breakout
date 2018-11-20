#include "SDL.h"
#include "SDL_syswm.h"
#include <SDL_image.h>
#include <string>
#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <Fcntl.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = NULL;

SDL_Surface* gScreenSurface = NULL;

SDL_Renderer* gRenderer = NULL;
SDL_Texture* gTexture = NULL;

const int WALKING_ANIMATION_FRAMES = 4;
SDL_Rect gSpriteClips[WALKING_ANIMATION_FRAMES];
SDL_Texture* gSpriteSheetTexture;

void create_debug_console()
{
    // Get the window handle (we need it below)
    SDL_SysWMinfo systemInfo; 
    SDL_VERSION(&systemInfo.version);
    SDL_GetWindowWMInfo(gWindow, &systemInfo);

    HWND handle = systemInfo.info.win.window;
    
    // Source: https://stackoverflow.com/questions/8482363/c-sdl-debugging-with-console-window
    int hConHandle;
    intptr_t lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE *fp;

    // allocate a console for this app
    AllocConsole();

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = 500;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    lStdHandle = (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console
    lStdHandle = (intptr_t)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "r" );
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );

    // redirect unbuffered STDERR to the console
    lStdHandle = (intptr_t)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );

    // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
    std::ios::sync_with_stdio();

    //Keep our window in focus
    SetForegroundWindow(handle);                        // Slightly Higher Priority
    SetFocus(handle);                                   // Sets Keyboard Focus To The Window
}

bool init()
{
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
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
                }
            }
        }
    }
    
    return success;
}

SDL_Surface* load_surface(std::string path)
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

SDL_Texture* load_texture(std::string path)
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
            printf("Uanble to create texture from %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
        }
        
        SDL_FreeSurface(loadedSurface);
    }
    
    return newTexture;
}

void load_media()
{
    gSpriteSheetTexture = load_texture("foo.png");
    if(gSpriteSheetTexture != NULL)
    {
        for(int i = 0; i < WALKING_ANIMATION_FRAMES; i++)
        {
            gSpriteClips[i].x = i * 64;
            gSpriteClips[i].y = 0;
            gSpriteClips[i].w = 64;
            gSpriteClips[i].h = 205;
        }
        
        // gSpriteClips[0].x = 0;
        // gSpriteClips[0].y = 0;
        // gSpriteClips[0].w = 100;
        // gSpriteClips[0].h = 100;
        
        // gSpriteClips[1].x = 100;
        // gSpriteClips[1].y = 0;
        // gSpriteClips[1].w = 100;
        // gSpriteClips[1].h = 100;
        
        // gSpriteClips[2].x = 0;
        // gSpriteClips[2].y = 100;
        // gSpriteClips[2].w = 100;
        // gSpriteClips[2].h = 100;
        
        // gSpriteClips[3].x = 100;
        // gSpriteClips[3].y = 100;
        // gSpriteClips[3].w = 100;
        // gSpriteClips[3].h = 100;
    }
    
    SDL_SetTextureBlendMode(gSpriteSheetTexture, SDL_BLENDMODE_BLEND);
}

void close()
{
    SDL_DestroyTexture(gSpriteSheetTexture);
    gTexture = NULL;
    
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    gRenderer = NULL;
    
    IMG_Quit();
    SDL_Quit();
}

// NOTE(chris) deviating from the tutorial because i don't think we need a class for this
void render_texture_at_pos(SDL_Texture* texture, int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect renderQuad = {x, y, w, h};
    
    if(clip != NULL)
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    
    SDL_RenderCopyEx(gRenderer, texture, clip, &renderQuad, angle, center, flip);
}

// NOTE(chris) ctrl + shift + b builds & runs! (see tasks.json)
int main(int argc, char* args[])
{
    init();
    // TODO(chris) this should be debug builds only
    create_debug_console();
    load_media();
    
    bool quit = false;
    SDL_Event e;
    
    double degrees = 0;
    SDL_RendererFlip flipType = SDL_FLIP_NONE;
    
    int frame = 0;
    
    while(!quit)
    {
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
                    
                    case SDLK_a:
                    degrees -= 60;
                    break;
                    
                    case SDLK_d:
                    degrees += 60;
                    break;

                    case SDLK_q:
                    flipType = SDL_FLIP_HORIZONTAL;
                    break;

                    case SDLK_w:
                    flipType = SDL_FLIP_NONE;
                    break;

                    case SDLK_e:
                    flipType = SDL_FLIP_VERTICAL;
                    break;
                    
                    default:
                    break;
                }
            }
        }
        
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        
        // SDL_SetTextureColorMod(gSpriteSheetTexture, r, g, b);
        // SDL_SetTextureAlphaMod(gSpriteSheetTexture, a);
        
        SDL_Rect* currentFrame = &gSpriteClips[frame / 4];
        render_texture_at_pos(gSpriteSheetTexture, (SCREEN_WIDTH - currentFrame->w) / 2, (SCREEN_HEIGHT - currentFrame->h) / 2, currentFrame, degrees, NULL, flipType);
        
        SDL_RenderPresent(gRenderer);
        
        ++frame;
        
        if(frame / 4 >= WALKING_ANIMATION_FRAMES)
        {
            frame = 0;
        }
    }

    close();
    
    return 0;
}