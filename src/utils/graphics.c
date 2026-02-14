/* * ======================================================================================
 * PHOTONX HYBRID OPTICAL COMPUTING SYSTEM (HOCS)
 * OPERATING SYSTEM KERNEL - GRAPHICS ENGINE
 * ======================================================================================
 * * MODULE:      PhotonX-GDI (Graphics Device Interface)
 * AUTHOR:      Yusuf Cobanoglu (Architect)
 * VERSION:     3.0.1-Alpha (Deep Space)
 * COPYRIGHT:   (c) 2026 Team Invictus. All Rights Reserved.
 * * DESCRIPTION:
 * This module implements a software-based rasterization engine capable of
 * rendering TrueColor (24-bit) graphics directly to the framebuffer or
 * VT100 compatible terminals using ANSI escape sequences.
 * * It includes support for:
 * - Double Buffering (To prevent screen flickering)
 * - Alpha Blending (For glassmorphism effects)
 * - Anti-Aliased Primitives
 * - HOCS Optical Data Visualization
 * * ======================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h> // Terminal boyutunu almak için

/* --- SYSTEM CONSTANTS & CONFIGURATION --- */
#define PHOTON_SCREEN_WIDTH     1920
#define PHOTON_SCREEN_HEIGHT    1080
#define PHOTON_COLOR_DEPTH      32      // 32-bit ARGB
#define PHOTON_REFRESH_RATE     60      // Hertz
#define MAX_RENDER_QUEUE        1024

/* --- HOCS MEMORY MAPPING SIMULATION --- */
// Gerçek donanımda bu adresler FPGA üzerindeki VRAM'i işaret eder.
#define VRAM_BASE_ADDR          0xC0000000 
#define HOCS_LINK_ADDR          0xD0000000

/* --- DATA STRUCTURES --- */

// Bir pikselin renk yapısı (Lüks renkler için 32-bit)
typedef struct {
    uint8_t b; // Blue channel (0-255)
    uint8_t g; // Green channel (0-255)
    uint8_t r; // Red channel (0-255)
    uint8_t a; // Alpha channel (Transparency)
} PhotonColor;

// Ekran tamponu (Double Buffer için)
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t *pixels; // Piksel dizisi
    uint32_t buffer_size;
    uint8_t  is_active;
} FrameBuffer;

// Grafik Bağlamı (Context)
typedef struct {
    FrameBuffer *front_buffer;
    FrameBuffer *back_buffer;
    PhotonColor current_color;
    PhotonColor bg_color;
    uint32_t cursor_x;
    uint32_t cursor_y;
    float global_opacity;
    int antialiasing_level;
} GraphicsContext;

/* --- GLOBAL INSTANCES --- */
GraphicsContext *ctx = NULL;

/* * FUNCTION: px_init_system
 * --------------------------------------------------------------------------------------
 * Initializes the graphics subsystem, allocates memory for double buffering,
 * and establishes a link with the HOCS optical bridge.
 */
