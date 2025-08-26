#include <Keyboard.h>
#include <Adafruit_NeoPixel.h>

#define NUM_COLUMNS 6
#define NUM_ROWS 5
#define ___ '\0'

#define LED_PIN 11
#define LED_BRIGHTNESS 64
#define LED_CYCLE_STEPS 16
#define LED_CYCLE_TIME_MICROS 1000000 / LED_CYCLE_STEPS
#define PERCENT_COLOR_PER_MICRO 0.000001


// Matrix pins.

const uint8_t PIN_COLUMNS[NUM_COLUMNS] = { 10, 9, 8, 7, 6, 5 };
const uint8_t PIN_ROWS[NUM_ROWS] = { 0, 1, 2, 3, 4 };
uint32_t PIN_DESC_COLUMNS[NUM_COLUMNS];
uint32_t PIN_DESC_ROWS[NUM_ROWS];


/**
 * RGBW color components (which need to be reversed for some fucking reason) as struct fields for
 * manipulation of individual channels.
 */
typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t w;
} ColorComponents;


/**
 * A union between a packed 32 bit color and a struct with its components. Bit-wise these are
 * equivilant and therefor no tag is required and both formats may be accessed with no prerequisits.
 */
typedef union {
    ColorComponents components;
    uint32_t packed_color;
} Color;


/**
 * Layers available on the keeb.
 */
typedef enum {
    LAYER_BASE = 0xFC,
    LAYER_FN
} Layer;


// Keymaps, in a more intuitive layout. Later they will be rearranged into a format optomized for
// scanning and reading quickly.

const char _KEYMAP[NUM_ROWS][NUM_COLUMNS] = {
    KEY_ESC,         '1',       '2',           '3',  '4',   '5',
    KEY_TAB,         'q',       'w',           'e',  'r',   't',
    'v',             'a',       's',           'd',  'f',   'g',
    KEY_LEFT_SHIFT,  'z',       'x',           'c',  ___,   'b',
    KEY_LEFT_CTRL,   LAYER_FN,  KEY_LEFT_ALT,  ' ',  ___,   '.'
};

const char _KEYMAP_FN[NUM_ROWS][NUM_COLUMNS] = {
    '`',   KEY_F1,  KEY_F2,  KEY_F3,       KEY_F4,  KEY_F5,
    ___,   ___,     ___,     ___,          ___,     ___,
    ___,   ___,     ___,     ___,          ___,     ___,
    ___,   ___,     ___,     ___,          ___,     ___,
    ___,   ___,     ___,     KEY_RETURN,   ___,     ___
};


// Keymaps with the axes flipped, so that the columns can be scanned in the outer loop and rows in
// the inner loop. This reduces cache misses in the scanning function and simply requires that 
// reading the state arrays is done column-row rather than row-column.

char KEYMAP[NUM_COLUMNS][NUM_ROWS];
char KEYMAP_FN[NUM_COLUMNS][NUM_ROWS];


// Colors for LED light.

/*
const Color COLOR_IDLE = Color {
    // Yellow
    .components = ColorComponents {
        .w = 0,
        .r = 128,
        .g = 96,
        .b = 0,
    }
};

const Color COLOR_ACTIVE = Color {
    // Red
    .components = ColorComponents{
        .w = 0,
        .r = 255,
        .g = 0,
        .b = 0,
    }
};
*/

const Color COLOR_IDLE = Color {
    // Cyan
    .components = ColorComponents {
        .b = 255,
        .g = 192,
        .r = 0,
        .w = 0,
    }
};

const Color COLOR_ACTIVE = Color {
    // Purple
    .components = ColorComponents{
        .b = 255,
        .g = 0,
        .r = 255,
        .w = 0,
    }
};


/**
 * Pre-computed colors for activity decay.
 */
uint32_t COLOR_CYCLE[LED_CYCLE_STEPS];


// Keyboard state. The extra third keys array is used for debouncing, it is a "processing" array,
// which is used to fact check the current keys array.

bool keys_curr[NUM_COLUMNS][NUM_ROWS];
bool keys_proc[NUM_COLUMNS][NUM_ROWS];
bool keys_prev[NUM_COLUMNS][NUM_ROWS];

Layer current_layer = LAYER_BASE;

uint8_t fn_key_pos_row;
uint8_t fn_key_pos_column;
unsigned long scan_time = 0;
unsigned long scan_printout_timestamp = 0;


// LED state.

uint32_t led_cycle_step = 0;
unsigned long prev_time = 0;
bool show_pending = false;

Adafruit_NeoPixel neopixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);


