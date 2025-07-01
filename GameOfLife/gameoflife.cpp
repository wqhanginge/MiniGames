/*
 * Copyright (C) 2023, Game of Life by Gee Wang
 * Licensed under the GNU GPL v3.
 * C++14 standard is required.
 */


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>

#include <Windows.h>
#include <tchar.h>


#define GETXLPARAM(l)   (MAKEPOINTS(l).x)
#define GETYLPARAM(l)   (MAKEPOINTS(l).y)

#define WM_GLPAUSE      (WM_APP + 0)
#define WM_GLINSERT     (WM_APP + 1)
#define WM_GLSPEED      (WM_APP + 2)    /* wparam is the gears to increase */
#define WM_GLZOOM       (WM_APP + 3)    /* wparam is the pixels to zoom in, lparam is the mouse position */
#define WM_GLSETSCROLL  (WM_APP + 4)    /* wparam is a BOOL indicating if redraw */
#define WM_GLSETTEXT    (WM_APP + 5)
#define WM_GLHELP       (WM_APP + 6)

#define GL_BIRTHCNT     3
#define GL_ALIVECNT     2

#define GLM_DEFMAPW     100
#define GLM_DEFMAPH     100
#define GLM_DEFRATIO    0.25

#define GLWC_CLSNAME    "GAMEOFLIFE"
#define GLWC_EXSIZE     sizeof(LONG_PTR)
#define GLWC_EXOFFSET   0

#define GLRT_SF_PAUSE   0x01
#define GLRT_SF_INSERT  0x02
#define GLRT_SF_INFMAP  0x04
#define GLRT_SF_HELP    0x08
#define GLRTISFROZEN(s) ((s) & (GLRT_SF_PAUSE | GLRT_SF_INSERT))

#define GLRT_TIMERID    1

#define GLW_MINWNDW     400
#define GLW_MINWNDH     400
#define GLW_WNDNAME     "Game of Life"
#define GLW_DEFSCALE    5
#define GLW_MINSCALE    1
#define GLW_MAXSCALE    10
#define GLW_SCROLLPAGE  10
#define GLW_ZOOMDELTA   3

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

#define GLW_COLBLANK    RGB(0, 0, 0)
#define GLW_COLCELL     RGB(255, 255, 255)
#define GLW_COLBARRIER  RGB(255, 255, 0)
#define GLW_COLBORDER   RGB(0, 0, 255)
#define GLW_DEFCOLTBG   RGB(0, 255, 255)

#define GLDT_RATIO      0
#define GLDT_BITMAP     1
#define GLDT_COORD      2
#define GLDT_RECT       3

#define GLSTR_WNDHELP   "\
 ESC\texit \n\
 w\tzoom in \n\
 s\tzoom out \n\
 a\tslow down \n\
 d\tspeed up \n\
 h\thelp \n\
 p\tpause \n\
 i\tinsert mode \n\
 n\tnext step \n\
 e\ttoggle edge \n\
 BS\tclear map \n\
 LMB\tspawn cell \n\
 RMB\tkill cell \
"

#define RETVAL_EXIT     0
#define RETVAL_CMDFAIL  1
#define RETVAL_BADARGS  2
#define RETVAL_ERROPEN  3
#define RETVAL_BADDATA  4


using tstring = std::basic_string<TCHAR>;
using tstringstream = std::basic_stringstream<TCHAR>;

class Speed {
    static const int _count = 8;
    static const struct _Val { int interval; int rps; LPCTSTR description; } _vlist[_count];

public:
    enum Option :int { MANUAL, EXSLOW, VSLOW, SLOW, NORMAL, FAST, VFAST, EXFAST };

    Speed() :_opt(NORMAL) {}
    Speed(const Option opt) { _assign(opt); }

    friend bool operator==(const Speed& lhs, const Speed& rhs) { return lhs._opt == rhs._opt; }
    friend bool operator!=(const Speed& lhs, const Speed& rhs) { return lhs._opt != rhs._opt; }

