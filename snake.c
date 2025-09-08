#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <time.h>

#define SNAKE_SIZE 20 // Size of a cell 20x20

#define WIN_W 800
#define WIN_H 640

#define GRID_W (WIN_W / SNAKE_SIZE)
#define GRID_H (WIN_H / SNAKE_SIZE)

// Variables that can be changes from commands
int mute = FALSE;

int warpper = FALSE;

double snake_speed = 0.25;
//------------------------------------------------

HDC buffer_display = NULL; // buffer display
HBITMAP newbitmap = NULL;
HBITMAP oldbitmap = NULL;

// Grid to mark occupied cells
int occupied[GRID_H][GRID_W] = {FALSE};

// Art tools
HBRUSH snake_brush, apple_brush, background_brush;
HPEN outline_pen;
HFONT g_hFont;
//------------------------------------------------

// Snake
typedef enum { UP, DOWN, RIGHT, LEFT } Direction;

typedef struct { int x; int y; } trail;

trail snake[1280];
int snake_length = 3;

Direction snake_dir = RIGHT;
Direction buffer_dir = RIGHT;
//------------------------------------------------

// Apple
int apple_x = 0, apple_y = 0;
int apple_eaten = FALSE;
//------------------------------------------------

// Game variables
double move_timer = 0.0;
int active = TRUE;
int game_over = FALSE;
int score = 0;
//------------------------------------------------

#define TO_GRID_X(px) ((px) / SNAKE_SIZE)
#define TO_GRID_Y(py) ((py) / SNAKE_SIZE)

void update_score (HWND hwnd) {

    char caption[64];
    sprintf(caption, "Snake - score: %d", score);
    SetWindowText(hwnd, caption);

}

// Drwaing square
void drawsquare (HDC hdc, int x, int y, int length) {

    int half_length = length / 2;
    int left   = x - half_length;
    int top    = y - half_length;
    int right  = x + half_length;
    int bottom = y + half_length;

    // Outline pen
    HPEN oldPen = (HPEN)SelectObject(hdc, outline_pen);

    // Fill brush
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, snake_brush);

    // Draw the "square"
    Rectangle(hdc, left, top, right, bottom);

    // Restore
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

}

// Draw ellipse
void drawellipse (HDC hdc, int x, int y, int length) {

    int half_length = length / 2;
    int left   = x - half_length;
    int top    = y - half_length;
    int right  = x + half_length;
    int bottom = y + half_length;

    // Outline pen
    HPEN oldPen = (HPEN)SelectObject(hdc, outline_pen);

    // Fill brush
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, apple_brush);

    // Draw the ellipse
    Ellipse(hdc, left, top, right, bottom);

    // Restore
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

}

// Beep function
DWORD WINAPI BeepThread (LPVOID lpParam) {

    DWORD param = (DWORD)(uintptr_t)lpParam; // unpack
    int hz  = (param >> 16) & 0xFFFF;       // high 16 bits
    int dur = param & 0xFFFF;              // low 16 bits
    Beep(hz, dur);
    return 0;

}

// Beep thread function
void snake_beep (int hz, int dur) {

    if (!mute) {

        // Pack hz and dur into a single DWORD_PTR
        DWORD param = ((hz & 0xFFFF) << 16) | (dur & 0xFFFF);
        HANDLE hThread = CreateThread(NULL, 0, BeepThread, (LPVOID)(uintptr_t)param, 0, NULL);
        if (hThread) CloseHandle(hThread);

    }

}

// Check for collision with apple
int collision_checker () {

    return (snake[0].x == apple_x && snake[0].y == apple_y);

}

// Warpping function
void warp () {

    if (snake[0].x >= WIN_W) {
        snake[0].x = SNAKE_SIZE/2;
    }

    else if (snake[0].x < 0) {
        snake[0].x = WIN_W - SNAKE_SIZE/2; // wrap to rightmost cell
    }

    if (snake[0].y >= WIN_H) {
        snake[0].y = SNAKE_SIZE/2;
    }

    else if (snake[0].y < 0) {
        snake[0].y = WIN_H - SNAKE_SIZE/2; // wrap to bottommost cell
    }

}

