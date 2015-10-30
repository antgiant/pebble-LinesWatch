#include <pebble.h>
#include <time.h>

#define ConstantGRect(x, y, w, h) {{(x), (y)}, {(w), (h)}}

/* CONSTANTS
    - BackgroundColor is the main filling (black)
    - ForegroundColor is the filling of the cross, points and segments (white)
    - AnimationTime is the duration in milliseconds of transitions (2000)
    - Points are the coordinates of the two points at of each quadrant */
const int AnimationTime = 2000;
const int MiniAnimationTime = 4000;

const GRect Points[2] = {
    ConstantGRect(33, 25, 4, 4),
    ConstantGRect(33, 53, 4, 4)
};

const GRect MiniPoints[2] = {
    ConstantGRect(11, 8, 2, 2),
    ConstantGRect(11, 17, 2, 2)
};
/* Each number is contained in a quadrant which has a layer (its coordinates), 
    2 permanent points and 8 possible segments. Also storing the animations
    that need to be globally accessible and the current segments byte for
    future comparison. */
typedef struct {
    Layer *layer;
    Layer *points[2];
    Layer *segments[8];
    PropertyAnimation *animations[8];
    bool cancelAnimations[8];
    char currentSegments;
} Quadrant;
Quadrant quadrants[4];
Quadrant miniquadrants[2];

/* Each quadrant has 8 possible segments, whose coordinates are expressed here.
    There are two coordinates for each segment : 
    - visible is the rectangle when the segment is displayed
    - invisible is the rectangle/line to which the segment retracts when
      disappearing. */
typedef struct {
    GRect visible;
    GRect invisible;
} Segment;
const Segment Segments[8] = {
    {ConstantGRect(29, 0, 4, 29), ConstantGRect(29, 29, 4, 0)},
    {ConstantGRect(33, 53, 4, 29), ConstantGRect(33, 57, 4, 0)},
    {ConstantGRect(33, 53, 37, 4), ConstantGRect(37, 53, 0, 4)},
    {ConstantGRect(33, 25, 4, 32), ConstantGRect(33, 53, 4, 0)},
    {ConstantGRect(0, 53, 37, 4), ConstantGRect(33, 53, 0, 4)},
    {ConstantGRect(33, 25, 37, 4), ConstantGRect(37, 25, 0, 4)},
    {ConstantGRect(33, 0, 4, 29), ConstantGRect(33, 25, 4, 0)},
    {ConstantGRect(0, 25, 37, 4), ConstantGRect(33, 25, 0, 4)}
};
const Segment MiniSegments[8] = {
    {ConstantGRect(9, 0, 2, 9), ConstantGRect(9, 9, 2, 0)},
    {ConstantGRect(11, 17, 2, 9), ConstantGRect(11, 19, 2, 0)},
    {ConstantGRect(11, 17, 12, 2), ConstantGRect(12, 17, 0, 2)},
    {ConstantGRect(11, 8, 2, 10), ConstantGRect(11, 17, 2, 0)},
    {ConstantGRect(0, 17, 12, 2), ConstantGRect(11, 17, 0, 2)},
    {ConstantGRect(11, 8, 12, 2), ConstantGRect(12, 8, 0, 2)},
    {ConstantGRect(11, 0, 2, 9), ConstantGRect(11, 8, 2, 0)},
    {ConstantGRect(0, 8, 12, 2), ConstantGRect(11, 8, 0, 2)}
};

/* Other globals */
Window *window;
Layer *cross;
time_t current_time;
struct tm *t;
GColor BackgroundColor;
GColor ForegroundColor;

/*******************/
/* GENERAL PURPOSE */
/*******************/
/* fill_layer ensures that a layer is always filled with the ForegroundColor */
void fill_layer(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

/* draw_cross draws the main cross (horizontal and vertical lines accross
    the screen */
void draw_cross(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);

	//Main Vertical Line
	graphics_fill_rect(ctx, GRect(70, 0, 4, 68), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(71, 68, 2, 29), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(70, 97, 4, 69), 0, GCornerNone);

	//Smaller Vertical Lines
	graphics_fill_rect(ctx, GRect(46, 68, 2, 29), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(96, 68, 2, 29), 0, GCornerNone);

	//Main Horizontal Line
	graphics_fill_rect(ctx, GRect(0, 82, 46, 4), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(98, 82, 46, 4), 0, GCornerNone);
	
	//Smaller Horizontal Lines
	graphics_fill_rect(ctx, GRect(46, 68, 52, 2), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(46, 97, 52, 2), 0, GCornerNone);
}