    Speed& operator=(const Option opt) { _assign(opt); return *this; }

    Speed& operator+=(int n) { _add(n); return *this; }
    Speed& operator++() { return *this += 1; }
    const Speed operator++(int) { Speed tmp = *this; ++*this; return tmp; }
    Speed operator+(int n) const { return Speed(*this) += n; }

    Speed& operator-=(int n) { return *this += (-n); }
    Speed& operator--() { return *this -= 1; }
    const Speed operator--(int) { Speed tmp = *this; --*this; return tmp; }
    Speed operator-(int n) const { return *this + (-n); }

    int interval() const { return _vlist[_opt].interval; }
    int rps() const { return _vlist[_opt].rps; }
    LPCTSTR description() const { return _vlist[_opt].description; }

private:
    Option _opt;

    inline void _assign(const Option opt) { _opt = Option((opt % _count + _count) % _count); }
    inline void _add(int n) { _opt = Option((_opt + n % _count + _count) % _count); }
};

const Speed::_Val Speed::_vlist[Speed::_count] = {
    { -1, 0, TEXT("Manual") },
    { 1000, 1, TEXT("Extremly Slow") },
    { 500, 2, TEXT("Very Slow") },
    { 250, 4, TEXT("Slow") },
    { 100, 10, TEXT("Normal") },
    { 50, 20, TEXT("Fast") },
    { 25, 40, TEXT("Very Fast") },
    { 10, 100, TEXT("Extremly Fast") },
};

class GLMap {
public:
    LPCTSTR name = TEXT(GLW_WNDNAME);

    GLMap(size_t width, size_t height) :_width(width), _height(height), _gen(0) { _alloc(); }
    GLMap(const GLMap&) = delete;
    GLMap(GLMap&&) = delete;
    ~GLMap() { _free(); }

    bool* operator[](size_t row) { return _mfront + row * _width; }

    size_t width() const { return _width; }
    size_t height() const { return _height; }
    size_t gen() const { return _gen; }
    bool valid(size_t x, size_t y) const { return (x < _width && y < _height); }

    void init();
    void init(float ratio, unsigned seed = std::minstd_rand::default_seed);
    void init(const bool map[], size_t size);
    void init(const POINT coord[], size_t size);
    void init(const RECT rect[], size_t size);
    void next(bool boundless = false);

private:
    size_t _width, _height, _gen;
    bool* _mfront, * _mback;

    void _alloc() { _mfront = new bool[_width * _height]; _mback = new bool[_width * _height]; }
    void _free() { delete[] _mfront; delete[] _mback; }
    size_t _offset(size_t x, size_t y) const { return (x + _width) % _width + (y + _height) % _height * _width; }
};

void GLMap::init() {
    _gen = 0;
    memset(_mfront, 0, sizeof(bool) * _width * _height);
}

void GLMap::init(float ratio, unsigned seed /*std::minstd_rand::default_seed*/) {
    size_t msize = _width * _height;
    size_t csize = std::min(size_t(msize * std::abs(ratio)), msize);
    init();
    memset(_mfront, 1, sizeof(bool) * csize);
    if (csize > 0 && csize < msize) {
        std::minstd_rand urand(seed);
        for (size_t i = 0; i < msize; i++) {    //shuffle
            size_t k = urand() % (msize - i) + i;
            std::swap(_mfront[i], _mfront[k]);
        }
    }
}

void GLMap::init(const bool map[], size_t size) {
    init();
    memcpy(_mfront, map, sizeof(bool) * std::min(size, _width * _height));
}

void GLMap::init(const POINT coord[], size_t size) {
    init();
    for (size_t i = 0; i < size; i++) {
        auto& pt = coord[i];
        if (valid(pt.x, pt.y)) _mfront[pt.x + pt.y * _width] = true;
    }
}

