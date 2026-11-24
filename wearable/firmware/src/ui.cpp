#include "ui.h"

#include <Arduino.h>
#include <TouchDrvCSTXXX.hpp>
#include <Wire.h>
#include <lvgl.h>

#include "display.h"
#include "pin_config.h"
#include "prompts.h"
#include "wifi_setup.h"
#include "worker.h"

namespace {

TouchDrvCST92xx touch;
bool touch_ok = false;

lv_obj_t *home = nullptr;
lv_obj_t *chat = nullptr;
lv_obj_t *status_label = nullptr;
lv_obj_t *chat_label = nullptr;
lv_obj_t *chat_box = nullptr;
lv_obj_t *ring = nullptr;
lv_obj_t *orb = nullptr;
lv_obj_t *dots[3] = {nullptr, nullptr, nullptr};

lv_color_t *draw_buf = nullptr;
int spin = 0;
bool listening = false;

struct Line {
    char who[12];
    char text[240];
};

Line lines[8];
int line_count = 0;

void styleScreen(lv_obj_t *obj, uint32_t color) {
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void renderChat() {
    if (line_count == 0) {
        lv_label_set_text(chat_label,
                          "Replies land here.\n\nTap a prompt on the face, or tap Talk and speak.");
        return;
    }
    static char block[8 * 280];
    size_t used = 0;
    block[0] = '\0';
    for (int i = 0; i < line_count; i++) {
        int wrote = snprintf(block + used, sizeof(block) - used, "%s: %s\n\n", lines[i].who, lines[i].text);
        if (wrote < 0 || (size_t)wrote >= sizeof(block) - used) {
            break;
        }
        used += (size_t)wrote;
    }
    lv_label_set_text(chat_label, block);
    lv_obj_scroll_to_y(chat_box, LV_COORD_MAX, LV_ANIM_OFF);
}

void pushLine(const char *who, const char *text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    if (line_count == 8) {
        memmove(&lines[0], &lines[1], sizeof(Line) * 7);
        line_count = 7;
    }
    strncpy(lines[line_count].who, who, sizeof(lines[line_count].who) - 1);
    lines[line_count].who[sizeof(lines[line_count].who) - 1] = '\0';
    strncpy(lines[line_count].text, text, sizeof(lines[line_count].text) - 1);
    lines[line_count].text[sizeof(lines[line_count].text) - 1] = '\0';
    line_count++;
    renderChat();
}

void showChat() {
    lv_obj_add_flag(home, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(chat, LV_OBJ_FLAG_HIDDEN);
}

void showHome() {
    lv_obj_add_flag(chat, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(home, LV_OBJ_FLAG_HIDDEN);
}

void setStatus(const char *text) {
    lv_label_set_text(status_label, text);
}

void onPrompt(lv_event_t *event) {
    if (workerBusy()) {
        return;
    }
    const JarvisPrompt *prompt = (const JarvisPrompt *)lv_event_get_user_data(event);
    if (prompt == nullptr) {
        return;
    }
    pushLine("You", prompt->message);
    showChat();
    setStatus("Asking Jarvis...");
    workerRequestChat(prompt->message);
}

void onTalk(lv_event_t *event) {
    (void)event;
    if (!listening) {
        if (workerBusy()) {
            return;
        }
        listening = true;
        setStatus("Listening... tap Talk to send");
        workerRequestListen();
        showChat();
        return;
    }
    workerStopListen();
    setStatus("Sending audio...");
    listening = false;
}

void onOpenChat(lv_event_t *event) {
    (void)event;
    showChat();
}

void onBack(lv_event_t *event) {
    (void)event;
    showHome();
}

void animate(lv_timer_t *timer) {
    (void)timer;
    spin = (spin + 4) % 360;
    lv_arc_set_rotation(ring, spin);

    int sweep = 120 + (lv_trigo_sin(spin) * 12) / 32767;
    int size = 104 + sweep / 40;
    lv_obj_set_size(orb, size, size);
    lv_obj_align(orb, LV_ALIGN_CENTER, 0, -58);

    for (int i = 0; i < 3; i++) {
        int angle = (spin * (i == 1 ? -1 : 1) + i * 120 + 360) % 360;
        int radius = 64 + i * 6;
        int x = (lv_trigo_sin(angle + 90) * radius) / 32767;
        int y = (lv_trigo_sin(angle) * radius) / 32767;
        lv_obj_align(dots[i], LV_ALIGN_CENTER, x, y - 58);
    }
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user, int x, int y, int w,
                     uint32_t border) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, 40);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x10192A), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1C3148), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, 20, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xE8F4FF), 0);
    lv_obj_center(label);
    return btn;
}

