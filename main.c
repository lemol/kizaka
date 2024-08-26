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

void particle_init(Particle *p) {
  int y = rand() % 400;
  float r = 2 + rand() % 30;
  float vx = 10 + rand() % 500;
  p->position = (Vector2){0, y};
  p->velocity = (Vector2){vx, 0};
  p->color = RandomColor();
  p->radius = r;
}

void particles_init(Particles *particles, int count) {
  particles->count = count;
  particles->items = (Particle *)malloc(count * sizeof(Particle));

  for (int i = 0; i < count; i++) {
    particle_init(&particles->items[i]);
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
  Particles particles = state->particles;

  for (int i = 0; i < particles.count; i++) {
    bool is_out = particles.items[i].position.x > GetScreenWidth() ||
                  particles.items[i].position.y > GetScreenHeight();

    bool is_in_bars =
        CheckCollisionCircleRec(particles.items[i].position,
                                particles.items[i].radius, bars.top) ||
        CheckCollisionCircleRec(particles.items[i].position,
                                particles.items[i].radius, bars.bottom);

    if (is_out || is_in_bars) {
      particle_init(&particles.items[i]);
      continue;
    }

    float dx_mru = particles.items[i].velocity.x * dt;
    Vector2 d = fnDx(state, &particles.items[i], dt);
    particles.items[i].position.x += dx_mru;
    particles.items[i].velocity.y += d.x;
    particles.items[i].position.y += d.y;
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
  bars_init(&bars, bars_top_left, 300, 10, 90);

  Particles particles;
  particles_init(&particles, MAX_PARTICLES);

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
