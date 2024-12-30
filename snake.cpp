#include "snake.h"
#include <playsoundapi.h>
#include <time.h>
#include "resource.h"

using namespace D2D1;

#define PLAY_TIMER 10001
#define DEF_SPEED 80
#define MIN_SPEED 150
#define MAX_SPEED 32
#define CELL_SIZE 25
#define instance GetModuleHandle(NULL)
#define ToFloat(i) static_cast<float>(i)
#define AdjustCell(c) ToFloat((static_cast<int>(c) % 2) ? (c - CELL_SIZE) / 2 : (c / 2));

CONST WCHAR WindowName[] = L"Snake                    [Score]-[%d]                    [HScore]-[%d]";

Snake::Snake() : factory(NULL), render(NULL), writefct(NULL), foodfmt(NULL),
                 brush(NULL), window(NULL), grid(0), losser(0), mute(0),
                 score(0), hscore(0), speed(DEF_SPEED), row(0), col(0),
                 foods({0}), direction(up), status(idle)
{
}

Snake::~Snake()
{
    SafeRelease(&brush);
    SafeRelease(&foodfmt);
    SafeRelease(&writefct);
    SafeRelease(&render);
    SafeRelease(&factory);
}

LRESULT CALLBACK Snake::GameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Snake *game = (Snake *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
        game = (Snake *)pcs->lpCreateParams;
        game->window = hWnd;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
        return TRUE;
    }

    case WM_TIMER:
    {
        if (game)
            game->Move();
        break;
    }

    case WM_RBUTTONUP:
    {
        if (game)
            game->Play(VK_ESCAPE);
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        if (game && BeginPaint(hWnd, &ps))
        {
            game->Draw();
            EndPaint(hWnd, &ps);
        }
        break;
    }

    case WM_SIZE:
    {
        if (game)
            game->Resize(LOWORD(lParam), HIWORD(lParam));
        break;
    }

    case WM_KEYDOWN:
    {
        if (game)
            game->Play(wParam);
        break;
    }

    case WM_DESTROY:
    {
        if (game)
            game->window = 0;
        break;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL Snake::Initialize(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    return Initialize(hWnd, rc.left, rc.top, rc.right, rc.bottom);
}

BOOL Snake::Initialize(HWND hWnd, INT x, INT y, INT cx, INT cy)
{
    cx -= (cx % CELL_SIZE);
    cy -= (cy % CELL_SIZE);

    row = cx / CELL_SIZE;
    col = cy / CELL_SIZE;

    { // Create Window
        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = GameProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"D2DSnakeClass";

        if (!RegisterClassEx(&wc))
            return FALSE;

        window = CreateWindow(wc.lpszClassName, NULL, WS_CHILD | WS_VISIBLE,
                              x, y, cx, cy, hWnd, (HMENU)10000, wc.hInstance, this);
    }

    { // Create Device resources
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);

        if (hr != S_OK)
            return FALSE;

        hr = factory->CreateHwndRenderTarget(RenderTargetProperties(),
                                             HwndRenderTargetProperties(window, SizeU(cx, cy)),
                                             &render);

        if (hr != S_OK)
            return FALSE;

        render->SetDpi(96.f, 96.f);
        hr = render->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 1.0f), &brush);

        if (hr != S_OK)
            return FALSE;

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writefct),
                                 reinterpret_cast<IUnknown **>(&writefct));

        if (hr != S_OK)
            return FALSE;

        hr = writefct->CreateTextFormat(L"Arial", NULL,
                                        DWRITE_FONT_WEIGHT_NORMAL,
                                        DWRITE_FONT_STYLE_NORMAL,
                                        DWRITE_FONT_STRETCH_NORMAL,
                                        CELL_SIZE, L"", &foodfmt);

        if (hr != S_OK)
            return FALSE;

        foodfmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        foodfmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    { // init cells
        POINTF pt, pt1, pt2;
        pt.x = pt1.x = pt2.x = AdjustCell(cx);
        pt.y = AdjustCell(cy);
        pt1.y = pt.y + CELL_SIZE;
        pt2.y = pt.y + (2 * CELL_SIZE);

        body.push_back(pt);
        body.push_back(pt1);
        body.push_back(pt2);

        lstrcpy(foods[0], L"🍎");
        lstrcpy(foods[1], L"🍋");
        lstrcpy(foods[2], L"🍇");
        lstrcpy(foods[3], L"🥝");
        lstrcpy(foods[4], L"🍓");
        lstrcpy(foods[5], L"🍉");
        lstrcpy(foods[6], L"🍒");
        lstrcpy(foods[7], L"🥭");
        lstrcpy(foods[8], L"🍑");
        lstrcpy(foods[9], L"🍍");

        srand((unsigned)time(NULL));
        // 🌲 🌳 🌴
        Food();
    }

    return TRUE;
}