void rounderCb(lv_disp_drv_t *disp, lv_area_t *area) {
    (void)disp;
    // The CO5300 wants even/odd edges. This is the same adjustment as
    // Waveshare's 05_LVGL_Widgets demo, plus a clamp so we stay on the panel.
    if (area->x1 % 2 != 0) {
        area->x1--;
    }
    if (area->y1 % 2 != 0) {
        area->y1--;
    }
    if (area->x2 % 2 == 0) {
        area->x2++;
    }
    if (area->y2 % 2 == 0) {
        area->y2++;
    }
    if (area->x1 < 0) {
        area->x1 = 0;
    }
    if (area->y1 < 0) {
        area->y1 = 0;
    }
    if (area->x2 >= LCD_WIDTH) {
        area->x2 = LCD_WIDTH - 1;
    }
    if (area->y2 >= LCD_HEIGHT) {
        area->y2 = LCD_HEIGHT - 1;
    }
}

void flushCb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
#if LV_COLOR_16_SWAP
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
    lv_disp_flush_ready(disp);
}

void touchCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;
    if (!touch_ok) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    int16_t x[5];
    int16_t y[5];
    uint8_t n = touch.getPoint(x, y, touch.getSupportTouchPoint());
    if (n > 0) {
        data->point.x = x[0];
        data->point.y = y[0];
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void startTouch() {
    pinMode(TP_RST, OUTPUT);
    digitalWrite(TP_RST, LOW);
    delay(30);
    digitalWrite(TP_RST, HIGH);
    delay(50);
    touch.setPins(TP_RST, TP_INT);
    touch_ok = touch.begin(Wire, 0x5A, IIC_SDA, IIC_SCL);
    if (!touch_ok) {
        Serial.println("Touch controller did not answer. The face will still draw.");
        return;
    }
    // Official 05_LVGL_Widgets demo mirrors both axes and sets 466x466.
    touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
    touch.setMirrorXY(true, true);
    Serial.print("Touch: ");
    Serial.println(touch.getModelName());
}

void buildHome() {
    home = lv_obj_create(lv_scr_act());
    styleScreen(home, 0x070B12);

    ring = lv_arc_create(home);
    lv_obj_set_size(ring, 210, 210);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, -58);
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_arc_set_angles(ring, 0, 70);
    lv_arc_set_rotation(ring, 270);
    lv_obj_remove_style(ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(ring, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ring, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0x163044), LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0x4DE2FF), LV_PART_INDICATOR);

    orb = lv_obj_create(home);
    lv_obj_set_size(orb, 112, 112);
    lv_obj_align(orb, LV_ALIGN_CENTER, 0, -58);
    lv_obj_set_style_radius(orb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(orb, lv_color_hex(0x0E2A3A), 0);
    lv_obj_set_style_bg_grad_color(orb, lv_color_hex(0x123848), 0);
    lv_obj_set_style_bg_grad_dir(orb, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(orb, lv_color_hex(0xE6B35A), 0);
    lv_obj_set_style_border_width(orb, 3, 0);
    lv_obj_set_style_shadow_color(orb, lv_color_hex(0x4DE2FF), 0);
    lv_obj_set_style_shadow_width(orb, 28, 0);
    lv_obj_set_style_shadow_opa(orb, LV_OPA_50, 0);
    lv_obj_clear_flag(orb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(orb, LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i < 3; i++) {
        dots[i] = lv_obj_create(home);
        lv_obj_set_size(dots[i], 10, 10);
        lv_obj_set_style_radius(dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dots[i], lv_color_hex(i == 2 ? 0xE6B35A : 0x7AF0FF), 0);
        lv_obj_set_style_border_width(dots[i], 0, 0);
        lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *title = lv_label_create(home);
    lv_label_set_text(title, "JARVIS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE6B35A), 0);
    lv_obj_set_style_text_letter_space(title, 4, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);

    status_label = lv_label_create(home);
    lv_label_set_text(status_label, "Starting");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x9BB4C9), 0);
    lv_obj_set_width(status_label, 300);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 58);

    makeButton(home, kPrompts[0].label, onPrompt, (void *)&kPrompts[0], 78, 312, 140, 0x4DE2FF);
    makeButton(home, kPrompts[1].label, onPrompt, (void *)&kPrompts[1], 248, 312, 140, 0x4DE2FF);
    makeButton(home, kPrompts[2].label, onPrompt, (void *)&kPrompts[2], 78, 360, 140, 0x4DE2FF);
    makeButton(home, kPrompts[3].label, onPrompt, (void *)&kPrompts[3], 248, 360, 140, 0x4DE2FF);
    makeButton(home, "Talk", onTalk, nullptr, 78, 408, 140, 0xE6B35A);
    makeButton(home, "Chat", onOpenChat, nullptr, 248, 408, 140, 0xE6B35A);

    lv_timer_create(animate, 40, nullptr);
}

void buildChat() {
    chat = lv_obj_create(lv_scr_act());
    styleScreen(chat, 0x070B12);
    lv_obj_add_flag(chat, LV_OBJ_FLAG_HIDDEN);

    makeButton(chat, "Back", onBack, nullptr, 40, 28, 100, 0x4DE2FF);

    lv_obj_t *heading = lv_label_create(chat);
    lv_label_set_text(heading, "Conversation");
    lv_obj_set_style_text_font(heading, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(heading, lv_color_hex(0xE6B35A), 0);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 30, 36);

    chat_box = lv_obj_create(chat);
    lv_obj_set_size(chat_box, 380, 300);
    lv_obj_align(chat_box, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(chat_box, lv_color_hex(0x101826), 0);
    lv_obj_set_style_border_color(chat_box, lv_color_hex(0x245066), 0);
    lv_obj_set_style_border_width(chat_box, 1, 0);
    lv_obj_set_style_radius(chat_box, 24, 0);
    lv_obj_set_style_pad_all(chat_box, 14, 0);
    lv_obj_set_scrollbar_mode(chat_box, LV_SCROLLBAR_MODE_AUTO);

    chat_label = lv_label_create(chat_box);
    lv_obj_set_width(chat_label, 340);
    lv_label_set_long_mode(chat_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(chat_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(chat_label, lv_color_hex(0xE7F2FA), 0);
    renderChat();
}

}  // namespace

void uiBegin() {
    startTouch();
    lv_init();

    const int lines = 24;
    size_t pixels = (size_t)LCD_WIDTH * lines;
    draw_buf = (lv_color_t *)heap_caps_malloc(pixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (draw_buf == nullptr) {
        draw_buf = (lv_color_t *)heap_caps_malloc(pixels * sizeof(lv_color_t), MALLOC_CAP_8BIT);
    }
    if (draw_buf == nullptr) {
        Serial.println("No memory for the screen buffer.");
        return;
    }

    static lv_disp_draw_buf_t buf;
    lv_disp_draw_buf_init(&buf, draw_buf, nullptr, pixels);

    static lv_disp_drv_t disp;
    lv_disp_drv_init(&disp);
    disp.hor_res = LCD_WIDTH;
    disp.ver_res = LCD_HEIGHT;
    disp.flush_cb = flushCb;
    disp.rounder_cb = rounderCb;
    disp.draw_buf = &buf;
    lv_disp_drv_register(&disp);

    static lv_indev_drv_t indev;
    lv_indev_drv_init(&indev);
    indev.type = LV_INDEV_TYPE_POINTER;
    indev.read_cb = touchCb;
    lv_indev_drv_register(&indev);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x070B12), 0);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    buildHome();
    buildChat();
    Serial.println("Face is up.");
}

void uiLoop() {
    if (draw_buf == nullptr) {
        return;
    }
    lv_timer_handler();

    char status[72];
    if (workerTakeStatus(status, sizeof(status)) && !listening) {
        setStatus(status);
    }

    bool ok = false;
    char heard[180];
    char answer[480];
    if (workerTakeResult(&ok, heard, sizeof(heard), answer, sizeof(answer))) {
        listening = false;
        if (heard[0] != '\0') {
            pushLine("You", heard);
        }
        pushLine("Jarvis", answer);
        showChat();
        (void)ok;
    }
}
