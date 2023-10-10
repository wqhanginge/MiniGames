#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <tchar.h>
#include <random>

#include <fstream>
#include <sstream>

#include <string>
#include <vector>

#ifdef UNICODE
using tstring = std::wstring;
using tstring_view = std::wstring_view;
using tstringstream = std::wstringstream;
#else
using tstring = std::string;
using tstring_view = std::string_view;
using tstringstream = std::stringstream;
#endif


#define GETXLPARAM(l)   (MAKEPOINTS(l).x)
#define GETYLPARAM(l)   (MAKEPOINTS(l).y)

#define WM_GLPAUSE      (WM_APP + 0)
#define WM_GLINSERT     (WM_APP + 1)
#define WM_GLSPEED      (WM_APP + 2)    //wparam is the gears to increase
#define WM_GLZOOM       (WM_APP + 3)    //wparam is the pixels to zoom in, lparam is the mouse position
#define WM_GLSETSCROLL  (WM_APP + 4)    //wparam is a BOOL indicating if redraw
#define WM_GLSETTEXT    (WM_APP + 5)    //wparam is a BOOL indicating if lparam is valid, lparam is the mouse position
#define WM_GLHELP       (WM_APP + 6)

#define GL_BIRTHCNT     3
#define GL_ALIVECNT     2

#define GLM_DEFMAPW     100
#define GLM_DEFMAPH     100
#define GLM_DEFRATIO    0.25

#define GLFT_BITMAP     0
#define GLFT_COORD      1
#define GLFT_RECT       2

#define GLWC_CLSNAME    "GAMEOFLIFE"
#define GLWC_EXDATASIZE sizeof(LONG_PTR)
#define GLWC_EXDATAOFF  0

#define GLRT_SF_PAUSE   0x01
#define GLRT_SF_INSERT  0x02    //should be used with GLRT_SF_PAUSE
#define GLRT_SF_HELP    0x10
#define GLRT_SF_UNBOUND 0x20
#define GLRT_GETSV(s)   ((BYTE)(s) & 0x0F)
#define GLRT_SETSV(s,v) ((s) = ((BYTE)(s) & 0xF0) | ((BYTE)(v) & 0x0F))
#define GLRT_SV_RUNNING 0x00
#define GLRT_SV_PAUSE   (GLRT_SF_PAUSE)
#define GLRT_SV_INSERT  (GLRT_SF_PAUSE | GLRT_SF_INSERT)

#define GLRT_TIMERID    1

#define GLW_MINWNDW     400
#define GLW_MINWNDH     400
#define GLW_WNDNAME     "Game of Life"
#define GLW_SBPAGE      10

#define GLW_CHAR_ESC    '\x1B'
#define GLW_CHAR_BS     '\x08'
#define GLW_CHAR_EDGE   'e'
#define GLW_CHAR_PAUSE  'p'
#define GLW_CHAR_ZIN    'w'
#define GLW_CHAR_ZOUT   's'
#define GLW_CHAR_INS    'i'
#define GLW_CHAR_INC    'd'
#define GLW_CHAR_DEC    'a'
#define GLW_CHAR_NEXT   'n'
#define GLW_CHAR_HELP   'h'

#define GLW_DEFPIXW     5
#define GLW_MINPIXW     1
#define GLW_MAXPIXW     10
#define GLW_SETPW(pw,w) ((pw) = (BYTE)min(max((int)(w), GLW_MINPIXW), GLW_MAXPIXW))
#define GLW_ZOOMDELTA   3

#define GLW_DEFCOLMBG   RGB(0, 0, 0)
#define GLW_DEFCOLCELL  RGB(255, 255, 255)
#define GLW_DEFCOLEDGE  RGB(255, 255, 0)
#define GLW_DEFCOLUBEG  RGB(0, 0, 255)
#define GLW_DEFCOLTBG   RGB(0, 255, 255)

#define __TOSTR(n)      #n
#define TOSTR(n)        __TOSTR(n)
#define GLCMD_HELP      "\
CmdLine Syntax:\n\
   <exe> -r WIDTH HEIGHT RATIO [PIXELW]\n\
   <exe> -f FILENAME\n\
   <exe> -h\n\
File Format:\n\
   BITMAP\t{" TOSTR(GLFT_BITMAP) " WIDTH HEIGHT PIXELW  0 0 1 0 ... 1 0 1}\n\
   COORD\t{" TOSTR(GLFT_COORD) " WIDTH HEIGHT PIXELW  x y x y x y ...}\n\
   RECT\t{" TOSTR(GLFT_RECT) " WIDTH HEIGHT PIXELW  left top right bottom ...}\
"
#define GLW_HELP        "\
 ESC\tclose \n\
 w\tzoom in \n\
 s\tzoom out \n\
 d\tspeed up \n\
 a\tslow down \n\
 n\tnext step \n\
 h\thelp \n\
 p\tpause \n\
 e\ttoggle edge \n\
 i\tinsert mode \n\
 BS\tclear map \n\
 MLB\tadd cells \n\
 MRB\tremove cells \
