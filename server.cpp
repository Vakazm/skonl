#include "server.h"
#include "params.h"
#include <fstream>
#include <queue>
#include <list>
#include <vector>
#include <ctime>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>

using namespace std;

struct snake //structure to store the information about player's snake
{
    bool alive = false;
    unsigned char id = 0;
    char direction = 0;
    short cx = 0;
    short cy = 0;
    unsigned char defview = 0, view = 0, dview = 2*view + 1;
    list < pair < short, short > > body;
    queue < pair < short, short > > belly;
    unsigned short score = 0;
};

struct player//structure to store the information about player
{
    SOCKET sock;
    sockaddr_in saddr;
    int cssl = sizeof (saddr);
    char nick [global::nick_length] = {};
    bool connected = true;
    bool disconnected = false;
    snake sk;
};

void filepick (HWND target) //function to pick the file
{
    char fname [MAX_PATH];
    fname [0] = 0;
    OPENFILENAME ofn;
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = 0;
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = fname;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select settings file";
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if (GetOpenFileName (&ofn)) {SetWindowText (target, fname);}
}

DWORD WINAPI accepter (LPVOID lparam) //thread-based function to accept new players
{
    struct toacc {vector < player >* players; SOCKET slisten; unsigned int pin; HWND lb; unsigned long long int* logcount;} params = *(toacc*) lparam; //decode arguments
    while (true)
    {
        player pl;
        char spin [8];
        if ((pl.sock = accept (params.slisten, (sockaddr*) &(pl.saddr), &(pl.cssl))) != 1ULL * SOCKET_ERROR &&
        recv (pl.sock, spin, 8, 0) != SOCKET_ERROR &&
        recv (pl.sock, pl.nick, global::nick_length, 0) != SOCKET_ERROR) //trying to receive data from player
        {
            unsigned int lpin = atoi (spin);
            if (lpin != params.pin) {send (pl.sock, "f", 1, 0);} //send wrong PIN response
            else
            {
                SendMessage (params.lb, LB_INSERTSTRING, *(params.logcount), (LPARAM) ("Player \"" + string (pl.nick) + "\" connected, IP: " + inet_ntoa (pl.saddr.sin_addr)).c_str ()); //add string to logs
                ++*(params.logcount);
                params.players -> push_back (pl); //add player to list
                send (pl.sock, "s", 1, 0); //send successfull connection response
            }
        }
    }
    return 0;
}

LRESULT CALLBACK scb (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) //default server window procedure
{
    switch (umsg)
    {
        case WM_CLOSE:
            if (MessageBox (hwnd, "Are you sure you want to exit?", "Exit", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {PostQuitMessage (0);}
            break;
        case WM_DESTROY:
            return 0;
        case WM_KEYDOWN:
        {
            switch (wparam)
            {
                case VK_ESCAPE:
                    //if (MessageBox (hwnd, "Are you sure you want to exit?", "Exit", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {PostQuitMessage (0);}
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
                    PostMessage (hwnd, WM_USER, 0, 2);
                    break;
                case 1011:
                    PostMessage (hwnd, WM_USER, 0, 3);
                    break;
                case 1012:
                    PostMessage (hwnd, WM_USER, 0, 4);
                    break;
            }
        }
            break;
        default:
            return DefWindowProc (hwnd, umsg, wparam, lparam);
    }
    return 0;
}

bool genfood (unsigned char* field, short fx, short fy, unsigned char mfree, unsigned char mfood, HWND lb_log, unsigned long long int& logcount) //food generation
{
    short rx = rand () % fx;
    short ry = rand () % fy;
    char seq [4] = {1, 2, 3, 4};
    random_shuffle (seq, seq + 4);
    short i, j;
    bool generated = false;
    for (int u = 0; u < 4; u ++) //4-squares method
    {
        switch (seq [u])
        {
            case 1:
                for (i = rx; i < fx; i ++)
                {
                    for (j = ry; j < fy; j ++)
                    {
                        if (field [fy*i + j] < mfree) {generated = true; goto out;}
                    }
                }
                break;
            case 2:
                for (i = rx; i < fx; i ++)
                {
                    for (j = ry; j >= 0; j --)
                    {
                        if (field [fy*i + j] < mfree) {generated = true; goto out;}
                    }
                }
                break;
            case 3:
                for (i = rx; i >= 0; i --)
                {
                    for (j = ry; j < fy; j ++)
                    {
                        if (field [fy*i + j] < mfree) {generated = true; goto out;}
                    }
                }
                break;
            case 4:
                for (i = rx; i >= 0; i --)
                {
                    for (j = ry; j >= 0; j --)
                    {
                        if (field [fy*i + j] < mfree) {generated = true; goto out;}
                    }
                }
                break;
        }
    }
    out:
    if (!generated) //no free space on the field
    {
        SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "Couldn't generate food"); logcount ++; //add string to logs
        return false;
    }
    field [fy*i + j] = mfree + rand () % (mfree - mfood); //place food
    SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("New food generated in " + to_string (i) + " " + to_string (j) + ", food id: " + to_string (field [fy*i + j])).c_str ()); logcount ++; //add string to logs
    return true;
}