/************/
/* SEGMENTS */
/************/
/* segment_show draws a segment with an animation */
void segment_show(Quadrant *quadrant, bool isBigQuadrant, int id) {
	GRect visible, invisible;
	int speed;
	if (isBigQuadrant) {
    	visible= Segments[id].visible;
    	invisible = Segments[id].invisible;
		speed = AnimationTime;
	} else {
    	visible = MiniSegments[id].visible;
	    invisible = MiniSegments[id].invisible;
		speed = MiniAnimationTime;
	}

    /* Ensures the segment is not animating to prevent bugs */
    if(animation_is_scheduled(property_animation_get_animation(quadrant->animations[id]))) {
        animation_unschedule(property_animation_get_animation(quadrant->animations[id]));
    }
    
    quadrant->animations[id] = property_animation_create_layer_frame(quadrant->segments[id], &invisible, &visible);
    animation_set_duration(property_animation_get_animation(quadrant->animations[id]), speed);
    animation_set_curve(property_animation_get_animation(quadrant->animations[id]), AnimationCurveLinear);
    animation_schedule(property_animation_get_animation(quadrant->animations[id]));
}

/* segment_hide removes a segment with an animation */
void segment_hide(Quadrant *quadrant, bool isBigQuadrant, int id) {
	GRect visible, invisible;
	int speed;
	if (isBigQuadrant) {
    	visible= Segments[id].visible;
    	invisible = Segments[id].invisible;
		speed = AnimationTime;
	} else {
    	visible = MiniSegments[id].visible;
	    invisible = MiniSegments[id].invisible;
		speed = MiniAnimationTime;
	}

    /* Ensures the segment is not animating to prevent bugs */
    if(animation_is_scheduled(property_animation_get_animation(quadrant->animations[id]))) {
        animation_unschedule(property_animation_get_animation(quadrant->animations[id]));
    }
    
    quadrant->animations[id] = property_animation_create_layer_frame(quadrant->segments[id], &visible, &invisible);
    animation_set_duration(property_animation_get_animation(quadrant->animations[id]), speed);
    animation_set_curve(property_animation_get_animation(quadrant->animations[id]), AnimationCurveLinear);
    animation_schedule(property_animation_get_animation(quadrant->animations[id]));
}

/* segment_draw calculates which segments need to be showed or hided.
    new is a byte, each bit of which represent one of the 8 segments */
void segment_draw(Quadrant *quadrant, bool isBigQuadrant, char new) {
    char prev = quadrant->currentSegments;
    char compare = prev^new;
    for(int i=0; i<8; i++) {
        if(((compare & ( 1 << i )) >> i) == 0b00000001) {
            if(((new & ( 1 << i )) >> i) == 0b00000001) {
                segment_show(quadrant, isBigQuadrant, i);
            } else {
                segment_hide(quadrant, isBigQuadrant, i);
            }
        }
    }
    
    quadrant->currentSegments = new;
}

/*************/
/* QUADRANTS */
/*************/
/* quadrant_init initializes the layer of a quadrant and the layers of
    its points and segments */
void quadrant_init(Quadrant *quadrant, GRect coordinates, bool isBigQuadrant) {
	quadrant->layer = layer_create(coordinates);
	
    /* Two points visible for 6, 8 and 9 (always present) */
    for(int i=0; i<2; i++) {
		if (isBigQuadrant) {
			quadrant->points[i] = layer_create(Points[i]);
		}
		else {
			quadrant->points[i] = layer_create(MiniPoints[i]);
		}
		layer_set_update_proc(quadrant->points[i], fill_layer);
        layer_add_child(quadrant->layer, (quadrant->points[i]));        
    }    
    
    /* Now we create the 8 segments, invisible */
    for(int i=0; i<8; i++) {
		if (isBigQuadrant) {
			quadrant->segments[i] = layer_create(Segments[i].invisible);
		}
		else {
			quadrant->segments[i] = layer_create(MiniSegments[i].invisible);
		}
		layer_set_update_proc(quadrant->segments[i], fill_layer);
        layer_add_child(quadrant->layer, (quadrant->segments[i]));
    }

    quadrant->currentSegments = 0b00000000;
}

/* quadrant_number calculates the segments to show for the inputted number. The
    8 segments are expressed in a byte, each bit of which represent a segment */
