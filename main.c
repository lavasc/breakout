#include <math.h>
#include <raylib.h>
#include <raymath.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BRICK_ROWS 4
#define BRICK_COLS 10

#define GAME_PLAYING 0
#define GAME_LOSE 1
#define GAME_WIN 2

typedef struct {
    Vector2 pos;
    Vector2 size;
    float speed;
} paddle_t;

typedef struct {
    Vector2 pos;
    float radius;
    Vector2 speed;
} ball_t;

typedef struct {
    Vector2 pos;
    Vector2 size;
    bool active;
} brick_t;

typedef struct {
    paddle_t paddle;
    ball_t ball;
    brick_t bricks[BRICK_ROWS][BRICK_COLS];
    int score;
    int state;
} Context;

static Context default_context = {};

static void context_update(Context *ctx)
{
    float dt = GetFrameTime();
    if (ctx->state == GAME_PLAYING) {
        // Update paddle
        if (IsKeyDown(KEY_A) && ctx->paddle.pos.x > 0)
            ctx->paddle.pos.x -= ctx->paddle.speed * dt;

        if (IsKeyDown(KEY_D) && ctx->paddle.pos.x < SCREEN_WIDTH - ctx->paddle.size.x)
            ctx->paddle.pos.x += ctx->paddle.speed * dt;

        // Update ball
        if (ctx->ball.pos.y - ctx->ball.radius < 0) {
            ctx->ball.speed.y *= -1;
        }
        if (ctx->ball.pos.x - ctx->ball.radius < 0 || ctx->ball.pos.x + ctx->ball.radius > SCREEN_WIDTH)
            ctx->ball.speed.x *= -1;
        ctx->ball.pos.x += ctx->ball.speed.x * dt;
        ctx->ball.pos.y += ctx->ball.speed.y * dt;

        // Ball and paddle collision
        if (CheckCollisionCircleRec(
                ctx->ball.pos, ctx->ball.radius,
                (Rectangle){ ctx->paddle.pos.x, ctx->paddle.pos.y, ctx->paddle.size.x, ctx->paddle.size.y })) {
            float paddle_center = ctx->paddle.pos.x + ctx->paddle.size.x / 2;
            float hit_factor = (ctx->ball.pos.x - paddle_center) / (ctx->paddle.size.x / 2.0f);
            Vector2 dir = { hit_factor, -1.0f };
            dir = Vector2Normalize(dir);
            float speed_bonus = 1.0f + (fabsf(hit_factor) * 0.5f);
            float base_speed = 400.0f;
            ctx->ball.speed.x = dir.x * base_speed * speed_bonus;
            ctx->ball.speed.y = dir.y * base_speed * speed_bonus;
        }

        // Ball and bricks collision
        for (int i = 0; i < BRICK_ROWS; ++i) {
            for (int j = 0; j < BRICK_COLS; ++j) {
                if (ctx->bricks[i][j].active) {
                    Rectangle brick_rec = { ctx->bricks[i][j].pos.x, ctx->bricks[i][j].pos.y, ctx->bricks[i][j].size.x,
                                            ctx->bricks[i][j].size.y };

                    if (CheckCollisionCircleRec(ctx->ball.pos, ctx->ball.radius, brick_rec)) {
                        ctx->bricks[i][j].active = false;

                        float brick_center_x = brick_rec.x + brick_rec.width / 2.0f;
                        float brick_center_y = brick_rec.y + brick_rec.height / 2.0f;

                        float overlap_x =
                            (brick_rec.width / 2.0f) + ctx->ball.radius - fabsf(ctx->ball.pos.x - brick_center_x);
                        float overlap_y =
                            (brick_rec.height / 2.0f) + ctx->ball.radius - fabsf(ctx->ball.pos.y - brick_center_y);
                        if (overlap_y < overlap_x) {
                            if (ctx->ball.pos.y < brick_center_y) {
                                ctx->ball.speed.y = -fabsf(ctx->ball.speed.y);
                            } else {
                                ctx->ball.speed.y = fabsf(ctx->ball.speed.y);
                            }
                        } else {
                            if (ctx->ball.pos.x < brick_center_x) {
                                ctx->ball.speed.x = -fabsf(ctx->ball.speed.x);
                            } else {
                                ctx->ball.speed.x = fabsf(ctx->ball.speed.x);
                            }
                        }
                        ctx->score++;
                        break;
                    }
                }
            }
        }

        // Game over
        if (ctx->ball.pos.y + ctx->ball.radius > SCREEN_HEIGHT)
            ctx->state = GAME_LOSE;
        if (ctx->score >= BRICK_ROWS * BRICK_COLS)
            ctx->state = GAME_WIN;
    } else if (IsKeyPressed(KEY_ENTER)) {
        *ctx = default_context;
    }
}