"

#define RETVAL_EXIT     0
#define RETVAL_CMDFAIL  1
#define RETVAL_CMDHELP  2
#define RETVAL_ERRFOPEN 3
#define RETVAL_BADFTYPE 4


class Speed {
    struct Val {
        int interval;
        int rps;
        LPCTSTR desc;
        constexpr bool operator==(const Val& other) const { return (this == &other); }
        constexpr bool operator!=(const Val& other) const { return (this != &other); }
    };

public:
    static constexpr Val MANUAL = { -1, 0, TEXT("Manual") };
    static constexpr Val EXSLOW = { 1000, 1, TEXT("Extremly Slow") };
    static constexpr Val VERYSLOW = { 500, 2, TEXT("Very Slow") };
    static constexpr Val SLOW = { 250, 4, TEXT("Slow") };
    static constexpr Val NORMAL = { 100, 10, TEXT("Normal") };
    static constexpr Val FAST = { 50, 20, TEXT("Fast") };
    static constexpr Val VERYFAST = { 25, 40, TEXT("Very Fast") };
    static constexpr Val EXFAST = { 10, 100, TEXT("Extremly Fast") };

    Speed() :_pos(4) {}    //NORMAL
    Speed(const Val& v) { _assign(v); }

    const Val& operator*() const { return *_val_list[_pos]; }
    Speed& operator=(const Val& v) { _assign(v); return *this; }

    Speed& operator+=(int n) { _add(n); return *this; }
    Speed& operator++() { return *this += 1; }
    Speed operator+(int n) const { return Speed(*this) += n; }
    const Speed operator++(int) { Speed tmp = *this; ++*this; return tmp; }

    Speed& operator-=(int n) { return *this += (-n); }
    Speed& operator--() { return *this -= 1; }
    Speed operator-(int n) const { return *this + (-n); }
    const Speed operator--(int) { Speed tmp = *this; --*this; return tmp; }

private:
    static constexpr int _count = 8;
    static constexpr const Val* _val_list[_count] = {
        &MANUAL, &EXSLOW, &VERYSLOW, &SLOW, &NORMAL, &FAST, &VERYFAST, &EXFAST
    };
    short _pos;

    void _assign(const Val& v) {
        short i = 0;
        for (; i < _count; i++) if (v == *_val_list[i]) _pos = i;
        if (i >= _count) throw std::invalid_argument("bad value");
    }
    inline void _add(int n) { _pos = (_pos + n % _count + _count) % _count; };
};

class GameOfLifeInfo {
public:
    LPCTSTR name = TEXT(GLW_WNDNAME);
    COLORREF color_bg = GLW_DEFCOLMBG;
    COLORREF color_cell = GLW_DEFCOLCELL;
    COLORREF color_edge = GLW_DEFCOLEDGE;
    COLORREF color_ubeg = GLW_DEFCOLUBEG;
    Speed speed;
    BYTE state = GLRT_SF_PAUSE | GLRT_SF_HELP;
    BYTE pixelw = GLW_DEFPIXW;

    GameOfLifeInfo(WORD w, WORD h, double ratio, BYTE pw = GLW_DEFPIXW);
    GameOfLifeInfo(WORD w, WORD h, const bool* map, BYTE pw = GLW_DEFPIXW);
    GameOfLifeInfo(WORD w, WORD h, const std::vector<POINT>& coords, BYTE pw = GLW_DEFPIXW);
    GameOfLifeInfo(WORD w, WORD h, const std::vector<RECT>& rects, BYTE pw = GLW_DEFPIXW);
    GameOfLifeInfo(const GameOfLifeInfo&) = delete;
    GameOfLifeInfo(GameOfLifeInfo&&) = delete;
    ~GameOfLifeInfo() { delete[] _curr_map; delete[] _last_map; }

    WORD mapw() const { return _width; }
    WORD maph() const { return _height; }
    WORD fullw() const { return _width + 2; }
    WORD fullh() const { return _height + 2; }
    size_t gen() const { return _generation; }
    bool isinmap(int x, int y) const;
    void update(bool ub);
    void draw(HDC hdc, int left, int top, PRECT prgn, bool ub);
    void set(int x, int y, bool alive);
    void clear();

private:
    WORD _width;
    WORD _height;
    size_t _generation;
    bool* _curr_map;
    bool* _last_map;
};

GameOfLifeInfo::GameOfLifeInfo(WORD w, WORD h, double ratio, BYTE pw /*GLW_DEFPIXW*/)
    :pixelw(pw + !pw), _width(w + !w), _height(h + !h), _generation(0) {
    size_t size = (size_t)_width * _height, cells;
    _curr_map = new bool[size] {};
    _last_map = new bool[size] {};
    ratio = abs(ratio);
    cells = (ratio > 1) ? (size_t)ratio : (size_t)(size * ratio);
    cells = min(max(0, cells), size);
    memset(_curr_map, 1, sizeof(bool) * cells);
    if (cells > 0 && cells < size) {    //shuffle
        std::random_device rd;
        std::minstd_rand urand(rd());
        for (size_t i = 0; i < size; i++) {
            size_t k = urand() % (size - i) + i;
            std::swap(_curr_map[i], _curr_map[k]);
        }
    }
}