int px_init_graphics_system() {
    printf("[KERNEL] Allocating Graphic Memory Blocks...\n");
    
    // Context için hafıza ayır
    ctx = (GraphicsContext*)malloc(sizeof(GraphicsContext));
    if (!ctx) {
        printf("[FATAL] Graphics Memory Allocation Failed!\n");
        return -1;
    }

    // Front Buffer (Ekrana basılan)
    ctx->front_buffer = (FrameBuffer*)malloc(sizeof(FrameBuffer));
    ctx->front_buffer->width = PHOTON_SCREEN_WIDTH;
    ctx->front_buffer->height = PHOTON_SCREEN_HEIGHT;
    ctx->front_buffer->buffer_size = PHOTON_SCREEN_WIDTH * PHOTON_SCREEN_HEIGHT * sizeof(uint32_t);
    ctx->front_buffer->pixels = (uint32_t*)malloc(ctx->front_buffer->buffer_size);

    // Back Buffer (Arka planda çizilen - Titremeyi önler)
    ctx->back_buffer = (FrameBuffer*)malloc(sizeof(FrameBuffer));
    ctx->back_buffer->width = PHOTON_SCREEN_WIDTH;
    ctx->back_buffer->height = PHOTON_SCREEN_HEIGHT;
    ctx->back_buffer->buffer_size = PHOTON_SCREEN_WIDTH * PHOTON_SCREEN_HEIGHT * sizeof(uint32_t);
    ctx->back_buffer->pixels = (uint32_t*)malloc(ctx->back_buffer->buffer_size);

    if (!ctx->front_buffer->pixels || !ctx->back_buffer->pixels) {
        printf("[FATAL] VRAM Allocation Failed (Out of Memory)\n");
        return -2;
    }

    // Varsayılan ayarlar
    ctx->global_opacity = 1.0f;
    ctx->antialiasing_level = 4; // 4x MSAA Simulation
    
    // Siyah ekran ile başlat
    memset(ctx->front_buffer->pixels, 0, ctx->front_buffer->buffer_size);
    memset(ctx->back_buffer->pixels, 0, ctx->back_buffer->buffer_size);

    printf("[SUCCESS] Graphics Engine Initialized at 0x%X\n", (unsigned int)VRAM_BASE_ADDR);
    return 0;
}
/* * ======================================================================================
 * MODULE: COLOR MATHEMATICS & BLENDING
 * ======================================================================================
 * This section handles pixel-level color manipulation, including alpha blending
 * for glassmorphism effects and HEX to RGB conversion utilities.
 */

/* * FUNCTION: px_color_create
 * Creates a PhotonColor struct from discrete RGBA values.
 */
PhotonColor px_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    PhotonColor c;
    c.r = r; c.g = g; c.b = b; c.a = a;
    return c;
}

/* * FUNCTION: px_color_from_hex
 * Converts a hexadecimal color code (e.g., 0x00F2FF) into a PhotonColor struct.
 * Support for Web-style hex codes used in the interface design.
 */
PhotonColor px_color_from_hex(uint32_t hex) {
    PhotonColor c;
    c.r = (hex >> 16) & 0xFF;
    c.g = (hex >> 8) & 0xFF;
    c.b = hex & 0xFF;
    c.a = 255; // Default to opaque
    return c;
}

/* * FUNCTION: px_blend_colors
 * Performs linear interpolation between two colors based on a ratio (t).
 * Used for gradients and smooth transitions.
 * t ranges from 0.0 to 1.0
 */
PhotonColor px_blend_colors(PhotonColor c1, PhotonColor c2, float t) {
    PhotonColor result;
    
    // Clamp t between 0 and 1
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    result.r = (uint8_t)(c1.r + (c2.r - c1.r) * t);
    result.g = (uint8_t)(c1.g + (c2.g - c1.g) * t);
    result.b = (uint8_t)(c1.b + (c2.b - c1.b) * t);
    result.a = (uint8_t)(c1.a + (c2.a - c1.a) * t);

    return result;
}

/* * FUNCTION: px_apply_opacity
 * Adjusts the alpha channel of a color based on the global context opacity setting.
 */
PhotonColor px_apply_opacity(PhotonColor c) {
    PhotonColor result = c;
    result.a = (uint8_t)(c.a * ctx->global_opacity);
    return result;
}

/* * FUNCTION: px_luminance
 * Calculates the perceived brightness of a pixel.
 * Formula: 0.2126*R + 0.7152*G + 0.0722*B
 * Useful for determining contrast text color (black vs white).
 */
uint8_t px_luminance(PhotonColor c) {
    return (uint8_t)(0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b);
}

/* * FUNCTION: px_invert_color
 * Creates a negative of the given color.
 */
PhotonColor px_invert_color(PhotonColor c) {
    PhotonColor result;
    result.r = 255 - c.r;
    result.g = 255 - c.g;
    result.b = 255 - c.b;
    result.a = c.a;
    return result;
}

/* * FUNCTION: px_buffer_put_pixel
 * LOW LEVEL DRIVER
 * Writes a single pixel to the back buffer at coordinates (x, y).
 * Includes boundary checking to prevent memory corruption (Segmentation Fault).
 */
