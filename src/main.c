/**
 * Super But Old Mini on's is a hybrid watch face with a different super hero showing the time every hour.
 * Hero tells the time via hour in the bum, and points to minutes.
 *
 * Done:
 * > Move date to either side so as to reduce overlap with hand.
 * > Agressively optimize/cut away unneeded parts to accomodate more pics.
 **/
#include <pebble.h>

//Specify this flag to update every sec instead of min:
//#define DEBUG_MODE 1

//Specify this flag (in mins) to debug/try changing heroes
//#define TRY_HERO_CHANGE_INTERVAL 2

#define ANTIALIASING true
#define MINUTE_RADIUS 132

static Window *s_main_window;
static Layer *s_canvas_layer, *s_canvas_layer2;

static GPoint s_center;
//static int s_radius = 0;
//static int minute_stroke = 5;
//static int border_stroke = 4;

static TextLayer *s_hour_digit;
static TextLayer *s_day_date;
static char s_day_buffer[12];

#define MAX_HEROES 10
#define HERO_ID RESOURCE_ID_HERO_ARROW
#define MAX_HANDS MAX_HEROES
#define HAND_ID RESOURCE_ID_HAND_ARROW

static GBitmap *m_spbmPics = NULL;
static GBitmap *m_spbmPicsHands = NULL;
static RotBitmapLayer *m_spbmLayer[2] = {NULL};

#define DATERECT_X1 4
#define DATERECT_X2 76
#define DATERECT_Y 134
#define DATERECT_W 64
#define DATERECT_H 24

static int m_nHeroId = 0;
static int m_nHandId = 0;
static bool m_bFirstTime = true;
static bool m_bDateOnLeft = true;

/************************************ UI **************************************/

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
    // Store time
#ifdef DEBUG_MODE
    tick_time->tm_min = tick_time->tm_sec;
#endif

//APP_LOG(APP_LOG_LEVEL_DEBUG, "000 hero= %d", m_nHeroId);
#ifndef TRY_HERO_CHANGE_INTERVAL
    if ((tick_time->tm_min == 0) || (m_bFirstTime))
#else
#   ifndef DEBUG_MODE
//APP_LOG(APP_LOG_LEVEL_DEBUG, "min: %d", tick_time->tm_min);
    if (((tick_time->tm_min % (TRY_HERO_CHANGE_INTERVAL-1)) == 0) //DEBUG: change every (TRY_HERO_CHANGE_INTERVAL-1) mins
#   else
//APP_LOG(APP_LOG_LEVEL_DEBUG, "sec: %d", tick_time->tm_min);
    if (((tick_time->tm_min % (TRY_HERO_CHANGE_INTERVAL*2)) == 0) //DEBUG: change every (TRY_HERO_CHANGE_INTERVAL*2) secs
#   endif //DEBUG_MODE
        || (m_bFirstTime))
#endif //TRY_HERO_CHANGE_INTERVAL
    {
#ifndef TRY_HERO_CHANGE_INTERVAL
        int nNewHeroId = ((tick_time->tm_yday * 24) + tick_time->tm_hour) % MAX_HEROES;
#else
        int nNewHeroId = m_nHeroId + 1;
        if (nNewHeroId >= MAX_HEROES) nNewHeroId = 0;
//APP_LOG(APP_LOG_LEVEL_DEBUG, "hero= %d", nNewHeroId);
#endif //TRY_HERO_CHANGE_INTERVAL
        m_bFirstTime = false;
        if (nNewHeroId != m_nHeroId)
        {
//APP_LOG(APP_LOG_LEVEL_DEBUG, "free: %d", heap_bytes_free());
            if (m_spbmPics != NULL)
            {
                //free up memory:
                gbitmap_destroy(m_spbmPics);
                m_spbmPics = NULL;
//APP_LOG(APP_LOG_LEVEL_DEBUG, "X free: %d", heap_bytes_free());
            }
            m_nHeroId = nNewHeroId;
            //load only current needed pic:
            m_spbmPics = gbitmap_create_with_resource(HERO_ID + nNewHeroId);
//APP_LOG(APP_LOG_LEVEL_DEBUG, "N free: %d", heap_bytes_free());
            if (m_spbmPics != NULL)
            {
                bitmap_layer_set_bitmap((BitmapLayer *)m_spbmLayer[1], m_spbmPics);
                //layer_mark_dirty(s_canvas_layer2);
            }

            //Check if need to change hand
            if ((m_nHandId >= 0) && (m_spbmPicsHands != NULL))
            {
                //free up memory:
                gbitmap_destroy(m_spbmPicsHands);
                m_spbmPicsHands = NULL;
            }
            m_nHandId = nNewHeroId;
            //load only current needed pic:
            m_spbmPicsHands = gbitmap_create_with_resource(HAND_ID + m_nHandId);
            if (m_spbmPicsHands != NULL)
            {
                bitmap_layer_set_bitmap((BitmapLayer *)m_spbmLayer[0], m_spbmPicsHands);
            }
        }
    }

    if (m_bDateOnLeft)
    {
        if (tick_time->tm_min >= 30)
        {
            layer_set_frame(text_layer_get_layer(s_day_date), GRect(DATERECT_X2, DATERECT_Y, DATERECT_W, DATERECT_H));
            m_bDateOnLeft = false;
        }
    }
    else
    {
        if (tick_time->tm_min < 30)
        {
            layer_set_frame(text_layer_get_layer(s_day_date), GRect(DATERECT_X1, DATERECT_Y, DATERECT_W, DATERECT_H));
            m_bDateOnLeft = true;
        }
    }

    // Redraw
    if (s_canvas_layer)
    {
        layer_mark_dirty(s_canvas_layer);
    }
}

