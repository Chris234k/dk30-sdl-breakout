#include "SDL.h"
#include "SDL_syswm.h"
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <string>
#include <stdio.h>
#include <iostream>
#include <Fcntl.h>

#ifdef _WIN32
#include <windows.h>
#endif

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Texture* gTextTexture = NULL;
TTF_Font *gFont = NULL;

float textureWidth, textureHeight;


const int BUTTON_WIDTH = 300;
const int BUTTON_HEIGHT = 200;
const int TOTAL_BUTTONS = 4;

enum LButtonSprite
{
    BUTTON_SPRITE_MOUSE_OUT = 0,
    BUTTON_SPRITE_MOUSE_OVER_MOTION = 1,
    BUTTON_SPRITE_MOUSE_DOWN = 2,
    BUTTON_SPRITE_MOUSE_UP = 3,
    
    BUTTON_SPRITE_MOUSE_TOTAL = 4
};

class LButton
{
    public:
    LButton();
    
    SDL_Point position;
    
    void handleEvent(SDL_Event* e);
    
    void render();
    
    LButtonSprite currentSprite;
};

SDL_Rect gSpriteClips[4];
SDL_Texture* gButtonSpriteSheetTexture;

LButton gButtons[TOTAL_BUTTONS];

LButton::LButton()
{
    position.x = 0;
    position.y = 0;
    
    currentSprite = BUTTON_SPRITE_MOUSE_OUT;
}

void LButton::handleEvent(SDL_Event* e)
{
    if(e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP)
    {
        int x, y;
        SDL_GetMouseState(&x, &y);
        
        bool inside = true;
        
        if(x < position.x)
        {
            inside = false;
        }
        else if(x > position.x + BUTTON_WIDTH)
        {
            inside = false;
        }
        else if(y < position.y)
        {
            inside = false;
        }
        else if(y > position.y + BUTTON_HEIGHT)
        {
            inside = false;
        }
        
        if(inside)
        {
           switch(e-> type)
           {
               case SDL_MOUSEMOTION:
               currentSprite = BUTTON_SPRITE_MOUSE_OVER_MOTION;
               break;
               
               case SDL_MOUSEBUTTONDOWN:
               currentSprite = BUTTON_SPRITE_MOUSE_DOWN;
               break;
               
               case SDL_MOUSEBUTTONUP:
               currentSprite = BUTTON_SPRITE_MOUSE_UP;
               break;
           }
        }
        else
        {
            currentSprite = BUTTON_SPRITE_MOUSE_OUT; 
        }
    }
}

void LButton::render()
{
    render_texture_at_pos(gButtonSpriteSheetTexture, position.x, position.y, &gSpriteClips[currentSprite]);
}

SDL_Surface* load_surface_from_file(std::string path)
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

SDL_Texture* load_texture_from_file(std::string path)
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

#ifdef _SDL_TTF_H
SDL_Texture* load_text_from_rendered_text(std::string textureText, SDL_Color textColor)
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
            textureWidth = textSurface->w;
            textureHeight = textSurface->h;
        }
        
        SDL_FreeSurface(textSurface);
    }
    
    return result;
}
#endif

// NOTE(chris) deviating from the tutorial because i don't think we need a class for this
void render_texture_at_pos(SDL_Texture* texture, int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE)
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
    
#ifdef _WIN32
    { // create debug console
        // TODO(chris) this should be debug builds only
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
#endif

    { // load media
        gFont = TTF_OpenFont("lazy.ttf", 28);
        if(gFont == NULL)
        {
            printf("Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
        }
        else
        {
            SDL_Color textColor = {0, 0, 0};
            gTextTexture = load_text_from_rendered_text("The quick brown fox jumps over the lazy dog", textColor);
        }
    }
    
    bool quit = false;
    SDL_Event e;
    
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
                    
                    default:
                    break;
                }
            }
            
            for(int i = 0; i < TOTAL_BUTTONS; ++i)
            {
                gButtons[i].handleEvent(&e);
            }
        }
        
        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);
        
        for(int i = 0; i < TOTAL_BUTTONS; ++i)
        {
            gButtons[i].render();
        }
        
        SDL_RenderPresent(gRenderer);
    }

    { // close
        SDL_DestroyTexture(gTextTexture);
    
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