/**
 * Update the `keys_curr`, `keys_proc`, `keys_prev`, and `current_layer` variables with the current
 * keyboard state. This functions performs a scan of the key matrix.
 */
static void update_keys() {
    memcpy(&keys_prev, &keys_proc, sizeof(keys_curr));
    memcpy(&keys_proc, &keys_curr, sizeof(keys_curr));

    current_layer = LAYER_BASE;

    for (uint8_t column = 0; column < NUM_COLUMNS; column++) {
        uint32_t column_pin = PIN_DESC_COLUMNS[column];
        uint32_t column_mask = 1ul << column_pin;

        // Configure as output and clear pin.

        PORT->Group[PORTA].PINCFG[column_pin].reg = (uint8_t)(PORT_PINCFG_INEN | PORT_PINCFG_DRVSTR);
        PORT->Group[PORTA].DIRSET.reg = column_mask;
        PORT->Group[PORTA].OUTCLR.reg = column_mask;

        // The QT Py SAMD21 is a 48 MHz controller.
        //
        // 48 MHz = (48 * 1000 * 1000) Hz => 48^-1 μs per clock cycle.
        //
        // Each no-op should therefor take approx. 0.02 microseconds. Below is 25 such no-ops,
        // without which holding the 1, 2, and 3 keys simultaniously would rapidly register
        // erronious keypresses of the 4 key.
        //
        // Its pretty fucking jank but it seems to work pretty damn well. In total its about a 0.521
        // microsecond delay.

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");
        __asm__ __volatile__ ("nop");

        for (uint8_t row = 0; row < NUM_ROWS; row++) {
            uint32_t row_pin = PIN_DESC_ROWS[row];
            uint32_t row_mask = 1ul << row_pin;

            keys_curr[column][row] = (PORT->Group[PORTA].IN.reg & row_mask) == 0;
        }

        // For whatever reason this nearly eliminates false positives with successive key presses
        // within the same row. It probably grounds it or something.
        //
        // Configures pin an input and sets high.

        PORT->Group[PORTA].PINCFG[column_pin].reg = (uint8_t)(PORT_PINCFG_INEN);
        PORT->Group[PORTA].DIRCLR.reg = column_mask;
        PORT->Group[PORTA].OUTSET.reg = column_mask;
    }

    if (keys_curr[fn_key_pos_column][fn_key_pos_row]) {
        keys_curr[fn_key_pos_column][fn_key_pos_row] = false;
        current_layer = LAYER_FN;
    }

    unsigned long current_time = micros();
    
    if (current_time - scan_printout_timestamp >= 1000 * 250) {
        scan_printout_timestamp = current_time;
        unsigned long scan_duration = current_time - scan_time;
        unsigned long scan_rate = (1000 * 1000) / scan_duration;
        Serial.printf("Scan Time: %lu μs\tScan Rate: %lu hz\n", scan_duration, scan_rate);
    }

    scan_time = current_time;
}


/**
 * Returns `true` if the key at the given position was pressed in the last scan.
 */
static inline bool key_pressed(uint8_t column, uint8_t row) {
    return keys_curr[column][row] && !keys_prev[column][row];
}


/**
 * Returns `true` if the key at the given position was released in the last scan.
 */
static inline bool key_released(uint8_t column, uint8_t row) {
    return !keys_curr[column][row] && keys_prev[column][row];
}


/**
 * Scale all components/color channels by the given scalar.
 */
static inline ColorComponents scale_components(ColorComponents components, double scale) {
    return ColorComponents {
        .b = (uint8_t)((double)components.b * scale),
        .g = (uint8_t)((double)components.g * scale),
        .r = (uint8_t)((double)components.r * scale),
        .w = (uint8_t)((double)components.w * scale),
    };
}


/**
 * Add the corrosponding components of `lhs` and `rhs` and return the result.
 */
static inline ColorComponents add_components(ColorComponents lhs, ColorComponents rhs) {
    return ColorComponents {
        .b = (uint8_t)(lhs.b + rhs.b),
        .g = (uint8_t)(lhs.g + rhs.g),
        .r = (uint8_t)(lhs.r + rhs.r),
        .w = (uint8_t)(lhs.w + rhs.w),
    };
}