// Snake movement
void crawl (double delta) {

    move_timer += delta;
    if (move_timer < snake_speed) return;

    move_timer = 0.0; // Reset timer

    // Change direction
    if (buffer_dir != snake_dir) {
        snake_dir = buffer_dir;
        snake_beep(750, 100);  // Beep when direction changed
    }

    if (!collision_checker()) {
        int tailX = TO_GRID_X(snake[snake_length - 1].x);
        int tailY = TO_GRID_Y(snake[snake_length - 1].y);
        occupied[tailY][tailX] = FALSE;
    }

    // Move body
    for (int i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    // Move head
    switch (snake_dir) {
        case UP:    snake[0].y -= SNAKE_SIZE; break;
        case DOWN:  snake[0].y += SNAKE_SIZE; break;
        case LEFT:  snake[0].x -= SNAKE_SIZE; break;
        case RIGHT: snake[0].x += SNAKE_SIZE; break;
    }

    if (warpper) warp();

    // Check collision after moving head
    int headX = snake[0].x / SNAKE_SIZE;
    int headY = snake[0].y / SNAKE_SIZE;
    if (occupied[headY][headX]) {
        game_over = TRUE;
    }

    // Mark new head
    occupied[headY][headX] = TRUE;

}

// Apple random position generator
void generate_apple_coords () {

    int max_col = WIN_W / SNAKE_SIZE;
    int max_row = WIN_H / SNAKE_SIZE;

    int cell_x;
    int cell_y;

    do {
        cell_x = rand() % max_col;
        cell_y = rand() % max_row;
    } while (occupied[cell_y][cell_x]);

    apple_x = cell_x * SNAKE_SIZE + SNAKE_SIZE / 2;
    apple_y = cell_y * SNAKE_SIZE + SNAKE_SIZE / 2;

}

// Initialize snake
void init_snake () {

    srand((unsigned int)time(NULL));
    generate_apple_coords();
    snake[0].x = 10;
    snake[0].y = 10;

    snake[1] = snake[2] = snake[0];

    memset(occupied, 0, sizeof(occupied));
    score = 0;

    occupied[TO_GRID_Y(snake[0].y)][TO_GRID_X(snake[0].x)] = TRUE;
    occupied[TO_GRID_Y(snake[1].y)][TO_GRID_X(snake[1].x)] = TRUE;
    occupied[TO_GRID_Y(snake[2].y)][TO_GRID_X(snake[2].x)] = TRUE;

}

// Game restart
void snake_restart () {

    game_over = FALSE;
    snake_length = 3;
    snake_dir = RIGHT;
    buffer_dir = RIGHT;
    init_snake();

}

// Controls
void key_handler (WPARAM key) {

    switch (key) { 
    
    case 'W':
        if (snake_dir == UP) return;
        if (snake_dir == DOWN) return;
        buffer_dir = UP;
        break;

    case 'A':
        if (snake_dir == LEFT) return;
        if (snake_dir == RIGHT) return;
        buffer_dir = LEFT;
        break;

    case 'S':
        if (snake_dir == DOWN) return;
        if (snake_dir == UP) return;
        buffer_dir = DOWN;
        break;

    case 'D':
        if (snake_dir == RIGHT) return;
        if (snake_dir == LEFT) return;
        buffer_dir = RIGHT;
        break;

    case VK_SPACE:
        if (game_over) snake_restart();

    }

}

// Initialize display buffer
void init_buffer (HWND hwnd) {

    HDC hdc = GetDC(hwnd);
    buffer_display = CreateCompatibleDC(hdc);
    newbitmap = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
    oldbitmap = (HBITMAP)SelectObject(buffer_display, newbitmap);
    ReleaseDC(hwnd, hdc);

}

// Cleanup display buffer
void cleanup_buffer () {

    SelectObject(buffer_display, oldbitmap);
    DeleteObject(newbitmap);
    DeleteDC(buffer_display);

}

// Initialize art tools
void init_brushes () { 

    snake_brush = CreateSolidBrush(RGB(255,0,0));
    apple_brush = CreateSolidBrush(RGB(0,0,255));
    background_brush = CreateSolidBrush(RGB(0,0,0));
    outline_pen = CreatePen(PS_SOLID, 1, RGB(255,255,255));

}

// Cleanup art tools
void cleanup_brushes () {

    DeleteObject(snake_brush);
    DeleteObject(apple_brush);
    DeleteObject(background_brush);
    DeleteObject(outline_pen);

}

// Initialize font
void init_text () {

    g_hFont = CreateFont(
        48,             // height
        0,                     // width
        0,                     // escapement
        0,                     // orientation
        FW_BOLD,               // weight
        FALSE, FALSE, FALSE,   // italic, underline, strikeout
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Arial"

    );

}

// Cleanup font
void cleanup_text () {

    if (g_hFont) {

        DeleteObject(g_hFont);
        g_hFont = NULL;

    }

}

// Draw game over text
void draw_text (HDC hdc, const char* text, RECT rect, COLORREF color) {

    if (!g_hFont) return;

    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFont);

    // Measure the text height first
    RECT measure = rect;
    DrawText(hdc, text, -1, &measure, DT_CALCRECT | DT_WORDBREAK);

    int text_height = measure.bottom - measure.top;
    int offset_y = (rect.bottom - rect.top - text_height) / 2;

    RECT centered_rect = rect;
    centered_rect.top    = rect.top + offset_y;
    centered_rect.bottom = centered_rect.top + text_height;

    DrawText(hdc, text, -1, &centered_rect, DT_CENTER | DT_WORDBREAK);

    SelectObject(hdc, oldFont);

}