void GLMap::init(const RECT rect[], size_t size) {
    init();
    for (size_t i = 0; i < size; i++) {
        RECT reg = { 0, 0, (LONG)_width, (LONG)_height };
        IntersectRect(&reg, &reg, &rect[i]);
        for (LONG row = reg.top; row < reg.bottom; row++) {
            memset(&_mfront[row * _width + reg.left], 1, sizeof(bool) * (reg.right - reg.left));
        }
    }
}

void GLMap::next(bool boundless /*flase*/) {
    for (size_t i = 0; i < _width * _height; i++) {
        size_t x = i % _width, y = i / _width, cnt = 0;

        cnt += (boundless || valid(x - 1, y - 1)) && _mfront[_offset(x - 1, y - 1)];
        cnt += (boundless || valid(x, y - 1)) && _mfront[_offset(x, y - 1)];
        cnt += (boundless || valid(x + 1, y - 1)) && _mfront[_offset(x + 1, y - 1)];
        cnt += (boundless || valid(x - 1, y)) && _mfront[_offset(x - 1, y)];
        cnt += (boundless || valid(x + 1, y)) && _mfront[_offset(x + 1, y)];
        cnt += (boundless || valid(x - 1, y + 1)) && _mfront[_offset(x - 1, y + 1)];
        cnt += (boundless || valid(x, y + 1)) && _mfront[_offset(x, y + 1)];
        cnt += (boundless || valid(x + 1, y + 1)) && _mfront[_offset(x + 1, y + 1)];

        _mback[i] = (cnt == GL_BIRTHCNT) + (cnt == GL_ALIVECNT) * _mfront[i];
    }
    std::swap(_mfront, _mback);
    _gen++;
}

struct GLRuntime {
    GLMap* map = nullptr;
    BYTE scale = GLW_DEFSCALE;
    BYTE state = GLRT_SF_PAUSE | GLRT_SF_HELP;
    Speed speed = Speed::NORMAL;
    POINT target = { -1 , -1 };
    COLORREF col_blank = GLW_COLBLANK;
    COLORREF col_cell = GLW_COLCELL;
    COLORREF col_barrier = GLW_COLBARRIER;
    COLORREF col_border = GLW_COLBORDER;

    LONG width() const { return (map) ? ((LONG)map->width() + 2) * scale : 0; }
    LONG height() const { return (map) ? ((LONG)map->height() + 2) * scale : 0; }
    LONG mapoff(LONG wndpos, LONG offset) const { return (wndpos - offset) / scale - 1; }
    LONG wndpos(LONG mapoff, LONG offset) const { return (mapoff + 1) * scale + offset; }

    POINT offset(HWND hwnd) const;
    void draw(HDC hdc, RECT region, POINT offset) const;
};

POINT GLRuntime::offset(HWND hwnd) const {
    POINT pt = {};

    RECT crect;
    GetClientRect(hwnd, &crect);
    pt.x = std::max(0L, crect.right - width()) / 2;
    pt.y = std::max(0L, crect.bottom - height()) / 2;

    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    sci.fMask = SIF_POS;
    GetScrollInfo(hwnd, SB_HORZ, &sci);
    pt.x -= sci.nPos;
    GetScrollInfo(hwnd, SB_VERT, &sci);
    pt.y -= sci.nPos;

    return pt;
}

void GLRuntime::draw(HDC hdc, RECT region, POINT offset) const {
    HBRUSH hbr = (HBRUSH)GetStockObject(DC_BRUSH);
    COLORREF col_edge = (state & GLRT_SF_INFMAP) ? col_border : col_barrier;

    RECT visual, content = { 0, 0, width(), height() };
    OffsetRect(&content, offset.x, offset.y);

    IntersectRect(&visual, &content, &region);
    SetDCBrushColor(hdc, col_edge);
    FillRect(hdc, &visual, hbr);

    InflateRect(&content, -scale, -scale);
    IntersectRect(&visual, &content, &region);
    SetDCBrushColor(hdc, col_blank);
    FillRect(hdc, &visual, hbr);

    SetDCBrushColor(hdc, col_cell);
    for (size_t i = 0; map && i < map->width() * map->height(); i++) {
        LONG x = LONG(i % map->width()), y = LONG(i / map->width());
        RECT cell = { 0, 0, scale, scale };
        OffsetRect(&cell, wndpos(x, offset.x), wndpos(y, offset.y));
        if ((*map)[y][x] && IntersectRect(&visual, &cell, &region)) {
            FillRect(hdc, &visual, hbr);
        }
    }
}