void Snake::Food()
{
    food.x = ToFloat(CELL_SIZE * (rand() % (row - 2) + 1));
    food.y = ToFloat(CELL_SIZE * (rand() % (col - 2) + 1));
    for (auto it = body.begin(); it < body.end(); it++)
    {
        if (it->x == food.x)
        {
            Food();
            break;
        }
    }
    wchar_t c[sizeof("🍍") + 1];
    do
    {
        lstrcpy(c, foods[rand() % 10]);
    } while (c[0] == foods[10][0]);

    lstrcpy(foods[10], c);
}

void Snake::Play(WPARAM key)
{
    int sound = 0;
    switch (key)
    {
    case VK_ESCAPE:
    {
        if (status == pause || status == idle)
        {
            losser = FALSE;
            SetTimer(window, PLAY_TIMER, speed, NULL);
            status = play;
        }
        else if (status == play)
        {
            KillTimer(window, PLAY_TIMER);
            if (!mute)
                PlaySound(MAKEINTRESOURCE(IDR_PAUSE), instance, SND_RESOURCE | SND_ASYNC);
            status = pause;
            InvalidateRect(window, nullptr, 0);
        }
        return;
    }

    case VK_SPACE:
    {
        grid = !grid;
        InvalidateRect(window, nullptr, 0);
        return;
    }

    case VK_HOME:
    {
        Reset();
        return;
    }

    case VK_UP:
    {
        if (status == play && direction != up && direction != down)
        {
            direction = up;
            sound = 1;
        }
        break;
    }

    case VK_DOWN:
    {
        if (status == play && direction != down && direction != up)
        {
            direction = down;
            sound = 1;
        }
        break;
    }

    case VK_LEFT:
    {
        if (status == play && direction != left && direction != right)
        {
            direction = left;
            sound = 1;
        }
        break;
    }

    case VK_RIGHT:
    {
        if (status == play && direction != right && direction != left)
        {
            direction = right;
            sound = 1;
        }
        break;
    }
    }
    if (sound)
        PlaySound(MAKEINTRESOURCE(IDR_HISS), instance, SND_RESOURCE | SND_ASYNC | SND_NOSTOP);
}

