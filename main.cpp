#include "SDL.h"
#include "SDL_syswm.h"

#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <Fcntl.h>

enum KeyPressSurfaces
{
    KEY_PRESS_SURFACE_DEFAULT,
    KEY_PRESS_SURFACE_UP,
    KEY_PRESS_SURFACE_DOWN,
    KEY_PRESS_SURFACE_LEFT,
    KEY_PRESS_SURFACE_RIGHT,
    KEY_PRESS_SURFACE_TOTAL
};

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;

SDL_Surface* gKeyPressSurfaces[KEY_PRESS_SURFACE_TOTAL];
SDL_Surface* gCurrentSurface = NULL;


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
            gScreenSurface = SDL_GetWindowSurface(gWindow);
            
            // Color the screen white
            SDL_FillRect(gScreenSurface, NULL, SDL_MapRGB(gScreenSurface->format, 0xFF, 0xFF, 0xFF));
            
            SDL_UpdateWindowSurface(gWindow);
        }
    }
    
    return success;
}

SDL_Surface* load_surface(std::string path)
{
    SDL_Surface* optimizedSurface = NULL;
    
    SDL_Surface* loadedSurface = SDL_LoadBMP(path.c_str());
    if(loadedSurface == NULL)
    {
        printf("Unable to load image %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
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
    }
    
    return optimizedSurface;
}

bool load_media()
{
    bool success = true;

    gKeyPressSurfaces[KEY_PRESS_SURFACE_DEFAULT] = load_surface("stretch.bmp");
    if(gKeyPressSurfaces[KEY_PRESS_SURFACE_DEFAULT] == NULL)
    {
        printf("Failed to load default image!\n");
        success = false;
    }

    gKeyPressSurfaces[KEY_PRESS_SURFACE_UP] = load_surface("up.bmp");
    if(gKeyPressSurfaces[KEY_PRESS_SURFACE_UP] == NULL)
    {
        printf("Failed to load up image!\n");
        success = false;
    }

    gKeyPressSurfaces[KEY_PRESS_SURFACE_DOWN] = load_surface("down.bmp");
    if(gKeyPressSurfaces[KEY_PRESS_SURFACE_DOWN] == NULL)
    {
        printf("Failed to load down image!\n");
        success = false;
    }

    gKeyPressSurfaces[KEY_PRESS_SURFACE_LEFT] = load_surface("left.bmp");
    if(gKeyPressSurfaces[KEY_PRESS_SURFACE_LEFT] == NULL)
    {
        printf("Failed to load left image!\n");
        success = false;
    }

    gKeyPressSurfaces[KEY_PRESS_SURFACE_RIGHT] = load_surface("right.bmp");
    if(gKeyPressSurfaces[KEY_PRESS_SURFACE_RIGHT] == NULL)
    {
        printf("Failed to load right image!\n");
        success = false;
    }

    return success;
}

void close()
{
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    
    SDL_Quit();
}

// NOTE(chris) ctrl + shift + b builds & runs! (see tasks.json)
int main(int argc, char* args[])
{
    init();
    
    // TODO(chris) this should be debug builds only
    create_debug_console();
    
    if(load_media())
    {
        bool quit = false;
    
        SDL_Event e;
        
        gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_DEFAULT];
        while (!quit)
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
                        
                        case SDLK_UP:
                        gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_UP];
                        break;
                        
                        case SDLK_DOWN:
                        gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_DOWN];
                        break;
                        
                        case SDLK_LEFT:
                        gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_LEFT];
                        break;
                        
                        case SDLK_RIGHT:
                        gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_RIGHT];
                        break;
                        
                        default:
                        gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_DEFAULT];
                        break;
                    }
                }
            }
            
            SDL_Rect stretchRect;
            stretchRect.x = 0;
            stretchRect.y = 0;
            stretchRect.w = SCREEN_WIDTH;
            stretchRect.h = SCREEN_HEIGHT;
            SDL_BlitScaled(gCurrentSurface, NULL, gScreenSurface, &stretchRect);
            SDL_UpdateWindowSurface(gWindow);
        }
    }

    close();
    
    return 0;
}