LRESULT onCreate(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lparam;
    GLRuntime* pgl = (GLRuntime*)lpcs->lpCreateParams;
    SetWindowLongPtr(hwnd, GLWC_EXOFFSET, (LONG_PTR)pgl);

    RECT crect = { 0, 0, pgl->width(), pgl->height() };
    AdjustWindowRect(&crect, lpcs->style, FALSE);
    int wndw = std::max<int>(GLW_MINWNDW, crect.right - crect.left);
    int wndh = std::max<int>(GLW_MINWNDH, crect.bottom - crect.top);
    MoveWindow(hwnd, lpcs->x, lpcs->y, wndw, wndh, FALSE);

    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    sci.fMask = SIF_RANGE;
    SetScrollInfo(hwnd, SB_HORZ, &sci, FALSE);
    SetScrollInfo(hwnd, SB_VERT, &sci, FALSE);

    if (!GLRTISFROZEN(pgl->state) && pgl->speed != Speed::MANUAL)
        SetTimer(hwnd, GLRT_TIMERID, pgl->speed.interval(), nullptr);

    PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
    return 0;
}

LRESULT onDestroy(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    KillTimer(hwnd, GLRT_TIMERID);
    PostQuitMessage(RETVAL_EXIT);
    return 0;
}

LRESULT onGLPause(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    if (GLRTISFROZEN(pgl->state)) { //resume
        pgl->state &= ~(GLRT_SF_PAUSE | GLRT_SF_INSERT);
        if (pgl->speed != Speed::MANUAL)
            SetTimer(hwnd, GLRT_TIMERID, pgl->speed.interval(), nullptr);
    } else {    //pause
        pgl->state |= GLRT_SF_PAUSE;
        KillTimer(hwnd, GLRT_TIMERID);
    }
    PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
    return 0;
}

LRESULT onGLInsert(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    pgl->state ^= GLRT_SF_INSERT;
    if (pgl->state & GLRT_SF_INSERT) {  //enter insert mode
        KillTimer(hwnd, GLRT_TIMERID);
    } else {    //exit insert mode and keep pause
        pgl->state |= GLRT_SF_PAUSE;
    }
    PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
    return 0;
}

LRESULT onGLSpeed(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    pgl->speed += (int)wparam;
    if (pgl->speed == Speed::MANUAL) {
        KillTimer(hwnd, GLRT_TIMERID);
    } else if (!GLRTISFROZEN(pgl->state)) {
        SetTimer(hwnd, GLRT_TIMERID, pgl->speed.interval(), nullptr);
    }
    PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
    return 0;
}