void serverrun () //main server function
{
    srand (time (0));
    //create and register server window class
    WNDCLASSEX wcmc;
    ZeroMemory (&wcmc, sizeof (wcmc));
    wcmc.lpszClassName = "sv";
    wcmc.cbSize = sizeof (WNDCLASSEX);
    wcmc.style = CS_OWNDC;
    wcmc.lpfnWndProc = scb;
    wcmc.hbrBackground = CreateSolidBrush (RGB (64, 128, 255));
    if (!RegisterClassEx (&wcmc)) {return;}

    //get desktop parameters and define window sizes to place it in the center of screen
    RECT desktop;
    const HWND hwnd_desktop = GetDesktopWindow ();
    GetWindowRect (hwnd_desktop, &desktop);
    int winpx = desktop.right;
    int winpy = desktop.bottom;
    int wx = 686;
    int wy = 568;

    //initialize all global variables
    vector < player > players;
    short fx = 0;
    short fy = 0;
    unsigned char *field = nullptr, *original = nullptr;
    unsigned char mfree = 1, mfood = 2, mwall = 3;
    unsigned char sview = 8, viewgrow = 10;
    long long int delay = 0;
    unsigned long long int logcount = 0;
    HWND lb_log = CreateWindowEx (0, "ListBox", "", WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_DISABLENOSCROLL, 20, 60, 640, 440, 0, 0, 0, 0);

    //start server
    {
        sockaddr_in addr;
        SOCKET slisten = 0;
        unsigned short port = 0;
        unsigned int pin = 0;
        string slip;
        //get self IP address
        {
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
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
        //initialize other game and settings variables
        bool pht = false;
        bool goon = false;
        HANDLE thlis = 0;
        char mapname [global::mapname_length];
        char palette [256];
        unsigned char dkf = 1;
        unsigned short maxpls = 0;
        snake* fdt = nullptr;
        unsigned short gs = 0;
        char* gdata = nullptr;
        unsigned char foodprop = 1;

        //create server window and fill it with content
        HWND serv = CreateWindowEx (0, "sv", "Server", WS_SYSMENU, winpx/2 - wx/2, winpy/2 - wy/2, wx, 88, 0, 0, 0, 0);
        CreateWindowEx (0, "Edit", "Your IP: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 20, 20, 60, 20, serv, 0, 0, 0);
        HWND t_lip = CreateWindowEx (0, "Edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 80, 20, 110, 20, serv, 0, 0, 0);
        CreateWindowEx (0, "Edit", "Port: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 200, 20, 60, 20, serv, 0, 0, 0);
        HWND e_port = CreateWindowEx (0, "Edit", "1111", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 260, 20, 110, 20, serv, HMENU (1001), 0, 0);
        CreateWindowEx (0, "Edit", "PIN: ", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY, 380, 20, 60, 20, serv, 0, 0, 0);
        HWND e_pin = CreateWindowEx (0, "Edit", "1111", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER, 440, 20, 110, 20, serv, HMENU (1002), 0, 0);
        HWND b_start = CreateWindowEx (0, "Button", "Start server", WS_VISIBLE | WS_CHILD | WS_DISABLED, 560, 20, 100, 20, serv, (HMENU) 1003, 0, 0);
        CreateWindowEx (0, "Button", "Choose set file: ", WS_VISIBLE | WS_CHILD, 20, 500, 120, 20, serv, (HMENU) 1011, 0, 0);
        HWND t_mapfile = CreateWindowEx (0, "Edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_READONLY | ES_AUTOHSCROLL, 150, 500, 400, 20, serv, 0, 0, 0);
        HWND b_play = CreateWindowEx (0, "Button", "Start game", WS_VISIBLE | WS_CHILD | WS_DISABLED, 560, 500, 100, 20, serv, (HMENU) 1012, 0, 0);
        SetParent (lb_log, serv);
        ShowWindow (lb_log, SW_SHOW);
        SetWindowText (t_lip, slip.c_str ());
        SendMessage (e_port, EM_SETLIMITTEXT, 5, 0);
        SendMessage (e_pin, EM_SETLIMITTEXT, 6, 0);
        ShowWindow (serv, SW_SHOW);
        MSG msg;

        struct toacc {vector < player >* in_players; SOCKET in_slisten; unsigned int in_pin; HWND in_lb; unsigned long long int* in_logcount;} tc; //accepter thread parameters structure
        while (!goon) //server lobby loop
        {
            if (PeekMessage (&msg, 0, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT) {break;}
                else if (msg.message == WM_USER)
                {
                    switch (msg.lParam)
                    {
                        case 1: //check IP, port and PIN fields
                        {
                            if (pht) {break;}
                            char tcb1 [7];
                            int tl1 = GetWindowTextLength (e_port);
                            GetWindowText (e_port, tcb1, 6);
                            char tcb2 [8];
                            int tl2 = GetWindowTextLength (e_pin);
                            GetWindowText (e_pin, tcb2, 7);
                            bool estbt = true;
                            if (tl1 == 0 || tl2 == 0) {estbt = false;}
                            for (int i = 0; i < tl1; i ++)
                            {
                                if (tcb1 [i] < 48 || tcb1 [i] > 57) {estbt = false;}
                            }
                            for (int i = 0; i < tl2; i ++)
                            {
                                if (tcb2 [i] < 48 || tcb2 [i] > 57) {estbt = false;}
                            }
                            if (estbt && (atoi (tcb1) > 65534 || atoi (tcb1) < 1025)) {estbt = false;}
                            EnableWindow (b_start, estbt);
                        }
                            break;
                        case 2: //port binding and server starting
                        {
                            char tcb1 [6];
                            char tcb2 [8];
                            GetWindowText (e_port, tcb1, 5);
                            GetWindowText (e_pin, tcb2, 7);
                            port = atoi (tcb1);
                            pin = atoi (tcb2);
                            addr.sin_port = htons (port);
                            slisten = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
                            if (bind (slisten, (sockaddr*) &addr, sizeof (addr)) == SOCKET_ERROR) {MessageBox (serv, "Port binding error!", "Socket error!", MB_OK | MB_ICONERROR);}
                            else if (listen (slisten, 65536) == SOCKET_ERROR) {MessageBox (serv, "Port listening error!", "Socket error!", MB_OK | MB_ICONERROR);}
                            else
                            {
                                pht = true;
                                SendMessage (e_port, EM_SETREADONLY, 1, 0);
                                SendMessage (e_pin, EM_SETREADONLY, 1, 0);
                                MessageBox (serv, ("Server started on IP: " + slip + ", port: " + tcb1 + ". PIN to join: " + tcb2).c_str (), "Server started successfuly", MB_OK | MB_ICONASTERISK);
                                SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Server started on IP: " + slip + ", port: " + tcb1 + ". PIN to join: " + tcb2).c_str ()); logcount ++;
                                EnableWindow (b_start, FALSE);
                                SetWindowPos (serv, 0, 0, 0, wx, wy, SWP_NOMOVE);
                                tc = {&players, slisten, pin, lb_log, &logcount};
                                thlis = CreateThread (0, 0, accepter, (LPVOID) &tc, 0, 0); //launch accepter in a new thread
                            }
                        }
                            break;
                        case 3: //read settings, map and texture
                            EnableWindow (b_play, FALSE);
                            filepick (t_mapfile);
                        try
                        {
                            char fname [MAX_PATH];
                            GetWindowText (t_mapfile, fname, MAX_PATH);
                            int fnlen = GetWindowTextLength (t_mapfile);
                            int lastp = fnlen - 1;
                            for (; lastp >= 0; lastp --)
                            {
                                if (fname [lastp] == '\\') {break;}
                            }
                            string folder (fname, lastp + 1);

                            ZeroMemory (palette, 256);
                            short t;
                            string bmpfname, mapfname;

                            ifstream settings (fname);
                            settings.exceptions (ifstream::failbit);
                            settings >> bmpfname;
                            settings >> t; dkf = t;
                            unsigned short kt;
                            settings >> kt;
                            for (unsigned short i = 0; i < kt; i ++)
                            {
                                settings >> t;
                                palette [t] = i;
                            }

                            settings >> mapfname;
                            char tt1;
                            char tt2;

                            ifstream fmap (folder + mapfname, ios::binary);
                            fmap.exceptions (ifstream::failbit);
                            fmap.get (tt1);
                            fmap.get (tt2);
                            fx = tt1 + (tt2 << 8);
                            fmap.get (tt1);
                            fmap.get (tt2);
                            fy = tt1 + (tt2 << 8);
                            fmap.get (tt1);
                            fmap.get (tt1); mfree = tt1;
                            fmap.get (tt1); mfood = tt1;
                            fmap.get (tt1); mwall = tt1;
                            fmap.read (mapname, global::mapname_length);
                            for (char& i : mapname) {if (i == '`') {i = '\0'; break;}}
                            delete [] field;
                            delete [] original;
                            field = new unsigned char [fx*fy];
                            original = new unsigned char [fx*fy];
                            for (short i = 0; i < fx; i ++)
                            {
                                for (short j = 0; j < fy; j ++)
                                {
                                    fmap.get (tt1);
                                    original [fy*i + j] = tt1;
                                    field [fy*i + j] = tt1;
                                }
                            }
                            fmap.close ();

                            settings >> delay;
                            settings >> t; foodprop = t;
                            settings >> t; sview = 2*t + 1;
                            settings >> t; viewgrow = t;
                            settings >> maxpls;
                            delete [] fdt;
                            fdt = new snake [maxpls];

                            for (unsigned short i = 0; i < maxpls; i ++)
                            {
                                fdt [i].alive = true;
                                fdt [i].id = i;
                                settings >> t;
                                fdt [i].defview = t;
                                fdt [i].view = t;
                                fdt [i].dview = 2*fdt [i].view + 1;
                                settings >> fdt [i].direction;
                                settings >> t;
                                short kix, kiy;
                                for (short j = 0; j < t - 1; j ++)
                                {
                                    settings >> kix >> kiy;
                                    fdt [i].body.push_back ({kix, kiy});
                                }
                                settings >> kix >> kiy;
                                fdt [i].cx = kix;
                                fdt [i].cy = kiy;
                            }
                            settings.close ();

                            ifstream bmpin (folder + bmpfname, ios::binary);
                            bmpin.exceptions (ifstream::failbit);
                            char info [54];
                            bmpin.read (info, 54);
                            int fsize = info [2] + (info [3] << 8) + (info [4] << 16) + (info [5] << 24);
                            int width = info [18] + (info [19] << 8) + (info [20] << 16) + (info [21] << 24);
                            int height = info [22] + (info [23] << 8) + (info [24] << 16) + (info [25] << 24);
                            int mlen = 3 * width * height;
                            if (width < 0 || height < 0 || width != height || fsize - 54 != mlen) {throw (ifstream::failbit);}

                            gs = width;
                            delete [] gdata;
                            gdata = new char [mlen];
                            bmpin.read (gdata, mlen);
                            bmpin.close ();
                            SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "Settings, map and texture loaded successfully"); logcount ++;
                            EnableWindow (b_play, TRUE);
                        }
                        catch (const ifstream::failure& e)
                        {
                            EnableWindow (b_play, FALSE);
                            MessageBox (serv, "Couldn't read data!", "File reading error!", MB_OK | MB_ICONERROR);
                            SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "File reading error"); logcount ++;
                        }
                            break;
                        case 4: //send data to players and start the game
                        {
                            if (players.empty ()) //no one connected
                            {
                                MessageBox (serv, "Couldn't start game, no one has connected!", "Launch error!", MB_OK | MB_ICONERROR);
                                SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "Couldn't start game, no one has connected"); logcount ++;
                                break;
                            }
                            unsigned short pig = min ((unsigned short) players.size (), maxpls);
                            char tbuf [4] = {'i', char (dkf), char (gs), char (gs >> 8)};
                            int mlen = 3L * gs * gs;
                            //random_shuffle (fdt, fdt + maxpls);
                            random_shuffle (players.begin (), players.end ());
                            for (unsigned short i = 0; i < pig; i ++) //send data to players
                            {
                                if (send (players [i].sock, tbuf, 4, 0) == SOCKET_ERROR || send (players [i].sock, mapname, global::mapname_length, 0) == SOCKET_ERROR || send (players [i].sock, palette, 256, 0) == SOCKET_ERROR || send (players [i].sock, gdata, mlen, 0) == SOCKET_ERROR)
                                {
                                    //MessageBox (serv, ("Couldn't send data to player " + string (players [i].nick)).c_str (), "Socket error!", MB_OK | MB_ICONERROR);
                                    SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Couldn't send data to player " + string (players [i].nick)).c_str ()); logcount ++;
                                    closesocket (players [i].sock);
                                    players.erase (players.begin () + i);
                                    pig = min ((unsigned short) players.size (), maxpls);
                                    i --;
                                }
                                else
                                {
                                    players [i].sk = fdt [i];
                                    field [fy*players [i].sk.cx + players [i].sk.cy] = mwall + players [i].sk.id*2 + 1;
                                    for (pair <short, short>& sg : players [i].sk.body)
                                    {
                                        field [fy*sg.first + sg.second] = mwall + players [i].sk.id*2;
                                    }
                                }
                            }
                            tbuf [0] = 'o';
                            for (unsigned short i = pig; i < players.size (); i ++) //send data to spectators
                            {
                                if (send (players [i].sock, tbuf, 4, 0) == SOCKET_ERROR || send (players [i].sock, mapname, global::mapname_length, 0) == SOCKET_ERROR || send (players [i].sock, palette, 256, 0) == SOCKET_ERROR || send (players [i].sock, gdata, mlen, 0) == SOCKET_ERROR)
                                {
                                    //MessageBox (serv, ("Couldn't send data to player " + string (players [i].nick)).c_str (), "Socket error!", MB_OK | MB_ICONERROR);
                                    SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Couldn't send data to player " + string (players [i].nick)).c_str ()); logcount ++;
                                    closesocket (players [i].sock);
                                    players.erase (players.begin () + i);
                                    i --;
                                }
                                else
                                {
                                    players [i].sk.alive = false;
                                    players [i].sk.cx = fx / 2;
                                    players [i].sk.cy = fy / 2;
                                    players [i].sk.view = sview;
                                    players [i].sk.dview = 2*sview + 1;
                                }

                            }
                            if (players.empty ()) //everyone left
                            {
                                //SendMessage (lb_players, LB_RESETCONTENT, 0, 0);
                                MessageBox (serv, "Couldn't start game, everyone disconnected!", "Launch error!", MB_OK | MB_ICONERROR);
                                SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "Couldn't start game, everyone disconnected!"); logcount ++;
                            }
                            else //generate food and start the game
                            {
                                if (foodprop == 0) {genfood (field, fx, fy, mfree, mfood, lb_log, logcount);}
                                else
                                {
                                    for (unsigned int i = 0; i < 1L * foodprop * pig; i ++) {genfood (field, fx, fy, mfree, mfood, lb_log, logcount);}
                                }
                                goon = true;
                                TerminateThread (thlis, 0);
                                MessageBox (serv, "Game started!", "Launch successful", MB_OK | MB_ICONASTERISK);
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
        if (WaitForSingleObject (thlis, 10) != WAIT_OBJECT_0) {TerminateThread (thlis, 0);} //terminate accepter thread
        closesocket (slisten);
        delete [] fdt;
        delete [] gdata;
        SetParent (lb_log, 0);
        DestroyWindow (serv);
        if (!goon) //game cancelled
        {
            DestroyWindow (lb_log);
            UnregisterClass ("sv", 0);
            delete [] field;
            delete [] original;
            for (player& i : players) {closesocket (i.sock);}
            return;
        }
    }

    //proceed game
    {
        unsigned long long int tick = 0;
        short snalive = players.size ();
        bool endgame = false;
        unsigned short order [players.size ()];
        for (unsigned short i = 0; i < players.size (); i ++) {order [i] = i;}
        //create game window and fill it with content
        HWND game = CreateWindowEx (0, "sv", "Game logs", WS_SYSMENU, winpx/2 - wx/2, winpy/2 - wy/2, wx, wy, 0, 0, 0, 0);
        SetParent (lb_log, game);
        SetWindowPos (lb_log, 0, 20, 40, 640, 500, SWP_NOMOVE);
        CreateWindowEx (0, "Button", "End game", WS_VISIBLE | WS_CHILD, 560, 20, 100, 20, game, (HMENU) 1003, 0, 0);
        CreateWindowEx (0, "Button", "Pause game", WS_VISIBLE | WS_CHILD, 460, 20, 100, 20, game, (HMENU) 1001, 0, 0);
        SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "Game started!"); logcount ++;
        ShowWindow (game, SW_SHOW);
        MSG msg;
        while (true) //game loop
        {
            if (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) //respond to window events
            {
                if (msg.message == WM_QUIT) {break;}
                else if (msg.message == WM_USER)
                {
                    if (msg.lParam == 1) {MessageBox (game, "Game paused\nPress \"OK\" to resume", "Pause", MB_OK | MB_ICONASTERISK);}
                    else if (msg.lParam == 2) {break;}
                }
                else
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
            else //proceed game logic
            {
                long long int tbgn = time (0);
                SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Tick " + to_string (tick) + " started").c_str ()); logcount ++;
                random_shuffle (order, order + players.size ()); //reset order

                int food_eaten = 0;
                for (unsigned short i = 0; i < players.size (); i ++)
                {
                    player& p = players [order [i]];
                    if (p.disconnected) {continue;}
                    if (p.connected) //receive data from player
                    {
                        char trb [1] = {' '};
                        unsigned long block = 1;
                        if (ioctlsocket (p.sock, FIONBIO, &block) == SOCKET_ERROR) {p.connected = false;}
                        if (recv (p.sock, trb, 1, 0) == SOCKET_ERROR) {}// {p.connected = false;}
                        block = 0;
                        if (ioctlsocket (p.sock, FIONBIO, &block) == SOCKET_ERROR) {p.connected = false;}
                        if (!p.sk.alive) {p.sk.direction = trb [0];}
                        else if (trb [0] != ' ' && !((p.sk.direction == 'w' && trb [0] == 's') || (p.sk.direction == 's' && trb [0] == 'w') ||
                            (p.sk.direction == 'a' && trb [0] == 'd') || (p.sk.direction == 'd' && trb [0] == 'a'))) {p.sk.direction = trb [0];}
                    }

                    if (p.sk.alive) //move player's snake
                    {
                        unsigned short prevscore = p.sk.score;
                        if (p.sk.direction == 'w') //up
                        {
                            if (p.sk.cx != 0)
                            {
                                if (field [fy*(p.sk.cx - 1) + p.sk.cy] < mfood)
                                {
                                    if (field [fy*(p.sk.cx - 1) + p.sk.cy] >= mfree)
                                    {
                                        p.sk.belly.push ({p.sk.cx - 1, p.sk.cy});
                                        p.sk.score ++;
                                    }
                                    p.sk.body.push_back ({p.sk.cx, p.sk.cy});
                                    field [fy*p.sk.cx + p.sk.cy] = mwall + p.sk.id * 2;
                                    p.sk.cx --;
                                }
                                else {p.sk.alive = false;}
                            }
                            else {p.sk.alive = false;}
                        }
                        else if (p.sk.direction == 'd') //right
                        {
                            if (p.sk.cy != fy - 1)
                            {
                                if (field [fy*p.sk.cx + p.sk.cy + 1] < mfood)
                                {
                                    if (field [fy*p.sk.cx + p.sk.cy + 1] >= mfree)
                                    {
                                        p.sk.belly.push ({p.sk.cx, p.sk.cy + 1});
                                        p.sk.score ++;
                                    }
                                    p.sk.body.push_back ({p.sk.cx, p.sk.cy});
                                    field [fy*p.sk.cx + p.sk.cy] = mwall + p.sk.id * 2;
                                    p.sk.cy ++;
                                }
                                else {p.sk.alive = false;}
                            }
                            else {p.sk.alive = false;}
                        }
                        else if (p.sk.direction == 's') //down
                        {
                            if (p.sk.cx != fx - 1)
                            {
                                if (field [fy*(p.sk.cx + 1) + p.sk.cy] < mfood)
                                {
                                    if (field [fy*(p.sk.cx + 1) + p.sk.cy] >= mfree)
                                    {
                                        p.sk.belly.push ({p.sk.cx + 1, p.sk.cy});
                                        p.sk.score ++;
                                    }
                                    p.sk.body.push_back ({p.sk.cx, p.sk.cy});
                                    field [fy*p.sk.cx + p.sk.cy] = mwall + p.sk.id * 2;
                                    p.sk.cx ++;
                                }
                                else {p.sk.alive = false;}
                            }
                            else {p.sk.alive = false;}
                        }
                        else if (p.sk.direction == 'a') //left
                        {
                            if (p.sk.cy != 0)
                            {
                                if (field [fy*p.sk.cx + p.sk.cy - 1] < mfood)
                                {
                                    if (field [fy*p.sk.cx + p.sk.cy - 1] >= mfree)
                                    {
                                        p.sk.belly.push ({p.sk.cx, p.sk.cy - 1});
                                        p.sk.score ++;
                                    }
                                    p.sk.body.push_back ({p.sk.cx, p.sk.cy});
                                    field [fy*p.sk.cx + p.sk.cy] = mwall + p.sk.id * 2;
                                    p.sk.cy --;
                                }
                                else {p.sk.alive = false;}
                            }
                            else {p.sk.alive = false;}
                        }
                        //update snake
                        field [fy*p.sk.cx + p.sk.cy] = mwall + p.sk.id * 2 + 1;
                        pair < short, short > tail =  p.sk.body.front ();
                        if (!p.sk.belly.empty () && p.sk.belly.front () == tail) {p.sk.belly.pop ();}
                        else
                        {
                            field [fy*tail.first + tail.second] = original [fy*tail.first + tail.second];
                            p.sk.body.pop_front ();
                        }
                        if (p.sk.score != prevscore)
                        {
                            p.sk.view = min ((unsigned char) (p.sk.defview + p.sk.score / viewgrow), (unsigned char) (sview / 2));
                            p.sk.dview = p.sk.view * 2 + 1;
                            food_eaten ++;
                            if (p.sk.score >= 64000) {endgame = true;}
                            SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) (string (p.nick)  + " (id " + to_string (int (p.sk.id)) + ") ate food").c_str ()); logcount ++;
                        }
                        if (!p.sk.alive) //died during turn
                        {
                            p.sk.view = sview / 2;
                            p.sk.dview = sview;
                            SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) (string (p.nick)  + " (id " + to_string (int (p.sk.id)) + ") died").c_str ()); logcount ++;
                            snalive --;
                            //if (snalive == 0) {p.sk.score += players.size ();}
                            //p.sk.score += players.size () - snalive;
                        }
                    }
                    else //move spectator
                    {
                        if (p.sk.direction == 'w' && p.sk.cx != 0) {p.sk.cx --;}
                        else if (p.sk.direction == 's' && p.sk.cx != fx - 1) {p.sk.cx ++;}
                        else if (p.sk.direction == 'a' && p.sk.cy != 0) {p.sk.cy --;}
                        else if (p.sk.direction == 'd' && p.sk.cy != fy - 1) {p.sk.cy ++;}
                        p.sk.direction = ' ';
                    }
                }

                if (!endgame) //generate new food
                {
                    for (int i = 0; i < food_eaten; i ++)
                    {
                        if (!genfood (field, fx, fy, mfree, mfood, lb_log, logcount)) {endgame = true; break;}
                    }
                }

                for (unsigned short i = 0; i < players.size (); i ++) //send players data and map
                {
                    player& p = players [order [i]];
                    if (!p.connected) {continue;}
                    char tdv [2] = {(p.sk.alive) ? '1' : '0', char (p.sk.dview)};
                    if (send (p.sock, tdv, 2, 0) == SOCKET_ERROR)
                    {
                        p.connected = false;
                        SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Player \"" + string (players [i].nick)  + "\" (IP: " + string (inet_ntoa (players [i].saddr.sin_addr)) + ") disconnected").c_str ()); logcount ++;
                        continue;
                    }
                    char buf [p.sk.dview] [p.sk.dview];
                    short xbeg = max (0, p.sk.cx - p.sk.view);
                    short ybeg = max (0, p.sk.cy - p.sk.view);
                    short xend = min (int (fx), xbeg + p.sk.dview);
                    short yend = min (int (fy), ybeg + p.sk.dview);
                    xbeg = xend - p.sk.dview;
                    ybeg = yend - p.sk.dview;
                    for (short i = xbeg; i < xend; i ++)
                    {
                        for (short j = ybeg; j < yend; j ++)
                        {
                             buf [i - xbeg] [j - ybeg] = field [fy*i + j];
                        }
                    }
                    if (send (p.sock, buf [0], short (p.sk.dview) * p.sk.dview, 0) == SOCKET_ERROR) {p.connected = false;}
                }

                bool alldis = true;
                for (unsigned short i = 0; i < players.size (); i ++) //remove disconnected players' dead snakes
                {
                    if (!players [i].disconnected)
                    {
                        alldis = false;
                        if (!players [i].connected && !players [i].sk.alive)
                        {
                            closesocket (players [i].sock);
                            SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Player \"" + string (players [i].nick)  + "\" (IP: " + string (inet_ntoa (players [i].saddr.sin_addr)) + ") removed").c_str ()); logcount ++;
                            players [i].disconnected = true;
                        }
                    }
                }

                long long int ticktime = time (0) - tbgn;
                SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) ("Tick " + to_string (tick) + " ended in " + to_string (ticktime) + " milliseconds").c_str ()); logcount ++;
                SendMessage (lb_log, LB_SETTOPINDEX, (WPARAM) logcount - 1, 0);
                if (delay > ticktime) {Sleep (delay - ticktime);}
                tick ++;

                //if (alldis || snalive == 0 || endgame) {break;}
                if (alldis || endgame) {break;}
            }
        }
        Sleep (10);
        for (player& i : players) //notify every connected player about end of the game
        {
            if (i.connected)
            {
                char tdv [2] = {'q', char (players.size ())};
                if (send (i.sock, tdv, 2, 0) == SOCKET_ERROR) {i.connected = false;}
            }
        }
        SendMessage (lb_log, LB_INSERTSTRING, logcount, (LPARAM) "Game ended!"); logcount ++;
        SendMessage (lb_log, LB_SETTOPINDEX, (WPARAM) logcount - 1, 0);
        MessageBox (game, "Game ended!", "Game ended!", MB_OK);
        DestroyWindow (game);
        DestroyWindow (lb_log);
        delete [] field;
        delete [] original;
    }

    //leaderboard
    {
        //create leaderboard
        sort (players.begin (), players.end (), [] (player& a, player& b) -> bool {return a.sk.score > b.sk.score;} ); //sort players by their score
        unsigned int ldsize = 1 + players.size () * (global::nick_length + 2);
        char lddata [ldsize];
        for (unsigned char i = 0; i < players.size (); i ++)
        {
            lddata [(global::nick_length + 2)*i + 1] = players [i].sk.score;
            lddata [(global::nick_length + 2)*i + 2] = players [i].sk.score << 8;
            for (unsigned char j = 0; j < global::nick_length; j ++) {lddata [3 + (global::nick_length + 2)*i + j] = players [i].nick [j];}
        }
        Sleep (100);
        HWND leaderboard = CreateWindowEx (0, "sv", "Leaderboard", WS_SYSMENU, winpx/2 - 193, winpy/2 - wy/2, 386, wy, 0, 0, 0, 0);
        HWND lb_scores = CreateWindowEx (0, "ListBox", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_DISABLENOSCROLL, 0, 0, 380, 548, leaderboard, 0, 0, 0);
        for (unsigned char i = 0; i < players.size (); i ++) //send leaderboard data to connected players and fill local window with it
        {
            if (players [i].connected)
            {
                lddata [0] = i;
                send (players [i].sock, lddata, ldsize, 0);
            }
            char tempstr [global::nick_length + 20];
            itoa (players [i].sk.score, tempstr, 10);
            int j = 0;
            for (; j < 7; j ++) {if (tempstr [j] == '\0') {break;}}
            for (; j < 20; j ++) {tempstr [j] = ' ';}
            for (unsigned char u = 0; u < global::nick_length; u ++) {tempstr [20 + u] = players [i].nick [u];}
            SendMessage (lb_scores, LB_INSERTSTRING, i, (LPARAM) tempstr);
        }
        for (player& p : players) {closesocket (p.sock);}
        players.clear ();

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
    UnregisterClass ("sv", 0);
}
