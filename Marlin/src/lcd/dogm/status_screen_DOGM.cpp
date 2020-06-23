/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//
// status_screen_DOGM.cpp
// Standard Status Screen for Graphical Display
//

#include "../../inc/MarlinConfigPre.h"

#if HAS_GRAPHICAL_LCD && DISABLED(LIGHTWEIGHT_UI)

#include <math.h>
//#include "..\..\HAL\LINUX\hardware\Clock.h"

#include "dogm_Statusscreen.h"
#include "ultralcd_DOGM.h"
#include "../ultralcd.h"
#include "../lcdprint.h"
#include "../../libs/numtostr.h"

#include "../../module/motion.h"
#include "../../module/temperature.h"

#if ENABLED(FILAMENT_LCD_DISPLAY)
  #include "../../feature/filwidth.h"
  #include "../../module/planner.h"
  #include "../../gcode/parser.h"
#endif

#if HAS_CUTTER
  #include "../../feature/spindle_laser.h"
#endif

#if ENABLED(SDSUPPORT)
  #include "../../sd/cardreader.h"
#endif

#if HAS_PRINT_PROGRESS
  #include "../../module/printcounter.h"
#endif

#if DUAL_MIXING_EXTRUDER
  #include "../../feature/mixing.h"
#endif

#define PI               3.14159
#define X_LABEL_POS      3
#define X_VALUE_POS     11
#define XYZ_SPACING     37
#define XYZ_BASELINE    (30 + INFO_FONT_ASCENT)
#define EXTRAS_BASELINE (40 + INFO_FONT_ASCENT)
#define STATUS_BASELINE (LCD_PIXEL_HEIGHT - INFO_FONT_DESCENT)

#if ANIM_HBCC
  enum HeatBits : uint8_t {
    HEATBIT_HOTEND,
    HEATBIT_BED = HOTENDS,
    HEATBIT_CHAMBER,
    HEATBIT_CUTTER
  };
  IF<(HEATBIT_CUTTER > 7), uint16_t, uint8_t>::type heat_bits;
#endif

#if ANIM_HOTEND
  #define HOTEND_ALT(N) TEST(heat_bits, HEATBIT_HOTEND + N)
#else
  #define HOTEND_ALT(N) false
#endif
#if ANIM_BED
  #define BED_ALT() TEST(heat_bits, HEATBIT_BED)
#else
  #define BED_ALT() false
#endif
#if ANIM_CHAMBER
  #define CHAMBER_ALT() TEST(heat_bits, HEATBIT_CHAMBER)
#else
  #define CHAMBER_ALT() false
#endif
#if ANIM_CUTTER
  #define CUTTER_ALT(N) TEST(heat_bits, HEATBIT_CUTTER)
#else
  #define CUTTER_ALT() false
#endif

#if DO_DRAW_HOTENDS
  #define MAX_HOTEND_DRAW _MIN(HOTENDS, ((LCD_PIXEL_WIDTH - (STATUS_LOGO_BYTEWIDTH + STATUS_FAN_BYTEWIDTH) * 8) / (STATUS_HEATERS_XSPACE)))
  #define STATUS_HEATERS_BOT (STATUS_HEATERS_Y + STATUS_HEATERS_HEIGHT - 1)
#endif

#define PROGRESS_BAR_X 54
#define PROGRESS_BAR_WIDTH (LCD_PIXEL_WIDTH - PROGRESS_BAR_X)

FORCE_INLINE void _draw_centered_temp(const int16_t temp, const uint8_t tx, const uint8_t ty) {
  const char *str = i16tostr3rj(temp);
  const uint8_t len = str[0] != ' ' ? 3 : str[1] != ' ' ? 2 : 1;
  lcd_put_u8str(tx - len * (INFO_FONT_WIDTH) / 2 + 1, ty, &str[3-len]);
  lcd_put_wchar(LCD_STR_DEGREE[0]);
}