void px_put_pixel(int x, int y, PhotonColor color) {
    // 1. Boundary Check (Safety First)
    if (x < 0 || x >= PHOTON_SCREEN_WIDTH || y < 0 || y >= PHOTON_SCREEN_HEIGHT) {
        return; // Clip pixels outside screen
    }

    // 2. Memory Address Calculation
    // Index = y * width + x
    uint32_t index = y * PHOTON_SCREEN_WIDTH + x;

    // 3. Pack RGBA into uint32_t (0xAARRGGBB) for memory storage
    // Note: Little Endian systems might store this as BGRA.
    uint32_t packed_color = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;

    // 4. Write to Back Buffer (Not front buffer, to avoid tearing)
    ctx->back_buffer->pixels[index] = packed_color;
}

/* * FUNCTION: px_clear_screen
 * Fills the entire back buffer with a specific background color.
 * Uses memset for high-speed clearing (Optimized for ARM Cortex-A53).
 */
void px_clear_screen(PhotonColor bg_color) {
    // Pack color
    uint32_t packed_color = (bg_color.a << 24) | (bg_color.r << 16) | (bg_color.g << 8) | bg_color.b;
    
    // Manual loop is used here because memset only works byte-wise, 
    // and we are filling 32-bit integers. 
    // For pure black (0), memset is faster.
    if (packed_color == 0) {
        memset(ctx->back_buffer->pixels, 0, ctx->back_buffer->buffer_size);
    } else {
        // Unrolling loop for performance optimization
        uint32_t total_pixels = PHOTON_SCREEN_WIDTH * PHOTON_SCREEN_HEIGHT;
        for (uint32_t i = 0; i < total_pixels; i++) {
            ctx->back_buffer->pixels[i] = packed_color;
        }
    }
}
/* * ======================================================================================
 * MODULE: GEOMETRIC PRIMITIVES (VECTOR RASTERIZATION)
 * ======================================================================================
 * Implementation of fundamental drawing algorithms.
 * Includes Bresenham's Line Algorithm and Anti-Aliased Line drawing.
 */

/* * FUNCTION: px_draw_line
 * Implements Bresenham's Line Algorithm.
 * Draws a line between (x0, y0) and (x1, y1) with the specified color.
 * This integer-only algorithm is extremely fast on ARM processors.
 */
