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
  Rectangle top;
  Rectangle bottom;
  bool is_dragging;
} Bars;

typedef struct {
  Particle *particles;
  Bars bars;
  int particles_count;
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

Vector2 fnDx(State *state, Particle *particle, float dt) {
  float dx_mru = particle->velocity.x * dt;
  Bars bars = state->bars;

  bool inside_bars = bars.top.x < particle->position.x &&
                     particle->position.x < bars.top.x + bars.top.width &&
                     bars.top.y < particle->position.y &&
                     particle->position.y < bars.top.y + bars.top.height;

  if (!inside_bars) {
    return (Vector2){dx_mru, 0};
  }

  float q = 0.1;
  float e = 100;
  float m = 100;

  float dvy = q * e * (particle->position.x - state->bars.top.x) /
              (m * particle->velocity.x);
  float dy =
      q * e *
      pow(particle->position.x - state->bars.top.x + particle->radius, 2) /
      (2 * m * particle->velocity.x * particle->velocity.x);

  return (Vector2){dvy, dy};
}

void particles_update(State *state, float dt) {
  Bars bars = state->bars;

  for (int i = 0; i < state->particles_count; i++) {
    bool is_out = state->particles[i].position.x > GetScreenWidth() ||
                  state->particles[i].position.y > GetScreenHeight();

    bool is_in_bars =
        CheckCollisionCircleRec(state->particles[i].position,
                                state->particles[i].radius, bars.top) ||
        CheckCollisionCircleRec(state->particles[i].position,
                                state->particles[i].radius, bars.bottom);

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

Bars bars_new(Vector2 top_left, int width, int height, int dy) {
  Rectangle top = {top_left.x, top_left.y, width, height};
  Rectangle bottom = {top_left.x, top_left.y + dy, width, height};
  return (Bars){top, bottom, false};
}

State init() {
  srand(time(0));

  Vector2 bars_top_left = {200, 200};
  Bars bars = bars_new(bars_top_left, 300, 10, 90);
  Particle *particles = (Particle *)malloc(MAX_PARTICLES * sizeof(Particle));

  State state = (State){particles, bars, MAX_PARTICLES};

  init_particles(particles, MAX_PARTICLES);

  return state;
}

void close_state(State *state) { free(state->particles); }

void on_clock(State *state, float dt) {
  Vector2 mouse = GetMousePosition();

  OnBarsMouseButtonDown(&state->bars, mouse);
  OnBarsMouseButtonUp(&state->bars);
  OnBarsDrag(&state->bars, mouse);

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

  State state = DEV_WITH_HOT_RELOAD(init)();

  InitWindow(800, 450, "raylib [core] example - basic window");
  SetTargetFPS(60);

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
