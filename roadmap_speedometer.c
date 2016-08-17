/* roadmap_speedometer.c
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi Ben-Shoshan
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_gps.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_config.h"
#include "roadmap_res.h"
#include "roadmap_screen.h"
#include "roadmap_bar.h"
#include "roadmap_map_settings.h"
#include "roadmap_skin.h"
#include "ssd/ssd_widget.h"


#define SPEEDOMETER_SPEED_COLOR_DAY                   ("#ffffff")
#define SPEEDOMETER_SPEED_COLOR_NIGHT                 ("#ffffff")//("#d7ff00")


static RoadMapScreenSubscriber roadmap_speedometer_prev_after_refresh = NULL;
static RoadMapImage SpeedometerImage;
static int gOffset = 0;
static BOOL gHideSpeedometer = FALSE;

/////////////////////////////////////////////////////////////////////
void roadmap_speedometer_set_offset(int offset_y){
   gOffset = offset_y;
}


/////////////////////////////////////////////////////////////////////
static void after_refresh_callback (void){
   if (roadmap_speedometer_prev_after_refresh) {
      (*roadmap_speedometer_prev_after_refresh) ();
   }
}

/////////////////////////////////////////////////////////////////////
static void roadmap_speedometer_after_refresh (void){
   RoadMapGuiPoint image_position;
   RoadMapGuiPoint text_position;
   RoadMapGuiPoint units_position;
   RoadMapGpsPosition pos;
   RoadMapPen speedometer_pen;
   char str[30];
   char unit_str[30];
   int font_size = 16;
   int font_size_units = 6;
   int speed_offset = 6;
   int units_offset = 6;
   int speed;
   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;
   int text_offset;
#ifdef IPHONE_NATIVE
	font_size = 18;
	font_size_units = 8;
#else
   if ( roadmap_lang_rtl() )
      font_size_units--;     // Longer text for units
#endif


   if (SpeedometerImage == NULL){
      return;
   }

   if (gHideSpeedometer){
         after_refresh_callback();
         return;
   }

   if (!roadmap_map_settings_isShowSpeedometer()){
      after_refresh_callback();
      return;
   }

   if (roadmap_screen_is_hd_screen()){
      speed_offset *= 1.5;
      units_offset *= 1.5;
   }
   roadmap_navigate_get_current (&pos, NULL, NULL);
   speed = pos.speed;
#if DEBUG
	speed = rand() % 120;

#else
   if ((speed == -1) || !roadmap_gps_have_reception()){
      after_refresh_callback();
      return;
   }
#endif

   speedometer_pen = roadmap_canvas_create_pen ("speedometer_pen");
   if (roadmap_skin_state() == 1)
      roadmap_canvas_set_foreground(SPEEDOMETER_SPEED_COLOR_NIGHT);
   else
      roadmap_canvas_set_foreground(SPEEDOMETER_SPEED_COLOR_DAY);

   image_position.x = 15;//roadmap_canvas_width() - roadmap_canvas_image_width(SpeedometerImage);
   image_position.y = roadmap_canvas_height() - roadmap_canvas_image_height(SpeedometerImage) - roadmap_bar_bottom_height();// - gOffset;
   roadmap_canvas_draw_image (SpeedometerImage, &image_position,  0, IMAGE_NORMAL);

   text_position.y = image_position.y + roadmap_canvas_image_height(SpeedometerImage) *.6;
   units_position.y = image_position.y + roadmap_canvas_image_height(SpeedometerImage)*.75;

   if (speed != -1){
	   if (speed <10)
		text_offset = -6;
	   else if (speed <100)
		text_offset = -3;
	   else
		text_offset = -1;
      if (!roadmap_gps_is_show_raw()) {
         snprintf (str, sizeof(str), "%3d", roadmap_math_to_speed_unit(speed));
         snprintf (unit_str, sizeof(unit_str), "%s",  roadmap_lang_get(roadmap_math_speed_unit()));
      } else {
         snprintf (str, sizeof(str), "%3d", pos.accuracy);
         snprintf (unit_str, sizeof(unit_str), "%s",  "ac");
      }


   roadmap_canvas_get_text_extents
         (str, font_size, &text_width, &text_ascent, &text_descent, NULL);

      if (ssd_widget_rtl(NULL)){
         text_position.x = image_position.x + (roadmap_canvas_image_width(SpeedometerImage)/2) - (text_width/2) + text_offset;
		 roadmap_canvas_draw_string_size(&text_position, ROADMAP_CANVAS_BOTTOMLEFT, font_size, str);

		 roadmap_canvas_get_text_extents
         (unit_str, font_size_units, &text_width, &text_ascent, &text_descent, NULL);

		 units_position.x = image_position.x + (roadmap_canvas_image_width(SpeedometerImage)/2) - (text_width/2)-2;
         roadmap_canvas_draw_string_size(&units_position, ROADMAP_CANVAS_BOTTOMLEFT, font_size_units, unit_str);
      }
      else{
         text_position.x = image_position.x + (roadmap_canvas_image_width(SpeedometerImage)/2)- (text_width/2) + text_offset;
         roadmap_canvas_draw_string_size(&text_position, ROADMAP_CANVAS_BOTTOMMIDDLE, font_size, str);

		 roadmap_canvas_get_text_extents
         (unit_str, font_size_units, &text_width, &text_ascent, &text_descent, NULL);

         units_position.x = image_position.x + (roadmap_canvas_image_width(SpeedometerImage)/2)- (text_width/2)-2 ;
         roadmap_canvas_draw_string_size(&units_position, ROADMAP_CANVAS_BOTTOMMIDDLE, font_size_units, unit_str);
      }
   }

   after_refresh_callback();
}


/////////////////////////////////////////////////////////////////////
void roadmap_speedometer_hide(void){
   gHideSpeedometer = TRUE;
}

/////////////////////////////////////////////////////////////////////
void roadmap_speedometer_show(void){
   gHideSpeedometer = FALSE;
}

/////////////////////////////////////////////////////////////////////
void roadmap_speedometer_initialize(void){

   SpeedometerImage =  (RoadMapImage) roadmap_res_get (RES_BITMAP, RES_SKIN|RES_NOCACHE, "speedometer");
   if (SpeedometerImage == NULL){
      roadmap_log (ROADMAP_ERROR, "Can't find speedometer resource");
      return;
   }

   roadmap_speedometer_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (roadmap_speedometer_after_refresh);
}