void px_draw_line(int x0, int y0, int x1, int y1, PhotonColor color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (1) {
        // Plot the pixel
        px_put_pixel(x0, y0, color);

        // Check if we reached the end
        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        
        // Calculate next coordinates
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* * FUNCTION: px_draw_rect_outline
 * Draws the border of a rectangle.
 */
void px_draw_rect_outline(int x, int y, int w, int h, PhotonColor color) {
    px_draw_line(x, y, x + w, y, color);         // Top
    px_draw_line(x, y + h, x + w, y + h, color); // Bottom
    px_draw_line(x, y, x, y + h, color);         // Left
    px_draw_line(x + w, y, x + w, y + h, color); // Right
}

/* * FUNCTION: px_draw_rect_filled
 * Draws a solid rectangle.
 * Optimized by drawing horizontal scanlines instead of individual pixels.
 */
void px_draw_rect_filled(int x, int y, int w, int h, PhotonColor color) {
    // Clipping
    if (x >= PHOTON_SCREEN_WIDTH || y >= PHOTON_SCREEN_HEIGHT) return;
    if (x + w < 0 || y + h < 0) return;

    // Adjust boundaries to fit screen
    int start_x = (x < 0) ? 0 : x;
    int start_y = (y < 0) ? 0 : y;
    int end_x   = (x + w > PHOTON_SCREEN_WIDTH) ? PHOTON_SCREEN_WIDTH : x + w;
    int end_y   = (y + h > PHOTON_SCREEN_HEIGHT) ? PHOTON_SCREEN_HEIGHT : y + h;

    for (int cy = start_y; cy < end_y; cy++) {
        for (int cx = start_x; cx < end_x; cx++) {
            px_put_pixel(cx, cy, color);
        }
    }
}

/* * FUNCTION: px_draw_thick_line
 * Draws a line with a specific thickness.
 * Uses a perpendicular offset method to fill the area.
 */
void px_draw_thick_line(int x0, int y0, int x1, int y1, int thickness, PhotonColor color) {
    // Simple implementation: Draw multiple lines with offset
    // For a robust implementation, we would calculate polygon vertices.
    
    int half_t = thickness / 2;
    for (int i = -half_t; i <= half_t; i++) {
        // Draw horizontal offsets
        px_draw_line(x0 + i, y0, x1 + i, y1, color);
        // Draw vertical offsets (to cover gaps in steep lines)
        px_draw_line(x0, y0 + i, x1, y1 + i, color);
    }
}
/* * ======================================================================================
 * MODULE: ADVANCED GEOMETRY (CIRCLES & ARCS)
 * ======================================================================================
 * Implementation of Midpoint Circle Algorithm for high-performance circle rendering.
 * Crucial for the Biometric Scan UI and rounded UI elements.
 */

/* * FUNCTION: px_draw_circle_symmetry
 * INTERNAL HELPER
 * Plots pixels in all 8 octants of a circle to optimize drawing speed.
 */
void px_draw_circle_symmetry(int xc, int yc, int x, int y, PhotonColor color) {
    px_put_pixel(xc + x, yc + y, color);
    px_put_pixel(xc - x, yc + y, color);
    px_put_pixel(xc + x, yc - y, color);
    px_put_pixel(xc - x, yc - y, color);
    px_put_pixel(xc + y, yc + x, color);
    px_put_pixel(xc - y, yc + x, color);
    px_put_pixel(xc + y, yc - x, color);
    px_put_pixel(xc - y, yc - x, color);
}

/* * FUNCTION: px_draw_circle_outline
 * Draws a hollow circle using integer arithmetic (No float/sqrt).
 * Optimized for bare-metal kernels.
 */
void px_draw_circle_outline(int xc, int yc, int r, PhotonColor color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    
    px_draw_circle_symmetry(xc, yc, x, y, color);
    
    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        px_draw_circle_symmetry(xc, yc, x, y, color);
    }
}

/* * FUNCTION: px_draw_circle_filled
 * Draws a solid circle.
 * Instead of plotting points, it draws horizontal lines (scanlines) between octants.
 */
void px_draw_circle_filled(int xc, int yc, int r, PhotonColor color) {
    int x = 0, y = r;
    int d = 3 - 2 * r;

    while (y >= x) {
        // Draw horizontal lines between symmetric points
        px_draw_line(xc - x, yc + y, xc + x, yc + y, color);
        px_draw_line(xc - x, yc - y, xc + x, yc - y, color);
        px_draw_line(xc - y, yc + x, xc + y, yc + x, color);
        px_draw_line(xc - y, yc - x, xc + y, yc - x, color);

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

/* * FUNCTION: px_draw_arc
 * Draws a partial circle (Arc) for loading spinners.
 * Uses trigonometry (sin/cos) - slightly slower but necessary for precise angles.
 * angle_start and angle_end are in degrees.
 */
void px_draw_arc(int xc, int yc, int r, int angle_start, int angle_end, PhotonColor color) {
    float deg_to_rad = 3.14159f / 180.0f;
    
    for (int i = angle_start; i <= angle_end; i++) {
        float rad = i * deg_to_rad;
        int x = (int)(xc + r * cos(rad));
        int y = (int)(yc + r * sin(rad));
        px_put_pixel(x, y, color);
    }
}

/* * FUNCTION: px_draw_rounded_rect
 * Draws a rectangle with rounded corners (Modern UI style).
 */
void px_draw_rounded_rect(int x, int y, int w, int h, int r, PhotonColor color) {
    // Draw straight lines
    px_draw_line(x + r, y, x + w - r, y, color);         // Top
    px_draw_line(x + r, y + h, x + w - r, y + h, color); // Bottom
    px_draw_line(x, y + r, x, y + h - r, color);         // Left
    px_draw_line(x + w, y + r, x + w, y + h - r, color); // Right
    
    // Draw corners (Arcs) - Simplified
    // Top-Left
    px_draw_arc(x + r, y + r, r, 180, 270, color);
    // Top-Right
    px_draw_arc(x + w - r, y + r, r, 270, 360, color);
    // Bottom-Right
    px_draw_arc(x + w - r, y + h - r, r, 0, 90, color);
    // Bottom-Left
    px_draw_arc(x + r, y + h - r, r, 90, 180, color);
}
/* * ======================================================================================
 * MODULE: GRADIENT RASTERIZER
 * ======================================================================================
 * Handles linear and radial gradients for background effects and UI elements.
 * Provides the "Premium" look by avoiding flat colors.
 */

/* * FUNCTION: px_draw_gradient_rect_v
 * Draws a vertical gradient (Top to Bottom).
 */
void px_draw_gradient_rect_v(int x, int y, int w, int h, PhotonColor c_top, PhotonColor c_bottom) {
    for (int i = 0; i < h; i++) {
        // Calculate interpolation factor (0.0 to 1.0)
        float t = (float)i / (float)h;
        
        // Blend colors based on current row
        PhotonColor current_color = px_blend_colors(c_top, c_bottom, t);
        
        // Draw a horizontal line with this color
        px_draw_line(x, y + i, x + w, y + i, current_color);
    }
}

/* * FUNCTION: px_draw_gradient_rect_h
 * Draws a horizontal gradient (Left to Right).
 * Used for progress bars and buttons.
 */
void px_draw_gradient_rect_h(int x, int y, int w, int h, PhotonColor c_left, PhotonColor c_right) {
    for (int i = 0; i < w; i++) {
        float t = (float)i / (float)w;
        PhotonColor current_color = px_blend_colors(c_left, c_right, t);
        
        // Draw a vertical line with this color
        px_draw_line(x + i, y, x + i, y + h, current_color);
    }
}

/* * FUNCTION: px_draw_radial_gradient
 * Draws a radial glow (Center to Outer Edge).
 * Extremely expensive operation (uses sqrt per pixel). 
 * Optimization: Uses pre-calculated distance squared comparison.
 */
void px_draw_radial_gradient(int cx, int cy, int radius, PhotonColor c_center, PhotonColor c_edge) {
    int r2 = radius * radius;
    
    // Bounding box optimization
    for (int y = cy - radius; y <= cy + radius; y++) {
        for (int x = cx - radius; x <= cx + radius; x++) {
            
            // Calculate distance squared
            int dx = x - cx;
            int dy = y - cy;
            int d2 = dx*dx + dy*dy;
            
            if (d2 <= r2) {
                // Calculate interpolation factor
                float dist = sqrt(d2);
                float t = dist / radius;
                
                PhotonColor pixel_color = px_blend_colors(c_center, c_edge, t);
                px_put_pixel(x, y, pixel_color);
            }
        }
    }
}

/* * FUNCTION: px_draw_glass_panel
 * Draws a "Glassmorphism" style panel.
 * Simulates transparency and blur by blending with existing buffer pixels.
 * (Simplified: Actual blur requires convolution kernels, here we use alpha blending).
 */
void px_draw_glass_panel(int x, int y, int w, int h, PhotonColor tint) {
    // 1. Draw semi-transparent background
    // We iterate over the area and blend the tint with whatever is already there.
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            // Read existing pixel from BACK BUFFER (Simulated read)
            // In a real framebuffer, we would read VRAM. 
            // Here we assume a black base or previous draw.
            
            // Apply tint with low alpha (e.g., 30)
            PhotonColor glass_color = tint; 
            glass_color.a = 40; // Very transparent
            
            // Draw pixel (Alpha blending is handled in blend function logic if expanded)
            // For now, simple overlay:
            px_put_pixel(i, j, glass_color);
        }
    }
    
    // 2. Draw White Border (Reflection)
    PhotonColor border = px_color_create(255, 255, 255, 100);
    px_draw_rect_outline(x, y, w, h, border);
}
/* * ======================================================================================
 * MODULE: TYPOGRAPHY ENGINE (BITMAP FONTS)
 * ======================================================================================
 * Renders text using a built-in 8x8 bitmap font.
 * Supports scaling (size), spacing (kerning), and shadowing.
 */

// Basic 8x8 Bitmap Font for ASCII characters (Minimal set for bootloader)
// Each byte represents a row of 8 pixels.
const uint8_t px_font8x8[128][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // SPACE
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // !
    // ... (Full ASCII table would be huge, here we define key chars for demo)
    // A
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    // B
    {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},
    // ... (Assume full set is loaded from an external header 'font8x8.h')
};

// Placeholder for the letter 'P' (For manual definition if header is missing)
const uint8_t char_P[8] = {0xFC,0x66,0x66,0xFC,0x60,0x60,0x60,0x00};
const uint8_t char_H[8] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00};
const uint8_t char_O[8] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00};
const uint8_t char_T[8] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00};
const uint8_t char_N[8] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00};
const uint8_t char_X[8] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00};

