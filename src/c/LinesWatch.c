#include <pebble.h>
#include <time.h>

//Last number is the max # of integers to be passed
#define INBOX_SIZE  (1 + (7+4) * 6)
#define OUTBOX_SIZE (1 + (7+4) * 6)

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
int screenWidth, screenHeight, quadrantWidth, quadrantHeight, miniQuadrantWidth, miniQuadrantHeight, thickLine, thinLine, AnimationTime, MiniAnimationTime;

/* Each quadrant has 8 possible segments, whose coordinates are expressed here.
    There are two coordinates for each segment : 
    - visible is the rectangle when the segment is displayed
    - invisible is the rectangle/line to which the segment retracts when
      disappearing. */
typedef struct {
    GRect visible;
    GRect invisible;
} Segment;
Segment Segments[8], MiniSegments[8];
/* Other globals */
Window *window;
Layer *cross;
GRect Points[2], MiniPoints[2];
time_t current_time;
struct tm *t;
GColor BackgroundColor, ForegroundColor;
enum WatchStyles {
	ORIGINAL,
	ORIGINAL_DATE,
	ORIGINAL_SECONDS,
};
int watch_style = ORIGINAL_DATE;
bool reset_timers = false;
bool first_run = true;

/*******************/
/* GENERAL PURPOSE */
/*******************/
GRect set_screen_size() {
	/* Set Global Screen Size "Constants" */
	GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(window));
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
    Segments[0].visible = GRect(Points[0].origin.x - thickLine, 0, thickLine, Points[0].origin.y + thickLine);
    Segments[0].invisible = GRect(Points[0].origin.x - thickLine, Points[0].origin.y + thickLine, thickLine, 0);
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
    MiniSegments[0].visible = GRect(MiniPoints[0].origin.x - thinLine, 0, thinLine, MiniPoints[0].origin.y + thinLine);
    MiniSegments[0].invisible = GRect(MiniPoints[0].origin.x - thinLine, MiniPoints[0].origin.y + thinLine, thinLine, 0);
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

	return bounds;
}

//Function declaration so that next function can use it.
void handle_tick(struct tm *tickE, TimeUnits units_changed);
void quadrant_init(Quadrant *quadrant, GRect coordinates, bool isBigQuadrant);
void draw_cross(Layer *layer, GContext *ctx);

void set_watch_style(){
	switch(watch_style) {
		case ORIGINAL:
			if (!first_run) {
				layer_set_hidden(miniquadrants[0].layer, true);
				layer_set_hidden(miniquadrants[1].layer, true);
			}
			tick_timer_service_unsubscribe();
			tick_timer_service_subscribe(HOUR_UNIT|MINUTE_UNIT, handle_tick);
			break;
		case ORIGINAL_DATE:
			if (!first_run) {
				layer_set_hidden(miniquadrants[0].layer, false);
				layer_set_hidden(miniquadrants[1].layer, false);
			}
			tick_timer_service_unsubscribe();
			tick_timer_service_subscribe(DAY_UNIT|HOUR_UNIT|MINUTE_UNIT, handle_tick);
			break;
		case ORIGINAL_SECONDS:
			if (!first_run) {
				layer_set_hidden(miniquadrants[0].layer, false);
				layer_set_hidden(miniquadrants[1].layer, false);
			}
			tick_timer_service_unsubscribe();
			tick_timer_service_subscribe(HOUR_UNIT|MINUTE_UNIT|SECOND_UNIT, handle_tick);
			break;
	}
	
	if (first_run) {
		first_run = false;
	}

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Changed watch style. (%i)", watch_style);
}

