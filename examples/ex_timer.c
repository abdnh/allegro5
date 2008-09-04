/* A test of timer events. Since both al_current_time() as well as the timer
 * events may be the source of inaccuracy, it doesn't tell a lot.
 */
#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/* A structure holding all variables of our example program. */
struct Example
{
   ALLEGRO_FONT *myfont; /* Our font. */
   ALLEGRO_EVENT_QUEUE *queue; /* Our events queue. */

   double FPS; /* How often to update per second. */

   int x[4];

   bool first_tick;
   double this_time, prev_time, accum_time;
   double min_diff, max_diff, second_spread;
   double second;
   double timer_events;
   double timer_error;
   double timestamp;
} ex;

/* Initialize the example. */
static void init(void)
{
   ex.FPS = 50;
   ex.first_tick = true;

   ex.myfont = al_font_load_font("data/fixed_font.tga", 0);
   if (!ex.myfont) {
      TRACE("data/fixed_font.tga not found\n");
      exit(1);
   }
}

/* Cleanup. Always a good idea. */
static void cleanup(void)
{
   al_font_destroy_font(ex.myfont);
   ex.myfont = NULL;
}

/* Print some text. */
static void print(int x, int y, char const *format, ...)
{
   va_list list;
   char message[1024];

   va_start(list, format);
   uvszprintf(message, sizeof message, format, list);
   va_end(list);

   /* Actual text. */
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb_f(0, 0, 0));
   al_font_textout(ex.myfont, message, x, y);
}

/* Draw our example scene. */
static void draw(void)
{
   int h, y, i;
   double cur_time, event_overhead, total_error;
   
   cur_time = al_current_time();
   event_overhead = cur_time - ex.timestamp;
   total_error = event_overhead + ex.timer_error;

   h = al_font_text_height(ex.myfont);
   al_clear(al_map_rgb_f(1, 1, 1));

   print(0, 0, "%.9f target for %.0f Hz Timer", 1.0 / ex.FPS, ex.FPS);
   print(0, h, "%.9f now", ex.this_time - ex.prev_time);
   print(0, 2 * h, "%.9f accum over one second",
      ex.accum_time / ex.timer_events);
   print(0, 3 * h, "%.9f min", ex.min_diff);
   print(0, 4 * h, "%.9f max", ex.max_diff);
   print(300, 3.5 * h, "%.9f (max - min)", ex.second_spread);
   print(300, 4.5 * h, "%.9f (timer error)", ex.timer_error);
   print(300, 5.5 * h ,"%.9f (event overhead)", event_overhead);
   print(300, 6.5 * h, "%.9f (total error)" , total_error);

   y = 240;
   for (i = 0; i < 4; i++) {
      al_draw_rectangle(ex.x[i], y + i * 60, ex.x[i] + (1 << i),
         y + i * 60 + 60, al_map_rgb(1, 0, 0), ALLEGRO_FILLED);
   }
}

/* Called a fixed amount of times per second. */
static void tick(ALLEGRO_TIMER_EVENT* timer_event)
{
   int i;

   ex.this_time = al_current_time();

   if (ex.first_tick) {
      ex.first_tick = false;
   }
   else {
      double duration;
      if (ex.this_time - ex.second >= 1) {
         ex.second = ex.this_time;
         ex.accum_time = 0;
         ex.timer_events = 0;
         ex.second_spread = ex.max_diff - ex.min_diff;
         ex.max_diff = 0;
         ex.min_diff = 1;
      }
      duration = ex.this_time - ex.prev_time;
      if (duration < ex.min_diff) ex.min_diff = duration;
      if (duration > ex.max_diff) ex.max_diff = duration;
      ex.accum_time += duration;
      ex.timer_events++;
      ex.timer_error = timer_event->error;
      ex.timestamp = timer_event->timestamp;
   }

   draw();
   al_flip_display();

   for (i = 0; i < 4; i++) {
      ex.x[i] += 1 << i;
      ex.x[i] %= 640;
   }

   ex.prev_time = ex.this_time;
}

/* Run our test. */
static void run(void)
{
   ALLEGRO_EVENT event;
   while (1) {
      al_wait_for_event(ex.queue, &event);
      switch (event.type) {
         /* Was the X button on the window pressed? */
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return;

         /* Was a key pressed? */
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               return;
            break;

         /* Is it time for the next timer tick? */
         case ALLEGRO_EVENT_TIMER:
            tick(&event.timer);
            break;
      }
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;

   al_init();
   al_install_keyboard();
   al_install_mouse();
   al_font_init();

   display = al_create_display(640, 480);
   al_show_mouse_cursor();

   init();

   timer = al_install_timer(1.000 / ex.FPS);

   ex.queue = al_create_event_queue();
   al_register_event_source(ex.queue,
      (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(ex.queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(ex.queue, (ALLEGRO_EVENT_SOURCE *)display);
   al_register_event_source(ex.queue, (ALLEGRO_EVENT_SOURCE *)timer);

   al_start_timer(timer);
   run();

   al_destroy_event_queue(ex.queue);

   cleanup();

   return 0;
}
END_OF_MAIN()