static void update_proc(Layer *layer, GContext *ctx) {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // hour text
    // Create a long-lived buffer
    static char buffer[] = "12";

    // Write the current hours and minutes into the buffer
    if (clock_is_24h_style())
    {
        // Use 24 hour format
        strftime(buffer, sizeof("00"), "%H", tick_time);
    }
    else
    {
        int hours = tick_time->tm_hour;
        if (hours > 12)
        {
            hours -= 12;
        }
        // Use 12 hour format
        if (hours >= 10)
        {
            strftime(buffer, sizeof("00"), "%I", tick_time);
        }
        else if (hours > 0)
        {
            // remove the leading "0""
            snprintf(buffer, 2, "%d", hours);
        }
        else snprintf(buffer, 2, "12");
    }

    // Display this time on the TextLayer
    text_layer_set_text(s_hour_digit, buffer);

    // Color background?
    //graphics_context_set_fill_color(ctx, GColorWhite);
    //graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);

    //graphics_context_set_stroke_color(ctx, GColorBlack);
    //graphics_context_set_antialiased(ctx, ANTIALIASING);

#ifndef DEBUG_MODE
    int32_t angleM = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
#else
    int32_t angleM = TRIG_MAX_ANGLE * tick_time->tm_sec / 60;
#endif //DEBUG_MODE
/*
    // Plot hands
    GPoint minute_hand = (GPoint) {
        .x = (int16_t)(sin_lookup(angleM) * (int32_t)MINUTE_RADIUS / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(angleM) * (int32_t)MINUTE_RADIUS / TRIG_MAX_RATIO) + s_center.y,
    };

    // draw minute line
//    graphics_context_set_stroke_color(ctx, GColorWhite);
//    graphics_context_set_stroke_width(ctx, minute_stroke + border_stroke);
//    graphics_draw_line(ctx, s_center, minute_hand);
//
//    graphics_context_set_stroke_color(ctx, GColorBlack);
//    graphics_context_set_stroke_width(ctx, minute_stroke);
//    graphics_draw_line(ctx, s_center, minute_hand);
*/

    rot_bitmap_layer_set_angle(m_spbmLayer[0], angleM);

    // White clockface
//    graphics_context_set_fill_color(ctx, GColorWhite);
//    graphics_fill_circle(ctx, s_center, s_radius);


    // draw black circle outline
//    graphics_context_set_stroke_color(ctx, GColorBlack);
//    graphics_context_set_stroke_width(ctx, minute_stroke);
//    graphics_draw_circle(ctx, s_center, s_radius);


    // date box
//    graphics_context_set_stroke_color(ctx, GColorBlack);
//    graphics_context_set_stroke_width(ctx, 1);
//    GRect rect = GRect(m_bDateOnLeft? DATERECT_X1: DATERECT_X2, DATERECT_Y+1, DATERECT_W, DATERECT_H);
    //graphics_fill_rect(ctx, rect, 10, GCornersAll);
//    graphics_draw_round_rect(ctx, rect, 10);
    strftime(s_day_buffer, sizeof("ddd dd"), "%a %d", tick_time);
    text_layer_set_text(s_day_date, s_day_buffer);
}