#if DO_DRAW_HOTENDS

  // Draw hotend bitmap with current and target temperatures
  FORCE_INLINE void _draw_hotend_status(const heater_ind_t heater, const bool blink) {
    #if !HEATER_IDLE_HANDLER
      UNUSED(blink);
    #endif

    const bool isHeat = HOTEND_ALT(heater);

    const uint8_t tx = STATUS_HOTEND_TEXT_X(heater);

    const float temp = thermalManager.degHotend(heater),
              target = thermalManager.degTargetHotend(heater);

    #if DISABLED(STATUS_HOTEND_ANIM)
      #define STATIC_HOTEND true
      #define HOTEND_DOT    isHeat
    #else
      #define STATIC_HOTEND false
      #define HOTEND_DOT    false
    #endif

    #if ANIM_HOTEND && BOTH(STATUS_HOTEND_INVERTED, STATUS_HOTEND_NUMBERLESS)
      #define OFF_BMP(N) status_hotend_b_bmp
      #define ON_BMP(N)  status_hotend_a_bmp
    #elif ANIM_HOTEND && DISABLED(STATUS_HOTEND_INVERTED) && ENABLED(STATUS_HOTEND_NUMBERLESS)
      #define OFF_BMP(N) status_hotend_a_bmp
      #define ON_BMP(N)  status_hotend_b_bmp
    #elif ANIM_HOTEND && ENABLED(STATUS_HOTEND_INVERTED)
      #define OFF_BMP(N) status_hotend##N##_b_bmp
      #define ON_BMP(N)  status_hotend##N##_a_bmp
    #else
      #define OFF_BMP(N) status_hotend##N##_a_bmp
      #define ON_BMP(N)  status_hotend##N##_b_bmp
    #endif

    #if STATUS_HOTEND_BITMAPS > 1
      static const unsigned char* const status_hotend_gfx[STATUS_HOTEND_BITMAPS] PROGMEM = ARRAY_N(STATUS_HOTEND_BITMAPS, OFF_BMP(1), OFF_BMP(2), OFF_BMP(3), OFF_BMP(4), OFF_BMP(5), OFF_BMP(6));
      #if ANIM_HOTEND
        static const unsigned char* const status_hotend_on_gfx[STATUS_HOTEND_BITMAPS] PROGMEM = ARRAY_N(STATUS_HOTEND_BITMAPS, ON_BMP(1), ON_BMP(2), ON_BMP(3), ON_BMP(4), ON_BMP(5), ON_BMP(6));
        #define HOTEND_BITMAP(N,S) (unsigned char*)pgm_read_ptr((S) ? &status_hotend_on_gfx[(N) % (STATUS_HOTEND_BITMAPS)] : &status_hotend_gfx[(N) % (STATUS_HOTEND_BITMAPS)])
      #else
        #define HOTEND_BITMAP(N,S) (unsigned char*)pgm_read_ptr(&status_hotend_gfx[(N) % (STATUS_HOTEND_BITMAPS)])
      #endif
    #elif ANIM_HOTEND
      #define HOTEND_BITMAP(N,S) ((S) ? ON_BMP() : OFF_BMP())
    #else
      #define HOTEND_BITMAP(N,S) status_hotend_a_bmp
    #endif

    if (PAGE_CONTAINS(STATUS_HEATERS_Y, STATUS_HEATERS_BOT)) {

      #define BAR_TALL (STATUS_HEATERS_HEIGHT - 2)

      const float prop = target - 20,
                  perc = prop > 0 && temp >= 20 ? (temp - 20) / prop : 0;
      uint8_t tall = uint8_t(perc * BAR_TALL + 0.5f);
      NOMORE(tall, BAR_TALL);

      #if ANIM_HOTEND
        // Draw hotend bitmap, either whole or split by the heating percent
        const uint8_t hx = STATUS_HOTEND_X(heater),
                      bw = STATUS_HOTEND_BYTEWIDTH(heater);
        #if ENABLED(STATUS_HEAT_PERCENT)
          if (isHeat && tall <= BAR_TALL) {
            const uint8_t ph = STATUS_HEATERS_HEIGHT - 1 - tall;
            u8g.drawBitmapP(hx, STATUS_HEATERS_Y, bw, ph, HOTEND_BITMAP(heater, false));
            u8g.drawBitmapP(hx, STATUS_HEATERS_Y + ph, bw, tall + 1, HOTEND_BITMAP(heater, true) + ph * bw);
          }
          else
        #endif
            u8g.drawBitmapP(hx, STATUS_HEATERS_Y, bw, STATUS_HEATERS_HEIGHT, HOTEND_BITMAP(heater, isHeat));
      #endif

    } // PAGE_CONTAINS

    if (PAGE_UNDER(7)) {
      #if HEATER_IDLE_HANDLER
        const bool dodraw = (blink || !thermalManager.hotend_idle[heater].timed_out);
      #else
        constexpr bool dodraw = true;
      #endif
      if (dodraw) _draw_centered_temp(target + 0.5, tx, 7);
    }

    if (PAGE_CONTAINS(28 - INFO_FONT_ASCENT, 28 - 1))
      _draw_centered_temp(temp + 0.5f, tx, 28);

    if (STATIC_HOTEND && HOTEND_DOT && PAGE_CONTAINS(17, 19)) {
      u8g.setColorIndex(0); // set to white on black
      u8g.drawBox(tx, 20 - 3, 2, 2);
      u8g.setColorIndex(1); // restore black on white
    }

  }