void setup() {
    noInterrupts();

    for (uint8_t row = 0; row < NUM_ROWS; row++) {
        PIN_DESC_ROWS[row] = g_APinDescription[PIN_ROWS[row]].ulPin;

        uint32_t pin = PIN_DESC_ROWS[row];
        uint32_t mask = 1ul << pin;

        // Configure as input with pullup resistor.

        PORT->Group[PORTA].PINCFG[pin].reg = (uint8_t)(PORT_PINCFG_INEN | PORT_PINCFG_PULLEN);
        PORT->Group[PORTA].DIRCLR.reg = mask;

        // Set the "default" state of the pin.

        PORT->Group[PORTA].OUTSET.reg = mask;
    }

    for (uint8_t column = 0; column < NUM_COLUMNS; column++) {
        PIN_DESC_COLUMNS[column] = g_APinDescription[PIN_COLUMNS[column]].ulPin;

        uint32_t pin = PIN_DESC_COLUMNS[column];
        uint32_t mask = 1ul << pin;

        // Set pin to output mode and enable reading.

        PORT->Group[PORTA].PINCFG[pin].reg = (uint8_t)(PORT_PINCFG_INEN | PORT_PINCFG_DRVSTR);
        PORT->Group[PORTA].DIRSET.reg = mask;

        // Set pin.

        PORT->Group[PORTA].OUTSET.reg = mask;
    }

    // Flip the axes of the keymaps to reduce cache misses while scanning the matrix.

    for (uint8_t row = 0; row < NUM_ROWS; row++) {
        for (uint8_t column = 0; column < NUM_COLUMNS; column++) {
            KEYMAP[column][row] = _KEYMAP[row][column];
            KEYMAP_FN[column][row] = _KEYMAP_FN[row][column];
        }
    }

    // Find the FN key ahead of time, so that layer checks may be performed in a single check,
    // rather than checking each key while scanning the matrix.

    for (uint8_t column = 0; column < NUM_COLUMNS; column++) {
        for (uint8_t row = 0; row < NUM_ROWS; row++) {
            if (KEYMAP[column][row] == LAYER_FN) {
                fn_key_pos_row = row;
                fn_key_pos_column = column;
                goto found_fn;
            }
        }
    }

found_fn:

    Keyboard.begin();

    neopixel.begin();
    neopixel.setBrightness(LED_BRIGHTNESS);
    neopixel.fill(neopixel.gamma32(COLOR_IDLE.packed_color));
    neopixel.show();

    // Precompute LED light animations to avoid performing the computation and blocking further
    // matrix scans.
    
    for (uint8_t cycle_step = 0; cycle_step < LED_CYCLE_STEPS; cycle_step++) {
        double percent = (double)cycle_step / (double)LED_CYCLE_STEPS;

        Color current_color = Color {
            .components = add_components(
                scale_components(COLOR_IDLE.components, percent),
                scale_components(COLOR_ACTIVE.components, (1.0 - percent))
            )
        };

        COLOR_CYCLE[cycle_step] = neopixel.gamma32(current_color.packed_color);
    }

    Serial.begin(9600);
}


void loop() {
loop_start:
    update_keys();

    if (current_layer == LAYER_BASE) {
        for (uint8_t column = 0; column < NUM_COLUMNS; column++) {
            for (uint8_t row = 0; row < NUM_ROWS; row++) {
                char key = KEYMAP[column][row];

                if (key == ___) {
                    continue;
                }

                if (key_released(column, row)) {
                    Keyboard.release(key);
                } else if (key_pressed(column, row)) {
                    Keyboard.press(key);
                    led_cycle_step = 0;
                }
            }
        }
    } else if (current_layer == LAYER_FN) {
        for (uint8_t column = 0; column < NUM_COLUMNS; column++) {
            for (uint8_t row = 0; row < NUM_ROWS; row++) {
                char key = KEYMAP_FN[column][row];

                if (key == ___ && (key = KEYMAP[column][row]) == ___) {
                    continue;
                }

                if (key_released(column, row)) {
                    Keyboard.release(key);
                } else if (key_pressed(column, row)) {
                    Keyboard.press(key);
                    led_cycle_step = 0;
                }
            }
        }
    }

    // If we are at the end of an animation just skip the checks involved with animating the LED.

    if (led_cycle_step == LED_CYCLE_STEPS) {
        goto loop_start;
    }

    // Update LED light based on activity. The else-if deal delays the `show()` call until
    // `canShow()` is more likely to be `true`. It also seperates some computation between scan
    // frames, reducing inconsistency in frame times.

    if (micros() - prev_time >= LED_CYCLE_TIME_MICROS) {
        neopixel.setPixelColor(0, COLOR_CYCLE[led_cycle_step]);
        prev_time = micros();
        led_cycle_step++;
        show_pending = true;
    } else if (show_pending && neopixel.canShow()) {
        neopixel.show();
        show_pending = false;
    }
}