static void screen_size_changed(void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "----Begin Screen Size Change----");
  	// Stop any animation in progress to prevent race conditions
  	animation_unschedule_all();
	
	//Update Screen Size
	GRect bounds = set_screen_size();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Screen Sizes Updated");

	//Wipe record of currently displayed segments to force redraw
	quadrants[0].currentSegments = 0;
	quadrants[1].currentSegments = 0;
	quadrants[2].currentSegments = 0;
	quadrants[3].currentSegments = 0;
	miniquadrants[0].currentSegments = 0;
	miniquadrants[1].currentSegments = 0;

	//Mark background cross as needing to be redrawn
	layer_set_frame(cross, bounds);
	layer_mark_dirty(cross);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Update Background 'Cross'");
	
	//Redrawn center points in the correct spot
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			layer_set_frame(quadrants[i].points[j], Points[j]);
		}
	}
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			layer_set_frame(miniquadrants[i].points[j], MiniPoints[j]);
		}
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Center Points Moved");

    /* Update all the layer frames */
	layer_set_frame(quadrants[0].layer, GRect(0, 0, quadrantWidth, quadrantHeight));
    layer_set_frame(quadrants[1].layer, GRect(quadrantWidth + thickLine, 0, quadrantWidth, quadrantHeight));
    layer_set_frame(quadrants[2].layer, GRect(0, quadrantHeight + thickLine, quadrantWidth, quadrantHeight));
    layer_set_frame(quadrants[3].layer, GRect(quadrantWidth + thickLine, quadrantHeight + thickLine, quadrantWidth, quadrantHeight));
    layer_set_frame(miniquadrants[0].layer, GRect(quadrantWidth - miniQuadrantWidth + thinLine, quadrantHeight - (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth, miniQuadrantHeight));
    layer_set_frame(miniquadrants[1].layer, GRect(quadrantWidth + thickLine - (thinLine/2), quadrantHeight - (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth, miniQuadrantHeight));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Quadrants Moved");

	//Force a screen refresh
	time_t now = time(NULL);
	struct tm *tick_time = localtime(&now);
	TimeUnits units_changed = 0;
	handle_tick(tick_time, units_changed);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "----End Screen Size Change----");
}
void handle_appmessage_receive(DictionaryIterator *iter, void *context) {
	// Read watch style preference
	Tuple *tuple = dict_find(iter, MESSAGE_KEY_watch_style);
	if(tuple) {
		watch_style = atoi(tuple->value->cstring);
		persist_write_int(MESSAGE_KEY_watch_style, atoi(tuple->value->cstring));
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved new Watch Style (%i).", watch_style);
		set_watch_style();
	} else if (persist_exists(MESSAGE_KEY_watch_style)) {
		watch_style = persist_read_int(MESSAGE_KEY_watch_style); 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded watch style from watch storage. (%i)", watch_style);
		set_watch_style();
	} else {
		watch_style = 1; //Default to Original plus Date 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded default watch style. (No saved settings).");
	}
	
	// Read Background Color preference
	tuple = dict_find(iter, MESSAGE_KEY_background_color);
	if(tuple) {
		BackgroundColor = GColorFromHEX(tuple->value->int32);
		persist_write_int(MESSAGE_KEY_background_color, tuple->value->int32);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved new Background Color (%X).", BackgroundColor.argb);
		window_set_background_color(window, BackgroundColor);
	} else if (persist_exists(MESSAGE_KEY_background_color)) {
		BackgroundColor = GColorFromHEX(persist_read_int(MESSAGE_KEY_background_color)); 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded background color from watch storage., (%X)", BackgroundColor.argb);
	} else {
		BackgroundColor = GColorBlack; //Default to black background color 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded default watch background color. (No saved settings).");
	}
		
	// Read Foreground Color preference
	tuple = dict_find(iter, MESSAGE_KEY_foreground_color);
	if(tuple) {
		ForegroundColor = GColorFromHEX(tuple->value->int32);
		persist_write_int(MESSAGE_KEY_foreground_color, tuple->value->int32);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved new Foreground Color (%X).", ForegroundColor.argb);
		layer_mark_dirty(window_get_root_layer(window));
	} else if (persist_exists(MESSAGE_KEY_foreground_color)) {
		ForegroundColor = GColorFromHEX(persist_read_int(MESSAGE_KEY_foreground_color)); 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded foreground color from watch storage., (%X)", ForegroundColor.argb);
	} else {
		ForegroundColor = GColorWhite; //Default to white foreground color 
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded default watch foreground color. (No saved settings).");
	}	
	
	// Read big number animation speed preference
	tuple = dict_find(iter, MESSAGE_KEY_big_animation_speed);
	if(tuple) {
		AnimationTime = tuple->value->int32 * 500; //Slider moves in half second increments
		persist_write_int(MESSAGE_KEY_big_animation_speed, AnimationTime);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved new big number animation speed (%i).", AnimationTime);
	} else if (persist_exists(MESSAGE_KEY_big_animation_speed)) {
		AnimationTime = persist_read_int(MESSAGE_KEY_big_animation_speed);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded big number animation speed from watch storage. (%i)", AnimationTime);
	} else {
		AnimationTime = 2000; //Default to 2 second animation speed
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded default big number animation speed (%i). (No saved settings).", AnimationTime);
	}
	
	int temp;
	// Read date animation speed preference
	tuple = dict_find(iter, MESSAGE_KEY_date_animation_speed);
	if(tuple) {
		temp = tuple->value->int32 * 500; //Slider moves in half second increments
		if(watch_style == ORIGINAL_DATE) {
			MiniAnimationTime = temp;
		}
		persist_write_int(MESSAGE_KEY_date_animation_speed, temp);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved new date animation speed (%i).", temp);
	} else if (watch_style == ORIGINAL_DATE && persist_exists(MESSAGE_KEY_date_animation_speed)) {
		MiniAnimationTime = persist_read_int(MESSAGE_KEY_date_animation_speed);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded date animation speed from watch storage. (%i)", MiniAnimationTime);
	} else if (watch_style == ORIGINAL_DATE) {
		MiniAnimationTime = 4000; //Default to 4 second animation speed
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded seconds animation speed (%i). (No saved settings).", MiniAnimationTime);
	}
	
	// Read seconds animation speed preference
	tuple = dict_find(iter, MESSAGE_KEY_seconds_animation_speed);
	if(tuple) {
		temp = tuple->value->int32 * 100; //Slider moves in tenth of a second increments
		if(watch_style == ORIGINAL_SECONDS) {
			MiniAnimationTime = temp;
		}
		persist_write_int(MESSAGE_KEY_seconds_animation_speed, temp);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved new seconds animation speed (%i).", temp);
	} else if (watch_style == ORIGINAL_SECONDS && persist_exists(MESSAGE_KEY_seconds_animation_speed)) {
		MiniAnimationTime = persist_read_int(MESSAGE_KEY_seconds_animation_speed);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded seconds animation speed from watch storage. (%i)", MiniAnimationTime);
	} else if (watch_style == ORIGINAL_SECONDS) {
		MiniAnimationTime = 500; //Default to half a second
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded default seconds animation speed (%i). (No saved settings).", MiniAnimationTime);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Completed processing config data");
}

