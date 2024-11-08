#define _POSIX_C_SOURCE 199309L
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

const double FRICTION = 0.92;
const int BASE_PARTICLE_COUNT = 1000;
int msleep(long msec) {
  struct timespec ts;
  int res;

  if (msec < 0) {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  do {
    res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);

  return res;
}

typedef struct particle particle;
struct particle {
  double x;
  double y;
  double vel_x;
  double vel_y;
  double acc_x;
  double acc_y;
  double lifespan;
};
typedef struct worldCtx worldCtx;
struct worldCtx {
  int screen_width;
  int screen_height;
  int particle_count;
  int mouse_x;
  int mouse_y;
};

void draw_pixel(int x, int y, const char pixel) {
  move(y, x);
  addch(pixel);
}

void particle_update(particle *this, double delta) {
  this->vel_x += this->acc_x;
  this->vel_y += this->acc_y;

  this->acc_x = 0;
  this->acc_y = 0;

  this->x += this->vel_x * delta;
  this->y += this->vel_y * delta;

  this->vel_y *= FRICTION;
  this->vel_x *= FRICTION;
}

void particle_draw(particle *this) {
  double total_vel = fabsf(this->vel_x) + fabsf(this->vel_y);
  if (this->lifespan > 300) {
    attron(COLOR_PAIR(1));
    draw_pixel(roundf(this->x), roundf(this->y), '#');
    attroff(COLOR_PAIR(1));
  } else if (this->lifespan > 200) {
    attron(COLOR_PAIR(2));
    draw_pixel(roundf(this->x), roundf(this->y), '%');
    attroff(COLOR_PAIR(2));
  } else {
    attron(COLOR_PAIR(3));
    draw_pixel(roundf(this->x), roundf(this->y), '.');
    attroff(COLOR_PAIR(3));
  }
}

void particle_apply_force(particle *this, double x, double y) {
  this->acc_x += x;
  this->acc_y += y;
}

void init_particles(particle *particles, worldCtx *world_ctx) {

  for (int i = 0; i < world_ctx->particle_count; i++) {

    int randx = (rand() % world_ctx->screen_width);
    int randy = (rand() % world_ctx->screen_height);

    particles[i].x = randx;
    particles[i].y = randy;
    particles[i].lifespan = (rand() % 100) + 300;
  }
}

void update(double delta, particle *particles, worldCtx *world_ctx) {
  double target_x = (double)world_ctx->mouse_x;
  double target_y = (double)world_ctx->mouse_y;
  for (int i = 0; i < world_ctx->particle_count; i++) {
    double dist_x = target_x - particles[i].x;
    double dist_y = target_y - particles[i].y;

    particle_apply_force(
        &particles[i],
        dist_x * 0.001 * (particles[i].lifespan / 100),
        dist_y * 0.001 * (particles[i].lifespan / 100));
    // Gravity
    // particle_apply_force(&particles[i], 0, 0.02);

    particle_update(&particles[i], delta);
    particles[i].lifespan--;

    if (particles[i].lifespan < 0) {
      particles[i].x = (rand() % world_ctx->screen_width);
      particles[i].vel_x = 0;
      particles[i].acc_x = 0;
      particles[i].y = (rand() % world_ctx->screen_height);
      particles[i].vel_y = 0;
      particles[i].acc_y = 0;
      particles[i].lifespan = (rand() % 100) + 300;
    }
  }
}

void draw(particle *particles, worldCtx *world_ctx) {
  for (int i = 0; i < world_ctx->particle_count; i++) {
    particle_draw(&particles[i]);
  }
  draw_pixel(world_ctx->mouse_x, world_ctx->mouse_y, '+');
}
typedef struct argsForKeyListener argsForKeyListener;
struct argsForKeyListener {
  worldCtx *world_ctx;
  WINDOW *window;
};

void *listen_for_keys(void *args) {

  argsForKeyListener *_args = (argsForKeyListener *)args;

  while (1) {
    MEVENT event;

    int ch = wgetch(_args->window);
    if (ch == KEY_MOUSE) {
      if (getmouse(&event) == OK) {
        _args->world_ctx->mouse_x = event.x;
        _args->world_ctx->mouse_y = event.y;
      }
    }
  }
}
typedef struct statusWindowEntry {
  const char* name;
  double value;
} statusWindowEntry;

void draw_status_window(statusWindowEntry *statuses, int count) {
  for (int i = 0; i < count; i++) {
    int status_name_len = strlen(statuses[i].name);
    char status_entry_buffer[status_name_len + 50];

    sprintf(status_entry_buffer,"%s: %lf\n",statuses[i].name,statuses[i].value);
    move(1 + i,1);
    printw("%s", status_entry_buffer);
  }
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  particle particles[BASE_PARTICLE_COUNT];
  WINDOW *window = initscr();

  start_color();
  init_color(COLOR_RED,800,400,0);
  init_color(COLOR_YELLOW,800,400,0);
  init_color(COLOR_BLUE,1000,600,0);


  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  keypad(stdscr, TRUE);

  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

  worldCtx world_ctx = {.particle_count = BASE_PARTICLE_COUNT};

  world_ctx.screen_width = getmaxx(window);
  world_ctx.screen_height = getmaxy(window);

  init_particles(particles, &world_ctx);
  int frame_count = 0;

  char status_window[500];

  pthread_t thread_id;
  // I hate this
  argsForKeyListener args_for_key_listener = {.window = window,
                                              .world_ctx = &world_ctx};
  pthread_create(&thread_id, NULL, listen_for_keys,
                 (void *)&args_for_key_listener); // kms

  struct timeval pre_frame_time, post_frame_time;
  const double SIXTY_FPS_US = (1.0 / 60.0) * 1000.0 * 1000.0;
  while (1) {

    gettimeofday(&pre_frame_time, NULL);
    world_ctx.screen_width = getmaxx(window);
    world_ctx.screen_height = getmaxy(window);

    frame_count++;
    clear();
    move(1, 1);
    printw(status_window);
    update(1, particles, &world_ctx);
    draw(particles, &world_ctx);

    gettimeofday(&post_frame_time, NULL);
    double frame_time =
        (double)((pre_frame_time.tv_sec - post_frame_time.tv_sec) * 1000000 +
                 post_frame_time.tv_usec - pre_frame_time.tv_usec);
    msleep((SIXTY_FPS_US - frame_time) / 1000);
    statusWindowEntry status_window_entries[] = {
      (statusWindowEntry){.name = "frame time", .value = frame_time},
      (statusWindowEntry){.name = "mouse x", .value = world_ctx.mouse_x },
      (statusWindowEntry){.name = "mouse y", .value = world_ctx.mouse_y },
      (statusWindowEntry){.name = "screen width", .value = world_ctx.screen_width },
      (statusWindowEntry){.name = "screen height", .value = world_ctx.screen_height },
      (statusWindowEntry){.name = "target frame time", .value = SIXTY_FPS_US},
      (statusWindowEntry){.name = "frame diff(us)", .value = (SIXTY_FPS_US - frame_time)},
      (statusWindowEntry){.name = "particle count", .value = world_ctx.particle_count},
    };
    draw_status_window(status_window_entries,8);

    refresh();
  }

  return 0;
}
