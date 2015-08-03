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

//Specify this flag (in mins or *2secs if DEBUG_MODE is on) to debug/try changing heroes
//#define TRY_HERO_CHANGE_INTERVAL 5

//Specify the delay interval (in ms) before trying to load hero image again
#define INTERVAL_RETRY 500

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
#define BG_ID RESOURCE_ID_BG01
//hours to transit/change background (we've 2 at the moment)
#define HOUR_NIGHT 19
#define HOUR_DAY 8

static GBitmap *m_spbmPics = NULL;
static GBitmap *m_spbmPicsHands = NULL;
static GBitmap *m_spbmBg = NULL;
static BitmapLayer *m_spbmLayerBg = NULL;
static RotBitmapLayer *m_spbmLayer[2] = {NULL};

#define DATERECT_X1 4
#define DATERECT_X2 76
#define DATERECT_Y 134
#define DATERECT_W 64
#define DATERECT_H 24

static int m_nBgId = 0;
static int m_nHeroId = 0;
static int m_nHandId = 0;
static bool m_bFirstTime = true;
static bool m_bDateOnLeft = true;
static AppTimer *m_sptimer1;

/************************************ UI **************************************/
void loadHero(void *a_Retry)
{
#ifdef DEBUG_MODE
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s hero: %d, free: %d", (a_Retry==NULL)? "Load":"Retry", m_nHeroId, heap_bytes_free());
#endif
    if (m_spbmPics != NULL)
    {
        if (!a_Retry)
        {
            gbitmap_destroy(m_spbmPics); //free memory
        }
        else
        {
            return;
        }
    }
    //load only current needed pic:
    m_spbmPics = gbitmap_create_with_resource(HERO_ID + m_nHeroId);
    if (m_spbmPics != NULL)
    {
#ifdef DEBUG_MODE
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Load OK: %d", heap_bytes_free());
#endif
        bitmap_layer_set_bitmap((BitmapLayer *)m_spbmLayer[1], m_spbmPics);
    }
    else if (!a_Retry)
    {
#ifdef DEBUG_MODE
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Load failed: %d", heap_bytes_free());
#endif
        m_sptimer1 = app_timer_register(INTERVAL_RETRY, (AppTimerCallback) loadHero, (void*)1);
    }
#ifdef DEBUG_MODE
    else APP_LOG(APP_LOG_LEVEL_DEBUG, "RETRY failed: %d", heap_bytes_free());
#endif
}

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
#endif //TRY_HERO_CHANGE_INTERVAL
        m_bFirstTime = false;
        if (nNewHeroId != m_nHeroId)
        {
            m_nHeroId = nNewHeroId;
            loadHero(NULL);

            //Check if need to change hand
            if (m_spbmPicsHands != NULL)
            {
                gbitmap_destroy(m_spbmPicsHands); //free up memory
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
    int nNewBgId = 0;
    if ((tick_time->tm_hour >= HOUR_NIGHT)
        || (tick_time->tm_hour < HOUR_DAY))
    {
        nNewBgId = 1;
    }
    if (nNewBgId != m_nBgId)
    {
        m_nBgId = nNewBgId;
        //free up memory:
        gbitmap_destroy(m_spbmBg);
        m_spbmBg = gbitmap_create_with_resource(BG_ID + m_nBgId);
        if (m_spbmBg != NULL)
        {
            bitmap_layer_set_bitmap(m_spbmLayerBg, m_spbmBg);
        }
    }

    GRect RECT_LEFT = GRect(DATERECT_X2, DATERECT_Y, DATERECT_W, DATERECT_H),
        RECT_RIGHT = GRect(DATERECT_X1, DATERECT_Y, DATERECT_W, DATERECT_H);

    if (m_bDateOnLeft)
    {
        if (tick_time->tm_min >= 30)
        {
            layer_set_frame(text_layer_get_layer(s_day_date), RECT_LEFT);
            m_bDateOnLeft = false;
        }
    }
    else
    {
        if (tick_time->tm_min < 30)
        {
            layer_set_frame(text_layer_get_layer(s_day_date), RECT_RIGHT);
            m_bDateOnLeft = true;
        }
    }

    // Redraw
    layer_mark_dirty(s_canvas_layer);
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
            buffer[0] = '0' + hours;
            buffer[1] = 0;
        }
        //else snprintf(buffer, 2, "12");
        else
        {
            buffer[0] = '1';
            buffer[1] = '2';
            buffer[2] = 0;
        }
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
    m_spbmBg = gbitmap_create_with_resource(BG_ID + m_nBgId);
    m_spbmLayerBg = bitmap_layer_create(GRect(36, 42, 144, 168));
    bitmap_layer_set_bitmap(m_spbmLayerBg, m_spbmBg);
    //bitmap_layer_set_background_color(m_spbmLayerBg, GColorClear);
    bitmap_layer_set_compositing_mode(m_spbmLayerBg, GCompOpSet);
    layer_add_child(s_canvas_layer2, bitmap_layer_get_layer(m_spbmLayerBg));

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
    layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window)
{
    text_layer_destroy(s_hour_digit);
    text_layer_destroy(s_day_date);
    layer_destroy(s_canvas_layer);
    layer_destroy(s_canvas_layer2);
    bitmap_layer_destroy(m_spbmLayerBg);
    if (m_spbmBg)
    {
        gbitmap_destroy(m_spbmBg);
    }
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
