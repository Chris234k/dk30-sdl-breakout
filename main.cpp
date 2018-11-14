#include "SDL.h"
#include "SDL_syswm.h"

#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <Fcntl.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;
SDL_Surface* gHelloWorld = NULL;

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

bool load_media()
{
    bool success = true;
    
    gHelloWorld = SDL_LoadBMP("hello-world.png");
    if(gHelloWorld == NULL)
    {
        printf("Unable to load image %s! SDL Error %s\n", "hello-world.png", SDL_GetError());
        
        SDL_Delay(5000);
        success = false;
    }
    
    return success;
}

void close()
{
    SDL_FreeSurface(gHelloWorld);
    gHelloWorld = NULL;
    
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
        SDL_BlitSurface(gHelloWorld, NULL, gScreenSurface, NULL);
        SDL_UpdateWindowSurface(gWindow);
    }
    
    close();
    
    return 0;
}