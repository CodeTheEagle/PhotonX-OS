/* * ======================================================================================
 * MODULE: PHOTONX BOOT MANAGER (UEFI COMPLIANT UI)
 * ======================================================================================
 * Advanced boot selection interface with hardware diagnostics visualization.
 * Features:
 * - Glassmorphism UI Panel
 * - Real-time Hardware Telemetry (Simulated)
 * - Auto-boot Countdown
 * - Secure Boot Verification Indicators
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../utils/graphics.c" // Grafik motorunu dahil et

// --- YAPILANDIRMA ---
#define MENU_WIDTH      700
#define MENU_HEIGHT     450
#define ITEM_HEIGHT     50
#define AUTO_BOOT_SEC   5

// Boot Seçenekleri (Daha profesyonel isimler)
const char *boot_items[] = {
    "PhotonX OS [Kernel v1.0.4-HOCS]",
    "PhotonX Safe Mode (No GUI)",
    "HOCS Hardware Diagnostic Tool",
    "Network Boot (PXE / Optical Bridge)",
    "UEFI Firmware Settings",
    "System Shutdown"
};

// Donanım Durumu (Simülasyon Verileri)
typedef struct {
    float cpu_temp;
    float voltage;
    int fan_rpm;
    int secure_boot;
} SystemHealth;

/* * FUNCTION: px_get_system_health
 * Simulates reading sensors from the motherboard via I2C/SMBus.
 */
SystemHealth px_get_system_health() {
    SystemHealth h;
    // Hafif rastgelelik ekle (Canlı hissettirsin)
    h.cpu_temp = 34.0f + ((rand() % 20) / 10.0f); 
    h.voltage = 1.18f + ((rand() % 5) / 100.0f);
    h.fan_rpm = 1200 + (rand() % 50);
    h.secure_boot = 1; // 1 = Enabled
    return h;
}

/* * FUNCTION: px_draw_telemetry_bar
 * Draws the bottom strip containing live sensor data.
 */
void px_draw_telemetry_bar(int x, int y, int w) {
    SystemHealth health = px_get_system_health();
    
    // Arka Plan (Koyu Gri Şerit)
    px_draw_rect_filled(x, y, w, 30, px_color_create(20, 20, 25, 230));
    
    // Çizgi (Ayırıcı)
    px_draw_line(x, y, x + w, y, px_color_create(50, 50, 60, 255));

    // Sensör Verilerini Hazırla (String Formatlama)
    char buffer[128];
    
    // 1. CPU TEMP
    sprintf(buffer, "CPU: %.1f C", health.cpu_temp);
    PhotonColor col_temp = (health.cpu_temp > 45) ? px_color_create(255, 100, 0, 255) : px_color_create(0, 255, 150, 255);
    px_draw_string(x + 20, y + 8, buffer, col_temp, 1, 1);

    // 2. VOLTAGE
    sprintf(buffer, "VCORE: %.3fV", health.voltage);
    px_draw_string(x + 150, y + 8, buffer, px_color_create(0, 200, 255, 255), 1, 1);

    // 3. FAN SPEED
    sprintf(buffer, "FAN: %d RPM", health.fan_rpm);
    px_draw_string(x + 280, y + 8, buffer, px_color_create(150, 150, 150, 255), 1, 1);

    // 4. SECURE BOOT
    if (health.secure_boot) {
        px_draw_string(x + w - 120, y + 8, "SECURE BOOT", px_color_create(0, 255, 0, 255), 1, 1);
        // Kilit ikonu niyetine küçük bir kare
        px_draw_rect_filled(x + w - 135, y + 8, 8, 8, px_color_create(0, 255, 0, 255));
    } else {
        px_draw_string(x + w - 120, y + 8, "UNSECURE", px_color_create(255, 0, 0, 255), 1, 1);
    }
}

/* * FUNCTION: px_draw_boot_item
 * Renders a single selectable row with hover effects.
 */
void px_draw_boot_item(int x, int y, int w, int id, int selected) {
    int padding_left = 30;
    
    if (selected) {
        // --- SEÇİLİ DURUM (ACTIVE) ---
        
        // 1. Arka Plan (Neon Gradyan)
        PhotonColor c_start = px_color_create(0, 120, 215, 180); // Windows/Tech Blue
        PhotonColor c_end   = px_color_create(0, 20, 40, 50);    // Fade out
        px_draw_gradient_rect_h(x, y, w, ITEM_HEIGHT, c_start, c_end);
        
        // 2. Sol Kenar Çubuğu (Glow)
        px_draw_rect_filled(x, y, 4, ITEM_HEIGHT, px_color_create(0, 255, 255, 255));
        
        // 3. Yazı (Parlak Beyaz ve Kalın hissi için gölgeli)
        px_draw_string(x + padding_left, y + 16, boot_items[id], px_color_create(255, 255, 255, 255), 1, 2);
        
        // 4. Sağ Tarafa Ok İşareti ">"
        px_draw_string(x + w - 40, y + 16, ">", px_color_create(0, 255, 255, 255), 1, 2);

    } else {
        // --- NORMAL DURUM (PASSIVE) ---
        
        // Sadece hafif gri yazı
        px_draw_string(x + padding_left, y + 16, boot_items[id], px_color_create(140, 140, 150, 255), 1, 1);
    }
}

/* * FUNCTION: px_draw_countdown_bar
 * Visualizes the auto-boot timer at the bottom of the panel.
 */