GameOfLifeInfo::GameOfLifeInfo(WORD w, WORD h, const bool* map, BYTE pw /*GLW_DEFPIXW*/)
    :pixelw(pw + !pw), _width(w + !w), _height(h + !h), _generation(0) {
    size_t size = (size_t)_width * _height;
    _curr_map = new bool[size] {};
    _last_map = new bool[size] {};
    memcpy(_curr_map, map, sizeof(bool) * size);
}

GameOfLifeInfo::GameOfLifeInfo(WORD w, WORD h, const std::vector<POINT>& coords, BYTE pw /*GLW_DEFPIXW*/)
    :pixelw(pw + !pw), _width(w + !w), _height(h + !h), _generation(0) {
    size_t size = (size_t)_width * _height;
    _curr_map = new bool[size] {};
    _last_map = new bool[size] {};
    for (auto& pt : coords) {
        if (isinmap(pt.x, pt.y))
            _curr_map[pt.x + pt.y * _width] = true;
    }
}

GameOfLifeInfo::GameOfLifeInfo(WORD w, WORD h, const std::vector<RECT>& rects, BYTE pw /*GLW_DEFPIXW*/)
    :pixelw(pw + !pw), _width(w + !w), _height(h + !h), _generation(0) {
    size_t size = (size_t)_width * _height;
    _curr_map = new bool[size] {};
    _last_map = new bool[size] {};
    for (auto rect : rects) {
        if (rect.left > rect.right) std::swap(rect.left, rect.right);
        if (rect.top > rect.bottom) std::swap(rect.top, rect.bottom);
        for (int x = rect.left; x < rect.right; x++) {
            for (int y = rect.top; y < rect.bottom; y++) {
                if (isinmap(x, y))
                    _curr_map[x + y * _width] = true;
            }
        }
    }
}

bool GameOfLifeInfo::isinmap(int x, int y) const {
    return (x >= 0 && x < _width && y >= 0 && y < _height);
}

void GameOfLifeInfo::update(bool ub) {
    _generation++;
    std::swap(_curr_map, _last_map);
    for (int i = 0; i < _width * _height; i++) {
        int x = i % _width, y = i / _width;
        int xl = (x - 1 + _width) % _width, xr = (x + 1) % _width;
        int yl = (y - 1 + _height) % _height, yr = (y + 1) % _height;
        bool left = (x - 1 >= 0), right = (x + 1 < _width);
        bool up = (y - 1 >= 0), down = (y + 1 < _width);
        int neicnt = 0;

        neicnt += ((ub || up && left) && _last_map[xl + yl * _width]);
        neicnt += ((ub || up) && _last_map[x + yl * _width]);
        neicnt += ((ub || up && right) && _last_map[xr + yl * _width]);
        neicnt += ((ub || left) && _last_map[xl + y * _width]);
        neicnt += ((ub || right) && _last_map[xr + y * _width]);
        neicnt += ((ub || down && left) && _last_map[xl + yr * _width]);
        neicnt += ((ub || down) && _last_map[x + yr * _width]);
        neicnt += ((ub || down && right) && _last_map[xr + yr * _width]);

        _curr_map[i] = (neicnt == GL_BIRTHCNT) + (neicnt == GL_ALIVECNT) * _last_map[i];
    }
}

void GameOfLifeInfo::draw(HDC hdc, int left, int top, PRECT prgn, bool ub) {
    HBRUSH hbr = (HBRUSH)GetStockObject(DC_BRUSH);
    COLORREF ceg = (ub) ? color_ubeg : color_edge;

    SetDCBrushColor(hdc, ceg);
    RECT erect = { left, top, left + (_width + 2) * pixelw, top + (_height + 2) * pixelw };
    FillRect(hdc, &erect, hbr);
    SetDCBrushColor(hdc, color_bg);
    RECT brect = { left + pixelw, top + pixelw, left + (_width + 1) * pixelw, top + (_height + 1) * pixelw };
    FillRect(hdc, &brect, hbr);

    SetDCBrushColor(hdc, color_cell);
    for (int i = 0; i < _width * _height; i++) {
        if (_curr_map[i]) {
            int cleft = left + (1 + i % _width) * pixelw;
            int ctop = top + (1 + i / _width) * pixelw;
            RECT crect = { cleft, ctop, cleft + pixelw, ctop + pixelw };
            if (crect.left < prgn->right && crect.right > prgn->left && crect.top < prgn->bottom && crect.bottom > prgn->top)
                FillRect(hdc, &crect, hbr);
        }
    }
}

void GameOfLifeInfo::set(int x, int y, bool alive) {
    if (isinmap(x, y)) _curr_map[x + y * _width] = alive;
}

void GameOfLifeInfo::clear() {
    memset(_curr_map, 0, sizeof(bool) * _width * _height);
    _generation = 0;
}


