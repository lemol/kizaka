#include "dev.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES 50
#define COIL_COLOR                                                             \
  (Color) { 0x33, 0xAF, 0x0F, 0xFF }

typedef struct {
  Vector2 position;
  Vector2 velocity0;
  Vector2 velocity;
  Color color;
  float radius;
  float charge;
  float mass;
  bool is_in_coil_field;
} Particle;

typedef struct {
  Particle *items;
  int count;
} Particles;

typedef struct {
  Rectangle top;
  Rectangle bottom;
  bool is_dragging;
  float field_intensity;
} DeflectionCoil;

typedef struct {
  Particles particles;
  DeflectionCoil coil;
} State;

void coil_init(DeflectionCoil *coil, Vector2 top_left, int width, int height,
               int dy) {
  coil->top = (Rectangle){top_left.x, top_left.y, width, height};
  coil->bottom = (Rectangle){top_left.x, top_left.y + dy, width, height};
  coil->is_dragging = false;
  coil->field_intensity = 30;
}

void coil_on_mouse_button_down(DeflectionCoil *coil, Vector2 mouse) {
  if (coil->is_dragging) {
    return;
  }

  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    return;
  }

  bool mouse_over_top = CheckCollisionPointRec(mouse, coil->top);
  bool mouse_over_bottom = CheckCollisionPointRec(mouse, coil->bottom);

  if (!mouse_over_top && !mouse_over_bottom) {
    return;
  }

  coil->is_dragging = true;
}

void coil_on_mouse_button_up(DeflectionCoil *coil) {
  if (!coil->is_dragging) {
    return;
  }

  if (!IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    return;
  }

  coil->is_dragging = false;
}

void coil_on_drag(DeflectionCoil *coil, Vector2 mouse) {
  if (!coil->is_dragging) {
    return;
  }

  Vector2 dmouse = GetMouseDelta();

  coil->top.x += dmouse.x;
  coil->top.y += dmouse.y;
  coil->bottom.x = coil->top.x;
  coil->bottom.y += dmouse.y;
}

void coil_update(State *state, float dt) {
  Vector2 mouse = GetMousePosition();
  coil_on_mouse_button_down(&state->coil, mouse);
  coil_on_mouse_button_up(&state->coil);
  coil_on_drag(&state->coil, mouse);
}

void coil_draw(DeflectionCoil coil) {
  DrawRectangleRec(coil.top, COIL_COLOR);
  DrawRectangleRec(coil.bottom, COIL_COLOR);
}

#define DrawParticle(p) DrawCircleV(p.position, p.radius, p.color)
#define RandomColor()                                                          \
  (Color) { rand() % 256, rand() % 256, rand() % 256, 100 + rand() % 156 }

void particle_init(Particle *p, int min_height, int max_height) {
  float radius = 2 + rand() % 10;
  float vx0 = 50 + rand() % 500;
  int y = min_height + (int)round(radius) +
          rand() % (max_height - min_height - 2 * (int)round(radius));

  p->position = (Vector2){0, y};
  p->velocity0 = (Vector2){vx0, 0};
  p->velocity = (Vector2){vx0, 0};
  p->color = RandomColor();
  p->radius = 2 + rand() % 10;
  p->charge = rand() % 100 - 50;
  p->mass = p->radius * 25;
  p->is_in_coil_field = false;
}

void particles_init(Particles *particles, int count, int min_height,
                    int max_height) {
  particles->count = count;
  particles->items = (Particle *)malloc(count * sizeof(Particle));

  for (int i = 0; i < count; i++) {
    particle_init(&particles->items[i], min_height, max_height);
  }
}

void particle_acelerate(Particle *p, float dt, State *state) {
  p->position.x += p->velocity.x * dt;

  if (!p->is_in_coil_field) {
    p->position.y += p->velocity.y * dt;
    return;
  }

  p->velocity.y += p->charge * state->coil.field_intensity *
                   (p->position.x - state->coil.top.x) /
                   (p->mass * p->velocity0.x);
  p->position.y += p->charge * state->coil.field_intensity *
                   pow(p->position.x - state->coil.top.x + p->radius, 2) /
                   (2 * p->mass * p->velocity0.x * p->velocity0.x);
}

void particles_update(State *state, float dt) {
  DeflectionCoil coil = state->coil;
  Particles particles = state->particles;

  for (int i = 0; i < particles.count; i++) {
    bool has_coil_collision =
        CheckCollisionCircleRec(particles.items[i].position,
                                particles.items[i].radius, coil.top) ||
        CheckCollisionCircleRec(particles.items[i].position,
                                particles.items[i].radius, coil.bottom);

    bool is_out = particles.items[i].position.x > GetScreenWidth() ||
                  particles.items[i].position.y > GetScreenHeight();

    if (is_out || has_coil_collision) {
      particle_init(&particles.items[i],
                    state->coil.top.y + state->coil.top.height,
                    state->coil.bottom.y);
      continue;
    }

    Rectangle fieldRec = {coil.top.x, coil.top.y + coil.top.height,
                          coil.top.width,
                          coil.bottom.y - coil.top.y - coil.top.height};

    particles.items[i].is_in_coil_field = CheckCollisionCircleRec(
        particles.items[i].position, particles.items[i].radius, fieldRec);

    particle_acelerate(&particles.items[i], dt, state);
  }
}

void particles_draw(Particles *particles) {
  for (int i = 0; i < particles->count; i++) {
    DrawParticle(particles->items[i]);
  }
}

void init(State *state) {
  srand(time(0));

  DeflectionCoil coil;
  Vector2 bars_top_left = {200, 200};
  coil_init(&coil, bars_top_left, 300, 10, 160);

  Particles particles;
  particles_init(&particles, MAX_PARTICLES, coil.top.y + coil.top.height,
                 coil.bottom.y);

  state->particles = particles;
  state->coil = coil;
}

// WTF is this?
// Naming this as `close` causes SEGFAULT in linux
// From what I've debugged, it happens around libGLX, mesa, and stuff
// If you know why, please let me know
void close_WTFFFF(State *state) { free(state->particles.items); }

void on_clock(State *state, float dt) {
  coil_update(state, dt);
  particles_update(state, dt);
}

void draw(State *state, float dt) {
  coil_draw(state->coil);
  particles_draw(&state->particles);
}

int main(void) {
  DEV_START(SOURCE_PATHS("main.c", "dev.c"), DLL_NAME("./build/main"), 2);
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

    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, GREEN);

    EndDrawing();
  }

  CloseWindow();
  close_WTFFFF(&state);
  DEV_CLOSE();

  return 0;
}