void load_saved_config_options() {
	int current_version = 2;
	int saved_version;
	if (persist_exists(MESSAGE_KEY_config_version)) {
		saved_version = persist_read_int(MESSAGE_KEY_config_version);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded Config Version (%i)", saved_version);	
	} else {
		saved_version = current_version;
		persist_write_int(MESSAGE_KEY_config_version, saved_version);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved config version to persistent storage.");
	}
	if (saved_version < current_version) {
		//Deal with old data
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Upgrading Config data");	
		
		persist_write_int(MESSAGE_KEY_config_version, current_version);
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Upgrade of Config data complete");
	}
	else if (saved_version > current_version) {
		//Use default vaules as the data is from the future
		watch_style = ORIGINAL_DATE; //Default to Original plus Date 
		BackgroundColor = GColorBlack; //Default to black background color 
		ForegroundColor = GColorWhite; 
		AnimationTime = 2000;
		MiniAnimationTime = 4000;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Using defaults as config data is from the future");	
	}
	
	//Config is now up to date use handle_appmessage_receive to load the default values
	DictionaryIterator iter;
	uint8_t buffer[INBOX_SIZE];
	dict_write_begin(&iter, buffer, sizeof(buffer));
	void *context = NULL;
	handle_appmessage_receive(&iter, context);
	
}

/* fill_layer ensures that a layer is always filled with the ForegroundColor */
void fill_layer(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, ForegroundColor);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

/* draw_cross draws the main cross (horizontal and vertical lines accross
    the screen */
void draw_cross(Layer *layer, GContext *ctx) {
	graphics_context_set_fill_color(ctx, ForegroundColor);

	switch (watch_style) {
		case ORIGINAL:
			//Main Vertical Line
			graphics_fill_rect(ctx, GRect(quadrantWidth, 0, thickLine, screenHeight), 0, GCornerNone);

			//Main Horizontal Line
			graphics_fill_rect(ctx, GRect(0, quadrantHeight, screenWidth, thickLine), 0, GCornerNone);
			break;
		case ORIGINAL_DATE:
		case ORIGINAL_SECONDS:
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
			break;
	}
}