static void window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);

    s_center = grect_center_point(&window_bounds);

    s_canvas_layer = layer_create(window_bounds);
    layer_set_update_proc(s_canvas_layer, update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    // Create time TextLayer
    //s_hour_digit = text_layer_create(GRect(41, 57, 62, 50));
    s_hour_digit = text_layer_create(GRect(41+36, 57+30+45, 52, 50));
    text_layer_set_background_color(s_hour_digit, GColorClear);
    text_layer_set_text_color(s_hour_digit, GColorBlack);

    // create day/date layer
    s_day_date = text_layer_create(GRect(DATERECT_X1, DATERECT_Y, DATERECT_W, DATERECT_H));
    text_layer_set_background_color(s_day_date, GColorClear);
    text_layer_set_text_color(s_day_date, GColorBlack);

    // Improve the layout to be more like a watchface
    text_layer_set_font(s_hour_digit, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(s_hour_digit, GTextAlignmentRight); //GTextAlignmentCenter);

    text_layer_set_font(s_day_date, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_day_date, GTextAlignmentCenter);

    //Picture layers
    s_canvas_layer2 = layer_create(GRect(-36, -42, 288, 336));
    m_spbmPics = gbitmap_create_with_resource(HERO_ID + m_nHeroId);
    //m_nHandId = m_nHeroId;
    m_spbmPicsHands = gbitmap_create_with_resource(HAND_ID + m_nHandId);

    for (int i = 0; i < 2; ++i)
    {
        m_spbmLayer[i] = rot_bitmap_layer_create(i == 1? m_spbmPics: m_spbmPicsHands);
        bitmap_layer_set_background_color((BitmapLayer *)m_spbmLayer[i], GColorClear);
        rot_bitmap_set_compositing_mode(m_spbmLayer[i], GCompOpSet);
        //rot_bitmap_layer_set_angle(m_spbmLayer[i], 0);
        layer_add_child(s_canvas_layer2, bitmap_layer_get_layer((BitmapLayer *)m_spbmLayer[i]));
        layer_add_child(window_layer, s_canvas_layer2);
    }

    // Add it as a child layer to the Window's root layer
    layer_add_child(s_canvas_layer2, text_layer_get_layer(s_hour_digit));
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_day_date));
}

static void window_unload(Window *window)
{
    text_layer_destroy(s_hour_digit);
    text_layer_destroy(s_day_date);
    layer_destroy(s_canvas_layer);
    layer_destroy(s_canvas_layer2);
    if (m_spbmPics)
    {
        gbitmap_destroy(m_spbmPics);
    }
    if (m_spbmPicsHands)
    {
        gbitmap_destroy(m_spbmPicsHands);
    }
    rot_bitmap_layer_destroy(m_spbmLayer[0]);
    rot_bitmap_layer_destroy(m_spbmLayer[1]);
}

/*********************************** App **************************************/

static void init()
{
//    srand(time(NULL));

    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(s_main_window, true);

    time_t t = time(NULL);
    struct tm *time_now = localtime(&t);
    tick_handler(time_now, MINUTE_UNIT);

#ifdef DEBUG_MODE
    tick_timer_service_subscribe(SECOND_UNIT, &tick_handler);
#else
    tick_timer_service_subscribe(MINUTE_UNIT, &tick_handler);
#endif
}

static void deinit()
{
    window_destroy(s_main_window);
}

int main()
{
    init();
    app_event_loop();
    deinit();
}
