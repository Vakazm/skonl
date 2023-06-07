#include "server.h"
#include "client.h"
#include <windows.h>

#include "params.h"
#include <fstream>
#include <ctime>
using namespace std;
void genmap ()
{
    srand (time (0));
    ofstream fout ("snake.map", ios::binary | ios::trunc);
    char fx = 50, fy = 50;
    fout.put (fx); fout.put (0);
    fout.put (fy); fout.put (0);
    fout.put (0); fout.put (4); fout.put (6); fout.put (8);
    int mpl = 6;
    fout.put ('M'); fout.put ('a'); fout.put ('p'); fout.put ('k'); fout.put ('a'); fout.put ('`');
    while (mpl != global::mapname_length) {fout.put (' '); mpl ++;}
    char wallid = 6;
    for (int i = 0; i < fy; i ++) {fout.put (wallid);}
    for (int i = 1; i < fx - 1; i ++)
    {
        fout.put (wallid);
        for (int j = 1; j < fy - 1; j ++)
        {
            if (rand () % 40 == 0) {fout.put (7);}
            else {fout.put (rand () % 4);}
        }
        fout.put (wallid);
    }
    for (int i = 0; i < fy; i ++) {fout.put (wallid);}
    fout.close ();
}

LRESULT CALLBACK wcb (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) //default main window procedure
{
    switch (umsg)
    {
        case WM_CLOSE:
            PostQuitMessage (0);
            break;
        case WM_DESTROY:
            return 0;
        case WM_KEYDOWN:
        {
            switch (wparam)
            {
                case VK_ESCAPE:
                    PostQuitMessage (0);
                    break;
            }
        }
            break;
        case WM_COMMAND:
        {
            int cid = LOWORD (wparam);
            switch (cid)
            {
                case 1001:
                    PostMessage (hwnd, WM_USER, 0, 1);
                    break;
                case 1002:
                    PostMessage (hwnd, WM_USER, 0, 2);
                    break;
            }
        }
            break;
        default:
            return DefWindowProc (hwnd, umsg, wparam, lparam);
    }
    return 0;
}

int main ()
{
    //genmap (); return 0;
    //ShowWindow (GetConsoleWindow (), SW_HIDE);
    WSADATA wsd;
    if (WSAStartup (MAKEWORD (2, 0), &wsd) != 0) {MessageBox (0, "WSAStartup error!", "Startup error!", MB_OK | MB_ICONERROR); return 0;} //couldn't initialize WSA
    //create and register main window class
    WNDCLASSEX wcmc;
    ZeroMemory (&wcmc, sizeof (wcmc));
    wcmc.lpszClassName = "mc";
    wcmc.cbSize = sizeof (WNDCLASSEX);
    wcmc.style = CS_OWNDC;
    wcmc.lpfnWndProc = wcb;
    if (!RegisterClassEx (&wcmc)) {return 1;}

    //get desktop parameters and define window sizes to place it in the center of screen
    RECT desktop;
    const HWND hwnd_desktop = GetDesktopWindow ();
    GetWindowRect (hwnd_desktop, &desktop);
    int winpx = desktop.right;
    int winpy = desktop.bottom;
    int wx = 386;
    int wy = 128;

    //create main window and fill it with content
    HWND start = CreateWindowEx (0, "mc", "Choose mode", WS_SYSMENU, winpx/2 - wx/2, winpy/2 - wy/2, wx, wy, 0, 0, 0, 0);
    CreateWindowEx (0, "Button", "Client", WS_VISIBLE | WS_CHILD, 20, 20, 160, 60, start, (HMENU) 1001, 0, 0);
    CreateWindowEx (0, "Button", "Server", WS_VISIBLE | WS_CHILD, 200, 20, 160, 60, start, (HMENU) 1002, 0, 0);
    ShowWindow (start, SW_SHOW);
    MSG msg;
    while (true) //main loop
    {
        if (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {break;}
            else if (msg.message == WM_USER)
            {
                EnableWindow (start, FALSE);
                ShowWindow (start, SW_HIDE);
                if (msg.lParam == 1) {clientrun ();} //run client
                else if (msg.lParam == 2) {serverrun ();} //run server
                ShowWindow (start, SW_SHOW);
                EnableWindow (start, TRUE);
            }
            else
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
        }
    }
    DestroyWindow (start);
    WSACleanup ();
    return 0;
}
