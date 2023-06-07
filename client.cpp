#include "client.h"
#include "params.h"
#include <winsock2.h>
#include <windows.h>
#include <gl/gl.h>
#include <string>

int winmx = 406;
int winmy = 428;

LRESULT CALLBACK ccb (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) //default client window procedure
{
    switch (umsg)
    {
        case WM_CLOSE:
            if (MessageBox (hwnd, "Are you sure you want to leave?", "Exit", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
            {PostQuitMessage (0);}
            break;
        case WM_DESTROY:
            return 0;
        case WM_KEYDOWN:
        {
            switch (wparam)
            {
                case VK_ESCAPE:
                    if (MessageBox (hwnd, "Are you sure you want to leave?", "Exit", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
                    {PostQuitMessage (0);}
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
                    PostMessage (hwnd, WM_USER, 0, 1);
                    break;
                case 1003:
                    PostMessage (hwnd, WM_USER, 0, 1);
                    break;
                case 1004:
                    PostMessage (hwnd, WM_USER, 0, 1);
                    break;
                case 1005:
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

LRESULT CALLBACK gcb (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) //default game window procedure
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
                    //PostQuitMessage (0);
                    break;
                case VK_UP:
                    PostMessage (hwnd, WM_USER, 0, 'w');
                    break;
                case VK_DOWN:
                    PostMessage (hwnd, WM_USER, 0, 's');
                    break;
                case VK_LEFT:
                    PostMessage (hwnd, WM_USER, 0, 'a');
                    break;
                case VK_RIGHT:
                    PostMessage (hwnd, WM_USER, 0, 'd');
                    break;
                case 'W':
                    PostMessage (hwnd, WM_USER, 0, 'w');
                    break;
                case 'S':
                    PostMessage (hwnd, WM_USER, 0, 's');
                    break;
                case 'A':
                    PostMessage (hwnd, WM_USER, 0, 'a');
                    break;
                case 'D':
                    PostMessage (hwnd, WM_USER, 0, 'd');
                    break;
            }
        }
            break;
        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO mmi = (LPMINMAXINFO) lparam;
            mmi -> ptMinTrackSize.x = winmx;
            mmi -> ptMinTrackSize.y = winmy;
        }
            break;
        case WM_SIZE:
            glViewport (0, 0, LOWORD (lparam), HIWORD (lparam));
            break;
        default:
            return DefWindowProc (hwnd, umsg, wparam, lparam);
    }
    return 0;
}

DWORD WINAPI waiter (LPVOID lparam) //thread-based function to connect to the server
{
    struct towai {HWND clin; SOCKET sock; char* tar;} params = *(towai*) lparam; //decode arguments
    if (recv (params.sock, params.tar, 4, 0) == SOCKET_ERROR)
    {
        MessageBox (params.clin, "Couldn't receive data from server!", "Socket error!", MB_OK | MB_ICONERROR);
        params.tar [0] = '0';
    }
    return 0;
}

struct spack {HWND game; SOCKET sock; bool* alive; bool* gameon;  bool* cbuf; char* conchar; unsigned char* dview; unsigned char** buf;}; //receiver thread parameters structure
DWORD WINAPI receiver (LPVOID lparam) //thread-based function to receive map updates
{
    spack params = *(spack*) lparam;
    char tdv [2];
    if (recv (params.sock, tdv, 2, 0) == SOCKET_ERROR) //receive basic data
    {
        MessageBox (params.game, "Couldn't receive info from server!", "Socket error!", MB_OK | MB_ICONERROR);
        PostMessage (params.game, WM_USER, 0, 1);
        return 0;
    }
    if ((tdv [0] != '0' && tdv [0] != '1' && tdv [0] != 'q') || tdv [1] == 0) //wrong data, flush buffer
    {
        unsigned long block = 1;
        if (ioctlsocket (params.sock, FIONBIO, &block) == SOCKET_ERROR)
        {
            MessageBox (params.game, "Couldn't flush receive buffer!", "Socket error!", MB_OK | MB_ICONERROR);
            PostMessage (params.game, WM_USER, 0, 1);
            return 0;
        }
        while (recv (params.sock, tdv, 1, 0) != SOCKET_ERROR) {}
        block = 0;
        if (ioctlsocket (params.sock, FIONBIO, &block) == SOCKET_ERROR)
        {
            MessageBox (params.game, "Couldn't flush receive buffer!", "Socket error!", MB_OK | MB_ICONERROR);
            PostMessage (params.game, WM_USER, 0, 1);
            return 0;
        }
    }
    else //receive map
    {
        if (tdv [0] == '1') {*(params.alive) = true;}
        else if (tdv [0] == '0') {*(params.alive) = false;}
        if (tdv [0] == 'q')
        {
            *(params.dview) = tdv [1];
            *(params.gameon) = false;
            return 0;
        }
        else
        {
            if ((unsigned char) tdv [1] != *(params.dview))
            {
                *(params.dview) = tdv [1];
                delete [] *(params.buf);
                *(params.buf) = new unsigned char [short (*(params.dview)) * *(params.dview)];
            }
            if (recv (params.sock, reinterpret_cast <char*> (*(params.buf)), short (*(params.dview)) * *(params.dview), 0) == SOCKET_ERROR)
            {
                MessageBox (params.game, "Couldn't receive map from server!", "Socket error!", MB_OK | MB_ICONERROR);
                PostMessage (params.game, WM_USER, 0, 1);
                return 0;
            }
        }
    }
    char tosend [1] = {*(params.conchar)};
    send (params.sock, tosend, 1, 0);
    *(params.conchar) = ' ';
    *(params.cbuf) = !*(params.cbuf);
    return 0;
}

void clientrun () //main client function
{
    WSADATA wsd;
    if (WSAStartup (MAKEWORD (2, 0), &wsd) != 0) {MessageBox (0, "WSAStartup error!", "Startup error!", MB_OK | MB_ICONERROR); return;}

    //create and register client window class
    WNDCLASSEX wcmc;
    ZeroMemory (&wcmc, sizeof (wcmc));
    wcmc.lpszClassName = "cl";
    wcmc.cbSize = sizeof (WNDCLASSEX);
    wcmc.style = CS_OWNDC;
    wcmc.lpfnWndProc = ccb;
    wcmc.hbrBackground = CreateSolidBrush (RGB (255, 128, 64));
    if (!RegisterClassEx (&wcmc)) {return;}

    //get desktop parameters and define window sizes to place it in the center of screen
    RECT desktop;
    const HWND hwnd_desktop = GetDesktopWindow ();
    GetWindowRect (hwnd_desktop, &desktop);
    int winpx = desktop.right;
    int winpy = desktop.bottom;
    int wx = 476;
    int wy = 128;

    char frbuf [4];
    bool gameon = true;
    SOCKET sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //start client
    {
        char nickname [global::nick_length];
        bool pht = false;
        frbuf [0] = '-';
        HANDLE thcon = 0;
        std::string slip;
        //get self IP address
        {
            unsigned char lip [4];
            char tbuf [1024];
            gethostname (tbuf, sizeof (tbuf));
            hostent *host = gethostbyname (tbuf);
            lip [0] = ((in_addr*)(host->h_addr))->S_un.S_un_b.s_b1;
            lip [1] = ((in_addr*)(host->h_addr))->S_un.S_un_b.s_b2;
            lip [2] = ((in_addr*)(host->h_addr))->S_un.S_un_b.s_b3;
            lip [3] = ((in_addr*)(host->h_addr))->S_un.S_un_b.s_b4;
            char tcb [4];
            itoa (lip [0], tcb, 10); slip += tcb; slip += '.';
            itoa (lip [1], tcb, 10); slip += tcb; slip += '.';
            itoa (lip [2], tcb, 10); slip += tcb; slip += '.';
            itoa (lip [3], tcb, 10); slip += tcb;
        }

        //create client window and fill it with content
        HWND clin = CreateWindowEx (0, "cl", "Client", WS_SYSMENU, winpx/2 - wx/2, winpy/2 - wy/2, wx, wy, 0, 0, 0, 0);
        CreateWindowEx (0, "Edit", "Server IP: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 10, 10, 80, 20, clin, 0, 0, 0);
        HWND e_sip = CreateWindowEx (0, "Edit", slip.c_str (), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, 90, 10, 140, 20, clin, (HMENU) 1001, 0, 0);
        CreateWindowEx (0, "Edit", "Port: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 240, 10, 80, 20, clin, 0, 0, 0);
        HWND e_port = CreateWindowEx (0, "Edit", "1111", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 320, 10, 140, 20, clin, (HMENU) 1002, 0, 0);
        CreateWindowEx (0, "Edit", "PIN: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 10, 40, 80, 20, clin, 0, 0, 0);
        HWND e_pin = CreateWindowEx (0, "Edit", "1111", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 90, 40, 140, 20, clin, (HMENU) 1003, 0, 0);
        CreateWindowEx (0, "Edit", "Nickname: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 240, 40, 80, 20, clin, 0, 0, 0);
        HWND e_nick = CreateWindowEx (0, "Edit", "Player", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, 320, 40, 140, 20, clin, (HMENU) 1004, 0, 0);
        HWND b_connect = CreateWindowEx (0, "Button", "Connect", WS_VISIBLE | WS_CHILD | WS_DISABLED, 10, 70, 220, 20, clin, (HMENU) 1005, 0, 0);
        CreateWindowEx (0, "Edit", "Status: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 240, 70, 80, 20, clin, 0, 0, 0);
        HWND e_status = CreateWindowEx (0, "Edit", "Not connected", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 320, 70, 140, 20, clin, 0, 0, 0);
        SendMessage (e_sip, EM_SETLIMITTEXT, 15, 0);
        SendMessage (e_port, EM_SETLIMITTEXT, 4, 0);
        SendMessage (e_pin, EM_SETLIMITTEXT, 6, 0);
        SendMessage (e_nick, EM_SETLIMITTEXT, global::nick_length - 1, 0);
        ShowWindow (clin, SW_SHOW);
        MSG msg;

        struct towai {HWND in_hwnd; SOCKET in_sock; char* in_frbuf;} tc {clin, sock, frbuf}; //waiter thread parameters structure
        while (frbuf [0] == '-') //client lobby loop
        {
            if (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT) {break;}
                else if (msg.message == WM_USER)
                {
                    switch (msg.lParam)
                    {
                        case 1: //check all fields input
                            if (pht) {break;}
                            if (GetWindowTextLength (e_sip) > 0 && GetWindowTextLength (e_port) > 0 && GetWindowTextLength (e_pin) > 0 && GetWindowTextLength (e_nick) > 0)
                            {
                                char tcb1 [6];
                                int tl1 = GetWindowTextLength (e_port);
                                GetWindowText (e_port, tcb1, 5);
                                char tcb2 [8];
                                int tl2 = GetWindowTextLength (e_pin);
                                GetWindowText (e_pin, tcb2, 7);
                                char tcb3 [17];
                                GetWindowText (e_sip, tcb3, 16);
                                bool estbt = !(inet_addr (tcb3) == INADDR_NONE || inet_addr (tcb3) == 0);
                                for (int i = 0; i < tl1; i ++)
                                {
                                    if (tcb1 [i] < 48 || tcb1 [i] > 57) {estbt = false;}
                                }
                                for (int i = 0; i < tl2; i ++)
                                {
                                    if (tcb2 [i] < 48 || tcb2 [i] > 57) {estbt = false;}
                                }
                                EnableWindow (b_connect, estbt);
                            }
                            else {EnableWindow (b_connect, FALSE);}
                            break;
                        case 2: //connecting to server
                        {
                            char sip [17];
                            char sport [6];
                            char spin [8];
                            GetWindowText (e_sip, sip, 16);
                            GetWindowText (e_port, sport, 5);
                            GetWindowText (e_pin, spin, 7);
                            GetWindowText (e_nick, nickname, global::nick_length);

                            sockaddr_in addr;
                            addr.sin_family = AF_INET;
                            addr.sin_addr.s_addr = inet_addr (sip);
                            int lport = atoi (sport);
                            addr.sin_port = htons (lport);
                            char tresbuf [1];

                            if (connect (sock, (sockaddr*) &addr, sizeof (addr)) == SOCKET_ERROR)
                            {
                                MessageBox (clin, "Couldn't connect to server!", "Connection error!", MB_OK | MB_ICONERROR);
                                SetWindowText (e_status, "Connection error");
                            }
                            else if (send (sock, spin, 8, 0) == SOCKET_ERROR || send (sock, nickname, global::nick_length, 0) == SOCKET_ERROR)
                            {
                                MessageBox (clin, "Couldn't send data to server!", "Socket error!", MB_OK | MB_ICONERROR);
                                SetWindowText (e_status, "Sending data error");
                            }
                            else if (recv (sock, tresbuf, 1, 0) == SOCKET_ERROR)
                            {
                                MessageBox (clin, "Couldn't receive data from server!", "Socket error!", MB_OK | MB_ICONERROR);
                                SetWindowText (e_status, "Receiving data error");
                            }
                            else if (tresbuf [0] == 'f')
                            {
                                MessageBox (clin, "Wrong PIN!", "Connection error!", MB_OK | MB_ICONERROR);
                                SetWindowText (e_status, "Wrong PIN");
                                closesocket (sock);
                                sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
                            }
                            else
                            {
                                pht = true;
                                SendMessage (e_sip, EM_SETREADONLY, 1, 0);
                                SendMessage (e_port, EM_SETREADONLY, 1, 0);
                                SendMessage (e_pin, EM_SETREADONLY, 1, 0);
                                SendMessage (e_nick, EM_SETREADONLY, 1, 0);
                                EnableWindow (b_connect, FALSE);
                                //MessageBox (clin, "Connected to server!", "Success!", MB_OK | MB_ICONASTERISK);
                                SetWindowText (e_status, "Connected");
                                thcon = CreateThread (0, 0, waiter, (LPVOID) &tc, 0, 0);
                            }
                        }
                            break;
                    }
                }
                else
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
        }
        if (WaitForSingleObject (thcon, 100) != WAIT_OBJECT_0) {TerminateThread (thcon, 0);} //terminate waiter thread
        DestroyWindow (clin);
        if (frbuf [0] == '-' || frbuf [0] == '0') //game cancelled
        {
            closesocket (sock);
            UnregisterClass ("cl", 0);
            return;
        }
    }

    //proceed game
    spack pack;
    {
        unsigned int textid = 0;
        unsigned char palette [256];
        unsigned char dkf = 1;
        float gridkf = 1.0 / dkf;
        char mapname [global::mapname_length];

        dkf = frbuf [1];
        gridkf = 1.0 / dkf;
        unsigned short gs = frbuf [3];
        gs <<= 8;
        gs |= frbuf [2];
        int mlen = 3 * gs * gs;
        unsigned char* bmpbuf = new unsigned char [mlen];
        //receive all basic data
        if (recv (sock, mapname, global::mapname_length, 0) == SOCKET_ERROR)
        {MessageBox (0, "Couldn't receive map name!", "Socket error!", MB_OK | MB_ICONERROR); closesocket (sock); UnregisterClass ("cl", 0); return;}
        if (recv (sock, reinterpret_cast <char*> (palette), 256, 0) == SOCKET_ERROR)
        {MessageBox (0, "Couldn't receive palette!", "Socket error!", MB_OK | MB_ICONERROR); closesocket (sock); UnregisterClass ("cl", 0); return;}
        if (recv (sock, reinterpret_cast <char*> (bmpbuf), mlen, 0) == SOCKET_ERROR)
        {MessageBox (0, "Couldn't receive texture!", "Socket error!", MB_OK | MB_ICONERROR); closesocket (sock); UnregisterClass ("cl", 0); return;}

        //create and register game class
        WNDCLASSEX wcgc;
        ZeroMemory (&wcgc, sizeof (wcgc));
        wcgc.lpszClassName = "gc";
        wcgc.cbSize = sizeof (WNDCLASSEX);
        wcgc.style = CS_OWNDC;
        wcgc.lpfnWndProc = gcb;
        if (!RegisterClassEx (&wcgc)) {return;}

        //create game window and fill it with content
        //char gwname [16 + global::mapname_length] = "Playing on map "; strcat (gwname, mapname);
        HWND game = CreateWindowEx (0, "gc", (std::string ("Playing on map ") + mapname).c_str (), WS_OVERLAPPEDWINDOW, winpx/2 - winmx/2, winpy/2 - winmy/2, winmx, winmy, 0, 0, 0, 0);
        HDC hdc = GetDC (game);
        PIXELFORMATDESCRIPTOR pfd;
        ZeroMemory (&pfd, sizeof (pfd));

        //initialize OpenGL
        pfd.nSize = sizeof (pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cDepthBits = 16;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int iFormat = ChoosePixelFormat (hdc, &pfd);
        SetPixelFormat (hdc, iFormat, &pfd);
        HGLRC hrc = wglCreateContext (hdc);
        wglMakeCurrent (hdc, hrc);

        glGenTextures (1, &textid);
        glBindTexture (GL_TEXTURE_2D, textid);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, gs, gs, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bmpbuf);
        glBindTexture (GL_TEXTURE_2D, 0);
        delete [] bmpbuf;

        bool alive = true;
        char conchar = ' ';
        unsigned char dview0 = 0;
        unsigned char dview1 = 0;
        unsigned char* buf0 = nullptr;
        unsigned char* buf1 = nullptr;
        bool cbuf = 0;
        bool lastcbuf = 1;
        HANDLE curt = 0;
        pack = spack {game, sock, &alive, &gameon, &cbuf, &conchar, &dview0, &buf0}; //receiver thread parameters

        ShowWindow (game, SW_SHOW);
        SetForegroundWindow (game);
        SetFocus (game);
        SetActiveWindow (game);
        if (frbuf [0] == 'o') //spectator mode
        {
            CreateThread (0, 0, [](LPVOID data) -> DWORD {
                          MessageBox (0, "Sorry, there was no place for you...", "Spectator mode", MB_OK | MB_ICONINFORMATION); return 0;
                          }, nullptr, 0, 0);
        }
        if (send (sock, " ", 1, 0) == SOCKET_ERROR) {}
        MSG msg;
        while (gameon) //game loop
        {
            if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) //respond to window events
            {
                if (msg.message == WM_QUIT) {break;}
                else if (msg.message == WM_USER)
                {
                    if (msg.lParam == 1) {break;}
                    else {conchar = char (msg.lParam);}
                }
                else
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
            else //draw game
            {
                if (lastcbuf != cbuf) //received update, switch buffers
                {
                    if (!alive && frbuf [0] == 'i') //player died, spectator mode
                    {
                        CreateThread (0, 0, [](LPVOID data) -> DWORD {
                                      MessageBox (0, "You died, lol :D", "Spectator mode", MB_OK | MB_ICONINFORMATION); return 0;
                                      }, nullptr, 0, 0);
                        frbuf [0] = 'o';
                    }
                    lastcbuf = cbuf;
                    if (WaitForSingleObject (curt, 5) != WAIT_OBJECT_0) {TerminateThread (curt, 0);} //terminate receiver thread
                    if (!gameon) {break;}
                    if (cbuf)
                    {
                        pack.dview = &dview1;
                        pack.buf = &buf1;
                        curt = CreateThread (0, 0, receiver, (LPVOID) &pack, 0, 0);
                    }
                    else
                    {
                        pack.dview = &dview0;
                        pack.buf = &buf0;
                        curt = CreateThread (0, 0, receiver, (LPVOID) &pack, 0, 0);
                    }
                }
                unsigned char curdv = dview1;
                unsigned char* curbuf = buf1;
                if (cbuf)
                {
                    curdv = dview0;
                    curbuf = buf0;
                }

                //draw received data
                float cellsize = 2.0 / curdv;
                glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
                glClear (GL_COLOR_BUFFER_BIT);
                glEnable (GL_TEXTURE_2D);
                glBindTexture (GL_TEXTURE_2D, textid);
                glColor3f (1, 1, 1);
                glPushMatrix ();
                    glEnableClientState (GL_VERTEX_ARRAY);
                    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
                    static float vertex [] = {-1, -1,  1, -1,  1, 1,  -1, 1};
                    for (unsigned char i = 0; i < curdv; i ++)
                    {
                        for (unsigned char j = 0; j < curdv; j ++)
                        {
                            int px = palette [curbuf [curdv*i + j]] % dkf;
                            int py = palette [curbuf [curdv*i + j]] / dkf;
                            float tcord [] = {px*gridkf, 1 - py*gridkf - gridkf,    px*gridkf + gridkf, 1 - py*gridkf - gridkf,
                            px*gridkf + gridkf, 1 - py*gridkf,    px*gridkf, 1 - py*gridkf};
                            glLoadIdentity ();
                            glTranslatef (j*cellsize - 1 + cellsize/2, 1 - i*cellsize - cellsize/2, 0);
                            glScalef (cellsize/2, cellsize/2, 0);
                            glBindTexture (GL_TEXTURE_2D, textid);
                            glVertexPointer (2, GL_FLOAT, 0, vertex);
                            glTexCoordPointer (2, GL_FLOAT, 0, tcord);
                            glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
                        }
                    }
                    glDisableClientState (GL_VERTEX_ARRAY);
                    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
                glPopMatrix ();
                glBindTexture (GL_TEXTURE_2D, 0);
                glDisable (GL_TEXTURE_2D);
                SwapBuffers (hdc);
            }
        }
        if (WaitForSingleObject (curt, 100) != WAIT_OBJECT_0) {TerminateThread (curt, 0);} //terminate receiver thread
        //cleanup
        glDeleteTextures (1, &textid);
        wglMakeCurrent (NULL, NULL);
        wglDeleteContext (hrc);
        ReleaseDC (game, hdc);
        DestroyWindow (game);
        UnregisterClass ("gc", 0);
        delete [] buf0;
        delete [] buf1;
    }

    //leaderboard
    if (!gameon)
    {
        unsigned int ldsize = 1 + (global::nick_length + 2) * (*(pack.dview));
        char lddata [ldsize];
        recv (sock, lddata, ldsize, 0); //get leaderboard data
        //create leaderboard window and fill it with content
        HWND leaderboard = CreateWindowEx (0, "cl", "Leaderboard", WS_SYSMENU, winpx/2 - 193, winpy/2 - 284, 386, 568, 0, 0, 0, 0);
        HWND lb_scores = CreateWindowEx (0, "ListBox", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_DISABLENOSCROLL, 0, 0, 380, 548, leaderboard, 0, 0, 0);
        for (unsigned char i = 0; i < *(pack.dview); i ++) //fill leaderboard with players' scores list
        {
            unsigned short score = lddata [2 + (global::nick_length + 2)*i];
            score <<= 8;
            score |= lddata [1 + (global::nick_length + 2)*i];
            char* tempstr = nullptr;
            if (i == lddata [0]) {tempstr = new char [global::nick_length + 30];}
            else {tempstr = new char [global::nick_length + 20];}
            itoa (score, tempstr, 10);
            unsigned char j = 0;
            for (; j < 7; j ++) {if (tempstr [j] == '\0') {break;}}
            for (; j < 20; j ++) {tempstr [j] = ' ';}
            if (i == lddata [0])
            {
                tempstr [20] = ' ';
                tempstr [21] = ' ';
                tempstr [22] = '[';
                tempstr [23] = 'Y';
                tempstr [24] = 'o';
                tempstr [25] = 'u';
                tempstr [26] = ']';
                tempstr [27] = ' ';
                tempstr [28] = ' ';
                tempstr [29] = ' ';
                for (unsigned char u = 0; u < global::nick_length; u ++) {tempstr [30 + u] = lddata [3 + (global::nick_length + 2)*i + u];}
            }
            else {for (unsigned char u = 0; u < global::nick_length; u ++) {tempstr [20 + u] = lddata [3 + (global::nick_length + 2)*i + u];}}
            SendMessage (lb_scores, LB_INSERTSTRING, i, (LPARAM) tempstr);
            delete [] tempstr;
        }

        ShowWindow (leaderboard, SW_SHOW);
        MSG msg;
        while (true) //leaderboard loop
        {
            if (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT) {break;}
                else
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
        }
        DestroyWindow (leaderboard);
    }

    UnregisterClass ("cl", 0);
    closesocket (sock);
}