void quadrant_number(Quadrant *quadrant, bool isBigQuadrant, int number) {
    switch(number) {
        case 0:
            segment_draw(quadrant, isBigQuadrant, 0b00001000);
            break;
        case 1:
            segment_draw(quadrant, isBigQuadrant, 0b00001011);
            break;
        case 2:
            segment_draw(quadrant, isBigQuadrant, 0b10000100);
            break;
        case 3:
            segment_draw(quadrant, isBigQuadrant, 0b10010000);
            break;
        case 4:
            segment_draw(quadrant, isBigQuadrant, 0b01010010);
            break;
        case 5:
            segment_draw(quadrant, isBigQuadrant, 0b00110000);
            break;
        case 6:
            segment_draw(quadrant, isBigQuadrant, 0b00100000);
            break;
        case 7:
            segment_draw(quadrant, isBigQuadrant, 0b10001010);
            break;
        case 8:
            segment_draw(quadrant, isBigQuadrant, 0b00000000);
            break;
        case 9:
            segment_draw(quadrant, isBigQuadrant, 0b00010000);
            break;
    }
}

/****************/
/* MAIN METHODS */
/****************/
/* handle_tick is called at every time change. It gets the hour,
    minute and sends the numbers to each quadrant */
void handle_tick(struct tm *tickE, TimeUnits units_changed) {
	current_time = time(NULL);
    t = localtime(&current_time);
    
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick Happened");
	
	//NOTE: This is a Bit Mask Check not a and &&
	//Secondary Note: tickE->units_changed == 0 catches initialzation tick
  	if (units_changed == 0 || units_changed & DAY_UNIT) {
	  	// Update Seconds Layers
		int mon = t->tm_mday;
	   	quadrant_number(&miniquadrants[0], false, mon/10);
	  	quadrant_number(&miniquadrants[1], false, mon%10);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Day Changed. Now %d", mon);
	
	}
  	if (units_changed == 0 || units_changed & MINUTE_UNIT) {
  		// Update Minute Layers
    	int hour, min;
    	min = t->tm_min;
    	hour = t->tm_hour;

    	if(!clock_is_24h_style()) {
        	hour = hour%12;
        	if(hour == 0) hour=12;
	    }
    
	    quadrant_number(&quadrants[0], true, hour/10);
	    quadrant_number(&quadrants[1], true, hour%10);
	    quadrant_number(&quadrants[2], true, min/10);
	    quadrant_number(&quadrants[3], true, min%10);
	
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Minutes Changed. Now %d", min);

	}
}
/* handle_init is called when the watchface is loaded on screen. It initializes
    various layers for the main design */
void handle_init(void) {
	//Colors
	GColor BackgroundColor = GColorBlack;
	GColor ForegroundColor = GColorWhite;
	
	/* Window inits */
	window = window_create();

	window_stack_push(window, true /* Animated */);
    window_set_background_color(window, BackgroundColor);
    
    /* Cross */
	cross = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(cross, draw_cross);
	layer_add_child(window_get_root_layer(window), cross);
    
    /* Quadrants */
    /* Each quarter of screen is 70x82 pixels */
    quadrant_init(&quadrants[0], GRect(0, 0, 70, 82), true);
    quadrant_init(&quadrants[1], GRect(74, 0, 70, 82), true);
    quadrant_init(&quadrants[2], GRect(0, 86, 70, 82), true);
    quadrant_init(&quadrants[3], GRect(74, 86, 70, 82), true);
    
    layer_add_child(window_get_root_layer(window), quadrants[0].layer);
    layer_add_child(window_get_root_layer(window), quadrants[1].layer);
    layer_add_child(window_get_root_layer(window), quadrants[2].layer);
    layer_add_child(window_get_root_layer(window), quadrants[3].layer);

	/* Mini "Quadrants" */
    /* Each Mini "Quadrant" is 23x27 pixels */
    quadrant_init(&miniquadrants[0], GRect(48, 70, 23, 27), false);
    quadrant_init(&miniquadrants[1], GRect(73, 70, 23, 27), false);
    
    layer_add_child(window_get_root_layer(window), miniquadrants[0].layer);
    layer_add_child(window_get_root_layer(window), miniquadrants[1].layer);
	
	// Subscribe to TickTimerService
  	tick_timer_service_subscribe(DAY_UNIT|HOUR_UNIT|MINUTE_UNIT, handle_tick);
}

static void handle_deinit(void) {
  // Destroy main Window
  window_destroy(window);
}

/* main pebble sets */
int main() {
	handle_init();
  	app_event_loop();
  	handle_deinit();
}