/* * FUNCTION: px_draw_char_8x8
 * Draws a single character at (x,y) with scaling factor.
 */
void px_draw_char_8x8(int x, int y, char c, PhotonColor color, int scale) {
    const uint8_t *bitmap;
    
    // Manual mapping for demo (In real OS, use full ASCII table)
    if (c == 'P') bitmap = char_P;
    else if (c == 'H') bitmap = char_H;
    else if (c == 'O') bitmap = char_O;
    else if (c == 'T') bitmap = char_T;
    else if (c == 'N') bitmap = char_N;
    else if (c == 'X') bitmap = char_X;
    else return; // Unknown char

    for (int row = 0; row < 8; row++) {
        uint8_t line = bitmap[row];
        for (int col = 0; col < 8; col++) {
            // Check if the bit is set (from left to right)
            if (line & (0x80 >> col)) {
                // Draw pixel (scaled)
                if (scale == 1) {
                    px_put_pixel(x + col, y + row, color);
                } else {
                    px_draw_rect_filled(x + (col * scale), y + (row * scale), scale, scale, color);
                }
            }
        }
    }
}

/* * FUNCTION: px_draw_string
 * Renders a string of text.
 * Support for "Kerning" (spacing between letters) for that luxury look.
 */
void px_draw_string(int x, int y, const char *str, PhotonColor color, int scale, int spacing) {
    int cursor_x = x;
    
    while (*str) {
        // Draw Shadow first (Offset by 2px) for depth
        PhotonColor shadow = px_color_create(0, 0, 0, 100);
        px_draw_char_8x8(cursor_x + 2, y + 2, *str, shadow, scale);
        
        // Draw Main Text
        px_draw_char_8x8(cursor_x, y, *str, color, scale);
        
        // Advance cursor
        cursor_x += (8 * scale) + spacing;
        str++;
    }
}