LRESULT onGLZoom(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    SCROLLINFO sci = { sizeof(SCROLLINFO) };

    //get current scroll position
    POINT npos, origin = pgl->offset(hwnd);
    sci.fMask = SIF_POS;
    GetScrollInfo(hwnd, SB_HORZ, &sci);
    npos.x = sci.nPos - std::max(origin.x, 0L);
    GetScrollInfo(hwnd, SB_VERT, &sci);
    npos.y = sci.nPos - std::max(origin.y, 0L);

    //zoom and update scroll bar
    BYTE scale = std::min(std::max(pgl->scale + (int)wparam, GLW_MINSCALE), GLW_MAXSCALE);
    std::swap(pgl->scale, scale);
    SendMessage(hwnd, WM_GLSETSCROLL, FALSE, 0);

    //update scroll position
    POINTS mpos = MAKEPOINTS(lparam);
    sci.fMask = SIF_RANGE;  //PAGE is alternative, see onGLSetScroll()
    GetScrollInfo(hwnd, SB_HORZ, &sci); //NOTE: Get before any Set, otherwise it may get wrong data
    npos.x = ((npos.x + mpos.x) * pgl->scale / scale - mpos.x) * (bool)sci.nMax;
    GetScrollInfo(hwnd, SB_VERT, &sci);
    npos.y = ((npos.y + mpos.y) * pgl->scale / scale - mpos.y) * (bool)sci.nMax;

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
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    WINDOWINFO wndi = { sizeof(WINDOWINFO) };
    GetWindowInfo(hwnd, &wndi);
    int vw = pgl->width(), vh = pgl->height();
    int cw = wndi.rcWindow.right - wndi.rcWindow.left - wndi.cxWindowBorders * 2;
    int ch = wndi.rcWindow.bottom - wndi.rcWindow.top - wndi.cyWindowBorders * 2 - GetSystemMetrics(SM_CYCAPTION);
    bool hscroll = (vw > cw), vscroll = (vh > ch);
    hscroll |= (vscroll && vw > (cw - GetSystemMetrics(SM_CXVSCROLL)));
    vscroll |= (hscroll && vh > (ch - GetSystemMetrics(SM_CYHSCROLL)));
    hscroll |= (vscroll && vw > (cw - GetSystemMetrics(SM_CXVSCROLL))); //reevaluate after changing

    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    sci.fMask = SIF_RANGE | SIF_PAGE;   //MaxScrollPos = MaxRangeValue - (PageSize - 1)
    sci.nMax = hscroll * vw;
    sci.nPage = hscroll * (cw - vscroll * GetSystemMetrics(SM_CXVSCROLL) + 1);
    SetScrollInfo(hwnd, SB_HORZ, &sci, (BOOL)wparam);
    sci.nMax = vscroll * vh;
    sci.nPage = vscroll * (ch - hscroll * GetSystemMetrics(SM_CYHSCROLL) + 1);
    SetScrollInfo(hwnd, SB_VERT, &sci, (BOOL)wparam);
    return 0;
}

LRESULT onGLSetText(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    tstringstream text;

    text << pgl->map->name << TEXT(':') << TEXT("  ");
    text << pgl->map->gen() << TEXT("  ");

    if (pgl->state & GLRT_SF_INSERT) {
        text << TEXT("Insert") << TEXT("  ");
    } else if (pgl->state & GLRT_SF_PAUSE) {
        text << TEXT("Pause") << TEXT("  ");
    }

    text << pgl->speed.description();
    if (pgl->speed != Speed::MANUAL)
        text << TEXT('[') << pgl->speed.rps() << TEXT(" RPS") << TEXT(']');
    text << TEXT("  ");

    if (pgl->map->valid(pgl->target.x, pgl->target.y))
        text << TEXT('(') << pgl->target.x << TEXT(',') << pgl->target.y << TEXT(')');

    SetWindowText(hwnd, text.str().c_str());
    return 0;
}

LRESULT onGLHelp(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    HDC hdc = GetDC(hwnd);
    RECT uprect = {};
    pgl->state ^= GLRT_SF_HELP;
    DrawText(hdc, TEXT(GLSTR_WNDHELP), -1, &uprect, DT_CALCRECT | DT_EXPANDTABS);
    InvalidateRect(hwnd, &uprect, FALSE);
    ReleaseDC(hwnd, hdc);
    return 0;
}

