#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <AnimatedGIF.h>
#include "blinky_gif.h"

// ============================================================
// LILYGO T-Display-S3 AMOLED
// Regular version - RM67162 - 536 x 240 landscape
// ============================================================

// -------------------------
// Display pins
// -------------------------
#define TFT_CS    6
#define TFT_SCK   47
#define TFT_D0    18
#define TFT_D1    7
#define TFT_D2    48
#define TFT_D3    5
#define TFT_RST   17
#define TFT_PWR   38

// -------------------------
// Screen dimensions
// -------------------------
#define SCREEN_WIDTH   536
#define SCREEN_HEIGHT  240

// -------------------------
// GIF dimensions
// Your GIF is 400 x 225
// -------------------------
#define GIF_WIDTH   400
#define GIF_HEIGHT  225

// Center GIF on 536 x 240 screen
#define GIF_X ((SCREEN_WIDTH  - GIF_WIDTH)  / 2)
#define GIF_Y ((SCREEN_HEIGHT - GIF_HEIGHT) / 2)

// RGB565 black
#define BLACK 0x0000

// ============================================================
// Arduino_GFX QSPI setup
// ============================================================

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  TFT_CS,
  TFT_SCK,
  TFT_D0,
  TFT_D1,
  TFT_D2,
  TFT_D3
);

// IMPORTANT:
// RM67162 rotation 1 = LANDSCAPE
Arduino_GFX *display = new Arduino_RM67162(
  bus,
  TFT_RST,
  1
);

// ============================================================
// Framebuffer
// ============================================================

Arduino_Canvas *canvas = new Arduino_Canvas(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  display,
  0,
  0,
  0
);

// ============================================================
// GIF decoder
// ============================================================

AnimatedGIF gif;

// Temporary buffer for one scanline
uint16_t lineBuffer[GIF_WIDTH];

// ============================================================
// GIF DRAW CALLBACK
// ============================================================

void GIFDraw(GIFDRAW *pDraw)
{
  int x = GIF_X + pDraw->iX;
  int y = GIF_Y + pDraw->iY + pDraw->y;

  int width = pDraw->iWidth;

  // ----------------------------------------------------------
  // Check vertical bounds
  // ----------------------------------------------------------

  if (y < 0 || y >= SCREEN_HEIGHT)
  {
    return;
  }

  // ----------------------------------------------------------
  // Check horizontal bounds
  // ----------------------------------------------------------

  if (x >= SCREEN_WIDTH)
  {
    return;
  }

  // ----------------------------------------------------------
  // Clip left side
  // ----------------------------------------------------------

  int sourceStart = 0;

  if (x < 0)
  {
    sourceStart = -x;
    width -= sourceStart;
    x = 0;
  }

  // ----------------------------------------------------------
  // Clip right side
  // ----------------------------------------------------------

  if (x + width > SCREEN_WIDTH)
  {
    width = SCREEN_WIDTH - x;
  }

  if (width <= 0)
  {
    return;
  }

  uint8_t *src = pDraw->pPixels + sourceStart;
  uint16_t *palette = pDraw->pPalette;

  // ==========================================================
  // No transparency
  // ==========================================================

  if (!pDraw->ucHasTransparency)
  {
    for (int i = 0; i < width; i++)
    {
      lineBuffer[i] = palette[src[i]];
    }

    canvas->draw16bitRGBBitmap(
      x,
      y,
      lineBuffer,
      width,
      1
    );

    return;
  }

  // ==========================================================
  // Transparency
  // ==========================================================

  int i = 0;

  while (i < width)
  {
    // Skip transparent pixels
    while (
      i < width &&
      src[i] == pDraw->ucTransparent
    )
    {
      i++;
    }

    if (i >= width)
    {
      break;
    }

    int runStart = i;

    // Find end of opaque run
    while (
      i < width &&
      src[i] != pDraw->ucTransparent
    )
    {
      lineBuffer[i - runStart] = palette[src[i]];
      i++;
    }

    int runLength = i - runStart;

    canvas->draw16bitRGBBitmap(
      x + runStart,
      y,
      lineBuffer,
      runLength,
      1
    );
  }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println("LILYGO T-Display-S3 AMOLED GIF Player");
  Serial.println("----------------------------------------");

  // ----------------------------------------------------------
  // Enable AMOLED power
  // ----------------------------------------------------------

  pinMode(TFT_PWR, OUTPUT);
  digitalWrite(TFT_PWR, HIGH);

  delay(100);

  // ----------------------------------------------------------
  // Start display
  // ----------------------------------------------------------

  Serial.println("Starting display...");

  if (!canvas->begin())
  {
    Serial.println("ERROR: Display initialization failed!");

    while (true)
    {
      delay(1000);
    }
  }

  // ----------------------------------------------------------
  // IMPORTANT:
  //
  // DO NOT call:
  //
  // canvas->setRotation(0);
  // canvas->setRotation(1);
  //
  // The RM67162 display itself is already configured for
  // landscape using rotation 1 above.
  // ----------------------------------------------------------

  canvas->fillScreen(BLACK);
  canvas->flush();

  // ----------------------------------------------------------
  // Start AnimatedGIF
  // ----------------------------------------------------------

  gif.begin(GIF_PALETTE_RGB565_LE);

  Serial.println("Display initialized.");

  Serial.printf(
    "Screen: %d x %d\n",
    SCREEN_WIDTH,
    SCREEN_HEIGHT
  );

  Serial.printf(
    "GIF: %d x %d\n",
    GIF_WIDTH,
    GIF_HEIGHT
  );

  Serial.printf(
    "GIF position: X=%d Y=%d\n",
    GIF_X,
    GIF_Y
  );

  Serial.printf(
    "Free heap: %u bytes\n",
    ESP.getFreeHeap()
  );

  Serial.printf(
    "Free PSRAM: %u bytes\n",
    ESP.getFreePsram()
  );
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  Serial.println("Opening GIF...");

  // ----------------------------------------------------------
  // Open GIF from flash
  // ----------------------------------------------------------

  if (!gif.openFLASH(
        (uint8_t *)blinky_gif,
        blinky_gif_size,
        GIFDraw
      ))
  {
    Serial.printf(
      "ERROR: Could not open GIF. Error: %d\n",
      gif.getLastError()
    );

    delay(1000);
    return;
  }

  Serial.printf(
    "GIF opened: %d x %d\n",
    gif.getCanvasWidth(),
    gif.getCanvasHeight()
  );

  // ----------------------------------------------------------
  // Clear framebuffer before starting
  // ----------------------------------------------------------

  canvas->fillScreen(BLACK);

  // ----------------------------------------------------------
  // Play entire GIF
  // ----------------------------------------------------------

  int frameDelay = 0;

  while (
    gif.playFrame(
      false,
      &frameDelay
    )
  )
  {
    // Push completed framebuffer to AMOLED
    canvas->flush();

    // Preserve original GIF timing
    if (frameDelay > 0)
    {
      delay(frameDelay);
    }
  }

  // ----------------------------------------------------------
  // Close GIF
  // ----------------------------------------------------------

  gif.close();

  // Immediately start from frame 1 again
}