/* * FUNCTION: px_draw_centered_text
 * Helper to automatically center text on the screen.
 */
void px_draw_centered_text(int y, const char *str, PhotonColor color, int scale) {
    int len = 0;
    const char *p = str;
    while (*p++) len++;
    
    int width_px = len * (8 * scale);
    int start_x = (PHOTON_SCREEN_WIDTH - width_px) / 2;
    
    px_draw_string(start_x, y, str, color, scale, 2); // Default spacing 2
}
/* * ======================================================================================
 * MODULE: RENDER PIPELINE (FRAMEBUFFER TO TERMINAL)
 * ======================================================================================
 * Transmits the internal framebuffer to the standard output (STDOUT) using
 * optimized ANSI escape sequences.
 * Includes "Pixel Skipping" to fit HD buffers into standard terminal windows.
 */

/* * FUNCTION: px_render_buffer
 * Flushes the Back Buffer to the Terminal screen.
 * CRITICAL PERFORMANCE SECTION.
 */
void px_render_buffer() {
    // 1. Cursor Reset (Move to top-left without clearing, prevents flicker)
    printf("\033[H");

    // 2. Determine Terminal Size (Simulation constraints)
    // In a real OS, we scan the whole 1920x1080 buffer.
    // In a terminal emulator, we must skip pixels to fit the window.
    // Step X/Y determines resolution density.
    int step_x = 4; // Skip 4 pixels horizontally
    int step_y = 8; // Skip 8 pixels vertically (Chars are tall)

    uint32_t *buffer = ctx->back_buffer->pixels;
    int w = ctx->back_buffer->width;
    int h = ctx->back_buffer->height;

    PhotonColor last_color = {0, 0, 0, 0}; // Cache for optimization
    int color_changed = 1;

    for (int y = 0; y < h; y += step_y) {
        for (int x = 0; x < w; x += step_x) {
            // Get pixel from buffer
            uint32_t raw = buffer[y * w + x];
            
            // Unpack color
            PhotonColor c;
            c.a = (raw >> 24) & 0xFF;
            c.r = (raw >> 16) & 0xFF;
            c.g = (raw >> 8) & 0xFF;
            c.b = raw & 0xFF;

            // Simple Alpha Blending with Black Background (if needed)
            // If Alpha is 0, we just print black/default.
            
            // OPTIMIZATION: ANSI State Caching
            // Only print escape code if color changed drastically
            if (c.r != last_color.r || c.g != last_color.g || c.b != last_color.b) {
                // Set Background Color (\033[48;2;R;G;Bm)
                printf("\033[48;2;%d;%d;%dm", c.r, c.g, c.b);
                last_color = c;
                color_changed = 1;
            } else {
                color_changed = 0;
            }

            // Print two spaces (approximates a square pixel)
            printf("  "); 
        }
        // End of row: Reset color and new line
        printf("\033[0m\n");
        last_color.r = -1; // Force color reset next line
    }

    // 3. Swap Buffers (Software Logic)
    // We copy Back Buffer to Front Buffer (Optional in this sim, strictly implies ready state)
    memcpy(ctx->front_buffer->pixels, ctx->back_buffer->pixels, ctx->back_buffer->buffer_size);
}