LRESULT onChar(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
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
        if (pgl->state & GLRT_SF_INSERT) {
            pgl->map->init();
            PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
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
        if (!GLRTISFROZEN(pgl->state) && pgl->speed == Speed::MANUAL) {
            SendMessage(hwnd, WM_TIMER, GLRT_TIMERID, (LPARAM)NULL);
        }
        break;
    case TEXT(GLW_CHAR_HELP):   //show or hide help information
        PostMessage(hwnd, WM_GLHELP, 0, 0);
        break;
    case TEXT(GLW_CHAR_EDGE):   //change map type
        if (GLRTISFROZEN(pgl->state)) {
            pgl->state ^= GLRT_SF_INFMAP;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }
    return 0;
}

LRESULT onLRButtonDown(HWND hwnd, WPARAM wparam, LPARAM lparam, bool lbutton) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    if (pgl->state & GLRT_SF_INSERT) {
        POINT origin = pgl->offset(hwnd);
        pgl->target = { pgl->mapoff(GETXLPARAM(lparam), origin.x), pgl->mapoff(GETYLPARAM(lparam), origin.y) };
        if (pgl->map->valid(pgl->target.x, pgl->target.y)) {
            RECT uprect = { 0, 0, pgl->scale, pgl->scale };
            (*pgl->map)[pgl->target.y][pgl->target.x] = lbutton;
            OffsetRect(&uprect, pgl->wndpos(pgl->target.x, origin.x), pgl->wndpos(pgl->target.y, origin.y));
            InvalidateRect(hwnd, &uprect, FALSE);
        }
    }
    return 0;
}

LRESULT onMouseMove(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    POINT origin = pgl->offset(hwnd);
    pgl->target = { pgl->mapoff(GETXLPARAM(lparam), origin.x), pgl->mapoff(GETYLPARAM(lparam), origin.y) };
    if ((pgl->state & GLRT_SF_INSERT) && (wparam == MK_LBUTTON || wparam == MK_RBUTTON)) {
        bool alive = wparam & MK_LBUTTON;
        if (pgl->map->valid(pgl->target.x, pgl->target.y)) {
            RECT uprect = { 0, 0, pgl->scale, pgl->scale };
            (*pgl->map)[pgl->target.y][pgl->target.x] = alive;
            OffsetRect(&uprect, pgl->wndpos(pgl->target.x, origin.x), pgl->wndpos(pgl->target.y, origin.y));
            InvalidateRect(hwnd, &uprect, FALSE);
        }
    }
    SendMessage(hwnd, WM_GLSETTEXT, 0, 0);
    return 0;
}

LRESULT onScroll(HWND hwnd, WPARAM wparam, LPARAM lparam, bool hscroll) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    SCROLLINFO sci = { sizeof(SCROLLINFO) };
    int nbar = (hscroll) ? SB_HORZ : SB_VERT;
    sci.fMask = SIF_ALL;
    GetScrollInfo(hwnd, nbar, &sci);

    switch (LOWORD(wparam)) {   //NOTE: the msg for HSCROLL and VSCROLL are symmetric
    case SB_LEFT: sci.nPos = sci.nMin; break;
    case SB_RIGHT: sci.nPos = sci.nMax; break;
    case SB_LINELEFT: sci.nPos -= pgl->scale; break;
    case SB_LINERIGHT: sci.nPos += pgl->scale; break;
    case SB_PAGELEFT: sci.nPos -= pgl->scale * GLW_SCROLLPAGE; break;
    case SB_PAGERIGHT: sci.nPos += pgl->scale * GLW_SCROLLPAGE; break;
    case SB_THUMBTRACK: sci.nPos = sci.nTrackPos; break;
    }

    sci.fMask = SIF_POS;
    SetScrollInfo(hwnd, nbar, &sci, TRUE);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

LRESULT onMouseWheel(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    POINT pt = { GETXLPARAM(lparam), GETYLPARAM(lparam) };
    ScreenToClient(hwnd, &pt);

    if (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL) { //zoom
        int pws_to_zoom_in = GLW_ZOOMDELTA * GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
        SendMessage(hwnd, WM_GLZOOM, pws_to_zoom_in, MAKELPARAM(pt.x, pt.y));
    } else {    //scroll
        GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
        SCROLLINFO sci = { sizeof(SCROLLINFO) };
        UINT wheel_lines;
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheel_lines, 0);
        int nbar = (GET_KEYSTATE_WPARAM(wparam) & MK_SHIFT) ? SB_HORZ : SB_VERT;
        int cells_to_wheel = (WORD)wheel_lines * GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

        sci.fMask = SIF_POS;
        GetScrollInfo(hwnd, nbar, &sci);
        sci.nPos -= cells_to_wheel * pgl->scale;
        SetScrollInfo(hwnd, nbar, &sci, TRUE);
        InvalidateRect(hwnd, nullptr, FALSE);

        POINT origin = pgl->offset(hwnd);
        pgl->target = { pgl->mapoff(pt.x, origin.x), pgl->mapoff(pt.y, origin.y) };
        PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
    }
    return 0;
}