void getMapOriginPoint(HWND hwnd, PPOINT pori) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    RECT crect;
    GetClientRect(hwnd, &crect);
    pori->x = max(0, crect.right - pgl->fullw() * pgl->pixelw) / 2;
    pori->y = max(0, crect.bottom - pgl->fullh() * pgl->pixelw) / 2;
    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    sci.fMask = SIF_POS;
    GetScrollInfo(hwnd, SB_HORZ, &sci);
    pori->x -= sci.nPos;
    GetScrollInfo(hwnd, SB_VERT, &sci);
    pori->y -= sci.nPos;
}

void adjustMousePos(HWND hwnd, PPOINT pmouse) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    POINT ori;
    getMapOriginPoint(hwnd, &ori);
    pmouse->x = (pmouse->x - ori.x) / pgl->pixelw - 1;
    pmouse->y = (pmouse->y - ori.y) / pgl->pixelw - 1;
}


LRESULT onCreate(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lparam;
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)lpcs->lpCreateParams;
    SetWindowLongPtr(hwnd, GLWC_EXDATAOFF, (LONG_PTR)pgl);

    RECT crect = { 0, 0, pgl->fullw() * pgl->pixelw, pgl->fullh() * pgl->pixelw };
    AdjustWindowRect(&crect, lpcs->style, FALSE);
    int wndw = max(GLW_MINWNDW, crect.right - crect.left);
    int wndh = max(GLW_MINWNDH, crect.bottom - crect.top);
    MoveWindow(hwnd, lpcs->x, lpcs->y, wndw, wndh, FALSE);

    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    sci.fMask = SIF_RANGE;
    SetScrollInfo(hwnd, SB_HORZ, &sci, FALSE);
    SetScrollInfo(hwnd, SB_VERT, &sci, FALSE);

    if (GLRT_GETSV(pgl->state) == GLRT_SV_RUNNING && *pgl->speed != Speed::MANUAL)
        SetTimer(hwnd, GLRT_TIMERID, (*pgl->speed).interval, nullptr);

    PostMessage(hwnd, WM_GLSETTEXT, FALSE, 0);
    return 0;
}

LRESULT onDestroy(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    KillTimer(hwnd, GLRT_TIMERID);
    PostQuitMessage(RETVAL_EXIT);
    return 0;
}

LRESULT onGLPause(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    if (pgl->state & GLRT_SF_PAUSE) {   //if pause, resume
        GLRT_SETSV(pgl->state, GLRT_SV_RUNNING);
        if (*pgl->speed != Speed::MANUAL)
            SetTimer(hwnd, GLRT_TIMERID, (*pgl->speed).interval, nullptr);
    }
    else {  //if running, puase
        GLRT_SETSV(pgl->state, GLRT_SV_PAUSE);
        KillTimer(hwnd, GLRT_TIMERID);
    }
    PostMessage(hwnd, WM_GLSETTEXT, FALSE, 0);
    return 0;
}

LRESULT onGLInsert(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    if (pgl->state & GLRT_SF_INSERT) {  //exit insert mode but remain pause mode
        GLRT_SETSV(pgl->state, GLRT_SV_PAUSE);
    }
    else {  //enter insert mode
        GLRT_SETSV(pgl->state, GLRT_SV_INSERT);
        KillTimer(hwnd, GLRT_TIMERID);
    }
    PostMessage(hwnd, WM_GLSETTEXT, FALSE, 0);
    return 0;
}

LRESULT onGLSpeed(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    pgl->speed += (int)wparam;
    if (*pgl->speed == Speed::MANUAL) {
        KillTimer(hwnd, GLRT_TIMERID);
    }
    else if (GLRT_GETSV(pgl->state) == GLRT_SV_RUNNING) {
        SetTimer(hwnd, GLRT_TIMERID, (*pgl->speed).interval, nullptr);
    }
    PostMessage(hwnd, WM_GLSETTEXT, FALSE, 0);
    return 0;
}