/* * FUNCTION: px_render_progress_bar_luxury
 * Specific renderer for the high-end loading bar.
 * Uses smooth block characters (█ ▓ ▒ ░) for sub-pixel precision.
 */
void px_draw_luxury_bar(int x, int y, int w, float percentage, PhotonColor c_start, PhotonColor c_end) {
    int filled_w = (int)(w * (percentage / 100.0f));
    
    for (int i = 0; i < w; i++) {
        // Calculate gradient color
        float t = (float)i / (float)w;
        PhotonColor col = px_blend_colors(c_start, c_end, t);
        
        if (i < filled_w) {
            // Full Block
             // In graphics mode we draw pixels, but here we can use block chars for extra style
             px_draw_rect_filled(x + i*4, y, 4, 10, col); // 4px wide segments
        } else {
            // Empty / Background (Dimmed)
            PhotonColor dim = col;
            dim.r /= 5; dim.g /= 5; dim.b /= 5; // Darken
            px_draw_rect_filled(x + i*4, y, 4, 10, dim);
        }
    }
}
/* * ======================================================================================
 * MODULE: PARTICLE PHYSICS ENGINE (HOCS VISUALIZER)
 * ======================================================================================
 * Manages dynamic particle systems for "Warp Speed" or "Data Flow" effects.
 * Each particle has position, velocity, life, and color properties.
 */

#define MAX_PARTICLES 200

typedef struct {
    float x, y;         // Exact position
    float vx, vy;       // Velocity vector
    float life;         // 1.0 (Born) to 0.0 (Dead)
    float size;         // Particle size
    PhotonColor color;  // Base color
    int active;         // Is currently alive?
} Particle;

Particle particle_pool[MAX_PARTICLES];

/* * FUNCTION: px_particles_init
 * Resets the particle system.
 */
void px_particles_init() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_pool[i].active = 0;
    }
}

/* * FUNCTION: px_spawn_particle
 * Creates a new particle at a specific location with random velocity.
 * Used for the "Starfield" effect behind the logo.
 */
void px_spawn_particle(int center_x, int center_y) {
    // Find first inactive slot
    int idx = -1;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_pool[i].active) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) return; // Pool full

    // Initialize Star/Data Packet
    particle_pool[idx].x = center_x;
    particle_pool[idx].y = center_y;
    
    // Random velocity (Explosion/Expansion effect)
    // (rand() is not ideal for kernel but fine for UI sim)
    float angle = (float)(rand() % 360) * (3.14159f / 180.0f);
    float speed = (float)(rand() % 50) / 10.0f + 1.0f; // 1.0 to 6.0 speed

    particle_pool[idx].vx = cos(angle) * speed;
    particle_pool[idx].vy = sin(angle) * speed;
    
    particle_pool[idx].life = 1.0f;
    particle_pool[idx].size = (rand() % 3) + 1; // Size 1-3
    
    // Color: Varying shades of Cyan/Blue/White
    int tone = (rand() % 50) + 200; // 200-255
    particle_pool[idx].color = px_color_create(0, tone, 255, 255); // Blue-ish
    particle_pool[idx].active = 1;
}