LRESULT onSize(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
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
    //limit the window msize
    PRECT pwrect = (PRECT)lparam;
    bool left = (wparam == WMSZ_LEFT) || (wparam == WMSZ_TOPLEFT) || (wparam == WMSZ_BOTTOMLEFT);
    bool right = (wparam == WMSZ_RIGHT) || (wparam == WMSZ_TOPRIGHT) || (wparam == WMSZ_BOTTOMRIGHT);
    bool top = (wparam == WMSZ_TOP) || (wparam == WMSZ_TOPLEFT) || (wparam == WMSZ_TOPRIGHT);
    bool bottom = (wparam == WMSZ_BOTTOM) || (wparam == WMSZ_BOTTOMLEFT) || (wparam == WMSZ_BOTTOMRIGHT);

    if (left) pwrect->left = std::min(pwrect->left, pwrect->right - GLW_MINWNDW);
    if (right) pwrect->right = std::max(pwrect->right, pwrect->left + GLW_MINWNDW);
    if (top) pwrect->top = std::min(pwrect->top, pwrect->bottom - GLW_MINWNDH);
    if (bottom) pwrect->bottom = std::max(pwrect->bottom, pwrect->top + GLW_MINWNDH);

    //display or hide the scroll bar
    PostMessage(hwnd, WM_GLSETSCROLL, TRUE, 0);
    return 0;
}