// Draw to screen
void draw (HDC hdc) {

    HBRUSH newbrush, oldbrush;

    // Fill background
    RECT rect = {0,0,WIN_W,WIN_H};
    FillRect(buffer_display, &rect, background_brush);

    // Draw apple
    drawellipse(buffer_display, apple_x, apple_y, 10);

    // Draw snake
    for(int i=0;i<snake_length;i++) drawsquare(buffer_display, snake[i].x, snake[i].y, SNAKE_SIZE);

    if (game_over) {

        draw_text(buffer_display, "GAME OVER\nPress Spacebar...", rect, RGB(255, 255, 255));

    }

    // Draw to display screen
    BitBlt(hdc, 0,0, WIN_W, WIN_H, buffer_display, 0,0, SRCCOPY);

}

// Windows message handler
LRESULT CALLBACK snakewhndlr (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

    switch (msg) {

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        active = FALSE;
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        active = FALSE;
        return 0;

    case WM_KEYDOWN:
        key_handler(wparam);
        return 0;

    case WM_PAINT:
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        draw(hdc);
        EndPaint(hwnd, &ps);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);

    }

}

// Grow snake
void snake_grow () {

    trail last = snake[snake_length - 1];
    snake_length++;
    snake[snake_length-1] = last;

}

// Check for collision against walls
void death_cheaker () {

    if (snake[0].x < 0 || snake[0].x >= WIN_W ||
        snake[0].y < 0 || snake[0].y >= WIN_H) {

        game_over = TRUE;

    }

}

// Program arguments handler
void args_handler (LPSTR lpcmdline) {

    size_t len = strlen(lpcmdline);
    char *cmd_buf = malloc(len + 1);

    if (!cmd_buf) {

        return;

    }

    strcpy(cmd_buf, lpcmdline);

    char *token = strtok(cmd_buf, " ");
    while (token) {

        if (strcmp(token, "--mute") == 0) {
            mute = TRUE;
        }
        else if (strcmp(token, "--warp") == 0) {
            warpper = TRUE;
        }
        else if (strcmp(token, "--lv2") == 0) {
            snake_speed = 0.1;
        }
        else if (strcmp(token, "--lv3") == 0) {
            snake_speed = 0.05;
        }
        else if (strcmp(token, "--frenzy") == 0) {
            snake_speed = 0;
        }

        token = strtok(NULL, " ");

    }

    free(cmd_buf);

}

// Main entry
int WINAPI WinMain (HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int ncmdshow) {

    args_handler(lpcmdline);

    const char classname[] = "Snake_window";

    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    // Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = snakewhndlr;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hinstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = classname;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassEx(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    RECT rect = { 0, 0, WIN_W, WIN_H };
    AdjustWindowRect(&rect, style, FALSE);

    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    hwnd = CreateWindowEx(
        0,
        classname,
        "Snake",
        WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL, hinstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, ncmdshow);
    UpdateWindow(hwnd);

    MSG msg = {0};

    clock_t lastime = clock();

    // Initialize all
    init_snake();
    init_buffer(hwnd);
    init_brushes();
    init_text();

    // Game loop
    while (active) {

        // Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            TranslateMessage(&msg);
            DispatchMessage(&msg);

        }

        clock_t currentime = clock();

        double deltatime = (double)(currentime - lastime) / CLOCKS_PER_SEC;
        lastime = currentime;

        if (game_over) {

            InvalidateRect(hwnd, NULL, TRUE);
            Sleep(16);
            continue;

        }

        if (snake_length >= 1280) {

            game_over = TRUE;
            MessageBox(hwnd, "YOU WON", "Snake", MB_OK);

        }

        if (!warpper) death_cheaker();

        if (!game_over) crawl(deltatime);

        if (collision_checker()) apple_eaten = TRUE;

        if (apple_eaten) {

            snake_beep(500, 50);
            generate_apple_coords();
            snake_grow();
            score++;
            update_score(hwnd);
            apple_eaten = FALSE;

        }

        InvalidateRect(hwnd, NULL, TRUE);

        Sleep(16);

    }

    // Final cleanup
    cleanup_buffer();
    cleanup_brushes();
    cleanup_text();

    return msg.wParam;

}

/*---------------------END-------------------------*/