LRESULT onGLZoom(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    SCROLLINFO sci = { sizeof(SCROLLINFO) };

    //store old values
    BYTE oldpw = pgl->pixelw;
    POINT ori, npos;
    getMapOriginPoint(hwnd, &ori);

    sci.fMask = SIF_POS;
    GetScrollInfo(hwnd, SB_HORZ, &sci);
    npos.x = sci.nPos - max(ori.x, 0);
    GetScrollInfo(hwnd, SB_VERT, &sci);
    npos.y = sci.nPos - max(ori.y, 0);

    //zoom and update scroll bar
    GLW_SETPW(pgl->pixelw, pgl->pixelw + (int)wparam);
    SendMessage(hwnd, WM_GLSETSCROLL, FALSE, 0);

    //update scroll position
    POINTS mpos = MAKEPOINTS(lparam);
    sci.fMask = SIF_RANGE;  //PAGE is alternative, see onGLSetScroll()
    GetScrollInfo(hwnd, SB_HORZ, &sci); //NOTE: Get before any Set, otherwise it may get wrong data
    npos.x = ((npos.x + mpos.x) * pgl->pixelw / oldpw - mpos.x) * (bool)sci.nMax;
    GetScrollInfo(hwnd, SB_VERT, &sci);
    npos.y = ((npos.y + mpos.y) * pgl->pixelw / oldpw - mpos.y) * (bool)sci.nMax;

    sci.fMask = SIF_POS;
    sci.nPos = npos.x;
    SetScrollInfo(hwnd, SB_HORZ, &sci, TRUE);
    sci.nPos = npos.y;
    SetScrollInfo(hwnd, SB_VERT, &sci, TRUE);

    //update client
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

LRESULT onGLSetScroll(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    WINDOWINFO wndi = { sizeof(WINDOWINFO) };
    GetWindowInfo(hwnd, &wndi);
    int mw = pgl->fullw() * pgl->pixelw, mh = pgl->fullh() * pgl->pixelw;
    int cw = wndi.rcWindow.right - wndi.rcWindow.left - wndi.cxWindowBorders * 2;
    int ch = wndi.rcWindow.bottom - wndi.rcWindow.top - wndi.cyWindowBorders * 2 - GetSystemMetrics(SM_CYCAPTION);
    bool hscroll = (mw > cw), vscroll = (mh > ch);
    hscroll |= (vscroll && mw > (cw - GetSystemMetrics(SM_CXVSCROLL)));
    vscroll |= (hscroll && mh > (ch - GetSystemMetrics(SM_CYHSCROLL)));
    hscroll |= (vscroll && mw > (cw - GetSystemMetrics(SM_CXVSCROLL))); //reevaluate after changing

    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    sci.fMask = SIF_RANGE | SIF_PAGE;   //MaxScrollPos = MaxRangeValue - (PageSize - 1)
    sci.nMax = hscroll * mw;
    sci.nPage = hscroll * (cw - vscroll * GetSystemMetrics(SM_CXVSCROLL) + 1);
    SetScrollInfo(hwnd, SB_HORZ, &sci, (BOOL)wparam);
    sci.nMax = vscroll * mh;
    sci.nPage = vscroll * (ch - hscroll * GetSystemMetrics(SM_CYHSCROLL) + 1);
    SetScrollInfo(hwnd, SB_VERT, &sci, (BOOL)wparam);
    return 0;
}

LRESULT onGLSetText(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    static LPARAM slparam = 0;
    if (wparam) slparam = lparam;

    tstringstream text;
    text << pgl->name << TEXT(":");

    switch (GLRT_GETSV(pgl->state)) {
    case GLRT_SV_PAUSE: text << TEXT("  Pause"); break;
    case GLRT_SV_INSERT: text << TEXT("  Insert"); break;
    }

    text << TEXT("  ") << pgl->gen();

    text << TEXT("  ") << (*pgl->speed).desc;
    if (*pgl->speed != Speed::MANUAL)
        text << TEXT("[") << (*pgl->speed).rps << TEXT(" RPS]");

    POINT mpos = { GETXLPARAM(slparam), GETYLPARAM(slparam) };
    adjustMousePos(hwnd, &mpos);
    if (pgl->isinmap(mpos.x, mpos.y))
        text << TEXT("  (") << mpos.x << TEXT(",") << mpos.y << TEXT(")");

    SetWindowText(hwnd, text.str().c_str());
    return 0;
}

LRESULT onGLHelp(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    HDC hdc = GetDC(hwnd);
    RECT uprect = {};
    pgl->state ^= GLRT_SF_HELP;
    DrawText(hdc, TEXT(GLW_HELP), -1, &uprect, DT_CALCRECT | DT_EXPANDTABS);
    InvalidateRect(hwnd, &uprect, FALSE);
    return 0;
}

LRESULT onChar(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    switch (wparam) {
    case TEXT(GLW_CHAR_ESC):    //exit game
        DestroyWindow(hwnd);
        break;
    case TEXT(GLW_CHAR_PAUSE):  //pause game or resume
        PostMessage(hwnd, WM_GLPAUSE, 0, 0);
        break;
    case TEXT(GLW_CHAR_INS):    //insert mode
        PostMessage(hwnd, WM_GLINSERT, 0, 0);
        break;
    case TEXT(GLW_CHAR_BS):     //clear the whole map
        if (GLRT_GETSV(pgl->state) == GLRT_SV_INSERT) {
            pgl->clear();
            PostMessage(hwnd, WM_GLSETTEXT, FALSE, 0);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    case TEXT(GLW_CHAR_ZIN):    //zoom in
        PostMessage(hwnd, WM_GLZOOM, 1, 0);
        break;
    case TEXT(GLW_CHAR_ZOUT):   //zoom out
        PostMessage(hwnd, WM_GLZOOM, -1, 0);
        break;
    case TEXT(GLW_CHAR_INC):    //speed up
        PostMessage(hwnd, WM_GLSPEED, 1, 0);
        break;
    case TEXT(GLW_CHAR_DEC):    //slow down
        PostMessage(hwnd, WM_GLSPEED, -1, 0);
        break;
    case TEXT(GLW_CHAR_NEXT):   //next round when manual
        if (GLRT_GETSV(pgl->state) == GLRT_SV_RUNNING && *pgl->speed == Speed::MANUAL) {
            SendMessage(hwnd, WM_TIMER, GLRT_TIMERID, NULL);
        }
        break;
    case TEXT(GLW_CHAR_HELP):   //show or hide help information
        PostMessage(hwnd, WM_GLHELP, 0, 0);
        break;
    case TEXT(GLW_CHAR_EDGE):   //change boundary type
        if (pgl->state & GLRT_SF_PAUSE) {
            pgl->state ^= GLRT_SF_UNBOUND;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }
    return 0;
}

LRESULT onLRButtonDown(HWND hwnd, WPARAM wparam, LPARAM lparam, bool left) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    if (GLRT_GETSV(pgl->state) == GLRT_SV_INSERT) {
        POINT pos = { GETXLPARAM(lparam), GETYLPARAM(lparam) };
        RECT uprect = { pos.x - pgl->pixelw, pos.y - pgl->pixelw, pos.x + pgl->pixelw, pos.y + pgl->pixelw };
        adjustMousePos(hwnd, &pos);
        if (pgl->isinmap(pos.x, pos.y)) {
            pgl->set(pos.x, pos.y, left);
            InvalidateRect(hwnd, &uprect, FALSE);
        }
    }
    return 0;
}

LRESULT onMouseMove(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    POINT pos = { GETXLPARAM(lparam), GETYLPARAM(lparam) };
    RECT uprect = { pos.x - pgl->pixelw, pos.y - pgl->pixelw, pos.x + pgl->pixelw, pos.y + pgl->pixelw };
    adjustMousePos(hwnd, &pos);
    if (GLRT_GETSV(pgl->state) == GLRT_SV_INSERT && (wparam == MK_LBUTTON || wparam == MK_RBUTTON)) {
        bool alive = wparam & MK_LBUTTON;
        if (pgl->isinmap(pos.x, pos.y)) {
            pgl->set(pos.x, pos.y, alive);
            InvalidateRect(hwnd, &uprect, FALSE);
        }
    }
    SendMessage(hwnd, WM_GLSETTEXT, TRUE, lparam);
    return 0;
}

LRESULT onScroll(HWND hwnd, WPARAM wparam, LPARAM lparam, bool hscroll) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    int nbar = (hscroll) ? SB_HORZ : SB_VERT;
    sci.fMask = SIF_ALL;
    GetScrollInfo(hwnd, nbar, &sci);

    switch (LOWORD(wparam)) {   //NOTE: the msg for HSCROLL and VSCROLL are symmetric
    case SB_LEFT: sci.nPos = sci.nMin; break;
    case SB_RIGHT: sci.nPos = sci.nMax; break;
    case SB_LINELEFT: sci.nPos -= pgl->pixelw; break;
    case SB_LINERIGHT: sci.nPos += pgl->pixelw; break;
    case SB_PAGELEFT: sci.nPos -= pgl->pixelw * GLW_SBPAGE; break;
    case SB_PAGERIGHT: sci.nPos += pgl->pixelw * GLW_SBPAGE; break;
    case SB_THUMBTRACK: sci.nPos = sci.nTrackPos; break;
    }

    sci.fMask = SIF_POS;
    SetScrollInfo(hwnd, nbar, &sci, TRUE);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

LRESULT onMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    if (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL) { //zoom
        int pws_to_zoom_in = GLW_ZOOMDELTA * GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
        POINT pt = { GETXLPARAM(lparam), GETYLPARAM(lparam) };
        ScreenToClient(hwnd, &pt);
        SendMessage(hwnd, WM_GLZOOM, pws_to_zoom_in, MAKELPARAM(pt.x, pt.y));
        return 0;
    }

    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    UINT wheel_lines;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheel_lines, 0);
    int nbar = (GET_KEYSTATE_WPARAM(wparam) & MK_SHIFT) ? SB_HORZ : SB_VERT;
    int cells_to_wheel = (WORD)wheel_lines * GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

    sci.fMask = SIF_POS;
    GetScrollInfo(hwnd, nbar, &sci);
    sci.nPos -= cells_to_wheel * pgl->pixelw;
    SetScrollInfo(hwnd, nbar, &sci, TRUE);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

LRESULT onSize(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    switch (wparam) {
    case SIZE_RESTORED:
    case SIZE_MAXIMIZED:
        PostMessage(hwnd, WM_GLSETSCROLL, TRUE, 0);
        break;
    case SIZE_MINIMIZED:
        pgl->state |= GLRT_SF_PAUSE;
        break;
    }
    return 0;
}

LRESULT onSizing(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    //limit the window size
    PRECT pwrect = (PRECT)lparam;
    bool left = (wparam == WMSZ_LEFT) || (wparam == WMSZ_TOPLEFT) || (wparam == WMSZ_BOTTOMLEFT);
    bool right = (wparam == WMSZ_RIGHT) || (wparam == WMSZ_TOPRIGHT) || (wparam == WMSZ_BOTTOMRIGHT);
    bool top = (wparam == WMSZ_TOP) || (wparam == WMSZ_TOPLEFT) || (wparam == WMSZ_TOPRIGHT);
    bool bottom = (wparam == WMSZ_BOTTOM) || (wparam == WMSZ_BOTTOMLEFT) || (wparam == WMSZ_BOTTOMRIGHT);

    if (left) pwrect->left = min(pwrect->left, pwrect->right - GLW_MINWNDW);
    if (right) pwrect->right = max(pwrect->right, pwrect->left + GLW_MINWNDW);
    if (top) pwrect->top = min(pwrect->top, pwrect->bottom - GLW_MINWNDH);
    if (bottom) pwrect->bottom = max(pwrect->bottom, pwrect->top + GLW_MINWNDH);

    //display or hide the scroll bar
    PostMessage(hwnd, WM_GLSETSCROLL, TRUE, 0);
    return 0;
}

LRESULT onPaint(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT crect;
    GetClientRect(hwnd, &crect);
    HDC hdcbuffer = CreateCompatibleDC(hdc);
    HBITMAP hbmbuffer = CreateCompatibleBitmap(hdc, crect.right, crect.bottom);
    SelectObject(hdcbuffer, hbmbuffer);

    //paint the background
    HBRUSH hbr = (HBRUSH)GetStockObject(DC_BRUSH);
    SetDCBrushColor(hdcbuffer, GLW_DEFCOLMBG);
    FillRect(hdcbuffer, &crect, hbr);

    //decide the map position and draw map
    POINT ori;
    getMapOriginPoint(hwnd, &ori);
    pgl->draw(hdcbuffer, ori.x, ori.y, &ps.rcPaint, pgl->state & GLRT_SF_UNBOUND);

    //draw help information
    if (pgl->state & GLRT_SF_HELP) {
        RECT trect = {};
        SetBkColor(hdcbuffer, GLW_DEFCOLTBG);
        SetDCBrushColor(hdcbuffer, GLW_DEFCOLTBG);
        DrawText(hdcbuffer, TEXT(GLW_HELP), -1, &trect, DT_CALCRECT | DT_EXPANDTABS);
        FillRect(hdcbuffer, &trect, hbr);
        DrawText(hdcbuffer, TEXT(GLW_HELP), -1, &trect, DT_EXPANDTABS);
    }

    BitBlt(
        hdc,
        ps.rcPaint.left, ps.rcPaint.top,
        ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
        hdcbuffer,
        ps.rcPaint.left, ps.rcPaint.top,
        SRCCOPY
    );

    DeleteObject(hdcbuffer);
    DeleteObject(hbmbuffer);
    EndPaint(hwnd, &ps);
    return 0;
}

LRESULT onTimer(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GameOfLifeInfo* pgl = (GameOfLifeInfo*)GetWindowLongPtr(hwnd, GLWC_EXDATAOFF);
    pgl->update(pgl->state & GLRT_SF_UNBOUND);
    PostMessage(hwnd, WM_GLSETTEXT, FALSE, 0);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        return onCreate(hwnd, wparam, lparam);
    case WM_DESTROY:
        return onDestroy(hwnd, wparam, lparam);
    case WM_GLPAUSE:
        return onGLPause(hwnd, wparam, lparam);
    case WM_GLINSERT:
        return onGLInsert(hwnd, wparam, lparam);
    case WM_GLSPEED:
        return onGLSpeed(hwnd, wparam, lparam);
    case WM_GLZOOM:
        return onGLZoom(hwnd, wparam, lparam);
    case WM_GLSETSCROLL:
        return onGLSetScroll(hwnd, wparam, lparam);
    case WM_GLSETTEXT:
        return onGLSetText(hwnd, wparam, lparam);
    case WM_GLHELP:
        return onGLHelp(hwnd, wparam, lparam);
    case WM_CHAR:
        return onChar(hwnd, wparam, lparam);
    case WM_MOUSEMOVE:
        return onMouseMove(hwnd, wparam, lparam);
    case WM_HSCROLL:
        return onScroll(hwnd, wparam, lparam, true);
    case WM_VSCROLL:
        return onScroll(hwnd, wparam, lparam, false);
    case WM_LBUTTONDOWN:
        return onLRButtonDown(hwnd, wparam, lparam, true);
    case WM_RBUTTONDOWN:
        return onLRButtonDown(hwnd, wparam, lparam, false);
    case WM_MOUSEWHEEL:
        return onMouseWheel(hwnd, wparam, lparam);
    case WM_SIZE:
        return onSize(hwnd, wparam, lparam);
    case WM_SIZING:
        return onSizing(hwnd, wparam, lparam);
    case WM_PAINT:
        return onPaint(hwnd, wparam, lparam);
    case WM_TIMER:
        return onTimer(hwnd, wparam, lparam);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int runGame(GameOfLifeInfo* pgl, HINSTANCE hInstance, int nCmdShow) {
    HWND hwnd = CreateWindow(
        TEXT(GLWC_CLSNAME),
        TEXT(GLW_WNDNAME),
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        GLW_MINWNDW, GLW_MINWNDH,
        nullptr,
        nullptr,
        hInstance,
        pgl
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}


int fromFile(const tstring& fname, HINSTANCE hInstance, int nCmdShow) {
    std::ifstream file(fname);
    if (!file.is_open()) return RETVAL_ERRFOPEN;
    
    //header part: FT W H PW
    int ret = RETVAL_BADFTYPE;
    int ftype, w, h, pw;
    file >> ftype >> w >> h >> pw;
    if (file.fail()) return ret;
    if (w <= 0 || h <= 0) return ret;
    pw = min(max(pw, GLW_MINPIXW), GLW_MAXPIXW);

    switch (ftype) {
    case GLFT_BITMAP:{  //0 1 1 0 1 ...
        bool* map = new bool[w * h]{};
        for (int i = 0; i < w * h; i++) file >> map[i];
        if (!file.fail()) {
            file.close();
            GameOfLifeInfo* pgl = new GameOfLifeInfo(w, h, map, pw);
            ret = runGame(pgl, hInstance, nCmdShow);
            delete pgl;
        }
        delete[] map;
        break;
    }
    case GLFT_COORD: {  //x y x y ...
        std::vector<POINT> coords;
        POINT pos;
        while (!file.eof()) {
            file >> pos.x >> pos.y;
            if (file.fail()) break;
            coords.emplace_back(pos);
        }
        if (!coords.empty()) {
            file.close();
            GameOfLifeInfo* pgl = new GameOfLifeInfo(w, h, coords, pw);
            ret = runGame(pgl, hInstance, nCmdShow);
            delete pgl;
        }
        break;
    }
    case GLFT_RECT: {   //left top right bottom ...
        std::vector<RECT> rects;
        RECT rect;
        while (!file.eof()) {
            file >> rect.left >> rect.top >> rect.right >> rect.bottom;
            if (file.fail()) break;
            rects.emplace_back(rect);
        }
        if (!rects.empty()) {
            file.close();
            GameOfLifeInfo* pgl = new GameOfLifeInfo(w, h, rects, pw);
            ret = runGame(pgl, hInstance, nCmdShow);
            delete pgl;
        }
        break;
    }
    }
    return ret;
}

int fromCmdLine(LPTSTR lpCmdLine, HINSTANCE hInstance, int nCmdShow) {
    tstringstream cmdl(lpCmdLine);
    tstring str;
    int ret = RETVAL_CMDFAIL;
    cmdl >> str;
    if (cmdl.fail()) {  //empty cmdl
        GameOfLifeInfo* pgl = new GameOfLifeInfo(GLM_DEFMAPW, GLM_DEFMAPH, GLM_DEFRATIO);
        ret = runGame(pgl, hInstance, nCmdShow);
        delete pgl;
    }
    else if (!str.compare(TEXT("-h"))) {    //cmdl: -h
        ret = RETVAL_CMDHELP;
    }
    else if (!str.compare(TEXT("-f"))) {    //cmdl: -f FILENAME
        cmdl >> str;
        if (!cmdl.fail()) {
            ret = fromFile(str, hInstance, nCmdShow);
        }
    }
    else if (!str.compare(TEXT("-r"))) {    //cmdl: -r W H RATIO [PW]
        int w, h, pw = GLW_DEFPIXW;
        double ratio;
        cmdl >> w >> h >> ratio;
        if (!cmdl.fail()) {
            if (!cmdl.eof()) cmdl >> pw;
            pw = min(max(pw, GLW_MINPIXW), GLW_MAXPIXW);
            GameOfLifeInfo* pgl = new GameOfLifeInfo(w, h, ratio, pw);
            ret = runGame(pgl, hInstance, nCmdShow);
            delete pgl;
        }
    }
    return ret;
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wndc = { sizeof(WNDCLASSEX) };
    wndc.cbClsExtra = 0;
    wndc.cbWndExtra = GLWC_EXDATASIZE;
    wndc.hInstance = hInstance;
    wndc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wndc.hbrBackground = nullptr;
    wndc.lpfnWndProc = WndProc;
    wndc.lpszClassName = TEXT(GLWC_CLSNAME);
    wndc.lpszMenuName = nullptr;
    wndc.style = CS_VREDRAW | CS_HREDRAW;

    RegisterClassEx(&wndc);

    int ret = fromCmdLine(lpCmdLine, hInstance, nCmdShow);
    switch (ret) {
    case RETVAL_CMDFAIL:
        MessageBox(nullptr, TEXT("Invalid cmdl arguments"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    case RETVAL_CMDHELP:
        MessageBox(nullptr, TEXT(GLCMD_HELP), TEXT(GLW_WNDNAME), MB_OK | MB_ICONINFORMATION);
        break;
    case RETVAL_ERRFOPEN:
        MessageBox(nullptr, TEXT("Fail to open file"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    case RETVAL_BADFTYPE:
        MessageBox(nullptr, TEXT("Error when reading file"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    }
    return ret;
}