LRESULT onPaint(HWND hwnd, WPARAM wparam, LPARAM lparam) {
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT crect;
    GetClientRect(hwnd, &crect);
    HDC hdcbuffer = CreateCompatibleDC(hdc);
    HBITMAP hbmbuffer = CreateCompatibleBitmap(hdc, crect.right, crect.bottom);
    SelectObject(hdcbuffer, hbmbuffer);

    //paint the background
    HBRUSH hbr = (HBRUSH)GetStockObject(DC_BRUSH);
    SetDCBrushColor(hdcbuffer, GLW_COLBLANK);
    FillRect(hdcbuffer, &crect, hbr);

    //decide the map position and draw map
    pgl->draw(hdcbuffer, ps.rcPaint, pgl->offset(hwnd));

    //draw help information
    if (pgl->state & GLRT_SF_HELP) {
        RECT trect = {};
        SetBkColor(hdcbuffer, GLW_DEFCOLTBG);
        SetDCBrushColor(hdcbuffer, GLW_DEFCOLTBG);
        DrawText(hdcbuffer, TEXT(GLSTR_WNDHELP), -1, &trect, DT_CALCRECT | DT_EXPANDTABS);
        FillRect(hdcbuffer, &trect, hbr);
        DrawText(hdcbuffer, TEXT(GLSTR_WNDHELP), -1, &trect, DT_EXPANDTABS);
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
    GLRuntime* pgl = (GLRuntime*)GetWindowLongPtr(hwnd, GLWC_EXOFFSET);
    pgl->map->next(pgl->state & GLRT_SF_INFMAP);
    PostMessage(hwnd, WM_GLSETTEXT, 0, 0);
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

int runGame(GLRuntime* pRuntime, HINSTANCE hInstance, int nCmdShow) {
    HWND hwnd = CreateWindow(
        TEXT(GLWC_CLSNAME),
        TEXT(GLW_WNDNAME),
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        GLW_MINWNDW, GLW_MINWNDH,
        nullptr,
        nullptr,
        hInstance,
        pRuntime
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


bool initFromStream(GLMap& map, std::istream& data, int dtype) {
    switch (dtype) {
    case GLDT_RATIO: {  //ratio
        float ratio = 0;
        data >> ratio;
        map.init(ratio, std::random_device{}());
        return true;
    }
    case GLDT_BITMAP: { //0 1 1 0 1 ...
        std::vector<char> bitmap;   //std::vector<bool> is not an array
        bool cell;
        while (data >> cell) bitmap.push_back(cell);
        map.init((bool*)bitmap.data(), bitmap.size());
        return true;
    }
    case GLDT_COORD: {  //x y x y ...
        std::vector<POINT> coord;
        POINT pt;
        while (data >> pt.x >> pt.y) coord.push_back(pt);
        map.init(coord.data(), coord.size());
        return true;
    }
    case GLDT_RECT: {   //left top right bottom ...
        std::vector<RECT> rect;
        RECT rc;
        while (data >> rc.left >> rc.top >> rc.right >> rc.bottom) rect.push_back(rc);
        map.init(rect.data(), rect.size());
        return true;
    }
    }
    return false;
}

int fromCmdLine(LPTSTR lpCmdLine, HINSTANCE hInstance, int nCmdShow) {
    tstring cmdl(lpCmdLine);
    if (cmdl.empty()) { //empty cmdl
        GLMap map(GLM_DEFMAPW, GLM_DEFMAPH);
        GLRuntime runtime = { &map };
        map.init(GLM_DEFRATIO, std::random_device{}());
        return runGame(&runtime, hInstance, nCmdShow);
    } else if (!cmdl.compare(0, 2, TEXT("-r"))) {   //cmdl: -r WIDTH HEIGHT SCALE RATIO
        tstringstream data(cmdl.substr(2));
        int width, height, scale;
        float ratio;
        if (data >> width >> height >> scale >> ratio) {
            if (width > 0 && height > 0) {
                GLMap map(width, height);
                GLRuntime runtime = { &map, (BYTE)std::min(std::max(scale, GLW_MINSCALE), GLW_MAXSCALE) };
                map.init(ratio, std::random_device{}());
                return runGame(&runtime, hInstance, nCmdShow);
            }
        }
        return RETVAL_BADARGS;
    } else if (cmdl.find_first_of(TEXT(' ')) == cmdl.npos || cmdl.find_first_of(TEXT('"'), 1) == cmdl.size() - 1) { //cmdl: FILENAME
        tstring fname = (cmdl[0] == TEXT('"')) ? cmdl.substr(1, cmdl.size() - 2) : cmdl;    //remove quotes
        std::ifstream file(fname.c_str());  //compatible with GCC
        if (file.is_open()) {
            int width, height, scale, dtype;
            if (file >> width >> height >> scale >> dtype) {
                if (width > 0 && height > 0) {
                    GLMap map(width, height);
                    GLRuntime runtime = { &map, (BYTE)std::min(std::max(scale, GLW_MINSCALE), GLW_MAXSCALE) };
                    if (initFromStream(map, file, dtype)) {
                        file.close();
                        return runGame(&runtime, hInstance, nCmdShow);
                    }
                }
            }
            return RETVAL_BADDATA;
        }
        return RETVAL_ERROPEN;
    }
    return RETVAL_CMDFAIL;
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wndc = { sizeof(WNDCLASSEX) };
    wndc.cbClsExtra = 0;
    wndc.cbWndExtra = GLWC_EXSIZE;
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
        MessageBox(nullptr, TEXT("Syntax error"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    case RETVAL_BADARGS:
        MessageBox(nullptr, TEXT("Invalid arguments"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    case RETVAL_ERROPEN:
        MessageBox(nullptr, TEXT("Fail to open file"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    case RETVAL_BADDATA:
        MessageBox(nullptr, TEXT("Bad file contents"), TEXT(GLW_WNDNAME), MB_OK | MB_ICONERROR);
        break;
    }
    return ret;
}