static void context_draw(Context *ctx)
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (ctx->state == GAME_PLAYING) {
        // Draw paddle
        DrawRectangleV(ctx->paddle.pos, ctx->paddle.size, BLACK);

        // Draw ball
        DrawCircleV(ctx->ball.pos, ctx->ball.radius, RED);

        // Draw bricks
        for (int i = 0; i < BRICK_ROWS; ++i) {
            for (int j = 0; j < BRICK_COLS; ++j) {
                if (ctx->bricks[i][j].active) {
                    Color color;
                    if (i % 2 == 0) {
                        color = (j % 2 == 0) ? GRAY : DARKGRAY;
                    } else {
                        color = (j % 2 == 0) ? DARKGRAY : GRAY;
                    }
                    DrawRectangleV(ctx->bricks[i][j].pos, ctx->bricks[i][j].size, color);
                }
            }
        }
    } else if (ctx->state == GAME_LOSE) {
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 80, 40, RED);
        DrawText("PRESS [ENTER] TO PLAY AGAIN", SCREEN_WIDTH / 2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20) / 2,
                 SCREEN_HEIGHT / 2, 20, GRAY);
    } else if (ctx->state == GAME_WIN) {
        DrawText("YOU WIN!", SCREEN_WIDTH / 2 - MeasureText("YOU WIN!", 40) / 2, SCREEN_HEIGHT / 2 - 80, 40, GREEN);
        DrawText("PRESS [ENTER] TO PLAY AGAIN", SCREEN_WIDTH / 2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20) / 2,
                 SCREEN_HEIGHT / 2, 20, GRAY);
    }
    DrawText(TextFormat("SCORE: %d", ctx->score),
             SCREEN_WIDTH - MeasureText(TextFormat("SCORE: %d", ctx->score), 20) - 20, 20, 20, BLACK);

    EndDrawing();
}

int main(void)
{
    Context context = {
        .paddle = {
            .pos = {},
            .size = { 100.0f, 20.0f },
            .speed = 700.0f,
        },
        .ball = {
            .pos = {},
            .radius = 10.0f,
            .speed = { 0.0f, -400.0f },
        },
        .bricks = {},
        .score = 0,
        .state = false
    };

    context.paddle.pos =
        (Vector2){ SCREEN_WIDTH / 2 - context.paddle.size.x / 2, SCREEN_HEIGHT - context.paddle.size.y * 5 };
    context.ball.pos =
        (Vector2){ context.paddle.pos.x + context.paddle.size.x / 2, context.paddle.pos.y - context.ball.radius * 3 };

    float brick_w = SCREEN_WIDTH / BRICK_COLS;
    float brick_h = 50.0f;
    for (int i = 0; i < BRICK_ROWS; ++i) {
        for (int j = 0; j < BRICK_COLS; ++j) {
            context.bricks[i][j].pos = (Vector2){ j * brick_w, i * brick_h };
            context.bricks[i][j].size = (Vector2){ brick_w, brick_h };
            context.bricks[i][j].active = true;
        }
    }

    default_context = context;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "breakout");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        context_update(&context);
        context_draw(&context);
    }
    CloseWindow();
    return 0;
}