#endif // DO_DRAW_HOTENDS

#if DO_DRAW_BED

  // Draw bed bitmap with current and target temperatures
  FORCE_INLINE void _draw_bed_status(const bool blink) {
    #if !HEATER_IDLE_HANDLER
      UNUSED(blink);
    #endif

    const uint8_t tx = STATUS_BED_TEXT_X;

    const float temp = thermalManager.degBed(),
              target = thermalManager.degTargetBed();

    #if ENABLED(STATUS_HEAT_PERCENT) || DISABLED(STATUS_BED_ANIM)
      const bool isHeat = BED_ALT();
    #endif

    #if DISABLED(STATUS_BED_ANIM)
      #define STATIC_BED    true
      #define BED_DOT       isHeat
    #else
      #define STATIC_BED    false
      #define BED_DOT       false
    #endif

    if (PAGE_CONTAINS(STATUS_HEATERS_Y, STATUS_HEATERS_BOT)) {

      #define BAR_TALL (STATUS_HEATERS_HEIGHT - 2)

      const float prop = target - 20,
                  perc = prop > 0 && temp >= 20 ? (temp - 20) / prop : 0;
      uint8_t tall = uint8_t(perc * BAR_TALL + 0.5f);
      NOMORE(tall, BAR_TALL);

      // Draw a heating progress bar, if specified
      #if ENABLED(STATUS_HEAT_PERCENT)

        if (isHeat) {
          const uint8_t bx = STATUS_BED_X + STATUS_BED_WIDTH;
          u8g.drawFrame(bx, STATUS_HEATERS_Y, 3, STATUS_HEATERS_HEIGHT);
          if (tall) {
            const uint8_t ph = STATUS_HEATERS_HEIGHT - 1 - tall;
            if (PAGE_OVER(STATUS_HEATERS_Y + ph))
              u8g.drawVLine(bx + 1, STATUS_HEATERS_Y + ph, tall);
          }
        }

      #endif

    } // PAGE_CONTAINS

    if (PAGE_UNDER(7)) {
      #if HEATER_IDLE_HANDLER
        const bool dodraw = (blink || !thermalManager.bed_idle.timed_out);
      #else
        constexpr bool dodraw = true;
      #endif
      if (dodraw) _draw_centered_temp(target + 0.5, tx, 7);
    }

    if (PAGE_CONTAINS(28 - INFO_FONT_ASCENT, 28 - 1))
      _draw_centered_temp(temp + 0.5f, tx, 28);

    if (STATIC_BED && BED_DOT && PAGE_CONTAINS(17, 19)) {
      u8g.setColorIndex(0); // set to white on black
      u8g.drawBox(tx, 20 - 2, 2, 2);
      u8g.setColorIndex(1); // restore black on white
    }

  }

#endif // DO_DRAW_BED

#if DO_DRAW_CHAMBER

  FORCE_INLINE void _draw_chamber_status() {
    #if HAS_HEATED_CHAMBER
      if (PAGE_UNDER(7))
        _draw_centered_temp(thermalManager.degTargetChamber() + 0.5f, STATUS_CHAMBER_TEXT_X, 7);
    #endif

    if (PAGE_CONTAINS(28 - INFO_FONT_ASCENT, 28 - 1))
      _draw_centered_temp(thermalManager.degChamber() + 0.5f, STATUS_CHAMBER_TEXT_X, 28);
  }

#endif // DO_DRAW_CHAMBER

//
// Before homing, blink '123' <-> '???'.
// Homed but unknown... '123' <-> '   '.
// Homed and known, display constantly.
//
FORCE_INLINE void _draw_axis_value(const AxisEnum axis, const char *value, const bool blink) {
  const AxisEnum a = (
    #if ENABLED(LCD_SHOW_E_TOTAL)
      axis == E_AXIS ? X_AXIS :
    #endif
    axis
  );
  const uint8_t offs = (XYZ_SPACING) * a;
  lcd_put_wchar(X_LABEL_POS + offs, XYZ_BASELINE, axis_codes[axis]);
  lcd_moveto(X_VALUE_POS + offs, XYZ_BASELINE);
  if (blink)
    lcd_put_u8str(value);
  else {
    if (!TEST(axis_homed, axis))
      while (const char c = *value++) lcd_put_wchar(c <= '.' ? c : '?');
    else {
      #if NONE(HOME_AFTER_DEACTIVATE, DISABLE_REDUCED_ACCURACY_WARNING)
        if (!TEST(axis_known_position, axis))
          lcd_put_u8str_P(axis == Z_AXIS ? PSTR("       ") : PSTR("    "));
        else
      #endif
          lcd_put_u8str(value);
    }
  }
}

