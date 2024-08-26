#include "dev.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES 50
#define BARS_COLOR                                                             \
  (Color) { 0x00, 0xFF, 0x0F, 0xFF }

typedef struct {
  Vector2 position;
  Vector2 velocity0;
  Vector2 velocity;
  Color color;
  float radius;
  bool is_inside_field;
} Particle;

typedef struct {
  Particle *items;
  int count;
} Particles;

typedef struct {
  Rectangle top;
  Rectangle bottom;
  bool is_dragging;
} Bars;

typedef struct {
  Particles particles;
  Bars bars;
} State;

void OnBarsMouseButtonDown(Bars *bars, Vector2 mouse) {
  if (bars->is_dragging) {
    return;
  }

  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    return;
  }

  bool mouse_over_top = CheckCollisionPointRec(mouse, bars->top);
  bool mouse_over_bottom = CheckCollisionPointRec(mouse, bars->bottom);

  if (!mouse_over_top && !mouse_over_bottom) {
    return;
  }

  bars->is_dragging = true;
}

void OnBarsMouseButtonUp(Bars *bars) {
  if (!bars->is_dragging) {
    return;
  }

  if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    return;
  }

  bars->is_dragging = false;
}

void OnBarsDrag(Bars *bars, Vector2 mouse) {
  if (!bars->is_dragging) {
    return;
  }

  Vector2 dmouse = GetMouseDelta();

  bars->top.x += dmouse.x;
  bars->top.y += dmouse.y;
  bars->bottom.x = bars->top.x;
  bars->bottom.y += dmouse.y;
}

void bars_draw(Bars bars) {
  DrawRectangleRec(bars.top, BARS_COLOR);
  DrawRectangleRec(bars.bottom, BARS_COLOR);
}

#define DrawParticle(p) DrawCircleV(p.position, p.radius, p.color)
#define RandomColor()                                                          \
  (Color) { rand() % 256, rand() % 256, rand() % 256, rand() % 256 }

void particle_init(Particle *p, int screen_height) {
  int y = rand() % screen_height;
  float vx = 10 + rand() % 500;
  p->position = (Vector2){0, y};
  p->velocity0 = (Vector2){vx, 0};
  p->velocity = (Vector2){vx, 0};
  p->color = RandomColor();
  p->radius = 2 + rand() % 10;
  p->is_inside_field = false;
}

void particles_init(Particles *particles, int count, int screen_height) {
  particles->count = count;
  particles->items = (Particle *)malloc(count * sizeof(Particle));

  for (int i = 0; i < count; i++) {
    particle_init(&particles->items[i], screen_height);
  }
}

void particle_acelerate(Particle *p, float dt, State *state) {
  p->position.x += p->velocity.x * dt;

  if (!p->is_inside_field) {
    return;
  }

  float q = 2.1;
  float e = 400;
  float m = p->radius * 50;

  p->velocity.y =
      q * e * (p->position.x - state->bars.top.x) / (m * p->velocity0.x);
  p->position.y += q * e *
                   pow(p->position.x - state->bars.top.x + p->radius, 2) /
                   (2 * m * p->velocity0.x * p->velocity0.x);
}

void particles_update(State *state, float dt) {
  Bars bars = state->bars;
  Particles particles = state->particles;

  for (int i = 0; i < particles.count; i++) {
    bool is_in_bars =
        CheckCollisionCircleRec(particles.items[i].position,
                                particles.items[i].radius, bars.top) ||
        CheckCollisionCircleRec(particles.items[i].position,
                                particles.items[i].radius, bars.bottom);

    if (!particles.items[i].is_inside_field) {
      Rectangle fieldRec = {bars.top.x, bars.top.y + bars.top.height,
                            bars.top.width,
                            bars.bottom.y - bars.top.y - bars.top.height};

      particles.items[i].is_inside_field = CheckCollisionCircleRec(
          particles.items[i].position, particles.items[i].radius, fieldRec);
    }

    bool is_out = particles.items[i].position.x > GetScreenWidth() ||
                  particles.items[i].position.y > GetScreenHeight();

    if (is_out || is_in_bars) {
      particle_init(&particles.items[i], GetScreenHeight());
      continue;
    }

    particle_acelerate(&particles.items[i], dt, state);
  }
}

void particles_draw(Particles *particles) {
  for (int i = 0; i < particles->count; i++) {
    DrawParticle(particles->items[i]);
  }
}

void bars_init(Bars *bars, Vector2 top_left, int width, int height, int dy) {
  bars->top = (Rectangle){top_left.x, top_left.y, width, height};
  bars->bottom = (Rectangle){top_left.x, top_left.y + dy, width, height};
  bars->is_dragging = false;
}

void init(State *state) {
  srand(time(0));

  Bars bars;
  Vector2 bars_top_left = {200, 200};
  bars_init(&bars, bars_top_left, 400, 10, 160);

  Particles particles;
  particles_init(&particles, MAX_PARTICLES, GetScreenHeight());

  state->particles = particles;
  state->bars = bars;
}

void close(State *state) { free(state->particles.items); }

void on_clock(State *state, float dt) {
  Vector2 mouse = GetMousePosition();

  OnBarsMouseButtonDown(&state->bars, mouse);
  OnBarsMouseButtonUp(&state->bars);
  OnBarsDrag(&state->bars, mouse);

  particles_update(state, dt);
}

void draw(State *state, float dt) {
  bars_draw(state->bars);
  particles_draw(&state->particles);
}

int main(void) {
  DEV_START(SOURCE_PATHS("main.c", "dev.c"), "./build/main.dylib", 1);
  DEV_HOT_RELOAD_DEFINE(void, init, State *);
  DEV_HOT_RELOAD_DEFINE(void, on_clock, State *, float);
  DEV_HOT_RELOAD_DEFINE(void, draw, State *, float);

  State state;
  DEV_WITH_HOT_RELOAD(init)(&state);

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(800, 450, "raylib [core] example - basic window");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    DEV_HOT_RELOAD_HERE();

    float dt = GetFrameTime();
    DEV_WITH_HOT_RELOAD(on_clock)(&state, dt);

    if (IsKeyDown(KEY_R)) {
      DEV_WITH_HOT_RELOAD(init)(&state);
    }

    BeginDrawing();
    ClearBackground(BLACK);

    DEV_WITH_HOT_RELOAD(draw)(&state, dt);

    EndDrawing();
  }

  CloseWindow();
  close(&state);
  DEV_CLOSE();

  return 0;
}