void Snake::Draw()
{
    D2D1_RECT_F rect;
    float width = ToFloat(row * CELL_SIZE);
    float height = ToFloat(col * CELL_SIZE);
    float e = (CELL_SIZE / 2);
    float c = (CELL_SIZE / 6);

    if (!render)
    {
        RECT rc;
        GetClientRect(window, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        if (factory->CreateHwndRenderTarget(RenderTargetProperties(),
                                            HwndRenderTargetProperties(window, size),
                                            &render) == S_OK)
        {
            render->SetDpi(96.f, 96.f);
            render->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 1.0f), &brush);
        }
    }

    render->BeginDraw();
    render->SetTransform(Matrix3x2F::Identity());
    render->Clear(ColorF(0x1F1F1F));

    { // board
        brush->SetColor(ColorF(ColorF::DarkGreen));
        if (grid)
        {
            for (float i = 0; i < ToFloat(col) - 1; i++)
            {
                for (float j = 0; j < ToFloat(row); j++)
                {
                    rect.left = j * CELL_SIZE;
                    rect.top = i * CELL_SIZE;
                    rect.right = rect.left + CELL_SIZE;
                    rect.bottom = rect.top + CELL_SIZE;
                    render->DrawText(L"ִ", 1, foodfmt, rect, brush);
                }
            }
            // for (float x = 0; x < width; x += CELL_SIZE)
            //     render->DrawLine(Point2F(x, 0), Point2F(x, height), brush, 0.1f);
            // for (float x = 0; x < height; x += CELL_SIZE)
            //     render->DrawLine(Point2F(0, x), Point2F(width, x), brush, 0.1f);
        }
        brush->SetColor(ColorF(ColorF::WhiteSmoke));
        rect = RectF(0.f, 0.f, ToFloat(width), ToFloat(height));
        render->DrawRectangle(rect, brush, 0.2f);
    }

    { // snake
        // head
        brush->SetColor(ColorF(ColorF::GreenYellow));
        render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + e), e, e + 2), brush);
        brush->SetColor(ColorF(ColorF::DarkGreen));
        render->DrawEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + e), e, e + 2), brush);
        // eye
        if (direction == up || direction == down)
        {
            render->FillEllipse(Ellipse(Point2F(body[0].x + c, body[0].y + e), 5, 5), brush);
            render->FillEllipse(Ellipse(Point2F(body[0].x + (CELL_SIZE - c - 1), body[0].y + e), 5, 5), brush);
            brush->SetColor(ColorF(0xFF0000));
            render->FillEllipse(Ellipse(Point2F(body[0].x + c, body[0].y + e), 4, 4), brush);
            render->FillEllipse(Ellipse(Point2F(body[0].x + (CELL_SIZE - c - 1), body[0].y + e), 4, 4), brush);
            brush->SetColor(ColorF(ColorF::Yellow));
            render->FillEllipse(Ellipse(Point2F(body[0].x + c, body[0].y + e), 2, 2), brush);
            render->FillEllipse(Ellipse(Point2F(body[0].x + (CELL_SIZE - c - 1), body[0].y + e), 2, 2), brush);
        }
        else
        {
            render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + c), 5, 5), brush);
            render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + (CELL_SIZE - c - 1)), 5, 5), brush);
            brush->SetColor(ColorF(0xFF0000));
            render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + c), 4, 4), brush);
            render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + (CELL_SIZE - c - 1)), 4, 4), brush);
            brush->SetColor(ColorF(ColorF::Yellow));
            render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + c), 2, 2), brush);
            render->FillEllipse(Ellipse(Point2F(body[0].x + e, body[0].y + (CELL_SIZE - c - 1)), 2, 2), brush);
        }
        //  body
        for (auto it = body.begin() + 1; it < body.end(); it++)
        {
            brush->SetColor(ColorF(ColorF::GreenYellow));
            render->FillEllipse(Ellipse(Point2F(it->x + e, it->y + e), e, e + 2), brush);
            rect = RectF(it->x - 1, it->y - 4, it->x + CELL_SIZE, it->y + CELL_SIZE);
            brush->SetColor(ColorF(ColorF::Green));
            render->DrawText(L"▒", 1, foodfmt, rect, brush);
        }
    }

    { // food
        if (status == pause || status == play)
        {
            rect = RectF(food.x - 1, food.y - 1, food.x + CELL_SIZE, food.y + CELL_SIZE);
            render->DrawText(foods[10], lstrlen(foods[10]), foodfmt, rect, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        }
    }

    { // message
        if (status == pause || losser)
        {
            float xx = (width / 2) - 100;
            float yy = (height / 2) - 38;
            rect = RectF(xx, yy, xx + 226, yy + 76);
            brush->SetOpacity(0.75f);
            brush->SetColor(ColorF(0x660099));
            render->FillRoundedRectangle(RoundedRect(rect, 5, 5), brush);
            brush->SetColor(ColorF(ColorF::White));
            render->DrawRoundedRectangle(RoundedRect(rect, 5, 5), brush, 2.0f);
            brush->SetOpacity(1.f);
            if (status == pause)
                render->DrawText(L"🐍 Game paused!", 16, foodfmt, rect, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            else
                render->DrawText(L"🐍 Game over!", 14, foodfmt, rect, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        }
    }

    if (render->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET)
    {
        SafeRelease(&render);
        SafeRelease(&brush);
    }
}

void Snake::Resize(INT cx, INT cy)
{
    int x = (cx / 2) - ((row * CELL_SIZE) / 2);
    int y = (cy / 2) - ((col * CELL_SIZE) / 2);
    x = (x < 0) ? 0 : x;
    y = (y < 0) ? 0 : y;
    SetWindowPos(window, 0, x, y, 0, 0, SWP_NOSIZE);
}

void Snake::Reset()
{
    KillTimer(window, PLAY_TIMER);
    status = idle;
    InvalidateRect(window, nullptr, 0);

    body.clear();
    body.shrink_to_fit();

    POINTF pt, pt1, pt2;

    pt.x = pt1.x = pt2.x = AdjustCell(row * CELL_SIZE);
    pt.y = pt1.y = pt2.y = AdjustCell(col * CELL_SIZE);

    srand((unsigned)time(NULL));
    direction = static_cast<enum direction>(rand() % 4);

    switch (direction)
    {
    case up:
        pt1.y = pt.y + CELL_SIZE;
        pt2.y = pt.y + (2 * CELL_SIZE);
        break;
    case down:
        pt1.y = pt.y - CELL_SIZE;
        pt2.y = pt.y - (2 * CELL_SIZE);
        break;
    case left:
        pt1.x = pt.x + CELL_SIZE;
        pt2.x = pt.x + (2 * CELL_SIZE);
        break;
    case right:
        pt1.x = pt.x - CELL_SIZE;
        pt2.x = pt.x - (2 * CELL_SIZE);
        break;
    }

    body.push_back(pt);
    body.push_back(pt1);
    body.push_back(pt2);

    Food();
    hscore = score > hscore ? score : hscore;

    speed = DEF_SPEED;
    wchar_t buf[MAX_PATH];
    wsprintf(buf, WindowName, score * 10, hscore * 10);
    SetWindowText(GetParent(window), buf);
    score = 0;
    if (!mute)
        PlaySound(MAKEINTRESOURCE(IDR_END), instance, SND_RESOURCE | SND_SYNC);
}

void Snake::Move()
{
    for (size_t i = body.size() - 1; i > 0; i--)
    {
        body[i].x = body[i - 1].x;
        body[i].y = body[i - 1].y;
    }

    switch (direction)
    {
    case up:
    {
        body[0].y -= CELL_SIZE;
        if (body[0].y < 0.f)
        {
            losser = TRUE;
            Reset();
            return;
        }
        break;
    }
    case left:
    {
        body[0].x -= CELL_SIZE;
        if (body[0].x < 0.f)
        {
            losser = TRUE;
            Reset();
            return;
        }
        break;
    }
    case right:
    {
        body[0].x += CELL_SIZE;
        if ((body[0].x / CELL_SIZE) == ToFloat(row))
        {
            losser = TRUE;
            Reset();
            return;
        }
        break;
    }
    case down:
    {
        body[0].y += CELL_SIZE;
        if ((body[0].y / CELL_SIZE) == ToFloat(col))
        {
            losser = TRUE;
            Reset();
            return;
        }
        break;
    }
    }

    for (size_t it = 6; it < body.size(); it++)
    {
        if (body[0].x == body[it].x && body[0].y == body[it].y)
        {
            losser = TRUE;
            Reset();
            return;
        }
    }

    if ((body[0].x == food.x) && (body[0].y == food.y))
    {
        if (!mute)
            PlaySound(MAKEINTRESOURCE(IDR_EAT), instance, SND_RESOURCE | SND_ASYNC);
        wchar_t buf[MAX_PATH];

        POINTF f = {body.end()->x - CELL_SIZE, body.end()->y - CELL_SIZE};
        body.push_back(f);
        score++;
        wsprintf(buf, WindowName, score * 10, hscore * 10);
        SetWindowText(GetParent(window), buf);
        if ((score % 10) == 0 && speed > MAX_SPEED)
        {
            speed -= 10;
            KillTimer(window, PLAY_TIMER);
            SetTimer(window, PLAY_TIMER, speed, NULL);
        }
        Food();
    }

    InvalidateRect(window, nullptr, 0);
}

int Snake::GetSpeed() const
{
    return speed;
}

int Snake::GetScore() const
{
    return score;
}

int Snake::GetHighScore() const
{
    return hscore;
}