void px_draw_countdown_bar(int x, int y, int w, int remaining_frames, int total_frames) {
    // Çerçeve
    px_draw_rect_outline(x, y, w, 4, px_color_create(50, 50, 50, 255));
    
    // Doluluk Oranı
    float percent = (float)remaining_frames / (float)total_frames;
    int fill_w = (int)(w * percent);
    
    // Renk (Zaman azaldıkça kırmızıya döner)
    PhotonColor bar_col;
    if (percent > 0.5) bar_col = px_color_create(0, 255, 0, 255); // Yeşil
    else if (percent > 0.2) bar_col = px_color_create(255, 200, 0, 255); // Sarı
    else bar_col = px_color_create(255, 0, 0, 255); // Kırmızı
    
    px_draw_rect_filled(x, y, fill_w, 4, bar_col);
    
    char buf[32];
    sprintf(buf, "Auto-boot in %.1fs", (remaining_frames * 0.05)); // 50ms per frame varsayımı
    px_draw_string(x, y + 10, buf, px_color_create(100, 100, 100, 255), 1, 1);
}

/* * FUNCTION: px_boot_manager_main
 * The main loop for the boot selection screen.
 * Handles input simulation and rendering.
 */
int px_boot_manager_main() {
    int selected_idx = 0;
    int total_items = 6;
    
    int panel_x = (PHOTON_SCREEN_WIDTH - MENU_WIDTH) / 2;
    int panel_y = (PHOTON_SCREEN_HEIGHT - MENU_HEIGHT) / 2;
    
    // Auto-boot Timer (300 frames * 20ms = ~6 seconds)
    int timer_frames = 300;
    int current_frame = timer_frames;
    int boot_aborted = 0; // Tuşa basarsa geri sayım durur

    // Parçacık Sistemi Başlat (Arka plan yıldızları)
    px_particles_init();
    for(int i=0; i<80; i++) px_spawn_particle(rand()%1920, rand()%1080);

    printf("[KERNEL] Entering Boot Manager GUI...\n");

    // --- MAIN RENDER LOOP ---
    // (Simülasyon için 500 döngü, gerçekte sonsuz döngü olur)
    while (current_frame > 0 || boot_aborted) {
        
        // 1. Ekranı Temizle (Hafif bir mor-siyah arka plan izi bırakarak)
        // px_clear_screen(px_color_create(5, 5, 10, 255));
        // (Performans için tam temizleme yerine üzerine çiziyoruz)
        
        // 2. Arka Plan Efektlerini Güncelle
        px_update_particles();
        px_draw_particles(); // Yıldızları çiz

        // 3. ANA PANEL (Glassmorphism)
        // Yarı saydam gövde
        px_draw_glass_panel(panel_x, panel_y, MENU_WIDTH, MENU_HEIGHT, px_color_create(15, 15, 20, 220));
        // İnce beyaz çerçeve (Border)
        px_draw_rect_outline(panel_x, panel_y, MENU_WIDTH, MENU_HEIGHT, px_color_create(255, 255, 255, 30));

        // 4. BAŞLIK BÖLÜMÜ
        // Logo
        px_draw_string(panel_x + 30, panel_y + 30, "PHOTON", px_color_create(255, 255, 255, 255), 2, 4);
        px_draw_string(panel_x + 160, panel_y + 30, "X", px_color_create(0, 200, 255, 255), 2, 4);
        
        // Alt Başlık
        px_draw_string(panel_x + 30, panel_y + 60, "HYBRID OPTICAL BOOT MANAGER v3.0", px_color_create(100, 100, 100, 255), 1, 1);
        
        // Ayırıcı Çizgi
        px_draw_line(panel_x, panel_y + 80, panel_x + MENU_WIDTH, panel_y + 80, px_color_create(255, 255, 255, 20));

        // 5. MENÜ LİSTESİ
        int list_start_y = panel_y + 100;
        for (int i = 0; i < total_items; i++) {
            px_draw_boot_item(panel_x + 20, list_start_y + (i * ITEM_HEIGHT), MENU_WIDTH - 40, i, (i == selected_idx));
        }

        // 6. TELEMETRİ VE ZAMANLAYICI
        int footer_y = panel_y + MENU_HEIGHT - 60;
        
        if (!boot_aborted) {
            // Geri Sayım Çubuğu
            px_draw_countdown_bar(panel_x + 30, footer_y - 20, MENU_WIDTH - 60, current_frame, timer_frames);
            current_frame--;
        } else {
             px_draw_centered_text(footer_y - 20, "Auto-boot stopped. Select manually.", px_color_create(255, 200, 0, 255), 1);
        }

        // En alttaki sensör verileri
        px_draw_telemetry_bar(panel_x, panel_y + MENU_HEIGHT - 30, MENU_WIDTH);

        // 7. EKRANA BAS (Render)
        px_render_buffer();

        // 8. GİRİŞ SİMÜLASYONU (Input Simulation)
        // Gerçek klavye olmadığı için rastgele tuş basımı simüle ediyoruz
        // Her 20 karede bir tuşa basılmış gibi yap
        if (rand() % 40 == 0) { 
            boot_aborted = 1; // Tuşa basıldığı an geri sayım durur
            selected_idx++;
            if (selected_idx >= total_items) selected_idx = 0;
        }

        // Bekle (FPS Limiti)
        delay_ms(50);
        
        // Simülasyon çıkış koşulu (Örnek: 300 kare sonra döngüyü kır)
        static int global_counter = 0;
        if (global_counter++ > 300 && !boot_aborted) break; // Auto-boot tetiklendi
    }

    return selected_idx; // Seçilen OS ID'sini döndür
}