#define GRAPHWIDTH 46                          //width of the temp graph on the lcd in pixels
millis_t time = 0;                             //timer for temp graph refreshing
uint8_t iterations = 0;                        //counts the graph refreshes. that way, the temp graphs can be drawn correctly on printer start up because then there exist no past temp values 
uint16_t bedTemp[GRAPHWIDTH] = {0};            //arrays which hold the past temp information so the graphs can be plotted
uint16_t bedTargetTemp[GRAPHWIDTH] = {0};
uint16_t hotendTemp[GRAPHWIDTH] = {0};
uint16_t hotendTargetTemp[GRAPHWIDTH] = {0};
uint16_t bedMinTemp;                           //maximum and minimum temp values to scale the graphs accordingly
uint16_t bedMaxTemp;
uint16_t hotendMinTemp;
uint16_t hotendMaxTemp;

void MarlinUI::draw_status_screen() {
  if(thermalManager.isHeatingHotend(0)){                    //decide on drawing the wiggly heat lines above the hotend bmp
    u8g.drawBitmapP(0, 2, 1, 19, status_hotend_a_bmp);      //draw hotend with wiggly heat lines
  }else{
    u8g.drawBitmapP(0, 2, 1, 19, status_hotend_b_bmp);      //draw hotend without wiggly heat lines
  }
  if(thermalManager.isHeatingBed()){                        //decide on drawing the wiggly heat lines above the bed bmp
    u8g.drawBitmapP(0, 28, 1, 12, status_bed_on_bmp);       //draw bed with wiggly heat lines
  }else{
    u8g.drawBitmapP(0, 39, 1, 1, status_bed_bmp);           //draw bed without wiggly heat lines
  }    
  _draw_centered_temp(thermalManager.degHotend(0), 18, 20);       //draws the temperature values next to the hotend/bed bmps
  _draw_centered_temp(thermalManager.degTargetHotend(0), 18, 11);
  _draw_centered_temp(thermalManager.degBed(), 18, 43);
  _draw_centered_temp(thermalManager.degTargetBed(), 18, 34);

  const xyz_pos_t lpos = current_position.asLogical();      //gets x, y and z position
  lcd_put_u8str(80, 7, "X: ");                              //draws the x, y and z letters and the x, y and z values
  lcd_put_u8str(92, 7, ftostr4sign(lpos.x));                //i think the function with this weird name converts the float into a string
  lcd_put_u8str(80, 16, "Y: ");
  lcd_put_u8str(92, 16, ftostr4sign(lpos.y));
  lcd_put_u8str(80, 25, "Z: ");
  lcd_put_u8str(92, 25, ftostr4sign(lpos.z));
  
  u8g.drawRFrame(80, 26, 9, 38, 0);                             //draws the hollow frame around the z bar graph
  for(uint16_t i = 63; i > 62 - lpos.z * 36 / Z_MAX_POS; i--){  //scales the z-axis value according to your z-axis size and the previously drawn bar graph size
    for(uint16_t j = 81; j < 88; j++){                          //i did not use the u8glib function for drawing boxes because it did not work
      u8g.drawPixel(j, i);                                      //fills the bargraph with pixels up to the desired level
    }
  }

  u8g.drawRFrame(90, 26, 38, 38, 0);                                            //draws the hollow frame for the x-y plot
  u8g.drawPixel(91 + lpos.x * 36 / X_MAX_POS, 62 - lpos.y * 36 / Y_MAX_POS);    //scales x and y values according to your bed size and the previously drawn frame size

  lcd_put_u8str(5, 53, "Fan");
  uint16_t fanSpeed = thermalManager.fanPercent(thermalManager.scaledFanSpeed(0, thermalManager.fan_speed[0])); //gets the fan speed. idk why so complicated, but it works
  lcd_put_u8str(2, 62, i16tostr3rj(fanSpeed));  //drawing the fan value and a % sign
  lcd_put_u8str(20, 62, "%");

  u8g.drawCircle(53, 63, 17, U8G_DRAW_UPPER_RIGHT); //draws the right half of the speedometer-circle
  u8g.drawCircle(53, 63, 17, U8G_DRAW_UPPER_LEFT);  //draws the left half of the speedometer-circle
  u8g.drawLine(53, 63, 53 - cos((fanSpeed * PI - PI) / 100) * 15, 63 - sin(fanSpeed * PI / 100) * 15); //draws the line and does fancy math

  if (millis() - time > 3000){  //updates the temp graphs every 3 seconds
    time = millis();            //resets timer
    bedMinTemp = bedTemp[0] < bedTargetTemp[0] ? bedTemp[0] : bedTargetTemp[0];                 //in order to scale the temp graph correctly,
    bedMaxTemp = bedTemp[0] > bedTargetTemp[0] ? bedTemp[0] : bedTargetTemp[0];                 //the minimum and maximum temp values have to 
    hotendMinTemp = hotendTemp[0] < hotendTargetTemp[0] ? hotendTemp[0] : hotendTargetTemp[0];  //be determined. this is the first step of
    hotendMaxTemp = hotendTemp[0] > hotendTargetTemp[0] ? hotendTemp[0] : hotendTargetTemp[0];  //doing this, second step comes later
    for (uint8_t i = GRAPHWIDTH - iterations; i < GRAPHWIDTH - 1; i++){                         //ensuring that only array indices who are initialized get processed, because on startup the arrays are empty and only slowly fill with temp values
      bedTemp[i] = bedTemp[i + 1];                                                              //shifting the array values one to the left as they get "older"
      bedTargetTemp[i] = bedTargetTemp[i + 1];
      hotendTemp[i] = hotendTemp[i + 1];
      hotendTargetTemp[i] = hotendTargetTemp[i + 1];
      bedMinTemp = bedTemp[i] < bedMinTemp ? bedTemp[i] : bedMinTemp;                           //second step of finding the maximum
      bedMinTemp = bedTargetTemp[i] < bedMinTemp ? bedTargetTemp[i] : bedMinTemp;               //and minimum temp values for scaling
      bedMaxTemp = bedTemp[i] > bedMaxTemp ? bedTemp[i] : bedMaxTemp;
      bedMaxTemp = bedTargetTemp[i] > bedMaxTemp ? bedTargetTemp[i] : bedMaxTemp;
      hotendMinTemp = hotendTemp[i] < hotendMinTemp ? hotendTemp[i] : hotendMinTemp;
      hotendMinTemp = hotendTargetTemp[i] < hotendMinTemp ? hotendTargetTemp[i] : hotendMinTemp;
      hotendMaxTemp = hotendTemp[i] > hotendMaxTemp ? hotendTemp[i] : hotendMaxTemp;
      hotendMaxTemp = hotendTargetTemp[i] > hotendMaxTemp ? hotendTargetTemp[i] : hotendMaxTemp;
    }
    bedTemp[GRAPHWIDTH - 1] = thermalManager.degBed();                                          //adding the latest temp values at the beginning of the arrays
    bedTargetTemp[GRAPHWIDTH - 1] = thermalManager.degTargetBed();
    hotendTemp[GRAPHWIDTH - 1] = thermalManager.degHotend(0);
    hotendTargetTemp[GRAPHWIDTH - 1] = thermalManager.degTargetHotend(0);
    iterations = iterations >= GRAPHWIDTH ? GRAPHWIDTH : iterations + 1;                        //increase the iteration counter until its the same as the graphwidth
  }                                                                                             //because then there are no uninitialized array indices left
  for (uint8_t i = GRAPHWIDTH - iterations; i < GRAPHWIDTH; i++){
    u8g.drawPixel(33 + i, 21 - (hotendTemp[i] - hotendMinTemp + 1) * 21 / (hotendMaxTemp - hotendMinTemp + 2));       //scaling the values and plotting the pixels
    u8g.drawPixel(33 + i, 21 - (hotendTargetTemp[i] - hotendMinTemp + 1) * 21 / (hotendMaxTemp - hotendMinTemp + 2));
    u8g.drawPixel(33 + i, 44 - (bedTemp[i] - bedMinTemp + 1) * 21 / (bedMaxTemp - bedMinTemp + 2));
    u8g.drawPixel(33 + i, 44 - (bedTargetTemp[i] - bedMinTemp + 1) * 21 / (bedMaxTemp - bedMinTemp + 2));
  }
  u8g.drawLine(32, 0, 32, 21);  //draws the left border line of the graph areas
  u8g.drawLine(32, 23, 32, 44);
  }

void MarlinUI::draw_status_message(const bool blink) {
  //sorry status massage, no space left for you :(
}

#endif // HAS_GRAPHICAL_LCD && !LIGHTWEIGHT_UI