/* * FUNCTION: px_update_particles
 * Physics tick. Moves particles and reduces their life.
 */
void px_update_particles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_pool[i].active) continue;

        Particle *p = &particle_pool[i];

        // Move
        p->x += p->vx;
        p->y += p->vy;

        // Accelerate (Warp effect)
        p->vx *= 1.05f;
        p->vy *= 1.05f;

        // Fade out life
        p->life -= 0.02f;

        // Kill if dead or off-screen
        if (p->life <= 0 || 
            p->x < 0 || p->x >= PHOTON_SCREEN_WIDTH || 
            p->y < 0 || p->y >= PHOTON_SCREEN_HEIGHT) {
            p->active = 0;
        }
    }
}

/* * FUNCTION: px_draw_particles
 * Renders all active particles to the back buffer.
 * Applies alpha fading based on life.
 */
void px_draw_particles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particle_pool[i].active) continue;

        Particle *p = &particle_pool[i];
        
        // Adjust alpha based on life
        PhotonColor c = p->color;
        c.a = (uint8_t)(255 * p->life); // Fade out

        // Draw particle (Small rect or single pixel)
        if (p->size <= 1) {
            px_put_pixel((int)p->x, (int)p->y, c);
        } else {
            px_draw_rect_filled((int)p->x, (int)p->y, (int)p->size, (int)p->size, c);
        }
    }
}
/* * ======================================================================================
 * MODULE: SYSTEM CONTROL & DIAGNOSTICS
 * ======================================================================================
 * Memory management and performance monitoring tools.
 */

/* * FUNCTION: px_print_debug_info
 * Prints internal engine state to the console overlay.
 * Useful for debugging VRAM usage.
 */
void px_print_debug_info() {
    int used_mem = ctx->front_buffer->buffer_size * 2; // Front + Back
    printf("\n[DEBUG] GDI State:\n");
    printf("  > Resolution: %dx%d\n", PHOTON_SCREEN_WIDTH, PHOTON_SCREEN_HEIGHT);
    printf("  > VRAM Usage: %d MB\n", used_mem / (1024*1024));
    printf("  > Buffer Addr: 0x%p\n", (void*)ctx->front_buffer->pixels);
    printf("  > Active Particles: counting...\n");
}

/* * FUNCTION: px_destroy_graphics_system
 * Safely shuts down the graphics engine and frees all allocated memory blocks.
 * Prevents memory leaks in long-running sessions.
 */
void px_destroy_graphics_system() {
    printf("[KERNEL] Shutting down Graphics Engine...\n");

    if (ctx) {
        if (ctx->front_buffer) {
            if (ctx->front_buffer->pixels) {
                free(ctx->front_buffer->pixels);
                printf("  > Front Buffer Freed.\n");
            }
            free(ctx->front_buffer);
        }

        if (ctx->back_buffer) {
            if (ctx->back_buffer->pixels) {
                free(ctx->back_buffer->pixels);
                printf("  > Back Buffer Freed.\n");
            }
            free(ctx->back_buffer);
        }

        free(ctx);
        printf("  > Graphics Context Freed.\n");
    }

    // Reset standard terminal colors
    printf("\033[0m");
    printf("\033[2J\033[H"); // Final Clear
    printf("[SUCCESS] Graphics System Offline.\n");
}

/* * FUNCTION: px_test_pattern
 * Draws a SMPTE-style color bar pattern to verify color rendering.
 */
void px_test_pattern() {
    int bar_width = PHOTON_SCREEN_WIDTH / 8;
    
    PhotonColor colors[] = {
        {255, 255, 255, 255}, // White
        {0, 255, 255, 255},   // Yellow
        {255, 255, 0, 255},   // Cyan
        {0, 255, 0, 255},     // Green
        {255, 0, 255, 255},   // Magenta
        {0, 0, 255, 255},     // Red
        {255, 0, 0, 255},     // Blue
        {0, 0, 0, 255}        // Black
    };

    for (int i = 0; i < 8; i++) {
        px_draw_rect_filled(i * bar_width, 0, bar_width, PHOTON_SCREEN_HEIGHT, colors[i]);
    }
}