/************/
/* SEGMENTS */
/************/
/* segment_show draws a segment with an animation */
void segment_show(Quadrant *quadrant, bool isBigQuadrant, int id) {
	GRect visible, invisible;
	int speed;
	
	if (isBigQuadrant) {
    	visible = Segments[id].visible;
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
/* handle_tick is called at every time change. It gets the values
    to the correct quadrants */
void handle_tick(struct tm *tickE, TimeUnits units_changed) {
	current_time = time(NULL);
    t = localtime(&current_time);
    
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick Happened");
	
	//NOTE: This is a Bit Mask Check not a and &&
	//Secondary Note: tickE->units_changed == 0 catches initialzation tick
  	if (watch_style == ORIGINAL_DATE && (units_changed == 0 || units_changed & DAY_UNIT)) {
	  	// Update Mini Quadrant Layers
		int day = t->tm_mday;
	   	quadrant_number(&miniquadrants[0], false, day/10);
	  	quadrant_number(&miniquadrants[1], false, day%10);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "Day Changed. Now %d", day);
	}
  	if (units_changed == 0 || units_changed & HOUR_UNIT) {
  		// Update Quadrant Layers
    	int hour;
    	hour = t->tm_hour;

    	if(!clock_is_24h_style()) {
        	hour = hour%12;
        	if(hour == 0) hour=12;
	    }
    
	    quadrant_number(&quadrants[0], true, hour/10);
	    quadrant_number(&quadrants[1], true, hour%10);
	
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Hour Changed. Now %d", hour);
	}
  	if (units_changed == 0 || units_changed & MINUTE_UNIT) {
  		// Update Quadrants Layers
    	int min;
    	min = t->tm_min;

		quadrant_number(&quadrants[2], true, min/10);
	    quadrant_number(&quadrants[3], true, min%10);
	
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Minutes Changed. Now %d", min);
	}
  	if (watch_style == ORIGINAL_SECONDS && (units_changed == 0 || units_changed & SECOND_UNIT)) {
	  	// Update Mini Quadrant Layers
		int sec;
    	sec = t->tm_sec;

	   	quadrant_number(&miniquadrants[0], false, sec/10);
	  	quadrant_number(&miniquadrants[1], false, sec%10);
	
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Seconds Changed. Now %d", sec);
	}
}

/* handle_init is called when the watchface is loaded on screen. It initializes
    various layers for the main design */
void handle_init(void) {
	load_saved_config_options();
	app_message_register_inbox_received(&handle_appmessage_receive);
	app_message_open(INBOX_SIZE, OUTBOX_SIZE);

	/* Window inits */
	window = window_create();

	window_stack_push(window, true /* Animated */);
    window_set_background_color(window, BackgroundColor);
    
	//Set Screen Size & Get Notified of Changes to it
	GRect bounds = set_screen_size();
	 UnobstructedAreaHandlers handlers = {
    	.did_change = screen_size_changed
  	};
  	unobstructed_area_service_subscribe(handlers, NULL);
		
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
    quadrant_init(&miniquadrants[0], GRect(quadrantWidth - miniQuadrantWidth + thinLine, quadrantHeight - (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth, miniQuadrantHeight), false);
    quadrant_init(&miniquadrants[1], GRect(quadrantWidth + thickLine - (thinLine/2), quadrantHeight - (miniQuadrantHeight/2) + thinLine, miniQuadrantWidth, miniQuadrantHeight), false);
    
    layer_add_child(window_get_root_layer(window), miniquadrants[0].layer);
    layer_add_child(window_get_root_layer(window), miniquadrants[1].layer);
	
	// Setup timer triggers
	switch(watch_style) {
		case ORIGINAL:
			layer_set_hidden(miniquadrants[0].layer, true);
			layer_set_hidden(miniquadrants[1].layer, true);
			tick_timer_service_subscribe(HOUR_UNIT|MINUTE_UNIT, handle_tick);
			break;
		case ORIGINAL_DATE:
			tick_timer_service_subscribe(DAY_UNIT|HOUR_UNIT|MINUTE_UNIT, handle_tick);
			break;
		case ORIGINAL_SECONDS:
			tick_timer_service_subscribe(HOUR_UNIT|MINUTE_UNIT|SECOND_UNIT, handle_tick);
			break;
	}
}

static void handle_deinit(void) {
  	// Stop any animation in progress
  	animation_unschedule_all();

	//Lots of memory freeing
	for (int i = 0; i < 4; i++) {
		layer_destroy(quadrants[i].layer);
		for (int j = 0; j < 2; j++) {
			layer_destroy(quadrants[i].points[j]);
		}
		for (int j = 0; j < 8; j++) {
			layer_destroy(quadrants[i].segments[j]);
		}
		for (int j = 0; j < 8; j++) {
			property_animation_destroy(quadrants[i].animations[j]);
		}
	}
	for (int i = 0; i < 2; i++) {
		layer_destroy(miniquadrants[i].layer);
		for (int j = 0; j < 2; j++) {
			layer_destroy(miniquadrants[i].points[j]);
		}
		for (int j = 0; j < 8; j++) {
			layer_destroy(miniquadrants[i].segments[j]);
		}
		for (int j = 0; j < 8; j++) {
			property_animation_destroy(miniquadrants[i].animations[j]);
		}
	}	

	// Destroy cross Layer
  	layer_destroy(cross);
	
	// Destroy main Window
  	window_destroy(window);
}

/* main pebble sets */
int main() {
	handle_init();
  	app_event_loop();
  	handle_deinit();
}
