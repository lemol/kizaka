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

struct Bars {
  Vector2 tl;
  int width;
  int height;
  int dy;
};

bool is_dragging_bars = false;

typedef struct {
  Particle *particles;
  struct Bars bars;
  int n;
} State;

void OnBarsMouseButtonDown(struct Bars *bars, int x, int y) {
  if (is_dragging_bars) {
    return;
  }

  if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    return;
  }

  bool mouse_over_top = CheckCollisionPointRec(
      (Vector2){x, y},
      (Rectangle){bars->tl.x, bars->tl.y, bars->width, bars->height});

  bool mouse_over_bottom = CheckCollisionPointRec(
      (Vector2){x, y}, (Rectangle){bars->tl.x, bars->tl.y + bars->dy,
                                   bars->width, bars->height});

  if (!mouse_over_top && !mouse_over_bottom) {
    return;
  }

  is_dragging_bars = true;
}

void OnBarsMouseButtonUp() {
  if (!is_dragging_bars) {
    return;
  }

  if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    return;
  }

  is_dragging_bars = false;
}

void OnBarsDrag(struct Bars *bars, int x, int y) {
  if (!is_dragging_bars) {
    return;
  }

  bars->tl.x += x - bars->tl.x;
  bars->tl.y += y - bars->tl.y;
}

void DrawBars(struct Bars bars) {
  DrawRectangle(bars.tl.x, bars.tl.y, bars.width, bars.height, BARS_COLOR);
  DrawRectangle(bars.tl.x, bars.tl.y + bars.dy, bars.width, bars.height,
                (Color){0x04, 0xF0, 0x70, 0xFF});
}

#define DrawParticle(p) DrawCircleV(p.position, p.radius, p.color)
#define RandomColor()                                                          \
  (Color) { rand() % 256, rand() % 256, rand() % 256, rand() % 256 }

void InitParticles(Particle *particles, int n) {
  for (int i = 0; i < n; i++) {
    int y = rand() % 400;
    float r = 2 + rand() % 10;
    float vx = 10 + rand() % 100;

    particles[i] = (Particle){0};
    particles[i].position = (Vector2){0, y};
    particles[i].velocity = (Vector2){vx, 0};
    particles[i].color = RandomColor();
    particles[i].radius = r;
  }
}

Vector2 fnDx(State *s, Particle *p, float dt) {
  float dx_mru = p->velocity.x * dt;
  bool inside_bars =
      p->position.x + p->radius + dx_mru > s->bars.tl.x &&
      s->bars.tl.y + s->bars.height < p->position.y &&
      p->position.y < (s->bars.tl.y + s->bars.height + s->bars.dy);

  if (!inside_bars) {
    return (Vector2){dx_mru, 0};
  }

  float q = 0.1;
  float e = 100;
  float m = 100;

  float dvy = q * e * (p->position.x - s->bars.tl.x) / (m * p->velocity.x);
  float dy = q * e * pow(p->position.x - s->bars.tl.x + p->radius, 2) /
             (2 * m * p->velocity.x * p->velocity.x);

  return (Vector2){dvy, dy};
}

void MoveParticles(State *s, int n, float dt) {
  for (int i = 0; i < n; i++) {
    float dx_mru = s->particles[i].velocity.x * dt;
    Vector2 d = fnDx(s, &s->particles[i], dt);
    s->particles[i].position.x += dx_mru;
    s->particles[i].velocity.y += d.x;
    s->particles[i].position.y += d.y;
    DrawParticle(s->particles[i]);
  }
}

State init() {
  srand(time(0));

  Vector2 barsTl = {200, 200};
  Particle *particles = (Particle *)malloc(MAX_PARTICLES * sizeof(Particle));
  struct Bars bars = {barsTl, 300, 10, 90};

  State state = (State){particles, bars, MAX_PARTICLES};

  InitParticles(particles, MAX_PARTICLES);

  return state;
}

void close(State *system) {
  free(system->particles);
  free(system);
}

void update(State *system, float dt) {
  int x = GetMouseX();
  int y = GetMouseY();

  OnBarsMouseButtonDown(&system->bars, x, y);
  OnBarsMouseButtonUp();
  OnBarsDrag(&system->bars, x, y);

  DrawBars(system->bars);
  MoveParticles(system, system->n, dt);
}

int main(void) {
  DEV_START((char *[]){"main.c"}, "./build/main.dylib", 1);
  DEV_HOT_RELOAD_DEFINE(State, init);
  DEV_HOT_RELOAD_DEFINE(void, update, State *, float);

  InitWindow(800, 450, "raylib [core] example - basic window");

  State state = DEV_WITH_HOT_RELOAD(init)();

  while (!WindowShouldClose()) {
    DEV_HOT_RELOAD_HERE();

    float dt = GetFrameTime();

    if (IsKeyDown(KEY_R)) {
      state = DEV_WITH_HOT_RELOAD(init)();
    }

    BeginDrawing();
    ClearBackground(BLACK);

    DEV_WITH_HOT_RELOAD(update)(&state, dt);

    EndDrawing();
  }

  CloseWindow();

  DEV_CLOSE();

  return 0;
}
