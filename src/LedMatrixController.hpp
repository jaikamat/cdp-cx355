#ifndef LED_MATRIX_CONTROLLER_HPP
#define LED_MATRIX_CONTROLLER_HPP

#include "Arduino_LED_Matrix.h"
#include "ArduinoGraphics.h" // Required for text rendering

// Define available animations
enum class MatrixAnimation
{
    WifiSearch,
    Loading
};

class LedMatrixController
{
private:
    ArduinoLEDMatrix &matrix;

public:
    LedMatrixController(ArduinoLEDMatrix &matrixRef) : matrix(matrixRef) {}

    void playAnimation(MatrixAnimation animation, bool loop = false)
    {
        switch (animation)
        {
        case MatrixAnimation::WifiSearch:
            matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
            break;
        case MatrixAnimation::Loading:
            matrix.loadSequence(LEDMATRIX_ANIMATION_INFINITY_LOOP_LOADER);
            break;
        }
        matrix.play(loop);
    }

    void clearDisplay()
    {
        matrix.clear();
    }

    void displayText(const char *text, uint8_t x = 0, uint8_t y = 1, bool scroll = true)
    {
        clearDisplay(); // Ensure a clean display to start

        matrix.beginDraw();
        matrix.stroke(0xFFFFFF); // Set color to white

        matrix.textFont(Font_5x7);        // Use a readable font
        matrix.beginText(x, y, 0xFFFFFF); // Set the start position and color
        matrix.println(text);
        matrix.endText(); // Display static text

        matrix.endDraw();

        if (scroll)
        {
            delay(1000); // Wait before starting the scroll

            matrix.beginDraw();
            matrix.stroke(0xFFFFFF);
            matrix.textScrollSpeed(50); // Set scroll speed
            matrix.beginText(x, y, 0xFFFFFF);
            matrix.println(text);
            matrix.endText(SCROLL_LEFT); // Scroll left
            matrix.endDraw();
        }
    }
};

#endif // LED_MATRIX_CONTROLLER_HPP
