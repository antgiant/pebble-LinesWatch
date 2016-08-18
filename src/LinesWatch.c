#include <pebble.h>
#include <time.h>

#define ConstantGRect(x, y, w, h) {{(x), (y)}, {(w), (h)}}

/* CONSTANTS
    - AnimationTime is the duration in milliseconds of transitions (2000)
    - Points are the coordinates of the two points at of each quadrant */
//const int AnimationTime = 2000;
//const int MiniAnimationTime = 4000;

const int AnimationTime = 500;
const int MiniAnimationTime = 500;

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
/* Useful Constants*/
int screenWidth, screenHeight, quadrantWidth, quadrantHeight, miniQuadrantWidth, miniQuadrantHeight, thickLine, thinLine;

/* Each quadrant has 8 possible segments, whose coordinates are expressed here.
    There are two coordinates for each segment : 
    - visible is the rectangle when the segment is displayed
    - invisible is the rectangle/line to which the segment retracts when
      disappearing. */
typedef struct {
    GRect visible;
    GRect invisible;
} Segment;
GRect Points[2], MiniPoints[2];
Segment Segments[8], MiniSegments[8];
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
	graphics_fill_rect(ctx, GRect(quadrantWidth, 0, thickLine, quadrantHeight - (miniQuadrantHeight/2)), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(quadrantWidth, quadrantHeight + (miniQuadrantHeight/2) + thinLine*2, thickLine, quadrantHeight - (miniQuadrantHeight/2)), 0, GCornerNone);

	//Main Horizontal Line
	graphics_fill_rect(ctx, GRect(0, quadrantHeight, quadrantWidth - miniQuadrantWidth, thickLine), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(screenWidth - quadrantWidth + miniQuadrantWidth, quadrantHeight, quadrantWidth - miniQuadrantWidth, thickLine), 0, GCornerNone);
	

	//Smaller Vertical Lines
	graphics_fill_rect(ctx, GRect(quadrantWidth - miniQuadrantWidth, quadrantHeight - (miniQuadrantHeight/2) + thinLine, thinLine, miniQuadrantHeight), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(quadrantWidth + thinLine/2, quadrantHeight - (miniQuadrantHeight/2) + thinLine, thinLine, miniQuadrantHeight), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(screenWidth - quadrantWidth + miniQuadrantWidth - thinLine, quadrantHeight - (miniQuadrantHeight/2) + thinLine, thinLine, miniQuadrantHeight), 0, GCornerNone);

	//Smaller Horizontal Lines
	graphics_fill_rect(ctx, GRect(quadrantWidth - miniQuadrantWidth, quadrantHeight - (miniQuadrantHeight/2), miniQuadrantWidth*2 + thickLine, thinLine), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(quadrantWidth - miniQuadrantWidth, quadrantHeight + (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth*2 + thickLine, thinLine), 0, GCornerNone);
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
	if (units_changed == 0 || units_changed & SECOND_UNIT) {
	  	// Update Seconds Layers
		int sec = t->tm_sec;
	   	quadrant_number(&miniquadrants[0], false, sec/10);
	  	quadrant_number(&miniquadrants[1], false, sec%10);
		
		//******DEBUG*******
	    quadrant_number(&quadrants[2], true, sec/10);
	    quadrant_number(&quadrants[3], true, sec%10);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Second Changed. Now %d", sec);
	
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
	BackgroundColor = GColorBlack;
	ForegroundColor = GColorWhite;
	
	/* Window inits */
	window = window_create();

	window_stack_push(window, true /* Animated */);
    window_set_background_color(window, BackgroundColor);
    
    /* Set Constants*/
	GRect bounds = layer_get_bounds(window_get_root_layer(window));
	screenWidth = bounds.size.w;
	screenHeight = bounds.size.h;
	thickLine = (screenHeight*.02) + 1;
	thinLine = (screenHeight*.01) + 1;
	quadrantWidth = (screenWidth-thickLine)/2;
	quadrantHeight = (screenHeight-thickLine)/2;
	miniQuadrantWidth = (screenWidth-thickLine)/6;
	miniQuadrantHeight = (screenHeight-thickLine)/6;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Constants");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "screenWidth = %i", screenWidth);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "screenHeight = %i", screenHeight);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "thickLine = %i", thickLine);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "thinLine = %i", thinLine);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "quadrantWidth = %i", quadrantWidth);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "quadrantHeight = %i", quadrantHeight);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "miniQuadrantWidth = %i", miniQuadrantWidth);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "miniQuadrantHeight = %i", miniQuadrantHeight);
    
	/* Quadrant Animation Constants */
    Points[0] = GRect(quadrantWidth/2 - thickLine/2, (quadrantHeight*.31), thickLine, thickLine);
    Points[1] = GRect(quadrantWidth/2 - thickLine/2, (quadrantHeight*.65), thickLine, thickLine);
    Segments[0].visible = GRect(Points[0].origin.y + thickLine, 0, thickLine, Points[0].origin.y + thickLine);
    Segments[0].invisible = GRect(Points[0].origin.y + thickLine, Points[0].origin.y + thickLine, thickLine, 0);
    Segments[1].visible = GRect(Points[0].origin.x, Points[1].origin.y, thickLine, Points[0].origin.y + thickLine);
    Segments[1].invisible = GRect(Points[0].origin.x, Points[1].origin.y + thickLine, thickLine, 0);
    Segments[2].visible = GRect(Points[0].origin.x, Points[1].origin.y, Points[0].origin.x + thickLine, thickLine);
    Segments[2].invisible = GRect(Points[0].origin.x + thickLine, Points[1].origin.y, 0, thickLine);
    Segments[3].visible = GRect(Points[0].origin.x, Points[0].origin.y, thickLine, Points[1].origin.y - Points[0].origin.y);
    Segments[3].invisible = GRect(Points[0].origin.x, Points[1].origin.y, thickLine, 0);
    Segments[4].visible = GRect(0, Points[1].origin.y, Points[0].origin.x + thickLine, thickLine);
    Segments[4].invisible = GRect(Points[0].origin.x, Points[1].origin.y, 0, thickLine);
    Segments[5].visible = GRect(Points[0].origin.x, Points[0].origin.y, Points[0].origin.x + thickLine, thickLine);
    Segments[5].invisible = GRect(Points[0].origin.x + thickLine, Points[0].origin.y, 0, thickLine);
    Segments[6].visible = GRect(Points[0].origin.x, 0, thickLine, Points[0].origin.y + thickLine);
    Segments[6].invisible = GRect(Points[0].origin.x, Points[0].origin.y, thickLine, 0);
    Segments[7].visible = GRect(0, Points[0].origin.y, Points[0].origin.x + thickLine, thickLine);
    Segments[7].invisible = GRect(Points[0].origin.x, Points[0].origin.y, 0, thickLine);

	/* Mini Quadrant Animation Constants */
	MiniPoints[0] = GRect(miniQuadrantWidth/2 - thinLine/2, (miniQuadrantHeight*.31), thinLine, thinLine);
    MiniPoints[1] = GRect(miniQuadrantWidth/2 - thinLine/2, (miniQuadrantHeight*.65), thinLine, thinLine);
    MiniSegments[0].visible = GRect(MiniPoints[0].origin.y - thinLine, 0, thinLine, MiniPoints[0].origin.y + thinLine);
    MiniSegments[0].invisible = GRect(MiniPoints[0].origin.y - thinLine, MiniPoints[0].origin.y + thinLine, thinLine, 0);
    MiniSegments[1].visible = GRect(MiniPoints[0].origin.x, MiniPoints[1].origin.y, thinLine, MiniPoints[0].origin.y + thinLine);
    MiniSegments[1].invisible = GRect(MiniPoints[0].origin.x, MiniPoints[1].origin.y + thinLine, thinLine, 0);
    MiniSegments[2].visible = GRect(MiniPoints[0].origin.x, MiniPoints[1].origin.y, MiniPoints[0].origin.x + thinLine, thinLine);
    MiniSegments[2].invisible = GRect(MiniPoints[0].origin.x + thinLine, MiniPoints[1].origin.y, 0, thinLine);
    MiniSegments[3].visible = GRect(MiniPoints[0].origin.x, MiniPoints[0].origin.y, thinLine, MiniPoints[1].origin.y - MiniPoints[0].origin.y);
    MiniSegments[3].invisible = GRect(MiniPoints[0].origin.x, MiniPoints[1].origin.y, thinLine, 0);
    MiniSegments[4].visible = GRect(0, MiniPoints[1].origin.y, MiniPoints[0].origin.x + thinLine, thinLine);
    MiniSegments[4].invisible = GRect(MiniPoints[0].origin.x, MiniPoints[1].origin.y, 0, thinLine);
    MiniSegments[5].visible = GRect(MiniPoints[0].origin.x, MiniPoints[0].origin.y, MiniPoints[0].origin.x + thinLine, thinLine);
    MiniSegments[5].invisible = GRect(MiniPoints[0].origin.x + thinLine, MiniPoints[0].origin.y, 0, thinLine);
    MiniSegments[6].visible = GRect(MiniPoints[0].origin.x, 0, thinLine, MiniPoints[0].origin.y + thinLine);
    MiniSegments[6].invisible = GRect(MiniPoints[0].origin.x, MiniPoints[0].origin.y, thinLine, 0);
    MiniSegments[7].visible = GRect(0, MiniPoints[0].origin.y, MiniPoints[0].origin.x + thinLine, thinLine);
    MiniSegments[7].invisible = GRect(MiniPoints[0].origin.x, MiniPoints[0].origin.y, 0, thinLine);
	/*
	MiniPoints[0] = GRect(11, 8, 2, 2);
    MiniPoints[1] = GRect(11, 17, 2, 2);
	MiniSegments[0].visible = GRect(9, 0, 2, 9);
	MiniSegments[0].invisible = GRect(9, 9, 2, 0);
    MiniSegments[1].visible = GRect(11, 17, 2, 9);
    MiniSegments[1].invisible = GRect(11, 19, 2, 0);
    MiniSegments[2].visible = GRect(11, 17, 12, 2);
    MiniSegments[2].invisible = GRect(12, 17, 0, 2);
    MiniSegments[3].visible = GRect(11, 8, 2, 10);
    MiniSegments[3].invisible = GRect(11, 17, 2, 0);
    MiniSegments[4].visible = GRect(0, 17, 12, 2);
    MiniSegments[4].invisible = GRect(11, 17, 0, 2);
    MiniSegments[5].visible = GRect(11, 8, 12, 2);
    MiniSegments[5].invisible = GRect(12, 8, 0, 2);
    MiniSegments[6].visible = GRect(11, 0, 2, 9);
    MiniSegments[6].invisible = GRect(11, 8, 2, 0);
    MiniSegments[7].visible = GRect(0, 8, 12, 2);
    MiniSegments[7].invisible = GRect(11, 8, 0, 2);
	*/
	
    /* Cross */
	cross = layer_create(bounds);
	layer_set_update_proc(cross, draw_cross);
	layer_add_child(window_get_root_layer(window), cross);

    /* Quadrants */
	quadrant_init(&quadrants[0], GRect(0, 0, quadrantWidth, quadrantHeight), true);
    quadrant_init(&quadrants[1], GRect(quadrantWidth + thickLine, 0, quadrantWidth, quadrantHeight), true);
    quadrant_init(&quadrants[2], GRect(0, quadrantHeight + thickLine, quadrantWidth, quadrantHeight), true);
    quadrant_init(&quadrants[3], GRect(quadrantWidth + thickLine, quadrantHeight + thickLine, quadrantWidth, quadrantHeight), true);
    
    layer_add_child(window_get_root_layer(window), quadrants[0].layer);
    layer_add_child(window_get_root_layer(window), quadrants[1].layer);
    layer_add_child(window_get_root_layer(window), quadrants[2].layer);
    layer_add_child(window_get_root_layer(window), quadrants[3].layer);

	/* Mini "Quadrants" */
    /* Each Mini "Quadrant" is 23x27 pixels */
    quadrant_init(&miniquadrants[0], GRect(quadrantWidth - miniQuadrantWidth + thinLine, quadrantHeight - (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth, miniQuadrantHeight), false);
    quadrant_init(&miniquadrants[1], GRect(quadrantWidth + thickLine - (thinLine/2), quadrantHeight - (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth, miniQuadrantHeight), false);
    
    layer_add_child(window_get_root_layer(window), miniquadrants[0].layer);
    layer_add_child(window_get_root_layer(window), miniquadrants[1].layer);
	
	// Subscribe to TickTimerService
  	tick_timer_service_subscribe(DAY_UNIT|HOUR_UNIT|MINUTE_UNIT|SECOND_UNIT, handle_tick);
}

static void handle_deinit(void) {
  	// Stop any animation in progress
  	animation_unschedule_all();

	// Destroy main Window
  	window_destroy(window);
}

/* main pebble sets */
int main() {
	handle_init();
  	app_event_loop();
  	handle_deinit();
}
