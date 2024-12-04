#include <furi.h>
#include <furi_hal.h>
#include <stm32wbxx_ll_gpio.h>
#include <stm32wbxx_ll_bus.h>
#include <gui/gui.h>
#include <input/input.h>

// Define the GPIO pin for IR transmission (PB9 for Flipper Zero's internal IR LED)
#define IR_TX_GPIO_Port GPIOB
#define IR_TX_Pin       LL_GPIO_PIN_9

typedef struct {
    bool transmit_requested; // Flag indicating if the IR signal should be transmitted
    bool running; // Flag to control the main loop
} LightgunApp;

// Configure the IR GPIO pin
void ir_setup_tx_pin() {
    // Enable the GPIO clock
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    // Configure PB9 as output
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = IR_TX_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(IR_TX_GPIO_Port, &GPIO_InitStruct);
}

// Transmit a raw IR signal
void ir_transmit_raw() {
    // Example: Raw signal timings (ON/OFF durations in microseconds)
    const uint32_t signal_timings[] = {900, 450, 900, 450, 900, 450}; // Adjust as needed

    for(size_t i = 0; i < sizeof(signal_timings) / sizeof(signal_timings[0]); i++) {
        if(i % 2 == 0) {
            LL_GPIO_SetOutputPin(IR_TX_GPIO_Port, IR_TX_Pin); // Turn IR LED on
        } else {
            LL_GPIO_ResetOutputPin(IR_TX_GPIO_Port, IR_TX_Pin); // Turn IR LED off
        }
        furi_delay_us(signal_timings[i]); // Wait for the specified duration
    }

    // Ensure the IR LED is turned off at the end
    LL_GPIO_ResetOutputPin(IR_TX_GPIO_Port, IR_TX_Pin);
}

// Button callback function
static void lightgun_button_callback(InputEvent* input_event, void* context) {
    LightgunApp* app = context;

    if(input_event->type == InputTypePress) {
        if(input_event->key == InputKeyOk) {
            // Set the flag to request IR transmission
            app->transmit_requested = true;
        } else if(input_event->key == InputKeyBack) {
            // Set the running flag to false to exit the app
            app->running = false;
        }
    }
}

// Render callback to display UI
static void lightgun_render_callback(Canvas* canvas, void* context) {
    (void)context;
    canvas_draw_str_aligned(canvas, 54, 12, AlignCenter, AlignCenter, "Press OK to transmit IR");
    canvas_draw_str_aligned(canvas, 54, 22, AlignCenter, AlignCenter, "Press BACK to exit");
}

// Main entry point for the app
int32_t lightgun_app_main(void* p) {
    (void)p;

    // Allocate memory for the app context
    LightgunApp* app = malloc(sizeof(LightgunApp));
    app->transmit_requested = false;
    app->running = true;

    // Configure the IR GPIO pin
    ir_setup_tx_pin();

    // Set up the GUI ViewPort
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, lightgun_render_callback, app);
    view_port_input_callback_set(view_port, lightgun_button_callback, app);

    // Attach the viewport to the GUI
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Main loop
    while(app->running) {
        if(app->transmit_requested) {
            // Transmit the IR signal and reset the flag
            ir_transmit_raw();
            app->transmit_requested = false;
        }

        // Delay to avoid busy looping
        furi_delay_ms(100);
    }

    // Cleanup
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close("gui");
    free(app);

    return 0;
}
