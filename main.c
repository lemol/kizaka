#include "dev.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES 10
#define BARS_COLOR                                                             \
  (Color) { 0x00, 0xFF, 0x0F, 0xFF }

typedef struct {
  Vector2 position;
  Vector2 velocity;
  Color color;
  float radius;
} Particle;

typedef struct {
  Vector2 top_left;
  int width;
  int height;
  int dy;
  bool is_dragging;
} Bars;

typedef struct {
  Particle *particles;
  Bars bars;
  int particles_count;
} State;

void OnBarsMouseButtonDown(Bars *bars, int x, int y) {
  if (bars->is_dragging) {
    return;
  }

  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    return;
  }

  bool mouse_over_top = CheckCollisionPointRec(
      (Vector2){x, y}, (Rectangle){bars->top_left.x, bars->top_left.y,
                                   bars->width, bars->height});

  bool mouse_over_bottom = CheckCollisionPointRec(
      (Vector2){x, y},
      (Rectangle){bars->top_left.x, bars->top_left.y + bars->dy, bars->width,
                  bars->height});

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

void OnBarsDrag(Bars *bars, int x, int y) {
  if (!bars->is_dragging) {
    return;
  }

  bars->top_left.x += x - bars->top_left.x;
  bars->top_left.y += y - bars->top_left.y;
}

void bars_draw(Bars bars) {
  DrawRectangle(bars.top_left.x, bars.top_left.y, bars.width, bars.height,
                BARS_COLOR);
  DrawRectangle(bars.top_left.x, bars.top_left.y + bars.dy, bars.width,
                bars.height, (Color){0x54, 0xF0, 0x70, 0xFF});
}

#define DrawParticle(p) DrawCircleV(p.position, p.radius, p.color)
#define RandomColor()                                                          \
  (Color) { rand() % 256, rand() % 256, rand() % 256, rand() % 256 }

void new_particle(Particle *p) {
  int y = rand() % 400;
  float r = 2 + rand() % 30;
  float vx = 10 + rand() % 500;
  *p = (Particle){0};
  p->position = (Vector2){0, y};
  p->velocity = (Vector2){vx, 0};
  p->color = RandomColor();
  p->radius = r;
}

void init_particles(Particle *particles, int n) {
  for (int i = 0; i < n; i++) {
    new_particle(&particles[i]);
  }
}

Vector2 fnDx(State *s, Particle *p, float dt) {
  float dx_mru = p->velocity.x * dt;
  bool inside_bars =
      p->position.x + p->radius + dx_mru > s->bars.top_left.x &&
      s->bars.top_left.y + s->bars.height < p->position.y &&
      p->position.y < (s->bars.top_left.y + s->bars.height + s->bars.dy);

  if (!inside_bars) {
    return (Vector2){dx_mru, 0};
  }

  float q = 0.1;
  float e = 100;
  float m = 100;

  float dvy =
      q * e * (p->position.x - s->bars.top_left.x) / (m * p->velocity.x);
  float dy = q * e * pow(p->position.x - s->bars.top_left.x + p->radius, 2) /
             (2 * m * p->velocity.x * p->velocity.x);

  return (Vector2){dvy, dy};
}

void particles_update(State *state, float dt) {
  for (int i = 0; i < state->particles_count; i++) {
    bool is_out = state->particles[i].position.x > GetScreenWidth() ||
                  state->particles[i].position.y > GetScreenHeight();

    bool is_in_bars = state->particles[i].position.x > state->bars.top_left.x &&
                      state->particles[i].position.x <
                          state->bars.top_left.x + state->bars.width &&
                      state->particles[i].position.y > state->bars.top_left.y &&
                      state->particles[i].position.y <
                          state->bars.top_left.y + state->bars.height;

    if (is_out || is_in_bars) {
      new_particle(&state->particles[i]);
      continue;
    }

    float dx_mru = state->particles[i].velocity.x * dt;
    Vector2 d = fnDx(state, &state->particles[i], dt);
    state->particles[i].position.x += dx_mru;
    state->particles[i].velocity.y += d.x;
    state->particles[i].position.y += d.y;
  }
}

void particles_draw(Particle *particles, int count) {
  for (int i = 0; i < count; i++) {
    DrawParticle(particles[i]);
  }
}

State init() {
  srand(time(0));

  Vector2 bars_top_left = {200, 200};
  Bars bars = {bars_top_left, 300, 10, 90};
  Particle *particles = (Particle *)malloc(MAX_PARTICLES * sizeof(Particle));

  State state = (State){particles, bars, MAX_PARTICLES};

  init_particles(particles, MAX_PARTICLES);

  return state;
}

void close_state(State *state) { free(state->particles); }

void on_clock(State *state, float dt) {
  int x = GetMouseX();
  int y = GetMouseY();

  OnBarsMouseButtonDown(&state->bars, x, y);
  OnBarsMouseButtonUp(&state->bars);
  OnBarsDrag(&state->bars, x, y);

  particles_update(state, dt);
}

void draw(State *state, float dt) {
  bars_draw(state->bars);
  particles_draw(state->particles, state->particles_count);
}

int main(void) {
  DEV_START(SOURCE_PATHS("main.c", "dev.c"), "./build/main.dylib", 1);
  DEV_HOT_RELOAD_DEFINE(State, init);
  DEV_HOT_RELOAD_DEFINE(void, on_clock, State *, float);
  DEV_HOT_RELOAD_DEFINE(void, draw, State *, float);

  InitWindow(800, 450, "raylib [core] example - basic window");

  State state = DEV_WITH_HOT_RELOAD(init)();

  while (!WindowShouldClose()) {
    DEV_HOT_RELOAD_HERE();

    float dt = GetFrameTime();
    DEV_WITH_HOT_RELOAD(on_clock)(&state, dt);

    if (IsKeyDown(KEY_R)) {
      state = DEV_WITH_HOT_RELOAD(init)();
    }

    BeginDrawing();
    ClearBackground(BLACK);

    DEV_WITH_HOT_RELOAD(draw)(&state, dt);

    EndDrawing();
  }

  CloseWindow();
  close_state(&state);
  DEV_CLOSE();

  return 0;
}
