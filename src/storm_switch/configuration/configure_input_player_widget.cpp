// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QTimer>
#include <QWheelEvent>

#include "hid_core/frontend/emulated_controller.h"
#include "storm_switch/configuration/configure_input_player_widget.h"

namespace {
constexpr float PI_CONST = 3.14159265358979323846f;

constexpr std::array<float, 36 * 2> pro_left_trigger = {
    -65.2f,  -132.6f, -68.2f,  -134.1f, -71.3f,  -135.5f, -74.4f,  -136.7f, -77.6f,
    -137.6f, -80.9f,  -138.1f, -84.3f,  -138.3f, -87.6f,  -138.3f, -91.0f,  -138.1f,
    -94.3f,  -137.8f, -97.6f,  -137.3f, -100.9f, -136.7f, -107.5f, -135.3f, -110.7f,
    -134.5f, -120.4f, -131.8f, -123.6f, -130.8f, -126.8f, -129.7f, -129.9f, -128.5f,
    -132.9f, -127.1f, -135.9f, -125.6f, -138.8f, -123.9f, -141.6f, -122.0f, -144.1f,
    -119.8f, -146.3f, -117.3f, -148.4f, -114.7f, -150.4f, -112.0f, -152.3f, -109.2f,
    -155.3f, -104.0f, -152.0f, -104.3f, -148.7f, -104.5f, -145.3f, -104.8f, -35.5f,
    -117.2f, -38.5f,  -118.7f, -41.4f,  -120.3f, -44.4f,  -121.8f, -50.4f,  -124.9f,
};

constexpr std::array<float, 14 * 2> pro_body_top = {
    0.0f,   -115.4f, -4.4f,  -116.1f, -69.7f, -131.3f, -66.4f, -131.9f, -63.1f, -132.3f,
    -56.4f, -133.0f, -53.1f, -133.3f, -49.8f, -133.5f, -43.1f, -133.8f, -39.8f, -134.0f,
    -36.5f, -134.1f, -16.4f, -134.4f, -13.1f, -134.4f, 0.0f,   -134.1f,
};

constexpr std::array<float, 145 * 2> pro_left_handle = {
    -178.7f, -47.5f, -179.0f, -46.1f, -179.3f, -44.6f, -182.0f, -29.8f, -182.3f, -28.4f,
    -182.6f, -26.9f, -182.8f, -25.4f, -183.1f, -23.9f, -183.3f, -22.4f, -183.6f, -21.0f,
    -183.8f, -19.5f, -184.1f, -18.0f, -184.3f, -16.5f, -184.6f, -15.1f, -184.8f, -13.6f,
    -185.1f, -12.1f, -185.3f, -10.6f, -185.6f, -9.1f,  -185.8f, -7.7f,  -186.1f, -6.2f,
    -186.3f, -4.7f,  -186.6f, -3.2f,  -186.8f, -1.7f,  -187.1f, -0.3f,  -187.3f, 1.2f,
    -187.6f, 2.7f,   -187.8f, 4.2f,   -188.3f, 7.1f,   -188.5f, 8.6f,   -188.8f, 10.1f,
    -189.0f, 11.6f,  -189.3f, 13.1f,  -189.5f, 14.5f,  -190.0f, 17.5f,  -190.2f, 19.0f,
    -190.5f, 20.5f,  -190.7f, 21.9f,  -191.2f, 24.9f,  -191.4f, 26.4f,  -191.7f, 27.9f,
    -191.9f, 29.3f,  -192.4f, 32.3f,  -192.6f, 33.8f,  -193.1f, 36.8f,  -193.3f, 38.2f,
    -193.8f, 41.2f,  -194.0f, 42.7f,  -194.7f, 47.1f,  -194.9f, 48.6f,  -199.0f, 82.9f,
    -199.1f, 84.4f,  -199.1f, 85.9f,  -199.2f, 87.4f,  -199.2f, 88.9f,  -199.1f, 94.9f,
    -198.9f, 96.4f,  -198.8f, 97.8f,  -198.5f, 99.3f,  -198.3f, 100.8f, -198.0f, 102.3f,
    -197.7f, 103.7f, -197.4f, 105.2f, -197.0f, 106.7f, -196.6f, 108.1f, -195.7f, 111.0f,
    -195.2f, 112.4f, -194.1f, 115.2f, -193.5f, 116.5f, -192.8f, 117.9f, -192.1f, 119.2f,
    -190.6f, 121.8f, -189.8f, 123.1f, -188.9f, 124.3f, -187.0f, 126.6f, -186.0f, 127.7f,
    -183.9f, 129.8f, -182.7f, 130.8f, -180.3f, 132.6f, -179.1f, 133.4f, -177.8f, 134.1f,
    -176.4f, 134.8f, -175.1f, 135.5f, -173.7f, 136.0f, -169.4f, 137.3f, -167.9f, 137.7f,
    -166.5f, 138.0f, -165.0f, 138.3f, -163.5f, 138.4f, -162.0f, 138.4f, -160.5f, 138.3f,
    -159.0f, 138.0f, -157.6f, 137.7f, -156.1f, 137.3f, -154.7f, 136.9f, -153.2f, 136.5f,
    -151.8f, 136.0f, -150.4f, 135.4f, -149.1f, 134.8f, -147.7f, 134.1f, -146.5f, 133.3f,
    -145.2f, 132.5f, -144.0f, 131.6f, -142.8f, 130.6f, -141.7f, 129.6f, -139.6f, 127.5f,
    -138.6f, 126.4f, -137.7f, 125.2f, -135.1f, 121.5f, -134.3f, 120.3f, -133.5f, 119.0f,
    -131.9f, 116.5f, -131.1f, 115.2f, -128.8f, 111.3f, -128.0f, 110.1f, -127.2f, 108.8f,
    -126.5f, 107.5f, -125.7f, 106.2f, -125.0f, 104.9f, -124.2f, 103.6f, -123.5f, 102.3f,
    -122.0f, 99.6f,  -121.3f, 98.3f,  -115.8f, 87.7f,  -115.1f, 86.4f,  -114.4f, 85.0f,
    -113.7f, 83.7f,  -112.3f, 81.0f,  -111.6f, 79.7f,  -110.1f, 77.1f,  -109.4f, 75.8f,
    -108.0f, 73.1f,  -107.2f, 71.8f,  -106.4f, 70.6f,  -105.7f, 69.3f,  -104.8f, 68.0f,
    -104.0f, 66.8f,  -103.1f, 65.6f,  -101.1f, 63.3f,  -100.0f, 62.3f,  -98.8f,  61.4f,
    -97.6f,  60.6f,  -97.9f,  59.5f,  -98.8f,  58.3f,  -101.5f, 54.6f,  -102.4f, 53.4f,
};

constexpr std::array<float, 245 * 2> pro_body = {
    -0.7f,   -129.1f, -54.3f,  -129.1f, -55.0f,  -129.1f, -57.8f,  -129.0f, -58.5f,  -129.0f,
    -60.7f,  -128.9f, -61.4f,  -128.9f, -62.8f,  -128.8f, -63.5f,  -128.8f, -65.7f,  -128.7f,
    -66.4f,  -128.7f, -67.8f,  -128.6f, -68.5f,  -128.6f, -69.2f,  -128.5f, -70.0f,  -128.5f,
    -70.7f,  -128.4f, -71.4f,  -128.4f, -72.1f,  -128.3f, -72.8f,  -128.3f, -73.5f,  -128.2f,
    -74.2f,  -128.2f, -74.9f,  -128.1f, -75.7f,  -128.1f, -76.4f,  -128.0f, -77.1f,  -128.0f,
    -77.8f,  -127.9f, -78.5f,  -127.9f, -79.2f,  -127.8f, -80.6f,  -127.7f, -81.4f,  -127.6f,
    -82.1f,  -127.5f, -82.8f,  -127.5f, -83.5f,  -127.4f, -84.9f,  -127.3f, -85.6f,  -127.2f,
    -87.0f,  -127.1f, -87.7f,  -127.0f, -88.5f,  -126.9f, -89.2f,  -126.8f, -89.9f,  -126.8f,
    -90.6f,  -126.7f, -94.1f,  -126.3f, -94.8f,  -126.2f, -113.2f, -123.3f, -113.9f, -123.2f,
    -114.6f, -123.0f, -115.3f, -122.9f, -116.7f, -122.6f, -117.4f, -122.5f, -118.1f, -122.3f,
    -118.8f, -122.2f, -119.5f, -122.0f, -120.9f, -121.7f, -121.6f, -121.5f, -122.3f, -121.4f,
    -122.9f, -121.2f, -123.6f, -121.0f, -126.4f, -120.3f, -127.1f, -120.1f, -127.8f, -119.8f,
    -128.4f, -119.6f, -129.1f, -119.4f, -131.2f, -118.7f, -132.5f, -118.3f, -133.2f, -118.0f,
    -133.8f, -117.7f, -134.5f, -117.4f, -135.1f, -117.2f, -135.8f, -116.9f, -136.4f, -116.5f,
    -137.0f, -116.2f, -137.7f, -115.8f, -138.3f, -115.4f, -138.9f, -115.1f, -139.5f, -114.7f,
    -160.0f, -100.5f, -160.5f, -100.0f, -162.5f, -97.9f,  -162.9f, -97.4f,  -163.4f, -96.8f,
    -163.8f, -96.2f,  -165.3f, -93.8f,  -165.7f, -93.2f,  -166.0f, -92.6f,  -166.4f, -91.9f,
    -166.7f, -91.3f,  -167.3f, -90.0f,  -167.6f, -89.4f,  -167.8f, -88.7f,  -168.1f, -88.0f,
    -168.4f, -87.4f,  -168.6f, -86.7f,  -168.9f, -86.0f,  -169.1f, -85.4f,  -169.3f, -84.7f,
    -169.6f, -84.0f,  -169.8f, -83.3f,  -170.2f, -82.0f,  -170.4f, -81.3f,  -172.8f, -72.3f,
    -173.0f, -71.6f,  -173.5f, -69.5f,  -173.7f, -68.8f,  -173.9f, -68.2f,  -174.0f, -67.5f,
    -174.2f, -66.8f,  -174.5f, -65.4f,  -174.7f, -64.7f,  -174.8f, -64.0f,  -175.0f, -63.3f,
    -175.3f, -61.9f,  -175.5f, -61.2f,  -175.8f, -59.8f,  -176.0f, -59.1f,  -176.1f, -58.4f,
    -176.3f, -57.7f,  -176.6f, -56.3f,  -176.8f, -55.6f,  -176.9f, -54.9f,  -177.1f, -54.2f,
    -177.3f, -53.6f,  -177.4f, -52.9f,  -177.6f, -52.2f,  -177.9f, -50.8f,  -178.1f, -50.1f,
    -178.2f, -49.4f,  -178.2f, -48.7f,  -177.8f, -48.1f,  -177.1f, -46.9f,  -177.7f, -46.3f,
    -176.4f, -45.6f,  -176.0f, -45.0f,  -175.3f, -43.8f,  -174.9f, -43.2f,  -174.2f, -42.0f,
    -173.4f, -40.7f,  -173.1f, -40.1f,  -172.7f, -39.5f,  -172.0f, -38.3f,  -171.6f, -37.7f,
    -170.5f, -35.9f,  -170.1f, -35.3f,  -169.7f, -34.6f,  -169.3f, -34.0f,  -168.6f, -32.8f,
    -168.2f, -32.2f,  -166.3f, -29.2f,  -165.9f, -28.6f,  -163.2f, -24.4f,  -162.8f, -23.8f,
    -141.8f, 6.8f,    -141.4f, 7.4f,    -139.4f, 10.3f,   -139.0f, 10.9f,   -138.5f, 11.5f,
    -138.1f, 12.1f,   -137.3f, 13.2f,   -136.9f, 13.8f,   -136.0f, 15.0f,   -135.6f, 15.6f,
    -135.2f, 16.1f,   -134.8f, 16.7f,   -133.9f, 17.9f,   -133.5f, 18.4f,   -133.1f, 19.0f,
    -131.8f, 20.7f,   -131.4f, 21.3f,   -130.1f, 23.0f,   -129.7f, 23.6f,   -128.4f, 25.3f,
    -128.0f, 25.9f,   -126.7f, 27.6f,   -126.3f, 28.2f,   -125.4f, 29.3f,   -125.0f, 29.9f,
    -124.1f, 31.0f,   -123.7f, 31.6f,   -122.8f, 32.7f,   -122.4f, 33.3f,   -121.5f, 34.4f,
    -121.1f, 35.0f,   -120.6f, 35.6f,   -120.2f, 36.1f,   -119.7f, 36.7f,   -119.3f, 37.2f,
    -118.9f, 37.8f,   -118.4f, 38.4f,   -118.0f, 38.9f,   -117.5f, 39.5f,   -117.1f, 40.0f,
    -116.6f, 40.6f,   -116.2f, 41.1f,   -115.7f, 41.7f,   -115.2f, 42.2f,   -114.8f, 42.8f,
    -114.3f, 43.3f,   -113.9f, 43.9f,   -113.4f, 44.4f,   -112.4f, 45.5f,   -112.0f, 46.0f,
    -111.5f, 46.5f,   -110.5f, 47.6f,   -110.0f, 48.1f,   -109.6f, 48.6f,   -109.1f, 49.2f,
    -108.6f, 49.7f,   -107.7f, 50.8f,   -107.2f, 51.3f,   -105.7f, 52.9f,   -105.3f, 53.4f,
    -104.8f, 53.9f,   -104.3f, 54.5f,   -103.8f, 55.0f,   -100.7f, 58.0f,   -100.2f, 58.4f,
    -99.7f,  58.9f,   -99.1f,  59.3f,   -97.2f,  60.3f,   -96.5f,  60.1f,   -95.9f,  59.7f,
    -95.3f,  59.4f,   -94.6f,  59.1f,   -93.9f,  58.9f,   -92.6f,  58.5f,   -91.9f,  58.4f,
    -91.2f,  58.2f,   -90.5f,  58.1f,   -89.7f,  58.0f,   -89.0f,  57.9f,   -86.2f,  57.6f,
    -85.5f,  57.5f,   -84.1f,  57.4f,   -83.4f,  57.3f,   -82.6f,  57.3f,   -81.9f,  57.2f,
    -81.2f,  57.2f,   -80.5f,  57.1f,   -79.8f,  57.1f,   -78.4f,  57.0f,   -77.7f,  57.0f,
    -75.5f,  56.9f,   -74.8f,  56.9f,   -71.9f,  56.8f,   -71.2f,  56.8f,   0.0f,    56.8f,
};
} // namespace

PlayerControlPreview::PlayerControlPreview(QWidget* parent) : QFrame(parent) {
    is_controller_set = false;
    setCursor(Qt::OpenHandCursor);
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&PlayerControlPreview::UpdateInput));

    // refresh at 60hz
    timer->start(16);
}

PlayerControlPreview::~PlayerControlPreview() {
    UnloadController();
};

void PlayerControlPreview::SetController(Core::HID::EmulatedController* controller_) {
    UnloadController();
    is_controller_set = true;
    controller = controller_;
    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { ControllerUpdate(type); },
        .is_npad_service = false,
    };
    callback_key = controller->SetCallback(engine_callback);
    ControllerUpdate(Core::HID::ControllerTriggerType::All);
}

void PlayerControlPreview::UnloadController() {
    if (is_controller_set) {
        controller->DeleteCallback(callback_key);
        is_controller_set = false;
    }
}

void PlayerControlPreview::BeginMappingButton(std::size_t button_id) {
    button_mapping_index = button_id;
    mapping_active = true;
}

void PlayerControlPreview::BeginMappingAnalog(std::size_t stick_id) {
    button_mapping_index = Settings::NativeButton::LStick + stick_id;
    analog_mapping_index = stick_id;
    mapping_active = true;
}

void PlayerControlPreview::EndMapping() {
    button_mapping_index = Settings::NativeButton::BUTTON_NS_END;
    analog_mapping_index = Settings::NativeAnalog::NumAnalogs;
    mapping_active = false;
    blink_counter = 0;
    ResetInputs();
}

void PlayerControlPreview::UpdateColors() {
    const auto theme = QIcon::themeName();
    if (theme.contains(QStringLiteral("day"))) {
        colors.outline = QColor(0, 0, 0);
        colors.primary = QColor(225, 225, 225);
        colors.button = QColor(109, 111, 114);
        colors.button2 = QColor(77, 80, 84);
        colors.slider_arrow = QColor(65, 68, 73);
        colors.font2 = QColor(0, 0, 0);
        colors.indicator = QColor(37, 99, 235);
        colors.deadzone = QColor(170, 0, 0);
        colors.slider_button = QColor(153, 149, 149);
    } else {
        colors.primary = QColor(204, 204, 204);
        colors.button = QColor(35, 38, 41);
        colors.button2 = QColor(26, 27, 30);
        colors.slider_arrow = QColor(14, 15, 18);
        colors.font2 = QColor(255, 255, 255);
        colors.deadzone = QColor(204, 136, 136);
        colors.slider_button = colors.button;

        if (theme.contains(QStringLiteral("midnight"))) {
            colors.outline = QColor(145, 145, 175);
            colors.indicator = QColor(192, 132, 252);
        } else if (theme.contains(QStringLiteral("cyberpunk"))) {
            colors.outline = QColor(252, 238, 10);
            colors.indicator = QColor(252, 238, 10);
        } else if (theme.contains(QStringLiteral("gothic"))) {
            colors.outline = QColor(225, 29, 72);
            colors.indicator = QColor(255, 30, 86);
        } else {
            // storm_night (default)
            colors.outline = QColor(160, 160, 160);
            colors.indicator = QColor(0, 242, 254);
        }
    }

    // Constant colors
    colors.highlight = QColor(170, 0, 0);
    colors.highlight2 = QColor(119, 0, 0);
    colors.slider = QColor(103, 106, 110);
    colors.transparent = QColor(0, 0, 0, 0);
    colors.font = QColor(255, 255, 255);
    colors.led_on = QColor(255, 255, 0);
    colors.led_off = QColor(170, 238, 255);
    colors.indicator2 = QColor(59, 165, 93);
    colors.charging = QColor(250, 168, 26);
    colors.button_turbo = QColor(217, 158, 4);

    colors.left = colors.primary;
    colors.right = colors.primary;

    switch (current_skin) {
    case ControllerSkin::ClassicBlack:
    default:
        colors.primary = QColor(42, 45, 52, 235);
        colors.left = QColor(36, 38, 44);
        colors.right = QColor(36, 38, 44);
        colors.grip_left_highlight = QColor(62, 66, 74);
        colors.grip_right_highlight = QColor(62, 66, 74);
        colors.grip_left_shadow = QColor(20, 22, 26);
        colors.grip_right_shadow = QColor(20, 22, 26);
        colors.body_inner = QColor(28, 30, 36, 180);
        colors.body_rim = QColor(75, 82, 92);
        colors.emblem = QColor(0, 0, 0, 0);
        colors.emblem_secondary = QColor(0, 0, 0, 0);
        colors.home_led = QColor(0, 210, 255);
        break;

    case ControllerSkin::Xenoblade2:
        colors.primary = QColor(38, 41, 46, 235);
        colors.left = QColor(215, 38, 75);
        colors.right = QColor(215, 38, 75);
        colors.grip_left_highlight = QColor(245, 85, 120);
        colors.grip_right_highlight = QColor(245, 85, 120);
        colors.grip_left_shadow = QColor(140, 18, 45);
        colors.grip_right_shadow = QColor(140, 18, 45);
        colors.body_inner = QColor(30, 25, 32, 190);
        colors.body_rim = QColor(230, 60, 95);
        colors.emblem = QColor(0, 245, 185);
        colors.emblem_secondary = QColor(255, 215, 60);
        colors.home_led = QColor(0, 245, 185);
        break;

    case ControllerSkin::SmashBrosUltimate:
        colors.primary = QColor(36, 38, 42, 235);
        colors.left = QColor(238, 240, 244);
        colors.right = QColor(238, 240, 244);
        colors.grip_left_highlight = QColor(255, 255, 255);
        colors.grip_right_highlight = QColor(255, 255, 255);
        colors.grip_left_shadow = QColor(170, 175, 182);
        colors.grip_right_shadow = QColor(170, 175, 182);
        colors.body_inner = QColor(25, 26, 30, 190);
        colors.body_rim = QColor(200, 205, 215);
        colors.emblem = QColor(245, 248, 252, 220);
        colors.emblem_secondary = QColor(30, 32, 38, 200);
        colors.home_led = QColor(255, 225, 100);
        break;

    case ControllerSkin::ZeldaTotk:
        colors.primary = QColor(32, 36, 42, 240);
        colors.left = QColor(26, 28, 34);
        colors.right = QColor(245, 245, 248);
        colors.grip_left_highlight = QColor(54, 58, 68);
        colors.grip_right_highlight = QColor(255, 255, 255);
        colors.grip_left_shadow = QColor(14, 16, 20);
        colors.grip_right_shadow = QColor(185, 188, 195);
        colors.body_inner = QColor(22, 25, 30, 200);
        colors.body_rim = QColor(218, 168, 38);
        colors.emblem = QColor(56, 225, 176);
        colors.emblem_secondary = QColor(230, 185, 45);
        colors.home_led = QColor(56, 225, 176);
        break;

    case ControllerSkin::Splatoon3:
        colors.primary = QColor(38, 40, 46, 230);
        colors.left = QColor(218, 253, 33);
        colors.right = QColor(122, 38, 235);
        colors.grip_left_highlight = QColor(238, 255, 105);
        colors.grip_right_highlight = QColor(175, 95, 255);
        colors.grip_left_shadow = QColor(155, 185, 15);
        colors.grip_right_shadow = QColor(72, 18, 155);
        colors.body_inner = QColor(32, 28, 40, 190);
        colors.body_rim = QColor(218, 253, 33);
        colors.emblem = QColor(218, 253, 33, 215);
        colors.emblem_secondary = QColor(140, 50, 245, 215);
        colors.home_led = QColor(218, 253, 33);
        break;

    case ControllerSkin::MonsterHunterRise:
        colors.primary = QColor(40, 44, 48, 240);
        colors.left = QColor(48, 52, 58);
        colors.right = QColor(48, 52, 58);
        colors.grip_left_highlight = QColor(75, 80, 90);
        colors.grip_right_highlight = QColor(75, 80, 90);
        colors.grip_left_shadow = QColor(26, 28, 32);
        colors.grip_right_shadow = QColor(26, 28, 32);
        colors.body_inner = QColor(28, 30, 34, 200);
        colors.body_rim = QColor(220, 175, 65);
        colors.emblem = QColor(225, 180, 65);
        colors.emblem_secondary = QColor(175, 90, 250);
        colors.home_led = QColor(185, 125, 255);
        break;

    case ControllerSkin::PokemonScarletViolet:
        colors.primary = QColor(36, 38, 44, 235);
        colors.left = QColor(228, 45, 60);
        colors.right = QColor(75, 65, 215);
        colors.grip_left_highlight = QColor(255, 95, 110);
        colors.grip_right_highlight = QColor(125, 115, 255);
        colors.grip_left_shadow = QColor(150, 22, 35);
        colors.grip_right_shadow = QColor(42, 35, 145);
        colors.body_inner = QColor(30, 30, 38, 190);
        colors.body_rim = QColor(255, 180, 60);
        colors.emblem = QColor(255, 180, 50);
        colors.emblem_secondary = QColor(225, 230, 240);
        colors.home_led = QColor(255, 90, 90);
        break;

    case ControllerSkin::CyberStorm:
        colors.primary = QColor(14, 16, 22, 245);
        colors.left = QColor(0, 225, 250);
        colors.right = QColor(255, 30, 95);
        colors.grip_left_highlight = QColor(120, 245, 255);
        colors.grip_right_highlight = QColor(255, 110, 160);
        colors.grip_left_shadow = QColor(0, 120, 145);
        colors.grip_right_shadow = QColor(150, 15, 55);
        colors.body_inner = QColor(8, 10, 15, 230);
        colors.body_rim = QColor(0, 240, 255, 180);
        colors.emblem = QColor(0, 245, 255);
        colors.emblem_secondary = QColor(255, 35, 115);
        colors.home_led = QColor(0, 255, 240);
        break;
    }

    if (controller != nullptr) {
        const auto color_left = controller->GetColorsValues()[0].body;
        const auto color_right = controller->GetColorsValues()[1].body;
        if (color_left != 0 && color_right != 0 &&
            controller_type != Core::HID::NpadStyleIndex::Fullkey) {
            colors.left = QColor(color_left);
            colors.right = QColor(color_right);
        }
    }
}

void PlayerControlPreview::ResetInputs() {
    button_values.fill({
        .value = false,
    });
    stick_values.fill({
        .x = {.value = 0, .properties = {0, 1, 0}},
        .y = {.value = 0, .properties = {0, 1, 0}},
    });
    trigger_values.fill({
        .analog = {.value = 0, .properties = {0, 1, 0}},
        .pressed = {.value = false},
    });
    update();
}

void PlayerControlPreview::ControllerUpdate(Core::HID::ControllerTriggerType type) {
    if (type == Core::HID::ControllerTriggerType::All) {
        ControllerUpdate(Core::HID::ControllerTriggerType::Color);
        ControllerUpdate(Core::HID::ControllerTriggerType::Type);
        ControllerUpdate(Core::HID::ControllerTriggerType::Connected);
        ControllerUpdate(Core::HID::ControllerTriggerType::Button);
        ControllerUpdate(Core::HID::ControllerTriggerType::Stick);
        ControllerUpdate(Core::HID::ControllerTriggerType::Trigger);
        ControllerUpdate(Core::HID::ControllerTriggerType::Battery);
        ControllerUpdate(Core::HID::ControllerTriggerType::Motion);
        return;
    }


    switch (type) {
    case Core::HID::ControllerTriggerType::Connected:
        is_connected = true;
        led_pattern = controller->GetLedPattern();
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Disconnected:
        is_connected = false;
        led_pattern.raw = 0;
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Type:
        controller_type = controller->GetNpadStyleIndex(true);
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Color:
        UpdateColors();
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Button:
        button_values = controller->GetButtonsValues();
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Stick:
        using namespace Settings::NativeAnalog;
        stick_values = controller->GetSticksValues();
        // Y axis is inverted
        stick_values[LStick].y.value = -stick_values[LStick].y.value;
        stick_values[LStick].y.raw_value = -stick_values[LStick].y.raw_value;
        stick_values[RStick].y.value = -stick_values[RStick].y.value;
        stick_values[RStick].y.raw_value = -stick_values[RStick].y.raw_value;
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Trigger:
        trigger_values = controller->GetTriggersValues();
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Battery:
        battery_values = controller->GetBatteryValues();
        needs_redraw = true;
        break;
    case Core::HID::ControllerTriggerType::Motion:
        motion_values = controller->GetMotions();
        needs_redraw = true;
        break;
    default:
        break;
    }
}

void PlayerControlPreview::UpdateInput() {
    if (mapping_active) {
        for (std::size_t index = 0; index < button_values.size(); ++index) {
            bool blink = index == button_mapping_index;
            if (analog_mapping_index == Settings::NativeAnalog::NumAnalogs) {
                blink &= blink_counter > 25;
            }
            if (button_values[index].value != blink) {
                needs_redraw = true;
            }
            button_values[index].value = blink;
        }

        for (std::size_t index = 0; index < stick_values.size(); ++index) {
            const bool blink_analog = index == analog_mapping_index;
            if (blink_analog) {
                needs_redraw = true;
                stick_values[index].x.value = blink_counter < 25 ? -blink_counter / 25.0f : 0;
                stick_values[index].y.value =
                    blink_counter > 25 ? -(blink_counter - 25) / 25.0f : 0;
            }
        }
    } else if (is_controller_set && controller != nullptr) {
        // Continuous direct hardware polling from physical gamepad
        const auto new_buttons = controller->GetButtonsValues();
        for (std::size_t i = 0; i < new_buttons.size() && i < button_values.size(); ++i) {
            if (new_buttons[i].value != button_values[i].value) {
                button_values[i] = new_buttons[i];
                needs_redraw = true;
            }
        }

        using namespace Settings::NativeAnalog;
        auto new_sticks = controller->GetSticksValues();
        new_sticks[LStick].y.value = -new_sticks[LStick].y.value;
        new_sticks[LStick].y.raw_value = -new_sticks[LStick].y.raw_value;
        new_sticks[RStick].y.value = -new_sticks[RStick].y.value;
        new_sticks[RStick].y.raw_value = -new_sticks[RStick].y.raw_value;
        for (std::size_t s = 0; s < new_sticks.size() && s < stick_values.size(); ++s) {
            if (std::abs(new_sticks[s].x.value - stick_values[s].x.value) > 0.005f ||
                std::abs(new_sticks[s].y.value - stick_values[s].y.value) > 0.005f) {
                stick_values[s] = new_sticks[s];
                needs_redraw = true;
            }
        }

        const auto new_triggers = controller->GetTriggersValues();
        for (std::size_t t = 0; t < new_triggers.size() && t < trigger_values.size(); ++t) {
            if (new_triggers[t].pressed.value != trigger_values[t].pressed.value ||
                std::abs(new_triggers[t].analog.value - trigger_values[t].analog.value) > 0.005f) {
                trigger_values[t] = new_triggers[t];
                needs_redraw = true;
            }
        }

        // Direct hardware polling of Motion / Gyroscope
        if (!is_gyro_dragging) {
            const auto new_motions = controller->GetMotions();
            for (std::size_t m = 0; m < new_motions.size() && m < motion_values.size(); ++m) {
                if (std::abs(new_motions[m].euler.x - motion_values[m].euler.x) > 0.001f ||
                    std::abs(new_motions[m].euler.y - motion_values[m].euler.y) > 0.001f ||
                    std::abs(new_motions[m].euler.z - motion_values[m].euler.z) > 0.001f) {
                    motion_values[m] = new_motions[m];
                    needs_redraw = true;
                }
            }
        }

        // Direct hardware polling of Battery
        const auto new_batteries = controller->GetBatteryValues();
        for (std::size_t b = 0; b < new_batteries.size() && b < battery_values.size(); ++b) {
            if (new_batteries[b] != battery_values[b]) {
                battery_values[b] = new_batteries[b];
                needs_redraw = true;
            }
        }
    }

    // Smooth gyro animation interpolation towards target euler angles
    {
        const auto& target_euler = motion_values[Settings::NativeMotion::MotionLeft].euler;
        const float dx = target_euler.x - smooth_euler.x;
        const float dy = target_euler.y - smooth_euler.y;
        const float dz = target_euler.z - smooth_euler.z;
        if (std::abs(dx) > 0.0005f || std::abs(dy) > 0.0005f || std::abs(dz) > 0.0005f) {
            smooth_euler.x += dx * 0.35f;
            smooth_euler.y += dy * 0.35f;
            smooth_euler.z += dz * 0.35f;
            needs_redraw = true;
        }
    }

    if (needs_redraw) {
        needs_redraw = false;
        update();
    }

    if (mapping_active) {
        blink_counter = (blink_counter + 1) % 50;
    }
}

void PlayerControlPreview::SetSkin(ControllerSkin skin) {
    current_skin = skin;
    UpdateColors();
    update();
}

void PlayerControlPreview::ResetView() {
    rot_x = 0.0f;
    rot_y = 0.0f;
    zoom = 1.0f;
    is_rear_view = false;
    smooth_euler = {};
    motion_values[Settings::NativeMotion::MotionLeft].euler = {};
    update();
}


void PlayerControlPreview::SetRotation(float rx, float ry, float z) {
    rot_x = rx;
    rot_y = ry;
    zoom = std::clamp(z, 0.65f, 2.0f);
    update();
}

void PlayerControlPreview::ToggleRearView() {
    is_rear_view = !is_rear_view;
    rot_x = 0.0f;
    rot_y = 0.0f;
    update();
}

void PlayerControlPreview::SetRearView(bool rear) {
    if (is_rear_view != rear) {
        is_rear_view = rear;
        rot_x = 0.0f;
        rot_y = 0.0f;
        update();
    }
}

void PlayerControlPreview::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_mouse_dragging = true;
        last_mouse_pos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton) {
        is_gyro_dragging = true;
        last_mouse_pos = event->pos();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void PlayerControlPreview::mouseMoveEvent(QMouseEvent* event) {
    if (is_mouse_dragging) {
        const QPoint delta = event->pos() - last_mouse_pos;
        last_mouse_pos = event->pos();
        rot_y = std::clamp(rot_y + static_cast<float>(delta.x()) * 0.25f, -20.0f, 20.0f);
        rot_x = std::clamp(rot_x - static_cast<float>(delta.y()) * 0.25f, -15.0f, 15.0f);
        update();
        event->accept();
        return;
    }
    if (is_gyro_dragging) {
        const QPoint delta = event->pos() - last_mouse_pos;
        last_mouse_pos = event->pos();
        motion_values[Settings::NativeMotion::MotionLeft].euler.x += static_cast<float>(delta.y()) * 0.02f;
        motion_values[Settings::NativeMotion::MotionLeft].euler.z += static_cast<float>(delta.x()) * 0.02f;
        needs_redraw = true;
        update();
        event->accept();
        return;
    }
    QFrame::mouseMoveEvent(event);
}

void PlayerControlPreview::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_mouse_dragging = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton) {
        is_gyro_dragging = false;
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void PlayerControlPreview::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        ToggleRearView();
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton) {
        motion_values[Settings::NativeMotion::MotionLeft].euler = {};
        smooth_euler = {};
        update();
        event->accept();
        return;
    }
    QFrame::mouseDoubleClickEvent(event);
}


void PlayerControlPreview::wheelEvent(QWheelEvent* event) {
    const float numDegrees = static_cast<float>(event->angleDelta().y()) / 8.0f;
    const float numSteps = numDegrees / 15.0f;
    zoom = std::clamp(zoom + numSteps * 0.06f, 0.70f, 1.60f);
    update();
    event->accept();
}

void PlayerControlPreview::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const QPointF center = rect().center();

    if (controller_type == Core::HID::NpadStyleIndex::Fullkey) {
        DrawProController(p, center);
        return;
    }

    p.save();
    if (rot_x != 0.0f || rot_y != 0.0f || zoom != 1.0f) {
        const float rad_y = rot_y * PI_CONST / 180.0f;
        const float rad_x = rot_x * PI_CONST / 180.0f;
        const float cos_y = std::cos(rad_y);
        const float sin_y = std::sin(rad_y);
        const float cos_x = std::cos(rad_x);
        const float sin_x = std::sin(rad_x);

        QTransform t;
        t.translate(center.x(), center.y());
        t.scale(zoom * cos_y, zoom * cos_x);
        t.shear(-sin_y * sin_x * 0.30f, sin_y * 0.12f);
        t.translate(-center.x(), -center.y());
        p.setTransform(t, true);
    }

    switch (controller_type) {
    case Core::HID::NpadStyleIndex::Handheld:
        DrawHandheldController(p, center);
        break;
    case Core::HID::NpadStyleIndex::JoyconDual:
        DrawDualController(p, center);
        break;
    case Core::HID::NpadStyleIndex::JoyconLeft:
        DrawLeftController(p, center);
        break;
    case Core::HID::NpadStyleIndex::JoyconRight:
        DrawRightController(p, center);
        break;
    case Core::HID::NpadStyleIndex::GameCube:
        DrawGCController(p, center);
        break;
    default:
        DrawProController(p, center);
        break;
    }

    p.restore();
}

void PlayerControlPreview::DrawLeftController(QPainter& p, const QPointF center) {
    {
        using namespace Settings::NativeButton;

        // Sideview left joystick
        DrawJoystickSideview(p, center + QPoint(142, -69),
                             -stick_values[Settings::NativeAnalog::LStick].y.value, 1.15f,
                             button_values[LStick]);

        // Topview D-pad buttons
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-163, -21), button_values[DLeft], 11, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(-117, -21), button_values[DRight], 11, 5, Direction::Up);

        // Topview left joystick
        DrawJoystickSideview(p, center + QPointF(-140.5f, -28),
                             -stick_values[Settings::NativeAnalog::LStick].x.value + 15.0f, 1.15f,
                             button_values[LStick]);

        // Topview minus button
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-111, -22), button_values[Minus], 8, 4, Direction::Up,
                        1);

        // Left trigger
        DrawLeftTriggers(p, center, button_values[L]);
        DrawRoundButton(p, center + QPoint(151, -146), button_values[L], 8, 4, Direction::Down);
        DrawLeftZTriggers(p, center, button_values[ZL]);

        // Sideview D-pad buttons
        DrawRoundButton(p, center + QPoint(135, 14), button_values[DLeft], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(135, 36), button_values[DDown], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(135, -10), button_values[DUp], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(135, 14), button_values[DRight], 5, 11,
                        Direction::Right);
        DrawRoundButton(p, center + QPoint(135, 71), button_values[Screenshot], 3, 8,
                        Direction::Right, 1);

        // Sideview minus button
        DrawRoundButton(p, center + QPoint(135, -118), button_values[Minus], 4, 2.66f,
                        Direction::Right, 1);

        // Sideview SL and SR buttons
        button_color = colors.slider_button;
        DrawRoundButton(p, center + QPoint(59, 52), button_values[SRLeft], 5, 12, Direction::Left);
        DrawRoundButton(p, center + QPoint(59, -69), button_values[SLLeft], 5, 12, Direction::Left);

        DrawLeftBody(p, center);

        // Left trigger top view
        DrawLeftTriggersTopView(p, center, button_values[L]);
        DrawLeftZTriggersTopView(p, center, button_values[ZL]);
    }

    {
        // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawJoystick(p,
                     center + QPointF(9, -69) +
                         (QPointF(stick_values[LStick].x.value, stick_values[LStick].y.value) * 8),
                     1.8f, button_values[Settings::NativeButton::LStick]);
        DrawRawJoystick(p, center + QPointF(-140, 90), QPointF(0, 0));
    }

    {
        // Draw motion cubes
        using namespace Settings::NativeMotion;
        p.setPen(colors.outline);
        p.setBrush(colors.transparent);
        Draw3dCube(p, center + QPointF(-140, 90),
                   motion_values[Settings::NativeMotion::MotionLeft].euler, 20.0f);
    }

    using namespace Settings::NativeButton;

    // D-pad constants
    const QPointF dpad_center = center + QPoint(9, 14);
    constexpr int dpad_distance = 23;
    constexpr int dpad_radius = 11;
    constexpr float dpad_arrow_size = 1.2f;

    // D-pad buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, dpad_center + QPoint(dpad_distance, 0), button_values[DRight], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, dpad_distance), button_values[DDown], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, -dpad_distance), button_values[DUp], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(-dpad_distance, 0), button_values[DLeft], dpad_radius);

    // D-pad arrows
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, dpad_center + QPoint(dpad_distance, 0), Direction::Right, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, dpad_distance), Direction::Down, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, -dpad_distance), Direction::Up, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(-dpad_distance, 0), Direction::Left, dpad_arrow_size);

    // SR and SL buttons
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawRoundButton(p, center + QPoint(155, 52), button_values[SRLeft], 5.2f, 12, Direction::None,
                    4);
    DrawRoundButton(p, center + QPoint(155, -69), button_values[SLLeft], 5.2f, 12, Direction::None,
                    4);

    // SR and SL text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(155, 52), Symbol::SR, 1.0f);
    DrawSymbol(p, center + QPointF(155, -69), Symbol::SL, 1.0f);

    // Minus button
    button_color = colors.button;
    DrawMinusButton(p, center + QPoint(39, -118), button_values[Minus], 16);

    // Screenshot button
    DrawRoundButton(p, center + QPoint(26, 71), button_values[Screenshot], 8, 8);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(26, 71), 5);

    // Draw battery
    DrawBattery(p, center + QPoint(-160, -140),
                battery_values[Core::HID::EmulatedDeviceIndex::LeftIndex]);
}

void PlayerControlPreview::DrawRightController(QPainter& p, const QPointF center) {
    {
        using namespace Settings::NativeButton;

        // Sideview right joystick
        DrawJoystickSideview(p, center + QPoint(173 - 315, 11),
                             stick_values[Settings::NativeAnalog::RStick].y.value + 10.0f, 1.15f,
                             button_values[Settings::NativeButton::RStick]);

        // Topview right joystick
        DrawJoystickSideview(p, center + QPointF(140, -28),
                             -stick_values[Settings::NativeAnalog::RStick].x.value + 15.0f, 1.15f,
                             button_values[RStick]);

        // Topview face buttons
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(163, -21), button_values[A], 11, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(140, -21), button_values[B], 11, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(140, -21), button_values[X], 11, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(117, -21), button_values[Y], 11, 5, Direction::Up);

        // Topview plus button
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(111, -22), button_values[Plus], 8, 4, Direction::Up, 1);
        DrawRoundButton(p, center + QPoint(111, -22), button_values[Plus], 2.66f, 4, Direction::Up,
                        1);

        // Right trigger
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRightTriggers(p, center, button_values[R]);
        DrawRoundButton(p, center + QPoint(-151, -146), button_values[R], 8, 4, Direction::Down);
        DrawRightZTriggers(p, center, button_values[ZR]);

        // Sideview face buttons
        DrawRoundButton(p, center + QPoint(-135, -73), button_values[A], 5, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -50), button_values[B], 5, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -95), button_values[X], 5, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -73), button_values[Y], 5, 11, Direction::Left);

        // Sideview home and plus button
        DrawRoundButton(p, center + QPoint(-135, 66), button_values[Home], 3, 12, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -118), button_values[Plus], 4, 8, Direction::Left,
                        1);
        DrawRoundButton(p, center + QPoint(-135, -118), button_values[Plus], 4, 2.66f,
                        Direction::Left, 1);

        // Sideview SL and SR buttons
        button_color = colors.slider_button;
        DrawRoundButton(p, center + QPoint(-59, 52), button_values[SLRight], 5, 11,
                        Direction::Right);
        DrawRoundButton(p, center + QPoint(-59, -69), button_values[SRRight], 5, 11,
                        Direction::Right);

        DrawRightBody(p, center);

        // Right trigger top view
        DrawRightTriggersTopView(p, center, button_values[R]);
        DrawRightZTriggersTopView(p, center, button_values[ZR]);
    }

    {
        // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawJoystick(p,
                     center + QPointF(-9, 11) +
                         (QPointF(stick_values[RStick].x.value, stick_values[RStick].y.value) * 8),
                     1.8f, button_values[Settings::NativeButton::RStick]);
        DrawRawJoystick(p, QPointF(0, 0), center + QPointF(140, 90));
    }

    {
        // Draw motion cubes
        using namespace Settings::NativeMotion;
        p.setPen(colors.outline);
        p.setBrush(colors.transparent);
        Draw3dCube(p, center + QPointF(140, 90),
                   motion_values[Settings::NativeMotion::MotionRight].euler, 20.0f);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(-9, -73);
    constexpr int face_distance = 23;
    constexpr int face_radius = 11;
    constexpr float text_size = 1.1f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPoint(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPoint(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPoint(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPoint(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPoint(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPoint(-face_distance, 1), Symbol::Y, text_size);

    // SR and SL buttons
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawRoundButton(p, center + QPoint(-155, 52), button_values[SLRight], 5, 12, Direction::None,
                    4.0f);
    DrawRoundButton(p, center + QPoint(-155, -69), button_values[SRRight], 5, 12, Direction::None,
                    4.0f);

    // SR and SL text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    p.rotate(-180);
    DrawSymbol(p, QPointF(-center.x(), -center.y()) + QPointF(155, 69), Symbol::SR, 1.0f);
    DrawSymbol(p, QPointF(-center.x(), -center.y()) + QPointF(155, -52), Symbol::SL, 1.0f);
    p.rotate(180);

    // Plus Button
    DrawPlusButton(p, center + QPoint(-40, -118), button_values[Plus], 16);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(-26, 66), button_values[Home], 12);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(-26, 66), button_values[Home], 9);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(-26, 66), Symbol::House, 5);

    // Draw battery
    DrawBattery(p, center + QPoint(120, -140),
                battery_values[Core::HID::EmulatedDeviceIndex::RightIndex]);
}

void PlayerControlPreview::DrawDualController(QPainter& p, const QPointF center) {
    {
        using namespace Settings::NativeButton;

        // Left/Right trigger
        DrawDualTriggers(p, center, button_values[L], button_values[R]);

        // Topview right joystick
        DrawJoystickSideview(p, center + QPointF(180, -78),
                             -stick_values[Settings::NativeAnalog::RStick].x.value + 15.0f, 1,
                             button_values[RStick]);

        // Topview face buttons
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(200, -71), button_values[A], 10, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(180, -71), button_values[B], 10, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(180, -71), button_values[X], 10, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(160, -71), button_values[Y], 10, 5, Direction::Up);

        // Topview plus button
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(154, -72), button_values[Plus], 7, 4, Direction::Up, 1);
        DrawRoundButton(p, center + QPoint(154, -72), button_values[Plus], 2.33f, 4, Direction::Up,
                        1);

        // Topview D-pad buttons
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-200, -71), button_values[DLeft], 10, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(-160, -71), button_values[DRight], 10, 5, Direction::Up);

        // Topview left joystick
        DrawJoystickSideview(p, center + QPointF(-180.5f, -78),
                             -stick_values[Settings::NativeAnalog::LStick].x.value + 15.0f, 1,
                             button_values[LStick]);

        // Topview minus button
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-154, -72), button_values[Minus], 7, 4, Direction::Up,
                        1);

        // Left SR and SL sideview buttons
        button_color = colors.slider_button;
        DrawRoundButton(p, center + QPoint(-20, -62), button_values[SLLeft], 4, 11,
                        Direction::Left);
        DrawRoundButton(p, center + QPoint(-20, 47), button_values[SRLeft], 4, 11, Direction::Left);

        // Right SR and SL sideview buttons
        button_color = colors.slider_button;
        DrawRoundButton(p, center + QPoint(20, 47), button_values[SLRight], 4, 11,
                        Direction::Right);
        DrawRoundButton(p, center + QPoint(20, -62), button_values[SRRight], 4, 11,
                        Direction::Right);

        DrawDualBody(p, center);

        // Right trigger top view
        DrawDualTriggersTopView(p, center, button_values[L], button_values[R]);
        DrawDualZTriggersTopView(p, center, button_values[ZL], button_values[ZR]);
    }

    {
        // Draw joysticks
        using namespace Settings::NativeAnalog;
        const auto l_stick = QPointF(stick_values[LStick].x.value, stick_values[LStick].y.value);
        const auto l_button = button_values[Settings::NativeButton::LStick];
        const auto r_stick = QPointF(stick_values[RStick].x.value, stick_values[RStick].y.value);
        const auto r_button = button_values[Settings::NativeButton::RStick];

        DrawJoystick(p, center + QPointF(-65, -65) + (l_stick * 7), 1.62f, l_button);
        DrawJoystick(p, center + QPointF(65, 12) + (r_stick * 7), 1.62f, r_button);
        DrawRawJoystick(p, center + QPointF(-180, 90), center + QPointF(180, 90));
    }

    {
        // Draw motion cubes
        using namespace Settings::NativeMotion;
        p.setPen(colors.outline);
        p.setBrush(colors.transparent);
        Draw3dCube(p, center + QPointF(-180, 90),
                   motion_values[Settings::NativeMotion::MotionLeft].euler, 20.0f);
        Draw3dCube(p, center + QPointF(180, 90),
                   motion_values[Settings::NativeMotion::MotionRight].euler, 20.0f);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(65, -65);
    constexpr int face_distance = 20;
    constexpr int face_radius = 10;
    constexpr float text_size = 1.0f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPoint(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPoint(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPoint(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPoint(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPoint(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPoint(-face_distance, 1), Symbol::Y, text_size);

    // D-pad constants
    const QPointF dpad_center = center + QPoint(-65, 12);
    constexpr int dpad_distance = 20;
    constexpr int dpad_radius = 10;
    constexpr float dpad_arrow_size = 1.1f;

    // D-pad buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, dpad_center + QPoint(dpad_distance, 0), button_values[DRight], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, dpad_distance), button_values[DDown], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, -dpad_distance), button_values[DUp], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(-dpad_distance, 0), button_values[DLeft], dpad_radius);

    // D-pad arrows
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, dpad_center + QPoint(dpad_distance, 0), Direction::Right, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, dpad_distance), Direction::Down, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, -dpad_distance), Direction::Up, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(-dpad_distance, 0), Direction::Left, dpad_arrow_size);

    // Minus and Plus button
    button_color = colors.button;
    DrawMinusButton(p, center + QPoint(-39, -106), button_values[Minus], 14);
    DrawPlusButton(p, center + QPoint(39, -106), button_values[Plus], 14);

    // Screenshot button
    p.setPen(colors.outline);
    DrawRoundButton(p, center + QPoint(-52, 63), button_values[Screenshot], 8, 8);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(-52, 63), 5);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(50, 60), button_values[Home], 11);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(50, 60), button_values[Home], 8.5f);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(50, 60), Symbol::House, 4.2f);

    // Draw battery
    DrawBattery(p, center + QPoint(-200, -10),
                battery_values[Core::HID::EmulatedDeviceIndex::LeftIndex]);
    DrawBattery(p, center + QPoint(160, -10),
                battery_values[Core::HID::EmulatedDeviceIndex::RightIndex]);
}

void PlayerControlPreview::DrawHandheldController(QPainter& p, const QPointF center) {
    DrawHandheldTriggers(p, center, button_values[Settings::NativeButton::L],
                         button_values[Settings::NativeButton::R]);
    DrawHandheldBody(p, center);
    {
        // Draw joysticks
        using namespace Settings::NativeAnalog;
        const auto& l_stick = QPointF(stick_values[LStick].x.value, stick_values[LStick].y.value);
        const auto& l_button = button_values[Settings::NativeButton::LStick];
        const auto& r_stick = QPointF(stick_values[RStick].x.value, stick_values[RStick].y.value);
        const auto& r_button = button_values[Settings::NativeButton::RStick];

        DrawJoystick(p, center + QPointF(-171, -41) + (l_stick * 4), 1.0f, l_button);
        DrawJoystick(p, center + QPointF(171, 8) + (r_stick * 4), 1.0f, r_button);
        DrawRawJoystick(p, center + QPointF(-50, 0), center + QPointF(50, 0));
    }

    {
        // Draw motion cubes
        using namespace Settings::NativeMotion;
        p.setPen(colors.outline);
        p.setBrush(colors.transparent);
        Draw3dCube(p, center + QPointF(0, -115),
                   motion_values[Settings::NativeMotion::MotionLeft].euler, 15.0f);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(171, -41);
    constexpr float face_distance = 12.8f;
    constexpr float face_radius = 6.4f;
    constexpr float text_size = 0.6f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPointF(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPointF(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPointF(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPointF(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPointF(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPointF(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPointF(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPointF(-face_distance, 1), Symbol::Y, text_size);

    // D-pad constants
    const QPointF dpad_center = center + QPoint(-171, 8);
    constexpr float dpad_distance = 12.8f;
    constexpr float dpad_radius = 6.4f;
    constexpr float dpad_arrow_size = 0.68f;

    // D-pad buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, dpad_center + QPointF(dpad_distance, 0), button_values[DRight],
                     dpad_radius);
    DrawCircleButton(p, dpad_center + QPointF(0, dpad_distance), button_values[DDown], dpad_radius);
    DrawCircleButton(p, dpad_center + QPointF(0, -dpad_distance), button_values[DUp], dpad_radius);
    DrawCircleButton(p, dpad_center + QPointF(-dpad_distance, 0), button_values[DLeft],
                     dpad_radius);

    // D-pad arrows
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, dpad_center + QPointF(dpad_distance, 0), Direction::Right, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPointF(0, dpad_distance), Direction::Down, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPointF(0, -dpad_distance), Direction::Up, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPointF(-dpad_distance, 0), Direction::Left, dpad_arrow_size);

    // ZL and ZR buttons
    p.setPen(colors.outline);
    DrawTriggerButton(p, center + QPoint(-210, -120), Direction::Left, button_values[ZL]);
    DrawTriggerButton(p, center + QPoint(210, -120), Direction::Right, button_values[ZR]);
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, center + QPoint(-210, -120), Symbol::ZL, 1.5f);
    DrawSymbol(p, center + QPoint(210, -120), Symbol::ZR, 1.5f);

    // Minus and Plus button
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawMinusButton(p, center + QPoint(-155, -67), button_values[Minus], 8);
    DrawPlusButton(p, center + QPoint(155, -67), button_values[Plus], 8);

    // Screenshot button
    p.setPen(colors.outline);
    DrawRoundButton(p, center + QPoint(-162, 39), button_values[Screenshot], 5, 5);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(-162, 39), 3);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(161, 37), button_values[Home], 7);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(161, 37), button_values[Home], 5);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(161, 37), Symbol::House, 2.75f);

    // Draw battery
    DrawBattery(p, center + QPoint(-188, 95),
                battery_values[Core::HID::EmulatedDeviceIndex::LeftIndex]);
    DrawBattery(p, center + QPoint(150, 95),
                battery_values[Core::HID::EmulatedDeviceIndex::RightIndex]);
}

void PlayerControlPreview::DrawProController(QPainter& p, const QPointF center) {
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    struct ProjectedPt {
        QPointF pt;
        float scale;
    };

    // Parallax tilt angles (subtle 3D perspective without polygon tearing)
    const float rad_y = rot_y * PI_CONST / 180.0f;
    const float rad_x = rot_x * PI_CONST / 180.0f;
    const float cos_y = std::cos(rad_y);
    const float sin_y = std::sin(rad_y);
    const float cos_x = std::cos(rad_x);
    const float sin_x = std::sin(rad_x);

    // Scaling: 36% larger, shifted slightly up to give space below for modern stick monitors
    constexpr float base_scale = 1.36f;
    const QPointF controller_center = center + QPointF(0.0f, -32.0f);

    auto Project = [&](float x, float y, float z = 0.0f) -> ProjectedPt {
        const float x1 = x * cos_y + z * sin_y;
        const float y1 = y;
        const float z1 = -x * sin_y + z * cos_y;

        const float x2 = x1;
        const float y2 = y1 * cos_x - z1 * sin_x;
        const float z2 = y1 * sin_x + z1 * cos_x;

        constexpr float camera_dist = 900.0f;
        const float persp = (camera_dist / std::max(100.0f, camera_dist - z2)) * zoom * base_scale;

        return {
            controller_center + QPointF(x2 * persp, y2 * persp),
            persp
        };
    };

    using namespace Settings::NativeButton;
    using namespace Settings::NativeAnalog;

    const float zl_analog = std::clamp(
        std::max(trigger_values[Settings::NativeTrigger::LTrigger].analog.value,
                 button_values[ZL].value ? 1.0f : 0.0f),
        0.0f, 1.0f);
    const float zr_analog = std::clamp(
        std::max(trigger_values[Settings::NativeTrigger::RTrigger].analog.value,
                 button_values[ZR].value ? 1.0f : 0.0f),
        0.0f, 1.0f);

    // =========================================================================
    // HUD COMPONENT 1: High-Tech Battery Card (Top-Left, Clear of Controller)
    // =========================================================================
    auto DrawPhotorealisticBattery = [&](const QPointF bat_pos) {
        const auto bat_status = battery_values[Core::HID::EmulatedDeviceIndex::LeftIndex];
        int pct = 100;
        bool is_charging = false;
        QColor bar_color = QColor(0, 230, 118); // Neon Green

        switch (bat_status) {
        case Common::Input::BatteryLevel::Charging:
            pct = 100;
            is_charging = true;
            bar_color = QColor(0, 229, 255); // Cyan charging
            break;
        case Common::Input::BatteryLevel::Full:
            pct = 100;
            bar_color = QColor(0, 230, 118); // Bright green
            break;
        case Common::Input::BatteryLevel::Medium:
            pct = 75;
            bar_color = QColor(174, 234, 0); // Lime/Amber
            break;
        case Common::Input::BatteryLevel::Low:
            pct = 40;
            bar_color = QColor(255, 171, 0); // Amber/Orange
            break;
        case Common::Input::BatteryLevel::Critical:
            pct = 15;
            bar_color = QColor(255, 23, 68); // Red
            break;
        case Common::Input::BatteryLevel::Empty:
            pct = 5;
            bar_color = QColor(213, 0, 0); // Deep red
            break;
        default:
            pct = 100;
            break;
        }

        // Outer Card Container (Anchored away from controller)
        const float card_w = 126.0f;
        const float card_h = 44.0f;
        const QRectF card_rect(bat_pos.x(), bat_pos.y(), card_w, card_h);
        p.setPen(QPen(QColor(45, 52, 66), 1.2f));
        p.setBrush(QColor(12, 15, 22, 230));
        p.drawRoundedRect(card_rect, 8.0f, 8.0f);

        // Battery Shell
        const QRectF b_shell(bat_pos.x() + 8.0f, bat_pos.y() + 13.0f, 38.0f, 18.0f);
        p.setPen(QPen(QColor(70, 75, 88), 1.5f));
        p.setBrush(QColor(8, 10, 14));
        p.drawRoundedRect(b_shell, 3.5f, 3.5f);

        // Positive Terminal Pip
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(90, 96, 110));
        p.drawRoundedRect(QRectF(b_shell.right() + 1.0f, b_shell.top() + 4.5f, 3.5f, 9.0f), 1.5f, 1.5f);

        // Inner Charge Level Fill
        const float max_fill_w = b_shell.width() - 4.0f;
        const float fill_w = std::max(2.0f, max_fill_w * (pct / 100.0f));
        const QRectF fill_rect(b_shell.left() + 2.0f, b_shell.top() + 2.0f, fill_w, b_shell.height() - 4.0f);

        QLinearGradient fill_grad(fill_rect.topLeft(), fill_rect.bottomLeft());
        fill_grad.setColorAt(0.0, bar_color.lighter(135));
        fill_grad.setColorAt(0.5, bar_color);
        fill_grad.setColorAt(1.0, bar_color.darker(120));
        p.setBrush(fill_grad);
        p.drawRoundedRect(fill_rect, 2.0f, 2.0f);

        // If charging, draw a bright charging bolt icon
        if (is_charging) {
            p.setPen(QColor(255, 235, 59));
            p.setBrush(QColor(255, 235, 59));
            QPolygonF bolt;
            const float bx = b_shell.left() + b_shell.width() * 0.5f;
            const float by = b_shell.top() + b_shell.height() * 0.5f;
            bolt << QPointF(bx + 1.5f, by - 6.0f)
                 << QPointF(bx - 3.5f, by)
                 << QPointF(bx - 0.5f, by)
                 << QPointF(bx - 2.5f, by + 6.0f)
                 << QPointF(bx + 3.5f, by - 1.0f)
                 << QPointF(bx + 0.5f, by - 1.0f);
            p.drawPolygon(bolt);
        }

        // Percentage Text (Clean, crisp, bold 10pt)
        QFont font_pct(QStringLiteral("Segoe UI"), 10, QFont::Bold);
        p.setFont(font_pct);
        p.setPen(is_charging ? QColor(0, 240, 255) : QColor(245, 248, 255));
        const QString pct_str = is_charging ? QStringLiteral("⚡ %1%").arg(pct) : QStringLiteral("%1%").arg(pct);
        const QRectF pct_rect(bat_pos.x() + 52.0f, bat_pos.y() + 6.0f, 68.0f, 18.0f);
        p.drawText(pct_rect, Qt::AlignVCenter | Qt::AlignLeft, pct_str);

        // Subtext "Батарея" / "Зарядка" (Clean 8pt)
        QFont font_sub(QStringLiteral("Segoe UI"), 8, QFont::Normal);
        p.setFont(font_sub);
        p.setPen(QColor(150, 160, 175));
        const QRectF sub_rect(bat_pos.x() + 52.0f, bat_pos.y() + 24.0f, 68.0f, 15.0f);
        p.drawText(sub_rect, Qt::AlignVCenter | Qt::AlignLeft,
                   is_charging ? QStringLiteral("Зарядка") : QStringLiteral("Батарея"));
    };

    // =========================================================================
    // HUD COMPONENT 2: Colorful 3D Gyroscope (Top-Right, Clear of Controller)
    // =========================================================================
    auto DrawColorfulGyroscope = [&](const QPointF gyro_c, const Common::Vec3f& euler, float size) {
        // Instrument Backdrop Dial
        p.setPen(QPen(QColor(0, 229, 255, 90), 1.2f));
        p.setBrush(QColor(12, 15, 22, 220));
        p.drawEllipse(gyro_c, 36.0f, 36.0f);

        // Compass / Attitude Ring Ticks
        p.setPen(QPen(QColor(255, 255, 255, 40), 1.0f));
        for (int k = 0; k < 8; ++k) {
            const float ang = k * PI_CONST / 4.0f;
            const QPointF p1 = gyro_c + QPointF(std::cos(ang) * 31.0f, std::sin(ang) * 31.0f);
            const QPointF p2 = gyro_c + QPointF(std::cos(ang) * 35.0f, std::sin(ang) * 35.0f);
            p.drawLine(p1, p2);
        }

        // 8 Cube Vertices in local 3D space
        std::array<Common::Vec3f, 8> v = {
            Common::Vec3f{-0.85f, -0.85f, -0.85f}, // 0
            Common::Vec3f{ 0.85f, -0.85f, -0.85f}, // 1
            Common::Vec3f{ 0.85f,  0.85f, -0.85f}, // 2
            Common::Vec3f{-0.85f,  0.85f, -0.85f}, // 3
            Common::Vec3f{-0.85f, -0.85f,  0.85f}, // 4
            Common::Vec3f{ 0.85f, -0.85f,  0.85f}, // 5
            Common::Vec3f{ 0.85f,  0.85f,  0.85f}, // 6
            Common::Vec3f{-0.85f,  0.85f,  0.85f}, // 7
        };

        for (auto& pt : v) {
            pt.RotateFromOrigin(euler.x, euler.y, euler.z);
            pt *= size;
        }

        // 6 Colored Faces with distinct cyber/flight colors
        struct GyroFace {
            std::array<int, 4> idx;
            QColor color;
            float depth;
        };

        std::array<GyroFace, 6> faces = {
            GyroFace{{4, 5, 6, 7}, QColor(0, 229, 255, 205), (v[4].z + v[5].z + v[6].z + v[7].z) * 0.25f}, // Top (+Z, Cyan)
            GyroFace{{0, 3, 2, 1}, QColor(26, 35, 126, 185), (v[0].z + v[3].z + v[2].z + v[1].z) * 0.25f}, // Bottom (-Z, Blue)
            GyroFace{{3, 2, 6, 7}, QColor(0, 230, 118, 205), (v[3].z + v[2].z + v[6].z + v[7].z) * 0.25f}, // Front (+Y, Green)
            GyroFace{{0, 1, 5, 4}, QColor(0, 105, 92,  185), (v[0].z + v[1].z + v[5].z + v[4].z) * 0.25f}, // Back (-Y, Teal)
            GyroFace{{1, 2, 6, 5}, QColor(255, 23, 68, 205), (v[1].z + v[2].z + v[6].z + v[5].z) * 0.25f}, // Right (+X, Red/Magenta)
            GyroFace{{0, 4, 7, 3}, QColor(255, 145, 0, 205), (v[0].z + v[4].z + v[7].z + v[3].z) * 0.25f}, // Left (-X, Gold/Amber)
        };

        // Sort faces back to front (Painter's algorithm on the 3D gyro)
        std::sort(faces.begin(), faces.end(), [](const GyroFace& a, const GyroFace& b) {
            return a.depth < b.depth;
        });

        for (const auto& f : faces) {
            const QPointF p0 = gyro_c + QPointF(v[f.idx[0]].x, v[f.idx[0]].y);
            const QPointF p1 = gyro_c + QPointF(v[f.idx[1]].x, v[f.idx[1]].y);
            const QPointF p2 = gyro_c + QPointF(v[f.idx[2]].x, v[f.idx[2]].y);
            const float cross = (p1.x() - p0.x()) * (p2.y() - p0.y()) - (p1.y() - p0.y()) * (p2.x() - p0.x());
            if (cross > 0.0f) {
                QPolygonF poly;
                poly << p0 << p1 << p2 << (gyro_c + QPointF(v[f.idx[3]].x, v[f.idx[3]].y));
                p.setPen(QPen(f.color.lighter(130), 1.2f));
                p.setBrush(f.color);
                p.drawPolygon(poly);
            }
        }

        // Projecting 3D Coordinate Arrows
        auto DrawAxis = [&](Common::Vec3f dir, const QColor& col) {
            dir.RotateFromOrigin(euler.x, euler.y, euler.z);
            const QPointF end_pt = gyro_c + QPointF(dir.x * size * 1.55f, dir.y * size * 1.55f);
            p.setPen(QPen(col, 1.8f));
            p.drawLine(gyro_c, end_pt);
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawEllipse(end_pt, 2.5f, 2.5f);
        };
        DrawAxis(Common::Vec3f{1, 0, 0}, QColor(255, 23, 68));  // X = Red
        DrawAxis(Common::Vec3f{0, 1, 0}, QColor(0, 230, 118));  // Y = Green
        DrawAxis(Common::Vec3f{0, 0, 1}, QColor(0, 229, 255));  // Z = Cyan

        // Header label
        QFont font_hdr(QStringLiteral("Segoe UI"), 8, QFont::Bold);
        p.setFont(font_hdr);
        p.setPen(colors.indicator);
        const QRectF hdr_rect(gyro_c.x() - 50.0f, gyro_c.y() - 54.0f, 100.0f, 16.0f);
        p.drawText(hdr_rect, Qt::AlignCenter, QStringLiteral("ГИРОСКОП"));

        // Angles text badge with clear, visible degree values
        const float pitch_deg = euler.x * 180.0f / PI_CONST;
        const float roll_deg  = euler.z * 180.0f / PI_CONST;
        const QString angles_str = QStringLiteral("P: %1°  R: %2°")
                                      .arg(int(std::round(pitch_deg)))
                                      .arg(int(std::round(roll_deg)));
        const QRectF badge_r(gyro_c.x() - 54.0f, gyro_c.y() + 42.0f, 108.0f, 20.0f);
        p.setPen(QPen(QColor(40, 48, 64), 1.2f));
        p.setBrush(QColor(12, 16, 24, 230));
        p.drawRoundedRect(badge_r, 5.0f, 5.0f);

        QFont font_angles(QStringLiteral("Segoe UI"), 8, QFont::Bold);
        p.setFont(font_angles);
        p.setPen(QColor(230, 240, 255));
        p.drawText(badge_r, Qt::AlignCenter, angles_str);
    };

    // =========================================================================
    // HUD COMPONENT 3: Precision Stick Radars (Under Controller, Centered L/R)
    // =========================================================================
    auto DrawModernStickRadar = [&](bool is_left, const QPointF radar_c) {
        const auto stick_id = is_left ? Settings::NativeAnalog::LStick : Settings::NativeAnalog::RStick;
        const auto button_id = is_left ? Settings::NativeButton::LStick : Settings::NativeButton::RStick;
        const auto& stick = stick_values[stick_id];
        const bool is_clicked = button_values[button_id].value;

        const float sx = std::clamp(stick.x.value, -1.0f, 1.0f);
        const float sy = std::clamp(stick.y.value, -1.0f, 1.0f);

        constexpr float radar_r = 38.0f;

        // Dark Radar Glass Plate with subtle radial gradient
        QRadialGradient plate_grad(radar_c, radar_r * 1.2f);
        plate_grad.setColorAt(0.0, QColor(16, 20, 28, 220));
        plate_grad.setColorAt(0.7, QColor(10, 12, 18, 235));
        plate_grad.setColorAt(1.0, QColor(6, 8, 12, 245));
        p.setPen(QPen(is_clicked ? colors.indicator : QColor(40, 46, 58), 1.4f));
        p.setBrush(plate_grad);
        p.drawEllipse(radar_c, radar_r, radar_r);

        // Outer 100% Boundary Ring (dashed)
        p.setPen(QPen(QColor(0, 240, 255, 90), 1.0f, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(radar_c, radar_r * 0.90f, radar_r * 0.90f);

        // 50% Mid-Range Guide Ring
        p.setPen(QPen(QColor(255, 255, 255, 25), 0.8f));
        p.drawEllipse(radar_c, radar_r * 0.45f, radar_r * 0.45f);

        // Crosshair Lines
        p.setPen(QPen(QColor(255, 255, 255, 30), 0.8f));
        p.drawLine(QPointF(radar_c.x() - radar_r * 0.85f, radar_c.y()), QPointF(radar_c.x() + radar_r * 0.85f, radar_c.y()));
        p.drawLine(QPointF(radar_c.x(), radar_c.y() - radar_r * 0.85f), QPointF(radar_c.x(), radar_c.y() + radar_r * 0.85f));

        // Deadzone Circle (Amber/Red glow)
        const float deadzone_r = radar_r * 0.90f * std::clamp(stick.x.properties.deadzone, 0.05f, 0.45f);
        p.setPen(QPen(colors.deadzone, 0.9f, Qt::DotLine));
        p.setBrush(QColor(colors.deadzone.red(), colors.deadzone.green(), colors.deadzone.blue(), 25));
        p.drawEllipse(radar_c, deadzone_r, deadzone_r);

        // Deflection Vector Line from center to target
        const QPointF target_pt = radar_c + QPointF(sx * radar_r * 0.90f, sy * radar_r * 0.90f);
        p.setPen(QPen(colors.indicator, 1.8f));
        p.drawLine(radar_c, target_pt);

        // Stick Target Dot with Multi-Layer Glow
        QRadialGradient dot_glow(target_pt, 9.0f);
        dot_glow.setColorAt(0.0, colors.indicator);
        dot_glow.setColorAt(0.4, QColor(colors.indicator.red(), colors.indicator.green(), colors.indicator.blue(), 120));
        dot_glow.setColorAt(1.0, QColor(colors.indicator.red(), colors.indicator.green(), colors.indicator.blue(), 0));
        p.setPen(Qt::NoPen);
        p.setBrush(dot_glow);
        p.drawEllipse(target_pt, 9.0f, 9.0f);

        p.setBrush(QColor(255, 255, 255));
        p.drawEllipse(target_pt, 3.5f, 3.5f);

        // Stick Click Badge (L3 / R3)
        if (is_clicked) {
            const QRectF click_rect(radar_c.x() - 18.0f, radar_c.y() - 10.0f, 36.0f, 20.0f);
            p.setPen(QPen(colors.indicator, 1.2f));
            p.setBrush(colors.highlight);
            p.drawRoundedRect(click_rect, 4.0f, 4.0f);
            QFont font_click(QStringLiteral("Segoe UI"), 8, QFont::Bold);
            p.setFont(font_click);
            p.setPen(QColor(255, 255, 255));
            p.drawText(click_rect, Qt::AlignCenter, is_left ? QStringLiteral("L3") : QStringLiteral("R3"));
        }

        // Coordinate text badge below radar with clear visible numbers
        const QString coord_str = QStringLiteral("%1: X[%2] Y[%3]")
                                      .arg(is_left ? QStringLiteral("L") : QStringLiteral("R"))
                                      .arg(sx, 5, 'f', 2, QLatin1Char(' '))
                                      .arg(sy, 5, 'f', 2, QLatin1Char(' '));
        const QRectF badge_rect(radar_c.x() - 62.0f, radar_c.y() + radar_r + 6.0f, 124.0f, 20.0f);
        p.setPen(QPen(QColor(40, 48, 64), 1.2f));
        p.setBrush(QColor(12, 16, 24, 230));
        p.drawRoundedRect(badge_rect, 5.0f, 5.0f);

        QFont font_coord(QStringLiteral("Segoe UI"), 8, QFont::Bold);
        p.setFont(font_coord);
        p.setPen(QColor(0, 240, 255));
        p.drawText(badge_rect, Qt::AlignCenter, coord_str);
    };

    // Calculate non-overlapping HUD positions (Battery shifted safely to top-left)
    const float hud_cx = static_cast<float>(center.x());
    const float hud_cy = static_cast<float>(center.y());
    const float hud_w  = static_cast<float>(rect().width());

    const float bat_x = std::max(16.0f, hud_cx - 410.0f);
    const float bat_y = std::max(16.0f, hud_cy - 210.0f);
    const QPointF bat_pos(bat_x, bat_y);

    const float gyro_x = std::min(hud_w - 68.0f, hud_cx + 330.0f);
    const float gyro_y = std::max(60.0f, hud_cy - 170.0f);
    const QPointF gyro_pos(gyro_x, gyro_y);

    const QPointF left_radar  = center + QPointF(-115.0f, 182.0f);
    const QPointF right_radar = center + QPointF( 115.0f, 182.0f);


    // =========================================================================
    // MODE A: REAR VIEW (Р’РР” РЎР—РђР”Р) вЂ” PHOTOREALISTIC
    // =========================================================================
    if (is_rear_view) {
        // 1. Studio Drop Shadow beneath rear chassis
        {
            const auto sp = Project(0.0f, 115.0f, -10.0f);
            const float s_rx = 250.0f * sp.scale;
            const float s_ry = 90.0f * sp.scale;
            QRadialGradient floor_shadow(sp.pt, s_rx);
            floor_shadow.setColorAt(0.0, QColor(0, 0, 0, 160));
            floor_shadow.setColorAt(0.35, QColor(0, 0, 0, 95));
            floor_shadow.setColorAt(0.70, QColor(0, 0, 0, 35));
            floor_shadow.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(floor_shadow);
            p.drawEllipse(sp.pt, s_rx, s_ry);
        }

        // 2. Top Shoulder assembly from behind: USB-C port, Sync button, status LED
        {
            const auto usbc_pos = Project(0.0f, -88.0f, 2.0f);
            const float port_w = 8.5f * usbc_pos.scale;
            const float port_h = 4.0f * usbc_pos.scale;
            p.setPen(QPen(QColor(52, 58, 68), 1.2f));
            QRadialGradient port_g(usbc_pos.pt, port_w * 1.5f);
            port_g.setColorAt(0.0, QColor(6, 8, 12));
            port_g.setColorAt(0.6, QColor(12, 14, 18));
            port_g.setColorAt(1.0, QColor(28, 32, 38));
            p.setBrush(port_g);
            p.drawRoundedRect(QRectF(usbc_pos.pt.x() - port_w, usbc_pos.pt.y() - port_h,
                                     port_w * 2.0f, port_h * 2.0f), 3.0f, 2.0f);

            p.setPen(Qt::NoPen);
            QLinearGradient tongue_g(usbc_pos.pt - QPointF(0, port_h * 0.3f),
                                     usbc_pos.pt + QPointF(0, port_h * 0.3f));
            tongue_g.setColorAt(0.0, QColor(170, 175, 188));
            tongue_g.setColorAt(0.5, QColor(195, 200, 212));
            tongue_g.setColorAt(1.0, QColor(140, 145, 155));
            p.setBrush(tongue_g);
            p.drawRoundedRect(QRectF(usbc_pos.pt.x() - port_w * 0.6f, usbc_pos.pt.y() - port_h * 0.3f,
                                     port_w * 1.2f, port_h * 0.6f), 1.0f, 1.0f);

            const auto sync_pos = Project(22.0f, -88.0f, 2.0f);
            QRadialGradient sync_g(sync_pos.pt, 3.2f * sync_pos.scale);
            sync_g.setColorAt(0.0, QColor(48, 52, 60));
            sync_g.setColorAt(0.6, QColor(32, 35, 42));
            sync_g.setColorAt(1.0, QColor(20, 22, 28));
            p.setPen(QPen(QColor(55, 60, 70), 0.8f));
            p.setBrush(sync_g);
            p.drawEllipse(sync_pos.pt, 3.0f * sync_pos.scale, 3.0f * sync_pos.scale);

            const auto sync_led = Project(-22.0f, -88.0f, 2.0f);
            if (is_connected) {
                QRadialGradient led_glow(sync_led.pt, 6.0f * sync_led.scale);
                led_glow.setColorAt(0.0, QColor(0, 245, 140, 190));
                led_glow.setColorAt(0.5, QColor(0, 245, 140, 70));
                led_glow.setColorAt(1.0, QColor(0, 245, 140, 0));
                p.setPen(Qt::NoPen);
                p.setBrush(led_glow);
                p.drawEllipse(sync_led.pt, 6.0f * sync_led.scale, 6.0f * sync_led.scale);
            }
            p.setPen(Qt::NoPen);
            p.setBrush(is_connected ? QColor(0, 245, 140) : QColor(35, 40, 48));
            p.drawEllipse(sync_led.pt, 2.0f * sync_led.scale, 2.0f * sync_led.scale);
        }

        // 3. Triggers ZL & ZR and Bumpers L & R from behind with clear gap
        auto DrawRearBumper = [&](bool is_left, bool pressed, const QString& label) {
            const float sgn = is_left ? -1.0f : 1.0f;
            const float dy = pressed ? 3.5f : 0.0f;
            const auto p_t_in  = Project(sgn * 56.0f,  -174.0f + dy, 6.0f);
            const auto p_t_out = Project(sgn * 122.0f, -166.0f + dy, 6.0f);
            const auto p_b_out = Project(sgn * 124.0f, -153.0f + dy, 6.0f);
            const auto p_b_in  = Project(sgn * 54.0f,  -158.0f + dy, 6.0f);

            QPolygonF poly;
            poly << p_t_in.pt << p_t_out.pt << p_b_out.pt << p_b_in.pt;

            const QColor b_col = pressed ? colors.highlight : colors.button;
            QLinearGradient bg(p_t_in.pt, p_b_in.pt);
            bg.setColorAt(0.0, b_col.lighter(120));
            bg.setColorAt(0.5, b_col);
            bg.setColorAt(1.0, b_col.darker(125));

            p.setPen(QPen(pressed ? colors.indicator : QColor(55, 62, 75), 1.5f));
            p.setBrush(bg);
            p.drawPolygon(poly);

            const auto lbl_pos = Project(sgn * 88.0f, -163.0f + dy, 6.5f);
            const float lbl_w = 36.0f * lbl_pos.scale;
            const float lbl_h = 18.0f * lbl_pos.scale;
            const QRectF lbl_rect(lbl_pos.pt.x() - lbl_w * 0.5f, lbl_pos.pt.y() - lbl_h * 0.5f, lbl_w, lbl_h);

            QFont font_lbl(QStringLiteral("Segoe UI"), 9, QFont::Bold);
            p.setFont(font_lbl);
            p.setPen(pressed ? QColor(255, 255, 255) : QColor(220, 230, 245));
            p.drawText(lbl_rect, Qt::AlignCenter, label);
        };
        DrawRearBumper(true, button_values[L].value, QStringLiteral("L"));
        DrawRearBumper(false, button_values[R].value, QStringLiteral("R"));

        auto DrawRearTrigger = [&](bool is_left, float analog, bool pressed, const QString& label) {
            const float sgn = is_left ? -1.0f : 1.0f;
            const float dy = analog * 6.5f;
            const bool is_active = (analog > 0.05f || pressed);

            // Elevated trigger in rear view with gap from bumper
            const auto p_t_in  = Project(sgn * 58.0f,  -145.0f + dy, -6.0f);
            const auto p_t_out = Project(sgn * 126.0f, -138.0f + dy, -6.0f);
            const auto p_b_out = Project(sgn * 128.0f, -120.0f + dy, -6.0f);
            const auto p_b_in  = Project(sgn * 56.0f,  -125.0f + dy, -6.0f);

            QPolygonF trig_poly;
            trig_poly << p_t_in.pt << p_t_out.pt << p_b_out.pt << p_b_in.pt;

            const QColor t_col = is_active ? colors.indicator : colors.button;
            QLinearGradient tg(p_t_in.pt, p_b_in.pt);
            tg.setColorAt(0.0, t_col.lighter(125));
            tg.setColorAt(0.5, t_col);
            tg.setColorAt(1.0, t_col.darker(125));

            p.setPen(QPen(is_active ? colors.indicator.lighter(130) : QColor(50, 58, 72), 1.6f));
            p.setBrush(tg);
            p.drawPolygon(trig_poly);

            const auto lbl = Project(sgn * 92.0f, -132.0f + dy, -5.5f);
            const float lbl_w = 40.0f * lbl.scale;
            const float lbl_h = 20.0f * lbl.scale;
            const QRectF lbl_rect(lbl.pt.x() - lbl_w * 0.5f, lbl.pt.y() - lbl_h * 0.5f, lbl_w, lbl_h);

            QFont font_lbl(QStringLiteral("Segoe UI"), 10, QFont::Bold);
            p.setFont(font_lbl);
            p.setPen(is_active ? QColor(255, 255, 255) : QColor(220, 230, 245));
            p.drawText(lbl_rect, Qt::AlignCenter, label);
        };
        DrawRearTrigger(true, zl_analog, button_values[ZL].value, QStringLiteral("ZL"));
        DrawRearTrigger(false, zr_analog, button_values[ZR].value, QStringLiteral("ZR"));

        // 4. Rear Shell Body
        {
            QPolygonF rear_body;
            for (std::size_t i = 0; i < pro_body.size() / 2; ++i) {
                rear_body << Project(pro_body[i * 2 + 0] * 0.94f, pro_body[i * 2 + 1] * 0.95f, 0.0f).pt;
            }
            for (int i = static_cast<int>(pro_body.size() / 2) - 1; i >= 0; --i) {
                rear_body << Project(-pro_body[i * 2 + 0] * 0.94f, pro_body[i * 2 + 1] * 0.95f, 0.0f).pt;
            }

            const auto rt = Project(0.0f, -90.0f, 0.0f);
            const auto rb = Project(0.0f, 60.0f, 0.0f);
            QLinearGradient rear_grad(rt.pt, rb.pt);
            rear_grad.setColorAt(0.0, colors.primary.darker(130));
            rear_grad.setColorAt(0.2, colors.primary.darker(142));
            rear_grad.setColorAt(0.5, colors.primary.darker(152));
            rear_grad.setColorAt(0.8, colors.primary.darker(162));
            rear_grad.setColorAt(1.0, colors.primary.darker(175));

            p.setPen(QPen(QColor(18, 20, 25), 1.2f));
            p.setBrush(rear_grad);
            p.drawPolygon(rear_body);
        }

        // 5. Rear Handles with diamond anti-slip grip texture
        auto DrawRearHandle = [&](bool is_left) {
            const float sgn = is_left ? 1.0f : -1.0f;
            const QColor grip_col = is_left ? colors.right : colors.left;

            QPolygonF rear_handle;
            for (std::size_t i = 0; i < pro_left_handle.size() / 2; ++i) {
                const float lx = -sgn * pro_left_handle[i * 2 + 0] * 0.94f;
                const float ly = pro_left_handle[i * 2 + 1] * 0.95f;
                rear_handle << Project(lx, ly, 0.0f).pt;
            }

            const auto h_top = Project(sgn * 185.0f, -20.0f, 0.0f);
            const auto h_bot = Project(sgn * 145.0f, 130.0f, 0.0f);
            QLinearGradient h_grad(h_top.pt, h_bot.pt);
            h_grad.setColorAt(0.0, grip_col.darker(135));
            h_grad.setColorAt(0.3, grip_col.darker(150));
            h_grad.setColorAt(0.7, grip_col.darker(160));
            h_grad.setColorAt(1.0, grip_col.darker(145));

            p.setPen(QPen(QColor(18, 20, 25), 1.2f));
            p.setBrush(h_grad);
            p.drawPolygon(rear_handle);

            // Diamond Anti-Slip Texture
            p.setPen(Qt::NoPen);
            const int start_x = is_left ? 120 : -182;
            const int end_x   = is_left ? 182 : -120;
            for (int dy = -10; dy <= 95; dy += 11) {
                for (int dx = start_x; dx <= end_x; dx += 11) {
                    const int offset = ((dy / 11) % 2 == 0) ? 5 : 0;
                    const auto dot = Project(static_cast<float>(dx + offset), static_cast<float>(dy), 0.1f);
                    const float ds = 2.0f * dot.scale;
                    QPolygonF diamond;
                    diamond << QPointF(dot.pt.x(), dot.pt.y() - ds)
                            << QPointF(dot.pt.x() + ds * 0.7f, dot.pt.y())
                            << QPointF(dot.pt.x(), dot.pt.y() + ds)
                            << QPointF(dot.pt.x() - ds * 0.7f, dot.pt.y());
                    p.setBrush(QColor(255, 255, 255, 14));
                    p.drawPolygon(diamond);
                }
            }
        };
        DrawRearHandle(true);
        DrawRearHandle(false);

        // 6. Central Recessed Battery Hatch Panel
        {
            QPolygonF batt_hatch;
            batt_hatch << Project(-46.0f, -40.0f, 0.0f).pt
                       << Project( 46.0f, -40.0f, 0.0f).pt
                       << Project( 44.0f,  30.0f, 0.0f).pt
                       << Project(-44.0f,  30.0f, 0.0f).pt;

            const auto bht = Project(0.0f, -40.0f, 0.0f);
            const auto bhb = Project(0.0f, 30.0f, 0.0f);
            QLinearGradient bh_grad(bht.pt, bhb.pt);
            bh_grad.setColorAt(0.0, colors.primary.darker(165));
            bh_grad.setColorAt(0.3, colors.primary.darker(175));
            bh_grad.setColorAt(0.7, colors.primary.darker(180));
            bh_grad.setColorAt(1.0, colors.primary.darker(170));

            p.setPen(QPen(QColor(25, 28, 35), 1.2f));
            p.setBrush(bh_grad);
            p.drawPolygon(batt_hatch);

            p.setPen(QPen(QColor(85, 90, 102, 90), 0.8f));
            p.drawLine(Project(-45.5f, -39.5f, 0.0f).pt, Project(45.5f, -39.5f, 0.0f).pt);

            // Nintendo Switch Logo Engraving
            const auto logo_pos = Project(0.0f, -15.0f, 0.1f);
            p.setPen(QPen(QColor(140, 145, 155, 130), 1.0f));
            p.setBrush(Qt::NoBrush);
            const float lw = 14.0f * logo_pos.scale;
            const float lh = 18.0f * logo_pos.scale;
            p.drawRoundedRect(QRectF(logo_pos.pt.x() - lw, logo_pos.pt.y() - lh * 0.5f, lw * 2.0f, lh), 3.0f, 3.0f);
            p.drawLine(QPointF(logo_pos.pt.x(), logo_pos.pt.y() - lh * 0.5f),
                       QPointF(logo_pos.pt.x(), logo_pos.pt.y() + lh * 0.5f));
            p.setBrush(QColor(140, 145, 155, 130));
            p.drawEllipse(QPointF(logo_pos.pt.x() - lw * 0.5f, logo_pos.pt.y() - lh * 0.2f), 1.5f * logo_pos.scale, 1.5f * logo_pos.scale);
            p.drawEllipse(QPointF(logo_pos.pt.x() + lw * 0.5f, logo_pos.pt.y() + lh * 0.2f), 1.5f * logo_pos.scale, 1.5f * logo_pos.scale);

            p.setPen(QColor(110, 115, 125, 120));
            SetTextFont(p, 0.60f * logo_pos.scale);
            DrawText(p, Project(0.0f, 10.0f, 0.1f).pt, 0.60f * logo_pos.scale, QStringLiteral("MOD. HAC-013  5V=500mA"));

            // 4 Steel Phillips Screws
            auto DrawScrew = [&](float sx, float sy) {
                const auto sp = Project(sx, sy, 0.1f);
                const float sr = 3.0f * sp.scale;
                QRadialGradient screw_grad(sp.pt - QPointF(sr * 0.2f, sr * 0.2f), sr * 1.2f);
                screw_grad.setColorAt(0.0, QColor(120, 125, 140));
                screw_grad.setColorAt(0.4, QColor(85, 90, 100));
                screw_grad.setColorAt(0.8, QColor(55, 60, 68));
                screw_grad.setColorAt(1.0, QColor(28, 30, 36));
                p.setPen(QPen(QColor(20, 22, 28), 0.8f));
                p.setBrush(screw_grad);
                p.drawEllipse(sp.pt, sr, sr);
                p.setPen(QPen(QColor(35, 38, 45), 0.9f));
                p.drawLine(QPointF(sp.pt.x() - sr * 0.5f, sp.pt.y()), QPointF(sp.pt.x() + sr * 0.5f, sp.pt.y()));
                p.drawLine(QPointF(sp.pt.x(), sp.pt.y() - sr * 0.5f), QPointF(sp.pt.x(), sp.pt.y() + sr * 0.5f));
            };
            DrawScrew(-50.0f, -25.0f);
            DrawScrew( 50.0f, -25.0f);
            DrawScrew(-48.0f,  35.0f);
            DrawScrew( 48.0f,  35.0f);
        }

        // 7. Orientation Badge "Р’РР” РЎР—РђР”Р"
        {
            const auto badge_pos = center + QPointF(0.0f, -145.0f);
            p.setPen(QPen(colors.indicator, 1.0f));
            p.setBrush(QColor(15, 20, 28, 200));
            p.drawRoundedRect(QRectF(badge_pos.x() - 48.0f, badge_pos.y() - 11.0f, 96.0f, 22.0f), 5.0f, 5.0f);
            p.setPen(colors.indicator);
            SetTextFont(p, 0.70f);
            DrawText(p, badge_pos, 0.70f, QStringLiteral("Р’РР” РЎР—РђР”Р"));
        }

        // 8. Draw HUD elements in Rear View
        DrawPhotorealisticBattery(bat_pos);
        DrawColorfulGyroscope(gyro_pos, smooth_euler, 16.0f);
        DrawModernStickRadar(true, left_radar);
        DrawModernStickRadar(false, right_radar);
        return;
    }

    // =========================================================================
    // MODE B: FRONT VIEW (Р’РР” РЎРџР•Р Р•Р”Р) вЂ” PHOTOREALISTIC STUDIO PRESENTATION
    // Deterministic Back-to-Front Layering: Buttons & Sticks CAN NEVER DISAPPEAR!
    // =========================================================================

    // -------------------------------------------------------------------------
    // LAYER 1: Ambient Studio Drop Shadow with Contact Shadows
    // -------------------------------------------------------------------------
    {
        const auto sp = Project(0.0f, 115.0f, -10.0f);
        const float s_rx = 250.0f * sp.scale;
        const float s_ry = 90.0f * sp.scale;
        QRadialGradient floor_shadow(sp.pt, s_rx);
        floor_shadow.setColorAt(0.0, QColor(0, 0, 0, 160));
        floor_shadow.setColorAt(0.35, QColor(0, 0, 0, 95));
        floor_shadow.setColorAt(0.70, QColor(0, 0, 0, 35));
        floor_shadow.setColorAt(1.0, QColor(0, 0, 0, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(floor_shadow);
        p.drawEllipse(sp.pt, s_rx, s_ry);

        auto DrawContactShadow = [&](float cx, float cy, float rx, float ry, int alpha) {
            const auto cp = Project(cx, cy, -8.0f);
            QRadialGradient cs(cp.pt, rx * cp.scale);
            cs.setColorAt(0.0, QColor(0, 0, 0, alpha));
            cs.setColorAt(0.6, QColor(0, 0, 0, alpha / 3));
            cs.setColorAt(1.0, QColor(0, 0, 0, 0));
            p.setBrush(cs);
            p.drawEllipse(cp.pt, rx * cp.scale, ry * cp.scale);
        };
        DrawContactShadow(-155.0f, 128.0f, 38.0f, 14.0f, 160);
        DrawContactShadow( 155.0f, 128.0f, 38.0f, 14.0f, 160);
        DrawContactShadow(   0.0f,  68.0f, 55.0f, 16.0f, 130);
    }

    // -------------------------------------------------------------------------
    // LAYER 2: Triggers (ZL / ZR) & Bumpers (L / R) with Clear Offset & High Visibility
    // -------------------------------------------------------------------------
    auto DrawShoulderTrigger = [&](bool is_left, float analog, bool pressed, const QString& label) {
        const float sgn = is_left ? -1.0f : 1.0f;
        const bool is_active = (analog > 0.05f || pressed);
        const float dy = analog * 7.0f;

        const auto p_t_in  = Project(sgn * 60.0f,  -175.0f + dy, -16.0f);
        const auto p_t_out = Project(sgn * 126.0f, -166.0f + dy, -16.0f);
        const auto p_b_out = Project(sgn * 128.0f, -149.0f + dy, -16.0f);
        const auto p_b_in  = Project(sgn * 58.0f,  -154.0f + dy, -16.0f);

        QPolygonF trig_poly;
        trig_poly << p_t_in.pt << p_t_out.pt << p_b_out.pt << p_b_in.pt;

        QLinearGradient tg(p_t_in.pt, p_b_in.pt);
        if (is_active) {
            tg.setColorAt(0.0, colors.indicator.lighter(135));
            tg.setColorAt(0.4, colors.indicator);
            tg.setColorAt(1.0, colors.indicator.darker(120));
        } else {
            tg.setColorAt(0.0, QColor(64, 70, 84));
            tg.setColorAt(0.3, QColor(46, 52, 62));
            tg.setColorAt(0.8, QColor(30, 34, 42));
            tg.setColorAt(1.0, QColor(22, 25, 30));
        }

        p.setPen(QPen(is_active ? colors.indicator.lighter(130) : QColor(56, 64, 78), 1.8f));
        p.setBrush(tg);
        p.drawPolygon(trig_poly);

        // Top specular metallic rim
        p.setPen(QPen(is_active ? QColor(255, 255, 255, 190) : QColor(255, 255, 255, 50), 1.2f));
        p.drawLine(p_t_in.pt, p_t_out.pt);

        // Prominent bold label "ZL" / "ZR"
        const auto lbl_pos = Project(sgn * 92.0f, -161.0f + dy, -15.0f);
        const float lbl_w = 42.0f * lbl_pos.scale;
        const float lbl_h = 22.0f * lbl_pos.scale;
        const QRectF lbl_rect(lbl_pos.pt.x() - lbl_w * 0.5f, lbl_pos.pt.y() - lbl_h * 0.5f, lbl_w, lbl_h);

        QFont font_lbl(QStringLiteral("Segoe UI"), 10, QFont::Bold);
        p.setFont(font_lbl);
        p.setPen(is_active ? QColor(255, 255, 255) : QColor(215, 225, 240));
        p.drawText(lbl_rect, Qt::AlignCenter, label);
    };

    DrawShoulderTrigger(true, zl_analog, button_values[ZL].value, QStringLiteral("ZL"));
    DrawShoulderTrigger(false, zr_analog, button_values[ZR].value, QStringLiteral("ZR"));

    auto DrawShoulderBumper = [&](bool is_left, bool pressed, const QString& label) {
        const float sgn = is_left ? -1.0f : 1.0f;
        const float dy = pressed ? 3.5f : 0.0f;

        const auto p_t_in  = Project(sgn * 56.0f,  -144.0f + dy, 6.0f);
        const auto p_t_out = Project(sgn * 122.0f, -137.0f + dy, 6.0f);
        const auto p_b_out = Project(sgn * 124.0f, -123.0f + dy, 6.0f);
        const auto p_b_in  = Project(sgn * 54.0f,  -127.0f + dy, 6.0f);

        QPolygonF bump_poly;
        bump_poly << p_t_in.pt << p_t_out.pt << p_b_out.pt << p_b_in.pt;

        QLinearGradient bg(p_t_in.pt, p_b_in.pt);
        if (pressed) {
            bg.setColorAt(0.0, colors.highlight.lighter(135));
            bg.setColorAt(0.4, colors.highlight);
            bg.setColorAt(1.0, colors.highlight.darker(120));
        } else {
            bg.setColorAt(0.0, QColor(78, 85, 100));
            bg.setColorAt(0.3, colors.button);
            bg.setColorAt(0.8, colors.button.darker(110));
            bg.setColorAt(1.0, colors.button.darker(125));
        }

        p.setPen(QPen(pressed ? colors.indicator : QColor(64, 72, 88), 1.6f));
        p.setBrush(bg);
        p.drawPolygon(bump_poly);

        // Specular highlight line along top edge
        p.setPen(QPen(QColor(255, 255, 255, pressed ? 170 : 55), 1.2f));
        p.drawLine(p_t_in.pt, p_t_out.pt);

        // Prominent bold label "L" / "R"
        const auto lbl_pos = Project(sgn * 88.0f, -134.0f + dy, 7.0f);
        const float lbl_w = 36.0f * lbl_pos.scale;
        const float lbl_h = 18.0f * lbl_pos.scale;
        const QRectF lbl_rect(lbl_pos.pt.x() - lbl_w * 0.5f, lbl_pos.pt.y() - lbl_h * 0.5f, lbl_w, lbl_h);

        QFont font_lbl(QStringLiteral("Segoe UI"), 10, QFont::Bold);
        p.setFont(font_lbl);
        p.setPen(pressed ? QColor(255, 255, 255) : QColor(230, 240, 255));
        p.drawText(lbl_rect, Qt::AlignCenter, label);
    };

    DrawShoulderBumper(true, button_values[L].value, QStringLiteral("L"));
    DrawShoulderBumper(false, button_values[R].value, QStringLiteral("R"));

    // -------------------------------------------------------------------------
    // LAYER 3: Ergonomic Palm Handles (Left and Right)
    // -------------------------------------------------------------------------
    auto DrawErgonomicHandle = [&](bool is_left) {
        const float sgn = is_left ? -1.0f : 1.0f;
        const QColor main_col = is_left ? colors.left : colors.right;
        const QColor hi_col   = is_left ? colors.grip_left_highlight : colors.grip_right_highlight;
        const QColor sh_col   = is_left ? colors.grip_left_shadow : colors.grip_right_shadow;

        QPolygonF handle_poly;
        for (std::size_t i = 0; i < pro_left_handle.size() / 2; ++i) {
            const float hx = sgn * pro_left_handle[i * 2 + 0];
            const float ly = pro_left_handle[i * 2 + 1];
            handle_poly << Project(hx, ly, 4.0f).pt;
        }

        const auto p_outer = Project(sgn * 192.0f, 10.0f, 4.0f);
        const auto p_inner = Project(sgn * 100.0f, 10.0f, 4.0f);
        QLinearGradient h_grad(p_outer.pt, p_inner.pt);
        h_grad.setColorAt(0.0,  hi_col.lighter(120));
        h_grad.setColorAt(0.12, hi_col);
        h_grad.setColorAt(0.35, main_col.lighter(108));
        h_grad.setColorAt(0.55, main_col);
        h_grad.setColorAt(0.80, main_col.darker(114));
        h_grad.setColorAt(1.0,  sh_col);

        p.setPen(QPen(QColor(18, 20, 24, 180), 1.2f));
        p.setBrush(h_grad);
        p.drawPolygon(handle_poly);

        // Specular highlight streak
        {
            const auto sp = Project(sgn * 175.0f, -10.0f, 4.3f);
            QRadialGradient sg(sp.pt, 50.0f * sp.scale);
            sg.setColorAt(0.0, QColor(255, 255, 255, 40));
            sg.setColorAt(0.5, QColor(255, 255, 255, 12));
            sg.setColorAt(1.0, QColor(255, 255, 255, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(sg);
            p.drawEllipse(sp.pt, 9.0f * sp.scale, 52.0f * sp.scale);
        }

        // Diamond Anti-Slip Grip Texture
        p.setPen(Qt::NoPen);
        const int start_x = is_left ? -182 : 130;
        const int end_x   = is_left ? -130 : 182;
        for (int dy = -20; dy <= 95; dy += 11) {
            for (int dx = start_x; dx <= end_x; dx += 11) {
                const int offset = ((dy / 11) % 2 == 0) ? 5 : 0;
                const auto dot = Project(static_cast<float>(dx + offset), static_cast<float>(dy), 4.2f);
                const float ds = 1.8f * dot.scale;
                QPolygonF diamond;
                diamond << QPointF(dot.pt.x(), dot.pt.y() - ds)
                        << QPointF(dot.pt.x() + ds * 0.65f, dot.pt.y())
                        << QPointF(dot.pt.x(), dot.pt.y() + ds)
                        << QPointF(dot.pt.x() - ds * 0.65f, dot.pt.y());
                p.setBrush(QColor(255, 255, 255, 18));
                p.drawPolygon(diamond);
            }
        }

        // Zelda TotK: Gold metallic tip and Zonai swirl on right grip
        if (!is_left && current_skin == ControllerSkin::ZeldaTotk) {
            QPolygonF gold_tip;
            gold_tip << Project(135.0f, 85.0f, 4.2f).pt
                     << Project(180.0f, 80.0f, 4.2f).pt
                     << Project(160.0f, 138.0f, 4.2f).pt
                     << Project(135.0f, 120.0f, 4.2f).pt;
            QLinearGradient gold_grad(Project(135.0f, 85.0f, 4.2f).pt, Project(160.0f, 138.0f, 4.2f).pt);
            gold_grad.setColorAt(0.0, QColor(255, 235, 130));
            gold_grad.setColorAt(0.25, QColor(245, 210, 80));
            gold_grad.setColorAt(0.5, QColor(218, 168, 38));
            gold_grad.setColorAt(0.75, QColor(190, 140, 28));
            gold_grad.setColorAt(1.0, QColor(155, 110, 18));
            p.setPen(QPen(QColor(160, 120, 20), 1.0f));
            p.setBrush(gold_grad);
            p.drawPolygon(gold_tip);

            p.setPen(QPen(QColor(218, 168, 38, 220), 2.2f * zoom, Qt::SolidLine, Qt::RoundCap));
            p.setBrush(Qt::NoBrush);
            p.drawLine(Project(125.0f, -10.0f, 4.25f).pt, Project(148.0f, 18.0f, 4.25f).pt);
            p.drawLine(Project(148.0f, 18.0f, 4.25f).pt, Project(138.0f, 44.0f, 4.25f).pt);
            p.drawLine(Project(138.0f, 44.0f, 4.25f).pt, Project(158.0f, 72.0f, 4.25f).pt);
        }
    };

    DrawErgonomicHandle(true);
    DrawErgonomicHandle(false);

    // -------------------------------------------------------------------------
    // LAYER 4: Parting Line Seams (Grooves between Handles and Chassis)
    // -------------------------------------------------------------------------
    auto DrawHandleSeam = [&](bool is_left) {
        const float sgn = is_left ? -1.0f : 1.0f;
        p.setPen(QPen(QColor(10, 12, 16, 200), 1.5f));
        p.drawLine(Project(sgn * 98.0f, -35.0f, 3.5f).pt, Project(sgn * 98.0f, 55.0f, 3.5f).pt);
        p.setPen(QPen(QColor(255, 255, 255, 30), 0.8f));
        p.drawLine(Project(sgn * 99.5f, -34.0f, 3.5f).pt, Project(sgn * 99.5f, 54.0f, 3.5f).pt);
    };
    DrawHandleSeam(true);
    DrawHandleSeam(false);

    // -------------------------------------------------------------------------
    // LAYER 5: Central Chassis Faceplate (Curved Studio Lighting)
    // -------------------------------------------------------------------------
    {
        QPolygonF front_body;
        for (std::size_t i = 0; i < pro_body.size() / 2; ++i) {
            front_body << Project(pro_body[i * 2 + 0], pro_body[i * 2 + 1], 2.0f).pt;
        }
        for (int i = static_cast<int>(pro_body.size() / 2) - 1; i >= 0; --i) {
            front_body << Project(-pro_body[i * 2 + 0], pro_body[i * 2 + 1], 2.0f).pt;
        }

        const auto p_top = Project(0.0f, -95.0f, 2.0f);
        const auto p_bot = Project(0.0f,  60.0f, 2.0f);
        QLinearGradient body_grad(p_top.pt, p_bot.pt);
        body_grad.setColorAt(0.0,  colors.primary.lighter(130));
        body_grad.setColorAt(0.08, colors.primary.lighter(118));
        body_grad.setColorAt(0.22, colors.primary.lighter(108));
        body_grad.setColorAt(0.45, colors.primary);
        body_grad.setColorAt(0.65, colors.primary.darker(106));
        body_grad.setColorAt(0.85, colors.primary.darker(115));
        body_grad.setColorAt(1.0,  colors.primary.darker(128));

        p.setPen(QPen(QColor(18, 20, 24, 160), 1.2f));
        p.setBrush(body_grad);
        p.drawPolygon(front_body);

        // Overhead studio softbox specular bloom
        {
            const auto sp = Project(-15.0f, -65.0f, 2.3f);
            QRadialGradient sg(sp.pt, 50.0f * sp.scale);
            sg.setColorAt(0.0, QColor(255, 255, 255, 42));
            sg.setColorAt(0.4, QColor(255, 255, 255, 15));
            sg.setColorAt(1.0, QColor(255, 255, 255, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(sg);
            p.drawEllipse(sp.pt, 50.0f * sp.scale, 22.0f * sp.scale);
        }

        // Secondary subtle specular on lower bridge
        {
            const auto sp2 = Project(10.0f, 15.0f, 2.3f);
            QRadialGradient sg2(sp2.pt, 35.0f * sp2.scale);
            sg2.setColorAt(0.0, QColor(255, 255, 255, 22));
            sg2.setColorAt(0.6, QColor(255, 255, 255, 6));
            sg2.setColorAt(1.0, QColor(255, 255, 255, 0));
            p.setBrush(sg2);
            p.drawEllipse(sp2.pt, 35.0f * sp2.scale, 12.0f * sp2.scale);
        }

        // Anti-friction concentric glossy rings around joystick wells
        auto DrawAntiFrictionRing = [&](float cx, float cy) {
            const auto ring_p = Project(cx, cy, 2.05f);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(255, 255, 255, 18), 1.5f * ring_p.scale));
            p.drawEllipse(ring_p.pt, 30.5f * ring_p.scale, 30.5f * ring_p.scale);
            p.setPen(QPen(QColor(10, 12, 16, 60), 1.0f * ring_p.scale));
            p.drawEllipse(ring_p.pt, 32.0f * ring_p.scale, 32.0f * ring_p.scale);
        };
        DrawAntiFrictionRing(-64.0f, -42.0f);
        DrawAntiFrictionRing( 42.0f,  18.0f);

        // ClassicBlack: Translucent PCB & Easter Egg
        if (current_skin == ControllerSkin::ClassicBlack) {
            QPolygonF pcb;
            pcb << Project(-62.0f, -60.0f, 1.8f).pt
                << Project( 62.0f, -60.0f, 1.8f).pt
                << Project( 62.0f,  45.0f, 1.8f).pt
                << Project(-62.0f,  45.0f, 1.8f).pt;
            p.setPen(QPen(QColor(18, 48, 38, 180), 1.2f));
            p.setBrush(QColor(12, 34, 26, 175));
            p.drawPolygon(pcb);

            p.setPen(QPen(QColor(195, 160, 75, 140), 1.0f));
            p.drawLine(Project(-55.0f, -40.0f, 1.85f).pt, Project(-25.0f, -40.0f, 1.85f).pt);
            p.drawLine(Project(-25.0f, -40.0f, 1.85f).pt, Project(-15.0f, -20.0f, 1.85f).pt);
            p.drawLine(Project(-15.0f, -20.0f, 1.85f).pt, Project( 15.0f, -20.0f, 1.85f).pt);
            p.drawLine(Project( 15.0f, -20.0f, 1.85f).pt, Project( 25.0f, -40.0f, 1.85f).pt);
            p.drawLine(Project( 25.0f, -40.0f, 1.85f).pt, Project( 55.0f, -40.0f, 1.85f).pt);

            const auto ee_pos = Project(42.0f, 40.0f, 1.88f);
            p.setPen(QColor(195, 160, 75, 160));
            SetTextFont(p, 0.55f * ee_pos.scale);
            DrawText(p, ee_pos.pt, 0.55f * ee_pos.scale, QStringLiteral("thnx2 allgamefans!"));
        }
    }

    // -------------------------------------------------------------------------
    // LAYER 6: Authentic Limited Edition Factory Artwork (Р Р°СЃРєСЂР°СЃРєРё)
    // -------------------------------------------------------------------------
    switch (current_skin) {
    case ControllerSkin::ZeldaTotk: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        const auto c_proj = Project(0.0f, -8.0f, 2.2f);
        {
            QRadialGradient eye_glow(c_proj.pt, 36.0f * c_proj.scale);
            eye_glow.setColorAt(0.0, QColor(230, 185, 45, 0));
            eye_glow.setColorAt(0.65, QColor(230, 185, 45, 0));
            eye_glow.setColorAt(0.85, QColor(230, 185, 45, 50));
            eye_glow.setColorAt(1.0, QColor(230, 185, 45, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(eye_glow);
            p.drawEllipse(c_proj.pt, 36.0f * c_proj.scale, 36.0f * c_proj.scale);
        }
        p.setPen(QPen(QColor(230, 185, 45, 245), 3.2f * zoom, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c_proj.pt, 29.0f * c_proj.scale, 29.0f * c_proj.scale);
        p.setPen(QPen(QColor(56, 225, 176, 230), 2.2f * zoom));
        p.drawEllipse(c_proj.pt, 18.5f * c_proj.scale, 18.5f * c_proj.scale);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(56, 225, 176, 220));
        p.drawEllipse(c_proj.pt, 4.5f * c_proj.scale, 4.5f * c_proj.scale);
        {
            const auto tear_bot = Project(0.0f, 6.0f, 2.22f);
            QPainterPath tear_path;
            tear_path.moveTo(Project(0.0f, -18.0f, 2.22f).pt);
            tear_path.cubicTo(Project(8.0f, -8.0f, 2.22f).pt,
                              Project(6.0f, 2.0f, 2.22f).pt,
                              tear_bot.pt);
            tear_path.cubicTo(Project(-6.0f, 2.0f, 2.22f).pt,
                              Project(-8.0f, -8.0f, 2.22f).pt,
                              Project(0.0f, -18.0f, 2.22f).pt);
            p.setPen(QPen(QColor(230, 185, 45, 210), 1.6f * zoom));
            p.setBrush(QColor(230, 185, 45, 55));
            p.drawPath(tear_path);
        }
        p.setPen(QPen(QColor(230, 185, 45, 190), 1.5f * zoom));
        p.setBrush(Qt::NoBrush);
        p.drawLine(Project(28.0f, -30.0f, 2.2f).pt, Project(50.0f, -20.0f, 2.2f).pt);
        p.drawLine(Project(50.0f, -20.0f, 2.2f).pt, Project(45.0f, -8.0f, 2.2f).pt);
        p.drawLine(Project(45.0f, -8.0f, 2.2f).pt, Project(62.0f, 5.0f, 2.2f).pt);
        p.drawLine(Project(-28.0f, -30.0f, 2.2f).pt, Project(-50.0f, -20.0f, 2.2f).pt);
        p.drawLine(Project(-50.0f, -20.0f, 2.2f).pt, Project(-45.0f, -8.0f, 2.2f).pt);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(56, 225, 176, 200));
        for (auto& ep : {std::pair{50.0f, -20.0f}, std::pair{45.0f, -8.0f},
                         std::pair{-50.0f, -20.0f}, std::pair{-45.0f, -8.0f}}) {
            const auto dp = Project(ep.first, ep.second, 2.22f);
            p.drawEllipse(dp.pt, 2.4f * dp.scale, 2.4f * dp.scale);
        }
        p.restore();
        break;
    }

    case ControllerSkin::CyberStorm: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        auto DrawNeonLine = [&](float x1, float y1, float x2, float y2, const QColor& col, float width = 1.8f) {
            const auto pa = Project(x1, y1, 2.22f);
            const auto pb = Project(x2, y2, 2.22f);
            p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), 55), (width + 4.0f) * zoom));
            p.drawLine(pa.pt, pb.pt);
            p.setPen(QPen(col, width * zoom));
            p.drawLine(pa.pt, pb.pt);
        };

        DrawNeonLine(-60.0f, -25.0f, -30.0f, -25.0f, QColor(0, 240, 255, 245));
        DrawNeonLine(-30.0f, -25.0f, -12.0f, -45.0f, QColor(0, 240, 255, 245));
        DrawNeonLine(-12.0f, -45.0f,  12.0f, -45.0f, QColor(0, 240, 255, 245));
        DrawNeonLine( 12.0f, -45.0f,  30.0f, -25.0f, QColor(0, 240, 255, 245));
        DrawNeonLine( 30.0f, -25.0f,  60.0f, -25.0f, QColor(0, 240, 255, 245));
        DrawNeonLine(-50.0f, -10.0f, -20.0f, -10.0f, QColor(0, 240, 255, 170), 1.2f);
        DrawNeonLine( 20.0f, -10.0f,  50.0f, -10.0f, QColor(0, 240, 255, 170), 1.2f);

        p.setPen(Qt::NoPen);
        for (auto& np_pos : {std::pair{-60.0f, -25.0f}, std::pair{-30.0f, -25.0f},
                             std::pair{30.0f, -25.0f}, std::pair{60.0f, -25.0f},
                             std::pair{-12.0f, -45.0f}, std::pair{12.0f, -45.0f}}) {
            const auto np = Project(np_pos.first, np_pos.second, 2.24f);
            QRadialGradient dot_glow(np.pt, 5.0f * np.scale);
            dot_glow.setColorAt(0.0, QColor(0, 240, 255, 210));
            dot_glow.setColorAt(0.5, QColor(0, 240, 255, 60));
            dot_glow.setColorAt(1.0, QColor(0, 240, 255, 0));
            p.setBrush(dot_glow);
            p.drawEllipse(np.pt, 5.0f * np.scale, 5.0f * np.scale);
            p.setBrush(QColor(0, 240, 255));
            p.drawEllipse(np.pt, 2.5f * np.scale, 2.5f * np.scale);
        }

        DrawNeonLine( 4.0f, -26.0f, -4.0f, -14.0f, QColor(255, 20, 120, 255), 2.5f);
        DrawNeonLine(-4.0f, -14.0f,  2.0f, -14.0f, QColor(255, 20, 120, 255), 2.5f);
        DrawNeonLine( 2.0f, -14.0f, -2.0f,  -2.0f, QColor(255, 20, 120, 255), 2.5f);

        {
            const auto bolt_c = Project(0.0f, -14.0f, 2.25f);
            QRadialGradient bg(bolt_c.pt, 16.0f * bolt_c.scale);
            bg.setColorAt(0.0, QColor(255, 20, 120, 60));
            bg.setColorAt(0.6, QColor(255, 20, 120, 15));
            bg.setColorAt(1.0, QColor(255, 20, 120, 0));
            p.setBrush(bg);
            p.drawEllipse(bolt_c.pt, 16.0f * bolt_c.scale, 16.0f * bolt_c.scale);
        }
        p.restore();
        break;
    }

    case ControllerSkin::SmashBrosUltimate: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        const auto p1 = Project(-16.0f, -80.0f, 2.2f);
        const auto p2 = Project(-5.0f,   55.0f, 2.2f);
        QLinearGradient cross_v_grad(p1.pt, p2.pt);
        cross_v_grad.setColorAt(0.0,  QColor(255, 255, 255, 245));
        cross_v_grad.setColorAt(0.15, QColor(218, 222, 232, 235));
        cross_v_grad.setColorAt(0.35, QColor(195, 202, 214, 230));
        cross_v_grad.setColorAt(0.55, QColor(170, 175, 188, 225));
        cross_v_grad.setColorAt(0.80, QColor(140, 146, 158, 225));
        cross_v_grad.setColorAt(1.0,  QColor(230, 235, 245, 240));

        QPolygonF bar_v;
        bar_v << Project(-18.0f, -75.0f, 2.2f).pt
              << Project( -6.0f, -75.0f, 2.2f).pt
              << Project( -6.0f,  55.0f, 2.2f).pt
              << Project(-18.0f,  55.0f, 2.2f).pt;
        p.setPen(QPen(QColor(50, 52, 60, 200), 1.2f));
        p.setBrush(cross_v_grad);
        p.drawPolygon(bar_v);

        p.setPen(QPen(QColor(255, 255, 255, 110), 0.8f));
        p.drawLine(Project(-18.0f, -75.0f, 2.21f).pt, Project(-18.0f, 55.0f, 2.21f).pt);

        const auto p3 = Project(-110.0f, -28.0f, 2.2f);
        const auto p4 = Project( 110.0f, -16.0f, 2.2f);
        QLinearGradient cross_h_grad(p3.pt, p4.pt);
        cross_h_grad.setColorAt(0.0,  QColor(255, 255, 255, 245));
        cross_h_grad.setColorAt(0.20, QColor(218, 222, 232, 235));
        cross_h_grad.setColorAt(0.50, QColor(185, 190, 205, 230));
        cross_h_grad.setColorAt(0.80, QColor(155, 160, 175, 225));
        cross_h_grad.setColorAt(1.0,  QColor(230, 235, 245, 240));

        QPolygonF bar_h;
        bar_h << Project(-110.0f, -28.0f, 2.2f).pt
              << Project( 110.0f, -28.0f, 2.2f).pt
              << Project( 110.0f, -16.0f, 2.2f).pt
              << Project(-110.0f, -16.0f, 2.2f).pt;
        p.setPen(QPen(QColor(50, 52, 60, 200), 1.2f));
        p.setBrush(cross_h_grad);
        p.drawPolygon(bar_h);

        {
            const auto cross_c = Project(-12.0f, -22.0f, 2.25f);
            QRadialGradient flare(cross_c.pt, 14.0f * cross_c.scale);
            flare.setColorAt(0.0, QColor(255, 255, 255, 100));
            flare.setColorAt(0.5, QColor(255, 210, 80, 35));
            flare.setColorAt(1.0, QColor(255, 210, 80, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(flare);
            p.drawEllipse(cross_c.pt, 14.0f * cross_c.scale, 14.0f * cross_c.scale);
        }
        p.restore();
        break;
    }

    case ControllerSkin::Xenoblade2: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        const auto crystal_center = Project(0.0f, -16.0f, 2.2f);
        {
            QRadialGradient crystal_glow(crystal_center.pt, 28.0f * crystal_center.scale);
            crystal_glow.setColorAt(0.0, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 75));
            crystal_glow.setColorAt(0.5, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 25));
            crystal_glow.setColorAt(1.0, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 0));
            p.setPen(Qt::NoPen);
            p.setBrush(crystal_glow);
            p.drawEllipse(crystal_center.pt, 28.0f * crystal_center.scale, 28.0f * crystal_center.scale);
        }
        QPolygonF crystal;
        crystal << Project(  0.0f, -32.0f, 2.2f).pt
                << Project( 14.0f, -16.0f, 2.2f).pt
                << Project(  0.0f,   0.0f, 2.2f).pt
                << Project(-14.0f, -16.0f, 2.2f).pt;
        QLinearGradient crystal_grad(Project(0.0f, -32.0f, 2.2f).pt, Project(0.0f, 0.0f, 2.2f).pt);
        crystal_grad.setColorAt(0.0, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 255));
        crystal_grad.setColorAt(0.4, QColor(colors.emblem.lighter(135).red(), colors.emblem.lighter(135).green(), colors.emblem.lighter(135).blue(), 235));
        crystal_grad.setColorAt(0.7, colors.emblem);
        crystal_grad.setColorAt(1.0, colors.emblem.darker(125));
        p.setPen(QPen(QColor(255, 255, 255, 230), 1.5f));
        p.setBrush(crystal_grad);
        p.drawPolygon(crystal);

        p.setPen(QPen(QColor(255, 255, 255, 160), 1.0f));
        p.drawLine(Project(0.0f, -32.0f, 2.2f).pt, Project(0.0f, 0.0f, 2.2f).pt);
        p.drawLine(Project(-14.0f, -16.0f, 2.2f).pt, Project(14.0f, -16.0f, 2.2f).pt);

        p.setPen(QPen(colors.emblem_secondary, 2.2f * zoom));
        p.setBrush(QColor(colors.emblem_secondary.red(), colors.emblem_secondary.green(), colors.emblem_secondary.blue(), 85));
        QPolygonF wing_l;
        wing_l << Project(-20.0f, -16.0f, 2.2f).pt << Project(-45.0f, -28.0f, 2.2f).pt
               << Project(-38.0f, -12.0f, 2.2f).pt << Project(-20.0f,  -6.0f, 2.2f).pt;
        QPolygonF wing_r;
        wing_r << Project( 20.0f, -16.0f, 2.2f).pt << Project( 45.0f, -28.0f, 2.2f).pt
               << Project( 38.0f, -12.0f, 2.2f).pt << Project( 20.0f,  -6.0f, 2.2f).pt;
        p.drawPolygon(wing_l);
        p.drawPolygon(wing_r);
        p.restore();
        break;
    }

    case ControllerSkin::Splatoon3: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        {
            const auto sp1 = Project(-25.0f, -28.0f, 2.2f);
            QRadialGradient splat1(sp1.pt, 18.0f * sp1.scale);
            splat1.setColorAt(0.0, QColor(218, 253, 33, 235));
            splat1.setColorAt(0.6, QColor(200, 240, 25, 200));
            splat1.setColorAt(1.0, QColor(180, 220, 20, 110));
            p.setBrush(splat1);
            p.drawEllipse(sp1.pt, 18.0f * sp1.scale, 14.0f * sp1.scale);
            const auto sp1a = Project(-38.0f, -20.0f, 2.2f);
            p.drawEllipse(sp1a.pt, 10.0f * sp1a.scale, 9.0f * sp1a.scale);
            const auto sp1b = Project(-18.0f, -38.0f, 2.2f);
            p.drawEllipse(sp1b.pt, 7.0f * sp1b.scale, 6.0f * sp1b.scale);

            p.setBrush(QColor(255, 255, 255, 120));
            p.drawEllipse(Project(-27.0f, -30.0f, 2.22f).pt, 3.5f * sp1.scale, 2.5f * sp1.scale);
        }
        {
            const auto sp2 = Project(22.0f, 10.0f, 2.2f);
            QRadialGradient splat2(sp2.pt, 16.0f * sp2.scale);
            splat2.setColorAt(0.0, QColor(140, 50, 245, 235));
            splat2.setColorAt(0.6, QColor(122, 38, 235, 200));
            splat2.setColorAt(1.0, QColor(100, 25, 210, 110));
            p.setBrush(splat2);
            p.drawEllipse(sp2.pt, 16.0f * sp2.scale, 13.0f * sp2.scale);
            const auto sp2a = Project(35.0f, 18.0f, 2.2f);
            p.drawEllipse(sp2a.pt, 9.0f * sp2a.scale, 8.0f * sp2a.scale);
            const auto sp2b = Project(30.0f, 0.0f, 2.2f);
            p.drawEllipse(sp2b.pt, 6.0f * sp2b.scale, 5.5f * sp2b.scale);

            p.setBrush(QColor(255, 255, 255, 110));
            p.drawEllipse(Project(20.0f, 8.0f, 2.22f).pt, 3.0f * sp2.scale, 2.0f * sp2.scale);
        }
        p.restore();
        break;
    }

    case ControllerSkin::MonsterHunterRise: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        const auto mc = Project(0.0f, -25.0f, 2.2f);
        {
            QRadialGradient mh_glow(mc.pt, 28.0f * mc.scale);
            mh_glow.setColorAt(0.0, QColor(225, 180, 65, 55));
            mh_glow.setColorAt(0.7, QColor(225, 180, 65, 15));
            mh_glow.setColorAt(1.0, QColor(225, 180, 65, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(mh_glow);
            p.drawEllipse(mc.pt, 28.0f * mc.scale, 28.0f * mc.scale);
        }
        p.setPen(QPen(colors.emblem, 2.2f * zoom));
        p.setBrush(QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 95));
        QPolygonF blade1;
        blade1 << Project(0.0f, -42.0f, 2.2f).pt << Project(14.0f, -22.0f, 2.2f).pt << Project(0.0f, -8.0f, 2.2f).pt;
        QPolygonF blade2;
        blade2 << Project(0.0f, -42.0f, 2.2f).pt << Project(-14.0f, -22.0f, 2.2f).pt << Project(0.0f, -8.0f, 2.2f).pt;
        p.drawPolygon(blade1);
        p.drawPolygon(blade2);
        p.setPen(QPen(colors.emblem, 1.4f * zoom));
        p.drawLine(Project(0.0f, -42.0f, 2.22f).pt, Project(0.0f, -8.0f, 2.22f).pt);
        p.restore();
        break;
    }

    case ControllerSkin::PokemonScarletViolet: {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        QPolygonF shield;
        shield << Project(  0.0f, -36.0f, 2.2f).pt << Project( 18.0f, -30.0f, 2.2f).pt
               << Project( 18.0f,  -8.0f, 2.2f).pt << Project(  0.0f,  12.0f, 2.2f).pt
               << Project(-18.0f,  -8.0f, 2.2f).pt << Project(-18.0f, -30.0f, 2.2f).pt;
        QLinearGradient shield_grad(Project(-18.0f, -30.0f, 2.2f).pt, Project(18.0f, -8.0f, 2.2f).pt);
        shield_grad.setColorAt(0.0, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 130));
        shield_grad.setColorAt(0.5, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 90));
        shield_grad.setColorAt(1.0, QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 55));
        p.setPen(QPen(colors.emblem, 2.0f * zoom));
        p.setBrush(shield_grad);
        p.drawPolygon(shield);
        p.setPen(QPen(colors.emblem, 1.2f * zoom));
        p.drawLine(Project(0.0f, -36.0f, 2.2f).pt, Project(0.0f, 12.0f, 2.2f).pt);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(228, 45, 60, 190));
        p.drawEllipse(Project(-8.0f, -18.0f, 2.22f).pt, 4.5f * zoom, 4.5f * zoom);
        p.setBrush(QColor(75, 65, 215, 190));
        p.drawEllipse(Project(8.0f, -18.0f, 2.22f).pt, 4.5f * zoom, 4.5f * zoom);
        p.restore();
        break;
    }

    default:
        break;
    }

    // -------------------------------------------------------------------------
    // LAYER 7: Sockets & Wells with Ambient Shadows & Bevel Rings
    // -------------------------------------------------------------------------
    auto DrawStickSocket = [&](float well_x, float well_y) {
        constexpr int well_pts = 28;
        QPolygonF well_outer_poly;
        QPolygonF well_inner_poly;
        for (int k = 0; k < well_pts; ++k) {
            const float ang = 2.0f * PI_CONST * k / well_pts;
            const float cos_a = std::cos(ang);
            const float sin_a = std::sin(ang);
            well_outer_poly << Project(well_x + 30.0f * cos_a, well_y + 30.0f * sin_a, 2.0f).pt;
            well_inner_poly << Project(well_x + 24.0f * cos_a, well_y + 24.0f * sin_a, -4.0f).pt;
        }

        p.setPen(QPen(QColor(15, 17, 22), 1.2f));
        p.setBrush(QColor(18, 20, 24));
        p.drawPolygon(well_outer_poly);

        const auto sock_proj = Project(well_x, well_y, -4.0f);
        QRadialGradient socket_grad(sock_proj.pt, 28.0f * sock_proj.scale);
        socket_grad.setColorAt(0.0, QColor(6, 7, 10));
        socket_grad.setColorAt(0.4, QColor(10, 11, 14));
        socket_grad.setColorAt(0.7, QColor(16, 18, 22));
        socket_grad.setColorAt(0.9, QColor(28, 32, 38));
        socket_grad.setColorAt(1.0, QColor(42, 46, 54));
        p.setPen(Qt::NoPen);
        p.setBrush(socket_grad);
        p.drawPolygon(well_inner_poly);
    };

    DrawStickSocket(-64.0f, -42.0f);
    DrawStickSocket( 42.0f,  18.0f);

    // -------------------------------------------------------------------------
    // LAYER 8: 3D D-Pad (Directional Pad) at (-42.0f, 18.0f)
    // -------------------------------------------------------------------------
    {
        const float cx = -42.0f;
        const float cy = 18.0f;
        const bool up    = button_values[DUp].value;
        const bool down  = button_values[DDown].value;
        const bool left  = button_values[DLeft].value;
        const bool right = button_values[DRight].value;

        constexpr float arm_len = 24.0f;
        constexpr float arm_w = 8.0f;

        const struct { float x; float y; } cross_pts[12] = {
            {-arm_w, -arm_len}, { arm_w, -arm_len}, { arm_w, -arm_w},
            { arm_len, -arm_w}, { arm_len,  arm_w}, { arm_w,  arm_w},
            { arm_w,  arm_len}, {-arm_w,  arm_len}, {-arm_w,  arm_w},
            {-arm_len, arm_w},  {-arm_len, -arm_w}, {-arm_w, -arm_w}
        };

        auto GetTopZ = [&](float x, float y) -> float {
            float z = 8.5f;
            if (up)    z += (y < -arm_w ? -4.5f : (y > arm_w ? +1.8f : -2.0f));
            if (down)  z += (y > arm_w ? -4.5f : (y < -arm_w ? +1.8f : -2.0f));
            if (left)  z += (x < -arm_w ? -4.5f : (x > arm_w ? +1.8f : -2.0f));
            if (right) z += (x > arm_w ? -4.5f : (x < -arm_w ? +1.8f : -2.0f));
            return z;
        };

        std::array<QPointF, 12> top_proj;
        for (int i = 0; i < 12; ++i) {
            top_proj[i] = Project(cx + cross_pts[i].x, cy + cross_pts[i].y, GetTopZ(cross_pts[i].x, cross_pts[i].y)).pt;
        }

        const QColor dpad_col = (current_skin == ControllerSkin::ZeldaTotk)
                                    ? QColor(218, 168, 38)
                                    : colors.button;

        QPolygonF top_poly;
        for (int i = 0; i < 12; ++i) {
            top_poly << top_proj[i];
        }

        const auto dpad_c = Project(cx, cy, GetTopZ(0.0f, 0.0f));
        QRadialGradient dpad_grad(dpad_c.pt, 26.0f * dpad_c.scale);
        dpad_grad.setColorAt(0.0, dpad_col.lighter(112));
        dpad_grad.setColorAt(0.4, dpad_col);
        dpad_grad.setColorAt(0.8, dpad_col.darker(110));
        dpad_grad.setColorAt(1.0, dpad_col.darker(120));

        p.setPen(QPen(QColor(18, 20, 24), 1.2f));
        p.setBrush(dpad_grad);
        p.drawPolygon(top_poly);

        const auto center_proj = Project(cx, cy, GetTopZ(0.0f, 0.0f) - 1.2f);
        QRadialGradient hub_grad(center_proj.pt, 6.0f * center_proj.scale);
        hub_grad.setColorAt(0.0, dpad_col.darker(115));
        hub_grad.setColorAt(0.6, dpad_col.darker(130));
        hub_grad.setColorAt(1.0, dpad_col.darker(145));
        p.setPen(QPen(QColor(15, 17, 22), 0.8f));
        p.setBrush(hub_grad);
        p.drawEllipse(center_proj.pt, 5.5f * center_proj.scale, 5.5f * center_proj.scale);

        auto DrawDpadArm = [&](Direction dir, bool is_pressed, float ax, float ay) {
            const float top_z = GetTopZ(ax, ay);
            const auto arm_proj = Project(cx + ax, cy + ay, top_z + 0.1f);
            const QColor arr_col = is_pressed ? colors.indicator
                                 : ((current_skin == ControllerSkin::ZeldaTotk) ? QColor(56, 225, 176) : colors.font2);
            p.setPen(arr_col);
            p.setBrush(arr_col);
            DrawArrow(p, arm_proj.pt, dir, 0.88f * arm_proj.scale);
        };
        DrawDpadArm(Direction::Up, up, 0.0f, -16.0f);
        DrawDpadArm(Direction::Down, down, 0.0f, 16.0f);
        DrawDpadArm(Direction::Left, left, -16.0f, 0.0f);
        DrawDpadArm(Direction::Right, right, 16.0f, 0.0f);
    }

    // -------------------------------------------------------------------------
    // LAYER 9: 3D Face Buttons (ABXY) вЂ” Acrylic Glass Domes
    // -------------------------------------------------------------------------
    auto DrawPhotorealisticButton = [&](int btn_id, float bx, float by, Symbol sym) {
        const bool pressed = button_values[btn_id].value;
        const float top_z = pressed ? 5.0f : 12.0f;
        constexpr float r = 11.5f;

        const auto b_center = Project(bx, by, top_z);

        {
            const auto sock = Project(bx, by, 2.5f);
            p.setPen(QPen(QColor(10, 12, 16), 1.0f));
            p.setBrush(QColor(14, 16, 20));
            p.drawEllipse(sock.pt, (r + 1.8f) * sock.scale, (r + 1.8f) * sock.scale);
        }

        p.setPen(QPen(pressed ? colors.font : QColor(22, 24, 30), 1.2f));
        if (pressed) {
            QRadialGradient glow_grad(b_center.pt, r * b_center.scale);
            glow_grad.setColorAt(0.0, colors.indicator.lighter(140));
            glow_grad.setColorAt(0.3, colors.indicator.lighter(115));
            glow_grad.setColorAt(0.7, colors.indicator);
            glow_grad.setColorAt(1.0, colors.indicator.darker(115));
            p.setBrush(glow_grad);
        } else {
            QRadialGradient btn_grad(b_center.pt - QPointF(r * 0.35f * b_center.scale, r * 0.35f * b_center.scale),
                                     r * 1.4f * b_center.scale);
            btn_grad.setColorAt(0.0, QColor(92, 98, 112));
            btn_grad.setColorAt(0.2, QColor(74, 80, 92));
            btn_grad.setColorAt(0.5, colors.button);
            btn_grad.setColorAt(0.8, colors.button.darker(125));
            btn_grad.setColorAt(1.0, colors.button.darker(145));
            p.setBrush(btn_grad);
        }
        p.drawEllipse(b_center.pt, r * b_center.scale, r * b_center.scale);

        if (!pressed) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 55));
            const auto spec = Project(bx - 2.8f, by - 3.8f, top_z + 0.1f);
            p.drawEllipse(spec.pt, 5.0f * spec.scale, 3.0f * spec.scale);
        }

        p.setPen(colors.transparent);
        p.setBrush(pressed ? colors.font : colors.font2);
        DrawSymbol(p, b_center.pt, sym, 1.30f * b_center.scale);
    };

    DrawPhotorealisticButton(A, 82.0f, -42.0f, Symbol::A);
    DrawPhotorealisticButton(B, 64.0f, -24.0f, Symbol::B);
    DrawPhotorealisticButton(X, 64.0f, -60.0f, Symbol::X);
    DrawPhotorealisticButton(Y, 46.0f, -42.0f, Symbol::Y);

    // -------------------------------------------------------------------------
    // LAYER 10: 3D Analog Joysticks with Live 2D Tilt & Steel Shaft
    // -------------------------------------------------------------------------
    auto DrawPhotorealisticStick = [&](bool is_left) {
        const auto stick_id = is_left ? Settings::NativeAnalog::LStick : Settings::NativeAnalog::RStick;
        const auto button_id = is_left ? Settings::NativeButton::LStick : Settings::NativeButton::RStick;

        const float sx = std::clamp(stick_values[stick_id].x.value, -1.0f, 1.0f);
        const float sy = std::clamp(stick_values[stick_id].y.value, -1.0f, 1.0f);
        const bool is_click_pressed = button_values[button_id].value;

        const float well_x = is_left ? -64.0f : 42.0f;
        const float well_y = is_left ? -42.0f : 18.0f;

        const float pad_x = well_x + sx * 13.0f;
        const float pad_y = well_y + sy * 13.0f;

        float pad_z = 24.0f - (sx * sx + sy * sy) * 2.5f;
        if (is_click_pressed) {
            pad_z -= 5.0f;
        }

        // Metallic Steel Stem
        {
            const auto p_base = Project(well_x, well_y, -3.0f);
            const auto p_top = Project(pad_x, pad_y, pad_z - 3.0f);

            const QPointF dir = p_top.pt - p_base.pt;
            const float len = std::max(0.001f, std::sqrt(float(dir.x() * dir.x() + dir.y() * dir.y())));
            const QPointF norm(-dir.y() / len, dir.x() / len);

            const float shaft_r = 4.8f * p_base.scale;
            const QPointF w = norm * shaft_r;

            QPolygonF shaft_poly;
            shaft_poly << (p_base.pt - w)
                       << (p_base.pt + w)
                       << (p_top.pt + w * 0.90f)
                       << (p_top.pt - w * 0.90f);

            QLinearGradient shaft_grad(p_base.pt - w, p_base.pt + w);
            shaft_grad.setColorAt(0.0, QColor(50, 54, 62));
            shaft_grad.setColorAt(0.2, QColor(95, 100, 115));
            shaft_grad.setColorAt(0.35, QColor(195, 200, 215));
            shaft_grad.setColorAt(0.5, QColor(220, 225, 235));
            shaft_grad.setColorAt(0.65, QColor(155, 160, 175));
            shaft_grad.setColorAt(0.8, QColor(85, 90, 100));
            shaft_grad.setColorAt(1.0, QColor(38, 42, 48));

            p.setPen(QPen(QColor(25, 28, 32), 0.8f));
            p.setBrush(shaft_grad);
            p.drawPolygon(shaft_poly);
        }

        // Rubber Thumb-Pad
        {
            constexpr float r_rim = 22.0f;
            constexpr float r_bowl = 15.5f;

            const auto cap_center = Project(pad_x, pad_y, pad_z);

            p.setPen(QPen(is_click_pressed ? colors.indicator : QColor(20, 22, 28), 1.2f));
            if (is_click_pressed) {
                p.setBrush(colors.highlight);
            } else {
                QRadialGradient pad_grad(cap_center.pt - QPointF(r_rim * 0.3f * cap_center.scale, r_rim * 0.3f * cap_center.scale),
                                         r_rim * 1.4f * cap_center.scale);
                pad_grad.setColorAt(0.0, QColor(88, 94, 108));
                pad_grad.setColorAt(0.25, QColor(68, 72, 84));
                pad_grad.setColorAt(0.55, colors.button);
                pad_grad.setColorAt(0.8, colors.button.darker(120));
                pad_grad.setColorAt(1.0, colors.button.darker(145));
                p.setBrush(pad_grad);
            }
            p.drawEllipse(cap_center.pt, r_rim * cap_center.scale, r_rim * cap_center.scale);

            // Cardinal Notches
            p.setPen(Qt::NoPen);
            p.setBrush(is_click_pressed ? colors.font : QColor(16, 18, 22, 210));
            constexpr std::array<float, 4> notch_rads = {0.0f, float(PI_CONST * 0.5f), float(PI_CONST), float(PI_CONST * 1.5f)};
            for (float n_ang : notch_rads) {
                const auto notch_proj = Project(pad_x + std::cos(n_ang) * (r_rim - 2.5f),
                                                pad_y + std::sin(n_ang) * (r_rim - 2.5f),
                                                pad_z + 0.2f);
                p.drawEllipse(notch_proj.pt, 2.0f * notch_proj.scale, 2.0f * notch_proj.scale);
            }

            // Concave Dish
            const auto bowl_center = Project(pad_x, pad_y, pad_z - 1.8f);
            p.setPen(QPen(QColor(12, 14, 18), 0.8f));
            if (is_click_pressed) {
                p.setBrush(colors.highlight2);
            } else {
                QRadialGradient bowl_grad(bowl_center.pt, r_bowl * bowl_center.scale);
                bowl_grad.setColorAt(0.0, colors.button2.darker(135));
                bowl_grad.setColorAt(0.3, colors.button2.darker(120));
                bowl_grad.setColorAt(0.6, colors.button2);
                bowl_grad.setColorAt(0.85, colors.button2.lighter(105));
                bowl_grad.setColorAt(1.0, QColor(62, 66, 76));
                p.setBrush(bowl_grad);
            }
            p.drawEllipse(bowl_center.pt, r_bowl * bowl_center.scale, r_bowl * bowl_center.scale);

            if (!is_click_pressed) {
                const auto spec = Project(pad_x - 4.0f, pad_y - 6.0f, pad_z + 0.3f);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(255, 255, 255, 42));
                p.drawEllipse(spec.pt, 8.0f * spec.scale, 4.5f * spec.scale);
            }
        }
    };

    DrawPhotorealisticStick(true);
    DrawPhotorealisticStick(false);

    // -------------------------------------------------------------------------
    // LAYER 11: Auxiliary Buttons (-, +, Screenshot, Home) with Halo
    // -------------------------------------------------------------------------
    // Minus (-)
    {
        const bool pressed = button_values[Minus].value;
        const float z = pressed ? 3.5f : 7.0f;
        const auto proj = Project(-24.0f, -58.0f, z);
        QRadialGradient aux_grad(proj.pt, 7.5f * proj.scale);
        aux_grad.setColorAt(0.0, (pressed ? colors.indicator.lighter(120) : QColor(60, 65, 75)));
        aux_grad.setColorAt(0.6, (pressed ? colors.indicator : colors.button));
        aux_grad.setColorAt(1.0, (pressed ? colors.indicator.darker(120) : colors.button.darker(130)));
        p.setPen(QPen(pressed ? colors.font : QColor(18, 20, 24), 1.0f));
        p.setBrush(aux_grad);
        p.drawEllipse(proj.pt, 7.0f * proj.scale, 7.0f * proj.scale);

        p.setPen(colors.font2);
        p.setBrush(colors.font2);
        const auto sym_proj = Project(-24.0f, -58.0f, z + 0.1f);
        p.drawRect(QRectF(sym_proj.pt.x() - 3.5f * sym_proj.scale, sym_proj.pt.y() - 0.7f * sym_proj.scale,
                          7.0f * sym_proj.scale, 1.4f * sym_proj.scale));
    }

    // Plus (+)
    {
        const bool pressed = button_values[Plus].value;
        const float z = pressed ? 3.5f : 7.0f;
        const auto proj = Project(24.0f, -58.0f, z);
        QRadialGradient aux_grad(proj.pt, 7.5f * proj.scale);
        aux_grad.setColorAt(0.0, (pressed ? colors.indicator.lighter(120) : QColor(60, 65, 75)));
        aux_grad.setColorAt(0.6, (pressed ? colors.indicator : colors.button));
        aux_grad.setColorAt(1.0, (pressed ? colors.indicator.darker(120) : colors.button.darker(130)));
        p.setPen(QPen(pressed ? colors.font : QColor(18, 20, 24), 1.0f));
        p.setBrush(aux_grad);
        p.drawEllipse(proj.pt, 7.0f * proj.scale, 7.0f * proj.scale);

        p.setPen(colors.font2);
        p.setBrush(colors.font2);
        const auto sym_proj = Project(24.0f, -58.0f, z + 0.1f);
        p.drawRect(QRectF(sym_proj.pt.x() - 3.5f * sym_proj.scale, sym_proj.pt.y() - 0.7f * sym_proj.scale,
                          7.0f * sym_proj.scale, 1.4f * sym_proj.scale));
        p.drawRect(QRectF(sym_proj.pt.x() - 0.7f * sym_proj.scale, sym_proj.pt.y() - 3.5f * sym_proj.scale,
                          1.4f * sym_proj.scale, 7.0f * sym_proj.scale));
    }

    // Screenshot
    {
        const bool pressed = button_values[Screenshot].value;
        const float z = pressed ? 3.0f : 6.0f;
        const auto proj = Project(-16.0f, -28.0f, z);
        p.setPen(QPen(pressed ? colors.font : QColor(18, 20, 24), 1.0f));
        QRadialGradient scr_grad(proj.pt, 6.0f * proj.scale);
        scr_grad.setColorAt(0.0, (pressed ? colors.indicator.lighter(115) : QColor(55, 60, 70)));
        scr_grad.setColorAt(0.7, (pressed ? colors.indicator : colors.button));
        scr_grad.setColorAt(1.0, (pressed ? colors.indicator.darker(115) : colors.button.darker(125)));
        p.setBrush(scr_grad);
        const float sz = 5.5f * proj.scale;
        p.drawRoundedRect(QRectF(proj.pt.x() - sz, proj.pt.y() - sz, sz * 2.0f, sz * 2.0f), 2.0f, 2.0f);

        p.setPen(colors.font2);
        p.setBrush(colors.font2);
        p.drawEllipse(proj.pt, 3.0f * proj.scale, 3.0f * proj.scale);
    }

    // Home with glowing LED halo
    {
        const bool pressed = button_values[Home].value;
        const float z = pressed ? 3.0f : 6.0f;

        const auto halo_proj = Project(16.0f, -28.0f, 2.5f);
        QRadialGradient led_grad(halo_proj.pt, 14.0f * halo_proj.scale);
        led_grad.setColorAt(0.0, colors.home_led);
        led_grad.setColorAt(0.4, QColor(colors.home_led.red(), colors.home_led.green(), colors.home_led.blue(), 85));
        QColor led_fade = colors.home_led;
        led_fade.setAlpha(0);
        led_grad.setColorAt(1.0, led_fade);
        p.setPen(Qt::NoPen);
        p.setBrush(led_grad);
        p.drawEllipse(halo_proj.pt, 14.0f * halo_proj.scale, 14.0f * halo_proj.scale);

        const auto proj = Project(16.0f, -28.0f, z);
        QRadialGradient home_grad(proj.pt - QPointF(2.0f * proj.scale, 2.0f * proj.scale), 8.0f * proj.scale);
        home_grad.setColorAt(0.0, QColor(72, 78, 92));
        home_grad.setColorAt(0.4, (pressed ? colors.indicator : colors.button));
        home_grad.setColorAt(1.0, (pressed ? colors.indicator.darker(120) : colors.button.darker(135)));
        p.setPen(QPen(colors.home_led, 1.4f));
        p.setBrush(home_grad);
        p.drawEllipse(proj.pt, 7.0f * proj.scale, 7.0f * proj.scale);

        p.setPen(colors.transparent);
        p.setBrush(colors.font2);
        DrawSymbol(p, proj.pt, Symbol::House, 3.2f * proj.scale);
    }

    // -------------------------------------------------------------------------
    // LAYER 12: Player Indicator LEDs (Lower Chin)
    // -------------------------------------------------------------------------
    {
        constexpr std::array<float, 4> led_x = {-15.0f, -5.0f, 5.0f, 15.0f};
        const bool on[4] = {bool(led_pattern.position1), bool(led_pattern.position2),
                            bool(led_pattern.position3), bool(led_pattern.position4)};

        for (int i = 0; i < 4; ++i) {
            const auto proj = Project(led_x[i], 52.0f, 2.5f);
            p.setPen(Qt::NoPen);
            p.setBrush(on[i] ? colors.indicator : QColor(30, 35, 42));
            p.drawRoundedRect(QRectF(proj.pt.x() - 2.2f * proj.scale, proj.pt.y() - 1.2f * proj.scale,
                                     4.4f * proj.scale, 2.4f * proj.scale), 0.8f, 0.8f);
            if (on[i]) {
                QRadialGradient glow(proj.pt, 5.5f * proj.scale);
                glow.setColorAt(0.0, colors.indicator);
                QColor fade = colors.indicator;
                fade.setAlpha(0);
                glow.setColorAt(1.0, fade);
                p.setBrush(glow);
                p.drawEllipse(proj.pt, 5.5f * proj.scale, 5.5f * proj.scale);
            }
        }
    }

    // -------------------------------------------------------------------------
    // LAYER 13: HUD Overlays (Non-Overlapping, Beautifully Positioned)
    // -------------------------------------------------------------------------
    // Battery Card (Upper-Left)
    DrawPhotorealisticBattery(bat_pos);

    // Colorful 3D Gyroscope (Upper-Right)
    DrawColorfulGyroscope(gyro_pos, smooth_euler, 16.0f);

    // Precision Analog Stick Radars (Under Controller, Centered L/R)
    DrawModernStickRadar(true, left_radar);
    DrawModernStickRadar(false, right_radar);

    // Active Pressed Buttons Ribbon (Centered below stick radars, only when active)
    {
        struct ActiveBadge {
            QString name{};
            bool active{false};
        };
        const std::array<ActiveBadge, 14> active_badges = {
            ActiveBadge{QStringLiteral("A"), button_values[A].value},
            ActiveBadge{QStringLiteral("B"), button_values[B].value},
            ActiveBadge{QStringLiteral("X"), button_values[X].value},
            ActiveBadge{QStringLiteral("Y"), button_values[Y].value},
            ActiveBadge{QStringLiteral("L"), button_values[L].value},
            ActiveBadge{QStringLiteral("R"), button_values[R].value},
            ActiveBadge{QStringLiteral("ZL"), zl_analog > 0.15f || button_values[ZL].value},
            ActiveBadge{QStringLiteral("ZR"), zr_analog > 0.15f || button_values[ZR].value},
            ActiveBadge{QStringLiteral("L3"), button_values[Settings::NativeButton::LStick].value},
            ActiveBadge{QStringLiteral("R3"), button_values[Settings::NativeButton::RStick].value},
            ActiveBadge{QStringLiteral("\u25B2"), button_values[DUp].value},
            ActiveBadge{QStringLiteral("\u25BC"), button_values[DDown].value},
            ActiveBadge{QStringLiteral("\u25C0"), button_values[DLeft].value},
            ActiveBadge{QStringLiteral("\u25B6"), button_values[DRight].value},
        };

        std::vector<QString> pressed_names;
        for (const auto& b : active_badges) {
            if (b.active) {
                pressed_names.push_back(b.name);
            }
        }

        if (!pressed_names.empty()) {
            const float badge_w = 26.0f;
            const float badge_h = 18.0f;
            const float spacing = 6.0f;
            const float total_w = pressed_names.size() * badge_w + (pressed_names.size() - 1) * spacing;
            float start_x = center.x() - total_w * 0.5f;
            const float badge_y = center.y() + 252.0f;

            for (const auto& name : pressed_names) {
                QRectF b_rect(start_x, badge_y - badge_h * 0.5f, badge_w, badge_h);
                p.setPen(QPen(colors.indicator.lighter(130), 1.2f));
                QLinearGradient b_grad(b_rect.topLeft(), b_rect.bottomLeft());
                b_grad.setColorAt(0.0, colors.indicator.darker(110));
                b_grad.setColorAt(1.0, colors.indicator.darker(160));
                p.setBrush(b_grad);
                p.drawRoundedRect(b_rect, 4.0f, 4.0f);

                QFont font_badge(QStringLiteral("Segoe UI"), 8, QFont::Bold);
                p.setFont(font_badge);
                p.setPen(QColor(255, 255, 255));
                p.drawText(b_rect, Qt::AlignCenter, name);
                start_x += badge_w + spacing;
            }
        }
    }
}

void PlayerControlPreview::DrawGCController(QPainter& p, const QPointF center) {
    DrawGCTriggers(p, center, trigger_values[0], trigger_values[1]);
    DrawGCButtonZ(p, center, button_values[Settings::NativeButton::R]);
    DrawGCBody(p, center);
    {
        // Draw joysticks
        using namespace Settings::NativeAnalog;
        const auto l_stick = QPointF(stick_values[LStick].x.value, stick_values[LStick].y.value);
        const auto r_stick = QPointF(stick_values[RStick].x.value, stick_values[RStick].y.value);
        DrawGCJoystick(p, center + QPointF(-111, -44) + (l_stick * 10), {});
        button_color = colors.button2;
        DrawCircleButton(p, center + QPointF(61, 37) + (r_stick * 9.5f), {}, 15);
        p.setPen(colors.transparent);
        p.setBrush(colors.font);
        DrawSymbol(p, center + QPointF(61, 37) + (r_stick * 9.5f), Symbol::C, 1.0f);
        DrawRawJoystick(p, center + QPointF(-198, -125), center + QPointF(198, -125));
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    constexpr float text_size = 1.1f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(111, -44), button_values[A], 21);
    DrawCircleButton(p, center + QPoint(70, -23), button_values[B], 13);
    DrawGCButtonX(p, center, button_values[Settings::NativeButton::X]);
    DrawGCButtonY(p, center, button_values[Settings::NativeButton::Y]);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, center + QPoint(111, -44), Symbol::A, 1.5f);
    DrawSymbol(p, center + QPoint(70, -23), Symbol::B, text_size);
    DrawSymbol(p, center + QPoint(151, -53), Symbol::X, text_size);
    DrawSymbol(p, center + QPoint(100, -83), Symbol::Y, text_size);

    // D-pad buttons
    const QPointF dpad_position = center + QPoint(-61, 37);
    const float dpad_size = 0.8f;
    DrawArrowButton(p, dpad_position, Direction::Up, button_values[DUp], dpad_size);
    DrawArrowButton(p, dpad_position, Direction::Left, button_values[DLeft], dpad_size);
    DrawArrowButton(p, dpad_position, Direction::Right, button_values[DRight], dpad_size);
    DrawArrowButton(p, dpad_position, Direction::Down, button_values[DDown], dpad_size);
    DrawArrowButtonOutline(p, dpad_position, dpad_size);

    // Minus and Plus buttons
    p.setPen(colors.outline);
    DrawCircleButton(p, center + QPoint(0, -44), button_values[Plus], 8);

    // Draw battery
    DrawBattery(p, center + QPoint(-20, 110),
                battery_values[Core::HID::EmulatedDeviceIndex::LeftIndex]);
}

constexpr std::array<float, 13 * 2> symbol_a = {
    -1.085f, -5.2f,   1.085f, -5.2f,   5.085f, 5.0f,    2.785f,  5.0f,  1.785f,
    2.65f,   -1.785f, 2.65f,  -2.785f, 5.0f,   -5.085f, 5.0f,    -1.4f, 1.0f,
    0.0f,    -2.8f,   1.4f,   1.0f,    -1.4f,  1.0f,    -5.085f, 5.0f,
};
constexpr std::array<float, 134 * 2> symbol_b = {
    -4.0f, 0.0f,  -4.0f, 0.0f,  -4.0f, -0.1f, -3.8f, -5.1f, 1.8f,  -5.0f, 2.3f,  -4.9f, 2.6f,
    -4.8f, 2.8f,  -4.7f, 2.9f,  -4.6f, 3.1f,  -4.5f, 3.2f,  -4.4f, 3.4f,  -4.3f, 3.4f,  -4.2f,
    3.5f,  -4.1f, 3.7f,  -4.0f, 3.7f,  -3.9f, 3.8f,  -3.8f, 3.8f,  -3.7f, 3.9f,  -3.6f, 3.9f,
    -3.5f, 4.0f,  -3.4f, 4.0f,  -3.3f, 4.1f,  -3.1f, 4.1f,  -3.0f, 4.0f,  -2.0f, 4.0f,  -1.9f,
    3.9f,  -1.7f, 3.9f,  -1.6f, 3.8f,  -1.5f, 3.8f,  -1.4f, 3.7f,  -1.3f, 3.7f,  -1.2f, 3.6f,
    -1.1f, 3.6f,  -1.0f, 3.5f,  -0.9f, 3.3f,  -0.8f, 3.3f,  -0.7f, 3.2f,  -0.6f, 3.0f,  -0.5f,
    2.9f,  -0.4f, 2.7f,  -0.3f, 2.9f,  -0.2f, 3.2f,  -0.1f, 3.3f,  0.0f,  3.5f,  0.1f,  3.6f,
    0.2f,  3.8f,  0.3f,  3.9f,  0.4f,  4.0f,  0.6f,  4.1f,  0.7f,  4.3f,  0.8f,  4.3f,  0.9f,
    4.4f,  1.0f,  4.4f,  1.1f,  4.5f,  1.3f,  4.5f,  1.4f,  4.6f,  1.6f,  4.6f,  1.7f,  4.5f,
    2.8f,  4.5f,  2.9f,  4.4f,  3.1f,  4.4f,  3.2f,  4.3f,  3.4f,  4.3f,  3.5f,  4.2f,  3.6f,
    4.2f,  3.7f,  4.1f,  3.8f,  4.1f,  3.9f,  4.0f,  4.0f,  3.9f,  4.2f,  3.8f,  4.3f,  3.6f,
    4.4f,  3.6f,  4.5f,  3.4f,  4.6f,  3.3f,  4.7f,  3.1f,  4.8f,  2.8f,  4.9f,  2.6f,  5.0f,
    2.1f,  5.1f,  -4.0f, 5.0f,  -4.0f, 4.9f,

    -4.0f, 0.0f,  1.1f,  3.4f,  1.1f,  3.4f,  1.5f,  3.3f,  1.8f,  3.2f,  2.0f,  3.1f,  2.1f,
    3.0f,  2.3f,  2.9f,  2.3f,  2.8f,  2.4f,  2.7f,  2.4f,  2.6f,  2.5f,  2.3f,  2.5f,  2.2f,
    2.4f,  1.7f,  2.4f,  1.6f,  2.3f,  1.4f,  2.3f,  1.3f,  2.2f,  1.2f,  2.2f,  1.1f,  2.1f,
    1.0f,  1.9f,  0.9f,  1.6f,  0.8f,  1.4f,  0.7f,  -1.9f, 0.6f,  -1.9f, 0.7f,  -1.8f, 3.4f,
    1.1f,  3.4f,  -4.0f, 0.0f,

    0.3f,  -1.1f, 0.3f,  -1.1f, 1.3f,  -1.2f, 1.5f,  -1.3f, 1.8f,  -1.4f, 1.8f,  -1.5f, 1.9f,
    -1.6f, 2.0f,  -1.8f, 2.0f,  -1.9f, 2.1f,  -2.0f, 2.1f,  -2.1f, 2.0f,  -2.7f, 2.0f,  -2.8f,
    1.9f,  -2.9f, 1.9f,  -3.0f, 1.8f,  -3.1f, 1.6f,  -3.2f, 1.6f,  -3.3f, 1.3f,  -3.4f, -1.9f,
    -3.3f, -1.9f, -3.2f, -1.8f, -1.0f, 0.2f,  -1.1f, 0.3f,  -1.1f, -4.0f, 0.0f,
};

constexpr std::array<float, 9 * 2> symbol_y = {
    -4.79f, -4.9f, -2.44f, -4.9f, 0.0f,  -0.9f,  2.44f, -4.9f,  4.79f,
    -4.9f,  1.05f, 1.0f,   1.05f, 5.31f, -1.05f, 5.31f, -1.05f, 1.0f,

};

constexpr std::array<float, 12 * 2> symbol_x = {
    -4.4f, -5.0f, -2.0f, -5.0f, 0.0f, -1.7f, 2.0f,  -5.0f, 4.4f,  -5.0f, 1.2f,  0.0f,
    4.4f,  5.0f,  2.0f,  5.0f,  0.0f, 1.7f,  -2.0f, 5.0f,  -4.4f, 5.0f,  -1.2f, 0.0f,

};

constexpr std::array<float, 7 * 2> symbol_l = {
    2.4f, -3.23f, 2.4f, 2.1f, 5.43f, 2.1f, 5.43f, 3.22f, 0.98f, 3.22f, 0.98f, -3.23f, 2.4f, -3.23f,
};

constexpr std::array<float, 98 * 2> symbol_r = {
    1.0f, 0.0f,  1.0f, -0.1f, 1.1f, -3.3f, 4.3f, -3.2f, 5.1f, -3.1f, 5.4f, -3.0f, 5.6f, -2.9f,
    5.7f, -2.8f, 5.9f, -2.7f, 5.9f, -2.6f, 6.0f, -2.5f, 6.1f, -2.3f, 6.2f, -2.2f, 6.2f, -2.1f,
    6.3f, -2.0f, 6.3f, -1.9f, 6.2f, -0.8f, 6.2f, -0.7f, 6.1f, -0.6f, 6.1f, -0.5f, 6.0f, -0.4f,
    6.0f, -0.3f, 5.9f, -0.2f, 5.7f, -0.1f, 5.7f, 0.0f,  5.6f, 0.1f,  5.4f, 0.2f,  5.1f, 0.3f,
    4.7f, 0.4f,  4.7f, 0.5f,  4.9f, 0.6f,  5.0f, 0.7f,  5.2f, 0.8f,  5.2f, 0.9f,  5.3f, 1.0f,
    5.5f, 1.1f,  5.5f, 1.2f,  5.6f, 1.3f,  5.7f, 1.5f,  5.8f, 1.6f,  5.9f, 1.8f,  6.0f, 1.9f,
    6.1f, 2.1f,  6.2f, 2.2f,  6.2f, 2.3f,  6.3f, 2.4f,  6.4f, 2.6f,  6.5f, 2.7f,  6.6f, 2.9f,
    6.7f, 3.0f,  6.7f, 3.1f,  6.8f, 3.2f,  6.8f, 3.3f,  5.3f, 3.2f,  5.2f, 3.1f,  5.2f, 3.0f,
    5.1f, 2.9f,  5.0f, 2.7f,  4.9f, 2.6f,  4.8f, 2.4f,  4.7f, 2.3f,  4.6f, 2.1f,  4.5f, 2.0f,
    4.4f, 1.8f,  4.3f, 1.7f,  4.1f, 1.4f,  4.0f, 1.3f,  3.9f, 1.1f,  3.8f, 1.0f,  3.6f, 0.9f,
    3.6f, 0.8f,  3.5f, 0.7f,  3.3f, 0.6f,  2.9f, 0.5f,  2.3f, 0.6f,  2.3f, 0.7f,  2.2f, 3.3f,
    1.0f, 3.2f,  1.0f, 3.1f,  1.0f, 0.0f,

    4.2f, -0.5f, 4.4f, -0.6f, 4.7f, -0.7f, 4.8f, -0.8f, 4.9f, -1.0f, 5.0f, -1.1f, 5.0f, -1.2f,
    4.9f, -1.7f, 4.9f, -1.8f, 4.8f, -1.9f, 4.8f, -2.0f, 4.6f, -2.1f, 4.3f, -2.2f, 2.3f, -2.1f,
    2.3f, -2.0f, 2.4f, -0.5f, 4.2f, -0.5f, 1.0f, 0.0f,
};

constexpr std::array<float, 18 * 2> symbol_zl = {
    -2.6f, -2.13f, -5.6f, -2.13f, -5.6f, -3.23f, -0.8f, -3.23f, -0.8f, -2.13f, -4.4f, 2.12f,
    -0.7f, 2.12f,  -0.7f, 3.22f,  -6.0f, 3.22f,  -6.0f, 2.12f,  2.4f,  -3.23f, 2.4f,  2.1f,
    5.43f, 2.1f,   5.43f, 3.22f,  0.98f, 3.22f,  0.98f, -3.23f, 2.4f,  -3.23f, -6.0f, 2.12f,
};

constexpr std::array<float, 57 * 2> symbol_sl = {
    -3.0f,  -3.65f, -2.76f, -4.26f, -2.33f, -4.76f, -1.76f, -5.09f, -1.13f, -5.26f, -0.94f,
    -4.77f, -0.87f, -4.11f, -1.46f, -3.88f, -1.91f, -3.41f, -2.05f, -2.78f, -1.98f, -2.13f,
    -1.59f, -1.61f, -0.96f, -1.53f, -0.56f, -2.04f, -0.38f, -2.67f, -0.22f, -3.31f, 0.0f,
    -3.93f, 0.34f,  -4.49f, 0.86f,  -4.89f, 1.49f,  -5.05f, 2.14f,  -4.95f, 2.69f,  -4.6f,
    3.07f,  -4.07f, 3.25f,  -3.44f, 3.31f,  -2.78f, 3.25f,  -2.12f, 3.07f,  -1.49f, 2.7f,
    -0.95f, 2.16f,  -0.58f, 1.52f,  -0.43f, 1.41f,  -0.99f, 1.38f,  -1.65f, 1.97f,  -1.91f,
    2.25f,  -2.49f, 2.25f,  -3.15f, 1.99f,  -3.74f, 1.38f,  -3.78f, 1.06f,  -3.22f, 0.88f,
    -2.58f, 0.71f,  -1.94f, 0.49f,  -1.32f, 0.13f,  -0.77f, -0.4f,  -0.4f,  -1.04f, -0.25f,
    -1.69f, -0.32f, -2.28f, -0.61f, -2.73f, -1.09f, -2.98f, -1.69f, -3.09f, -2.34f,

    3.23f,  2.4f,   -2.1f,  2.4f,   -2.1f,  5.43f,  -3.22f, 5.43f,  -3.22f, 0.98f,  3.23f,
    0.98f,  3.23f,  2.4f,   -3.09f, -2.34f,
};
constexpr std::array<float, 109 * 2> symbol_zr = {
    -2.6f, -2.13f, -5.6f, -2.13f, -5.6f, -3.23f, -0.8f, -3.23f, -0.8f, -2.13f, -4.4f, 2.12f, -0.7f,
    2.12f, -0.7f,  3.22f, -6.0f,  3.22f, -6.0f,  2.12f,

    1.0f,  0.0f,   1.0f,  -0.1f,  1.1f,  -3.3f,  4.3f,  -3.2f,  5.1f,  -3.1f,  5.4f,  -3.0f, 5.6f,
    -2.9f, 5.7f,   -2.8f, 5.9f,   -2.7f, 5.9f,   -2.6f, 6.0f,   -2.5f, 6.1f,   -2.3f, 6.2f,  -2.2f,
    6.2f,  -2.1f,  6.3f,  -2.0f,  6.3f,  -1.9f,  6.2f,  -0.8f,  6.2f,  -0.7f,  6.1f,  -0.6f, 6.1f,
    -0.5f, 6.0f,   -0.4f, 6.0f,   -0.3f, 5.9f,   -0.2f, 5.7f,   -0.1f, 5.7f,   0.0f,  5.6f,  0.1f,
    5.4f,  0.2f,   5.1f,  0.3f,   4.7f,  0.4f,   4.7f,  0.5f,   4.9f,  0.6f,   5.0f,  0.7f,  5.2f,
    0.8f,  5.2f,   0.9f,  5.3f,   1.0f,  5.5f,   1.1f,  5.5f,   1.2f,  5.6f,   1.3f,  5.7f,  1.5f,
    5.8f,  1.6f,   5.9f,  1.8f,   6.0f,  1.9f,   6.1f,  2.1f,   6.2f,  2.2f,   6.2f,  2.3f,  6.3f,
    2.4f,  6.4f,   2.6f,  6.5f,   2.7f,  6.6f,   2.9f,  6.7f,   3.0f,  6.7f,   3.1f,  6.8f,  3.2f,
    6.8f,  3.3f,   5.3f,  3.2f,   5.2f,  3.1f,   5.2f,  3.0f,   5.1f,  2.9f,   5.0f,  2.7f,  4.9f,
    2.6f,  4.8f,   2.4f,  4.7f,   2.3f,  4.6f,   2.1f,  4.5f,   2.0f,  4.4f,   1.8f,  4.3f,  1.7f,
    4.1f,  1.4f,   4.0f,  1.3f,   3.9f,  1.1f,   3.8f,  1.0f,   3.6f,  0.9f,   3.6f,  0.8f,  3.5f,
    0.7f,  3.3f,   0.6f,  2.9f,   0.5f,  2.3f,   0.6f,  2.3f,   0.7f,  2.2f,   3.3f,  1.0f,  3.2f,
    1.0f,  3.1f,   1.0f,  0.0f,

    4.2f,  -0.5f,  4.4f,  -0.6f,  4.7f,  -0.7f,  4.8f,  -0.8f,  4.9f,  -1.0f,  5.0f,  -1.1f, 5.0f,
    -1.2f, 4.9f,   -1.7f, 4.9f,   -1.8f, 4.8f,   -1.9f, 4.8f,   -2.0f, 4.6f,   -2.1f, 4.3f,  -2.2f,
    2.3f,  -2.1f,  2.3f,  -2.0f,  2.4f,  -0.5f,  4.2f,  -0.5f,  1.0f,  0.0f,   -6.0f, 2.12f,
};

constexpr std::array<float, 148 * 2> symbol_sr = {
    -3.0f,  -3.65f, -2.76f, -4.26f, -2.33f, -4.76f, -1.76f, -5.09f, -1.13f, -5.26f, -0.94f, -4.77f,
    -0.87f, -4.11f, -1.46f, -3.88f, -1.91f, -3.41f, -2.05f, -2.78f, -1.98f, -2.13f, -1.59f, -1.61f,
    -0.96f, -1.53f, -0.56f, -2.04f, -0.38f, -2.67f, -0.22f, -3.31f, 0.0f,   -3.93f, 0.34f,  -4.49f,
    0.86f,  -4.89f, 1.49f,  -5.05f, 2.14f,  -4.95f, 2.69f,  -4.6f,  3.07f,  -4.07f, 3.25f,  -3.44f,
    3.31f,  -2.78f, 3.25f,  -2.12f, 3.07f,  -1.49f, 2.7f,   -0.95f, 2.16f,  -0.58f, 1.52f,  -0.43f,
    1.41f,  -0.99f, 1.38f,  -1.65f, 1.97f,  -1.91f, 2.25f,  -2.49f, 2.25f,  -3.15f, 1.99f,  -3.74f,
    1.38f,  -3.78f, 1.06f,  -3.22f, 0.88f,  -2.58f, 0.71f,  -1.94f, 0.49f,  -1.32f, 0.13f,  -0.77f,
    -0.4f,  -0.4f,  -1.04f, -0.25f, -1.69f, -0.32f, -2.28f, -0.61f, -2.73f, -1.09f, -2.98f, -1.69f,
    -3.09f, -2.34f,

    -1.0f,  0.0f,   0.1f,   1.0f,   3.3f,   1.1f,   3.2f,   4.3f,   3.1f,   5.1f,   3.0f,   5.4f,
    2.9f,   5.6f,   2.8f,   5.7f,   2.7f,   5.9f,   2.6f,   5.9f,   2.5f,   6.0f,   2.3f,   6.1f,
    2.2f,   6.2f,   2.1f,   6.2f,   2.0f,   6.3f,   1.9f,   6.3f,   0.8f,   6.2f,   0.7f,   6.2f,
    0.6f,   6.1f,   0.5f,   6.1f,   0.4f,   6.0f,   0.3f,   6.0f,   0.2f,   5.9f,   0.1f,   5.7f,
    0.0f,   5.7f,   -0.1f,  5.6f,   -0.2f,  5.4f,   -0.3f,  5.1f,   -0.4f,  4.7f,   -0.5f,  4.7f,
    -0.6f,  4.9f,   -0.7f,  5.0f,   -0.8f,  5.2f,   -0.9f,  5.2f,   -1.0f,  5.3f,   -1.1f,  5.5f,
    -1.2f,  5.5f,   -1.3f,  5.6f,   -1.5f,  5.7f,   -1.6f,  5.8f,   -1.8f,  5.9f,   -1.9f,  6.0f,
    -2.1f,  6.1f,   -2.2f,  6.2f,   -2.3f,  6.2f,   -2.4f,  6.3f,   -2.6f,  6.4f,   -2.7f,  6.5f,
    -2.9f,  6.6f,   -3.0f,  6.7f,   -3.1f,  6.7f,   -3.2f,  6.8f,   -3.3f,  6.8f,   -3.2f,  5.3f,
    -3.1f,  5.2f,   -3.0f,  5.2f,   -2.9f,  5.1f,   -2.7f,  5.0f,   -2.6f,  4.9f,   -2.4f,  4.8f,
    -2.3f,  4.7f,   -2.1f,  4.6f,   -2.0f,  4.5f,   -1.8f,  4.4f,   -1.7f,  4.3f,   -1.4f,  4.1f,
    -1.3f,  4.0f,   -1.1f,  3.9f,   -1.0f,  3.8f,   -0.9f,  3.6f,   -0.8f,  3.6f,   -0.7f,  3.5f,
    -0.6f,  3.3f,   -0.5f,  2.9f,   -0.6f,  2.3f,   -0.7f,  2.3f,   -3.3f,  2.2f,   -3.2f,  1.0f,
    -3.1f,  1.0f,   0.0f,   1.0f,

    0.5f,   4.2f,   0.6f,   4.4f,   0.7f,   4.7f,   0.8f,   4.8f,   1.0f,   4.9f,   1.1f,   5.0f,
    1.2f,   5.0f,   1.7f,   4.9f,   1.8f,   4.9f,   1.9f,   4.8f,   2.0f,   4.8f,   2.1f,   4.6f,
    2.2f,   4.3f,   2.1f,   2.3f,   2.0f,   2.3f,   0.5f,   2.4f,   0.5f,   4.2f,   -0.0f,  1.0f,
    -3.09f, -2.34f,

};

constexpr std::array<float, 30 * 2> symbol_c = {
    2.86f,  7.57f,  0.99f,  7.94f,  -0.91f, 7.87f,  -2.73f, 7.31f,  -4.23f, 6.14f,  -5.2f,  4.51f,
    -5.65f, 2.66f,  -5.68f, 0.75f,  -5.31f, -1.12f, -4.43f, -2.81f, -3.01f, -4.08f, -1.24f, -4.78f,
    0.66f,  -4.94f, 2.54f,  -4.67f, 4.33f,  -4.0f,  4.63f,  -2.27f, 3.37f,  -2.7f,  1.6f,   -3.4f,
    -0.3f,  -3.5f,  -2.09f, -2.87f, -3.34f, -1.45f, -3.91f, 0.37f,  -3.95f, 2.27f,  -3.49f, 4.12f,
    -2.37f, 5.64f,  -0.65f, 6.44f,  1.25f,  6.47f,  3.06f,  5.89f,  4.63f,  4.92f,  4.63f,  6.83f,
};

constexpr std::array<float, 6 * 2> symbol_charging = {
    6.5f, -1.0f, 1.0f, -1.0f, 1.0f, -3.0f, -6.5f, 1.0f, -1.0f, 1.0f, -1.0f, 3.0f,
};

constexpr std::array<float, 12 * 2> house = {
    -1.3f, 0.0f,  -0.93f, 0.0f, -0.93f, 1.15f, 0.93f,  1.15f, 0.93f, 0.0f, 1.3f,  0.0f,
    0.0f,  -1.2f, -1.3f,  0.0f, -0.43f, 0.0f,  -0.43f, .73f,  0.43f, .73f, 0.43f, 0.0f,
};

constexpr std::array<float, 11 * 2> up_arrow_button = {
    9.1f,   -9.1f, 9.1f,   -30.0f, 8.1f,   -30.1f, 7.7f,   -30.1f, -8.6f, -30.0f, -9.0f,
    -29.8f, -9.3f, -29.5f, -9.5f,  -29.1f, -9.1f,  -28.7f, -9.1f,  -9.1f, 0.0f,   0.6f,
};

constexpr std::array<float, 3 * 2> up_arrow_symbol = {
    0.0f, -3.0f, -3.0f, 2.0f, 3.0f, 2.0f,
};

constexpr std::array<float, 64 * 2> trigger_button = {
    5.5f,   -12.6f, 5.8f,   -12.6f, 6.7f,   -12.5f, 8.1f,   -12.3f, 8.6f,   -12.2f, 9.2f,   -12.0f,
    9.5f,   -11.9f, 9.9f,   -11.8f, 10.6f,  -11.5f, 11.0f,  -11.3f, 11.2f,  -11.2f, 11.4f,  -11.1f,
    11.8f,  -10.9f, 12.0f,  -10.8f, 12.2f,  -10.7f, 12.4f,  -10.5f, 12.6f,  -10.4f, 12.8f,  -10.3f,
    13.6f,  -9.7f,  13.8f,  -9.6f,  13.9f,  -9.4f,  14.1f,  -9.3f,  14.8f,  -8.6f,  15.0f,  -8.5f,
    15.1f,  -8.3f,  15.6f,  -7.8f,  15.7f,  -7.6f,  16.1f,  -7.0f,  16.3f,  -6.8f,  16.4f,  -6.6f,
    16.5f,  -6.4f,  16.8f,  -6.0f,  16.9f,  -5.8f,  17.0f,  -5.6f,  17.1f,  -5.4f,  17.2f,  -5.2f,
    17.3f,  -5.0f,  17.4f,  -4.8f,  17.5f,  -4.6f,  17.6f,  -4.4f,  17.7f,  -4.1f,  17.8f,  -3.9f,
    17.9f,  -3.5f,  18.0f,  -3.3f,  18.1f,  -3.0f,  18.2f,  -2.6f,  18.2f,  -2.3f,  18.3f,  -2.1f,
    18.3f,  -1.9f,  18.4f,  -1.4f,  18.5f,  -1.2f,  18.6f,  -0.3f,  18.6f,  0.0f,   18.3f,  13.9f,
    -17.0f, 13.8f,  -17.0f, 13.6f,  -16.4f, -11.4f, -16.3f, -11.6f, -16.1f, -11.8f, -15.7f, -12.0f,
    -15.5f, -12.1f, -15.1f, -12.3f, -14.6f, -12.4f, -13.4f, -12.5f,
};

constexpr std::array<float, 199 * 2> gc_body = {
    0.0f,     -138.03f, -4.91f,   -138.01f, -8.02f,   -137.94f, -11.14f,  -137.82f, -14.25f,
    -137.67f, -17.37f,  -137.48f, -20.48f,  -137.25f, -23.59f,  -137.0f,  -26.69f,  -136.72f,
    -29.8f,   -136.41f, -32.9f,   -136.07f, -35.99f,  -135.71f, -39.09f,  -135.32f, -42.18f,
    -134.91f, -45.27f,  -134.48f, -48.35f,  -134.03f, -51.43f,  -133.55f, -54.51f,  -133.05f,
    -57.59f,  -132.52f, -60.66f,  -131.98f, -63.72f,  -131.41f, -66.78f,  -130.81f, -69.84f,
    -130.2f,  -72.89f,  -129.56f, -75.94f,  -128.89f, -78.98f,  -128.21f, -82.02f,  -127.49f,
    -85.05f,  -126.75f, -88.07f,  -125.99f, -91.09f,  -125.19f, -94.1f,   -124.37f, -97.1f,
    -123.52f, -100.09f, -122.64f, -103.07f, -121.72f, -106.04f, -120.77f, -109.0f,  -119.79f,
    -111.95f, -118.77f, -114.88f, -117.71f, -117.8f,  -116.61f, -120.7f,  -115.46f, -123.58f,
    -114.27f, -126.44f, -113.03f, -129.27f, -111.73f, -132.08f, -110.38f, -134.86f, -108.96f,
    -137.6f,  -107.47f, -140.3f,  -105.91f, -142.95f, -104.27f, -145.55f, -102.54f, -148.07f,
    -100.71f, -150.51f, -98.77f,  -152.86f, -96.71f,  -155.09f, -94.54f,  -157.23f, -92.27f,
    -159.26f, -89.9f,   -161.2f,  -87.46f,  -163.04f, -84.94f,  -164.78f, -82.35f,  -166.42f,
    -79.7f,   -167.97f, -77.0f,   -169.43f, -74.24f,  -170.8f,  -71.44f,  -172.09f, -68.6f,
    -173.29f, -65.72f,  -174.41f, -62.81f,  -175.45f, -59.87f,  -176.42f, -56.91f,  -177.31f,
    -53.92f,  -178.14f, -50.91f,  -178.9f,  -47.89f,  -179.6f,  -44.85f,  -180.24f, -41.8f,
    -180.82f, -38.73f,  -181.34f, -35.66f,  -181.8f,  -32.57f,  -182.21f, -29.48f,  -182.57f,
    -26.38f,  -182.88f, -23.28f,  -183.15f, -20.17f,  -183.36f, -17.06f,  -183.54f, -13.95f,
    -183.71f, -10.84f,  -184.0f,  -7.73f,   -184.23f, -4.62f,   -184.44f, -1.51f,   -184.62f,
    1.6f,     -184.79f, 4.72f,    -184.95f, 7.83f,    -185.11f, 10.95f,   -185.25f, 14.06f,
    -185.38f, 17.18f,   -185.51f, 20.29f,   -185.63f, 23.41f,   -185.74f, 26.53f,   -185.85f,
    29.64f,   -185.95f, 32.76f,   -186.04f, 35.88f,   -186.12f, 39.0f,    -186.19f, 42.11f,
    -186.26f, 45.23f,   -186.32f, 48.35f,   -186.37f, 51.47f,   -186.41f, 54.59f,   -186.44f,
    57.7f,    -186.46f, 60.82f,   -186.46f, 63.94f,   -186.44f, 70.18f,   -186.41f, 73.3f,
    -186.36f, 76.42f,   -186.3f,  79.53f,   -186.22f, 82.65f,   -186.12f, 85.77f,   -185.99f,
    88.88f,   -185.84f, 92.0f,    -185.66f, 95.11f,   -185.44f, 98.22f,   -185.17f, 101.33f,
    -184.85f, 104.43f,  -184.46f, 107.53f,  -183.97f, 110.61f,  -183.37f, 113.67f,  -182.65f,
    116.7f,   -181.77f, 119.69f,  -180.71f, 122.62f,  -179.43f, 125.47f,  -177.89f, 128.18f,
    -176.05f, 130.69f,  -173.88f, 132.92f,  -171.36f, 134.75f,  -168.55f, 136.1f,   -165.55f,
    136.93f,  -162.45f, 137.29f,  -156.23f, 137.03f,  -153.18f, 136.41f,  -150.46f, 134.9f,
    -148.14f, 132.83f,  -146.14f, 130.43f,  -144.39f, 127.85f,  -142.83f, 125.16f,  -141.41f,
    122.38f,  -140.11f, 119.54f,  -138.9f,  116.67f,  -137.77f, 113.76f,  -136.7f,  110.84f,
    -135.68f, 107.89f,  -134.71f, 104.93f,  -133.77f, 101.95f,  -132.86f, 98.97f,   -131.97f,
    95.98f,   -131.09f, 92.99f,   -130.23f, 89.99f,   -129.36f, 86.99f,   -128.49f, 84.0f,
    -127.63f, 81.0f,    -126.76f, 78.01f,   -125.9f,  75.01f,   -124.17f, 69.02f,   -123.31f,
    66.02f,   -121.59f, 60.03f,   -120.72f, 57.03f,   -119.86f, 54.03f,   -118.13f, 48.04f,
    -117.27f, 45.04f,   -115.55f, 39.05f,   -114.68f, 36.05f,   -113.82f, 33.05f,   -112.96f,
    30.06f,   -110.4f,  28.29f,   -107.81f, 26.55f,   -105.23f, 24.8f,    -97.48f,  19.55f,
    -94.9f,   17.81f,   -92.32f,  16.06f,   -87.15f,  12.56f,   -84.57f,  10.81f,   -81.99f,
    9.07f,    -79.4f,   7.32f,    -76.82f,  5.57f,    -69.07f,  0.33f,    -66.49f,  -1.42f,
    -58.74f,  -6.66f,   -56.16f,  -8.41f,   -48.4f,   -13.64f,  -45.72f,  -15.22f,  -42.93f,
    -16.62f,  -40.07f,  -17.86f,  -37.15f,  -18.96f,  -34.19f,  -19.94f,  -31.19f,  -20.79f,
    -28.16f,  -21.55f,  -25.12f,  -22.21f,  -22.05f,  -22.79f,  -18.97f,  -23.28f,  -15.88f,
    -23.7f,   -12.78f,  -24.05f,  -9.68f,   -24.33f,  -6.57f,   -24.55f,  -3.45f,   -24.69f,
    0.0f,     -24.69f,
};

constexpr std::array<float, 99 * 2> gc_left_body = {
    -74.59f,  -97.22f,  -70.17f,  -94.19f,  -65.95f,  -90.89f,  -62.06f,  -87.21f,  -58.58f,
    -83.14f,  -55.58f,  -78.7f,   -53.08f,  -73.97f,  -51.05f,  -69.01f,  -49.46f,  -63.89f,
    -48.24f,  -58.67f,  -47.36f,  -53.39f,  -46.59f,  -48.09f,  -45.7f,   -42.8f,   -44.69f,
    -37.54f,  -43.54f,  -32.31f,  -42.25f,  -27.11f,  -40.8f,   -21.95f,  -39.19f,  -16.84f,
    -37.38f,  -11.8f,   -35.34f,  -6.84f,   -33.04f,  -2.0f,    -30.39f,  2.65f,    -27.26f,
    7.0f,     -23.84f,  11.11f,   -21.19f,  15.76f,   -19.18f,  20.73f,   -17.73f,  25.88f,
    -16.82f,  31.16f,   -16.46f,  36.5f,    -16.7f,   41.85f,   -17.63f,  47.13f,   -19.31f,
    52.21f,   -21.8f,   56.95f,   -24.91f,  61.3f,    -28.41f,  65.36f,   -32.28f,  69.06f,
    -36.51f,  72.35f,   -41.09f,  75.13f,   -45.97f,  77.32f,   -51.1f,   78.86f,   -56.39f,
    79.7f,    -61.74f,  79.84f,   -67.07f,  79.3f,    -72.3f,   78.15f,   -77.39f,  76.48f,
    -82.29f,  74.31f,   -86.76f,  71.37f,   -90.7f,   67.75f,   -94.16f,  63.66f,   -97.27f,
    59.3f,    -100.21f, 54.81f,   -103.09f, 50.3f,    -106.03f, 45.82f,   -109.11f, 41.44f,
    -112.37f, 37.19f,   -115.85f, 33.11f,   -119.54f, 29.22f,   -123.45f, 25.56f,   -127.55f,
    22.11f,   -131.77f, 18.81f,   -136.04f, 15.57f,   -140.34f, 12.37f,   -144.62f, 9.15f,
    -148.86f, 5.88f,    -153.03f, 2.51f,    -157.05f, -1.03f,   -160.83f, -4.83f,   -164.12f,
    -9.05f,   -166.71f, -13.73f,  -168.91f, -18.62f,  -170.77f, -23.64f,  -172.3f,  -28.78f,
    -173.49f, -34.0f,   -174.3f,  -39.3f,   -174.72f, -44.64f,  -174.72f, -49.99f,  -174.28f,
    -55.33f,  -173.37f, -60.61f,  -172.0f,  -65.79f,  -170.17f, -70.82f,  -167.79f, -75.62f,
    -164.84f, -80.09f,  -161.43f, -84.22f,  -157.67f, -88.03f,  -153.63f, -91.55f,  -149.37f,
    -94.81f,  -144.94f, -97.82f,  -140.37f, -100.61f, -135.65f, -103.16f, -130.73f, -105.26f,
    -125.62f, -106.86f, -120.37f, -107.95f, -115.05f, -108.56f, -109.7f,  -108.69f, -104.35f,
    -108.36f, -99.05f,  -107.6f,  -93.82f,  -106.41f, -88.72f,  -104.79f, -83.78f,  -102.7f,
};

constexpr std::array<float, 47 * 2> left_gc_trigger = {
    -99.69f,  -125.04f, -101.81f, -126.51f, -104.02f, -127.85f, -106.3f,  -129.06f, -108.65f,
    -130.12f, -111.08f, -130.99f, -113.58f, -131.62f, -116.14f, -131.97f, -121.26f, -131.55f,
    -123.74f, -130.84f, -126.17f, -129.95f, -128.53f, -128.9f,  -130.82f, -127.71f, -133.03f,
    -126.38f, -135.15f, -124.92f, -137.18f, -123.32f, -139.11f, -121.6f,  -140.91f, -119.75f,
    -142.55f, -117.77f, -144.0f,  -115.63f, -145.18f, -113.34f, -146.17f, -110.95f, -147.05f,
    -108.53f, -147.87f, -106.08f, -148.64f, -103.61f, -149.37f, -101.14f, -149.16f, -100.12f,
    -147.12f, -101.71f, -144.99f, -103.16f, -142.8f,  -104.53f, -140.57f, -105.83f, -138.31f,
    -107.08f, -136.02f, -108.27f, -133.71f, -109.42f, -131.38f, -110.53f, -129.04f, -111.61f,
    -126.68f, -112.66f, -124.31f, -113.68f, -121.92f, -114.67f, -119.53f, -115.64f, -117.13f,
    -116.58f, -114.72f, -117.51f, -112.3f,  -118.41f, -109.87f, -119.29f, -107.44f, -120.16f,
    -105.0f,  -121.0f,  -100.11f, -122.65f,
};

constexpr std::array<float, 50 * 2> gc_button_x = {
    142.1f,  -50.67f, 142.44f, -48.65f, 142.69f, -46.62f, 142.8f,  -44.57f, 143.0f,  -42.54f,
    143.56f, -40.57f, 144.42f, -38.71f, 145.59f, -37.04f, 147.08f, -35.64f, 148.86f, -34.65f,
    150.84f, -34.11f, 152.88f, -34.03f, 154.89f, -34.38f, 156.79f, -35.14f, 158.49f, -36.28f,
    159.92f, -37.74f, 161.04f, -39.45f, 161.85f, -41.33f, 162.4f,  -43.3f,  162.72f, -45.32f,
    162.85f, -47.37f, 162.82f, -49.41f, 162.67f, -51.46f, 162.39f, -53.48f, 162.0f,  -55.5f,
    161.51f, -57.48f, 160.9f,  -59.44f, 160.17f, -61.35f, 159.25f, -63.18f, 158.19f, -64.93f,
    157.01f, -66.61f, 155.72f, -68.2f,  154.31f, -69.68f, 152.78f, -71.04f, 151.09f, -72.2f,
    149.23f, -73.04f, 147.22f, -73.36f, 145.19f, -73.11f, 143.26f, -72.42f, 141.51f, -71.37f,
    140.0f,  -69.99f, 138.82f, -68.32f, 138.13f, -66.4f,  138.09f, -64.36f, 138.39f, -62.34f,
    139.05f, -60.41f, 139.91f, -58.55f, 140.62f, -56.63f, 141.21f, -54.67f, 141.67f, -52.67f,
};

constexpr std::array<float, 50 * 2> gc_button_y = {
    104.02f, -75.23f, 106.01f, -75.74f, 108.01f, -76.15f, 110.04f, -76.42f, 112.05f, -76.78f,
    113.97f, -77.49f, 115.76f, -78.49f, 117.33f, -79.79f, 118.6f,  -81.39f, 119.46f, -83.25f,
    119.84f, -85.26f, 119.76f, -87.3f,  119.24f, -89.28f, 118.33f, -91.11f, 117.06f, -92.71f,
    115.49f, -94.02f, 113.7f,  -95.01f, 111.77f, -95.67f, 109.76f, -96.05f, 107.71f, -96.21f,
    105.67f, -96.18f, 103.63f, -95.99f, 101.61f, -95.67f, 99.61f,  -95.24f, 97.63f,  -94.69f,
    95.69f,  -94.04f, 93.79f,  -93.28f, 91.94f,  -92.4f,  90.19f,  -91.34f, 88.53f,  -90.14f,
    86.95f,  -88.84f, 85.47f,  -87.42f, 84.1f,   -85.9f,  82.87f,  -84.26f, 81.85f,  -82.49f,
    81.15f,  -80.57f, 81.0f,   -78.54f, 81.41f,  -76.54f, 82.24f,  -74.67f, 83.43f,  -73.01f,
    84.92f,  -71.61f, 86.68f,  -70.57f, 88.65f,  -70.03f, 90.69f,  -70.15f, 92.68f,  -70.61f,
    94.56f,  -71.42f, 96.34f,  -72.43f, 98.2f,   -73.29f, 100.11f, -74.03f, 102.06f, -74.65f,
};

constexpr std::array<float, 47 * 2> gc_button_z = {
    95.74f,  -126.41f, 98.34f,  -126.38f, 100.94f, -126.24f, 103.53f, -126.01f, 106.11f, -125.7f,
    108.69f, -125.32f, 111.25f, -124.87f, 113.8f,  -124.34f, 116.33f, -123.73f, 118.84f, -123.05f,
    121.33f, -122.3f,  123.79f, -121.47f, 126.23f, -120.56f, 128.64f, -119.58f, 131.02f, -118.51f,
    133.35f, -117.37f, 135.65f, -116.14f, 137.9f,  -114.84f, 140.1f,  -113.46f, 142.25f, -111.99f,
    144.35f, -110.45f, 146.38f, -108.82f, 148.35f, -107.13f, 150.25f, -105.35f, 151.89f, -103.38f,
    151.43f, -100.86f, 149.15f, -100.15f, 146.73f, -101.06f, 144.36f, -102.12f, 141.98f, -103.18f,
    139.6f,  -104.23f, 137.22f, -105.29f, 134.85f, -106.35f, 132.47f, -107.41f, 127.72f, -109.53f,
    125.34f, -110.58f, 122.96f, -111.64f, 120.59f, -112.7f,  118.21f, -113.76f, 113.46f, -115.88f,
    111.08f, -116.93f, 108.7f,  -117.99f, 106.33f, -119.05f, 103.95f, -120.11f, 99.2f,   -122.23f,
    96.82f,  -123.29f, 94.44f,  -124.34f,
};

constexpr std::array<float, 84 * 2> left_joycon_body = {
    -145.0f, -78.9f, -145.0f, -77.9f, -145.0f, 85.6f,  -145.0f, 85.6f,  -168.3f, 85.5f,
    -169.3f, 85.4f,  -171.3f, 85.1f,  -172.3f, 84.9f,  -173.4f, 84.7f,  -174.3f, 84.5f,
    -175.3f, 84.2f,  -176.3f, 83.8f,  -177.3f, 83.5f,  -178.2f, 83.1f,  -179.2f, 82.7f,
    -180.1f, 82.2f,  -181.0f, 81.8f,  -181.9f, 81.3f,  -182.8f, 80.7f,  -183.7f, 80.2f,
    -184.5f, 79.6f,  -186.2f, 78.3f,  -186.9f, 77.7f,  -187.7f, 77.0f,  -189.2f, 75.6f,
    -189.9f, 74.8f,  -190.6f, 74.1f,  -191.3f, 73.3f,  -191.9f, 72.5f,  -192.5f, 71.6f,
    -193.1f, 70.8f,  -193.7f, 69.9f,  -194.3f, 69.1f,  -194.8f, 68.2f,  -196.2f, 65.5f,
    -196.6f, 64.5f,  -197.0f, 63.6f,  -197.4f, 62.6f,  -198.1f, 60.7f,  -198.4f, 59.7f,
    -198.6f, 58.7f,  -199.2f, 55.6f,  -199.3f, 54.6f,  -199.5f, 51.5f,  -199.5f, 50.5f,
    -199.5f, -49.4f, -199.4f, -50.5f, -199.3f, -51.5f, -199.1f, -52.5f, -198.2f, -56.5f,
    -197.9f, -57.5f, -197.2f, -59.4f, -196.8f, -60.4f, -196.4f, -61.3f, -195.9f, -62.2f,
    -194.3f, -64.9f, -193.7f, -65.7f, -193.1f, -66.6f, -192.5f, -67.4f, -191.8f, -68.2f,
    -191.2f, -68.9f, -190.4f, -69.7f, -188.2f, -71.8f, -187.4f, -72.5f, -186.6f, -73.1f,
    -185.8f, -73.8f, -185.0f, -74.4f, -184.1f, -74.9f, -183.2f, -75.5f, -182.4f, -76.0f,
    -181.5f, -76.5f, -179.6f, -77.5f, -178.7f, -77.9f, -177.8f, -78.4f, -176.8f, -78.8f,
    -175.9f, -79.1f, -174.9f, -79.5f, -173.9f, -79.8f, -170.9f, -80.6f, -169.9f, -80.8f,
    -167.9f, -81.1f, -166.9f, -81.2f, -165.8f, -81.2f, -145.0f, -80.9f,
};

constexpr std::array<float, 84 * 2> left_joycon_trigger = {
    -166.8f, -83.3f, -167.9f, -83.2f, -168.9f, -83.1f, -170.0f, -83.0f, -171.0f, -82.8f,
    -172.1f, -82.6f, -173.1f, -82.4f, -174.2f, -82.1f, -175.2f, -81.9f, -176.2f, -81.5f,
    -177.2f, -81.2f, -178.2f, -80.8f, -180.1f, -80.0f, -181.1f, -79.5f, -182.0f, -79.0f,
    -183.0f, -78.5f, -183.9f, -78.0f, -184.8f, -77.4f, -185.7f, -76.9f, -186.6f, -76.3f,
    -187.4f, -75.6f, -188.3f, -75.0f, -189.1f, -74.3f, -192.2f, -71.5f, -192.9f, -70.7f,
    -193.7f, -69.9f, -194.3f, -69.1f, -195.0f, -68.3f, -195.6f, -67.4f, -196.8f, -65.7f,
    -197.3f, -64.7f, -197.8f, -63.8f, -198.2f, -62.8f, -198.9f, -60.8f, -198.6f, -59.8f,
    -197.6f, -59.7f, -196.6f, -60.0f, -195.6f, -60.5f, -194.7f, -60.9f, -193.7f, -61.4f,
    -192.8f, -61.9f, -191.8f, -62.4f, -190.9f, -62.8f, -189.9f, -63.3f, -189.0f, -63.8f,
    -187.1f, -64.8f, -186.2f, -65.2f, -185.2f, -65.7f, -184.3f, -66.2f, -183.3f, -66.7f,
    -182.4f, -67.1f, -181.4f, -67.6f, -180.5f, -68.1f, -179.5f, -68.6f, -178.6f, -69.0f,
    -177.6f, -69.5f, -176.7f, -70.0f, -175.7f, -70.5f, -174.8f, -70.9f, -173.8f, -71.4f,
    -172.9f, -71.9f, -171.9f, -72.4f, -171.0f, -72.8f, -170.0f, -73.3f, -169.1f, -73.8f,
    -168.1f, -74.3f, -167.2f, -74.7f, -166.2f, -75.2f, -165.3f, -75.7f, -164.3f, -76.2f,
    -163.4f, -76.6f, -162.4f, -77.1f, -161.5f, -77.6f, -160.5f, -78.1f, -159.6f, -78.5f,
    -158.7f, -79.0f, -157.7f, -79.5f, -156.8f, -80.0f, -155.8f, -80.4f, -154.9f, -80.9f,
    -154.2f, -81.6f, -154.3f, -82.6f, -155.2f, -83.3f, -156.2f, -83.3f,
};

constexpr std::array<float, 70 * 2> handheld_body = {
    -137.3f, -81.9f, -137.6f, -81.8f, -137.8f, -81.6f, -138.0f, -81.3f, -138.1f, -81.1f,
    -138.1f, -80.8f, -138.2f, -78.7f, -138.2f, -78.4f, -138.3f, -78.1f, -138.7f, -77.3f,
    -138.9f, -77.0f, -139.0f, -76.8f, -139.2f, -76.5f, -139.5f, -76.3f, -139.7f, -76.1f,
    -139.9f, -76.0f, -140.2f, -75.8f, -140.5f, -75.7f, -140.7f, -75.6f, -141.0f, -75.5f,
    -141.9f, -75.3f, -142.2f, -75.3f, -142.5f, -75.2f, -143.0f, -74.9f, -143.2f, -74.7f,
    -143.3f, -74.4f, -143.0f, -74.1f, -143.0f, 85.3f,  -143.0f, 85.6f,  -142.7f, 85.8f,
    -142.4f, 85.9f,  -142.2f, 85.9f,  143.0f,  85.6f,  143.1f,  85.4f,  143.3f,  85.1f,
    143.0f,  84.8f,  143.0f,  -74.9f, 142.8f,  -75.1f, 142.5f,  -75.2f, 141.9f,  -75.3f,
    141.6f,  -75.3f, 141.3f,  -75.4f, 141.1f,  -75.4f, 140.8f,  -75.5f, 140.5f,  -75.7f,
    140.2f,  -75.8f, 140.0f,  -76.0f, 139.7f,  -76.1f, 139.5f,  -76.3f, 139.1f,  -76.8f,
    138.9f,  -77.0f, 138.6f,  -77.5f, 138.4f,  -77.8f, 138.3f,  -78.1f, 138.3f,  -78.3f,
    138.2f,  -78.6f, 138.2f,  -78.9f, 138.1f,  -79.2f, 138.1f,  -79.5f, 138.0f,  -81.3f,
    137.8f,  -81.6f, 137.6f,  -81.8f, 137.3f,  -81.9f, 137.1f,  -81.9f, 120.0f,  -70.0f,
    -120.0f, -70.0f, -120.0f, 70.0f,  120.0f,  70.0f,  120.0f,  -70.0f, 137.1f,  -81.9f,
};

constexpr std::array<float, 40 * 2> handheld_bezel = {
    -131.4f, -75.9f, -132.2f, -75.7f, -132.9f, -75.3f, -134.2f, -74.3f, -134.7f, -73.6f,
    -135.1f, -72.8f, -135.4f, -72.0f, -135.5f, -71.2f, -135.5f, -70.4f, -135.2f, 76.7f,
    -134.8f, 77.5f,  -134.3f, 78.1f,  -133.7f, 78.8f,  -133.1f, 79.2f,  -132.3f, 79.6f,
    -131.5f, 79.9f,  -130.7f, 80.0f,  -129.8f, 80.0f,  132.2f,  79.7f,  133.0f,  79.3f,
    133.7f,  78.8f,  134.3f,  78.3f,  134.8f,  77.6f,  135.1f,  76.8f,  135.5f,  75.2f,
    135.5f,  74.3f,  135.2f,  -72.7f, 134.8f,  -73.5f, 134.4f,  -74.2f, 133.8f,  -74.8f,
    133.1f,  -75.3f, 132.3f,  -75.6f, 130.7f,  -76.0f, 129.8f,  -76.0f, -112.9f, -62.2f,
    112.9f,  -62.2f, 112.9f,  62.2f,  -112.9f, 62.2f,  -112.9f, -62.2f, 129.8f,  -76.0f,
};

constexpr std::array<float, 58 * 2> handheld_buttons = {
    -82.48f,  -82.95f, -82.53f,  -82.95f, -106.69f, -82.96f, -106.73f, -82.98f, -106.78f, -83.01f,
    -106.81f, -83.05f, -106.83f, -83.1f,  -106.83f, -83.15f, -106.82f, -83.93f, -106.81f, -83.99f,
    -106.8f,  -84.04f, -106.78f, -84.08f, -106.76f, -84.13f, -106.73f, -84.18f, -106.7f,  -84.22f,
    -106.6f,  -84.34f, -106.56f, -84.37f, -106.51f, -84.4f,  -106.47f, -84.42f, -106.42f, -84.45f,
    -106.37f, -84.47f, -106.32f, -84.48f, -106.17f, -84.5f,  -98.9f,   -84.48f, -98.86f,  -84.45f,
    -98.83f,  -84.41f, -98.81f,  -84.36f, -98.8f,   -84.31f, -98.8f,   -84.26f, -98.79f,  -84.05f,
    -90.26f,  -84.1f,  -90.26f,  -84.15f, -90.25f,  -84.36f, -90.23f,  -84.41f, -90.2f,   -84.45f,
    -90.16f,  -84.48f, -90.11f,  -84.5f,  -82.79f,  -84.49f, -82.74f,  -84.48f, -82.69f,  -84.46f,
    -82.64f,  -84.45f, -82.59f,  -84.42f, -82.55f,  -84.4f,  -82.5f,   -84.37f, -82.46f,  -84.33f,
    -82.42f,  -84.3f,  -82.39f,  -84.26f, -82.3f,   -84.13f, -82.28f,  -84.08f, -82.25f,  -83.98f,
    -82.24f,  -83.93f, -82.23f,  -83.83f, -82.23f,  -83.78f, -82.24f,  -83.1f,  -82.26f,  -83.05f,
    -82.29f,  -83.01f, -82.33f,  -82.97f, -82.38f,  -82.95f,
};

constexpr std::array<float, 47 * 2> left_joycon_slider = {
    -23.7f, -118.2f, -23.7f, -117.3f, -23.7f, 96.6f,   -22.8f, 96.6f,  -21.5f, 97.2f,  -21.5f,
    98.1f,  -21.2f,  106.7f, -20.8f,  107.5f, -20.1f,  108.2f, -19.2f, 108.2f, -16.4f, 108.1f,
    -15.8f, 107.5f,  -15.8f, 106.5f,  -15.8f, 62.8f,   -16.3f, 61.9f,  -15.8f, 61.0f,  -17.3f,
    60.3f,  -19.1f,  58.9f,  -19.1f,  58.1f,  -19.1f,  57.2f,  -19.1f, 34.5f,  -17.9f, 33.9f,
    -17.2f, 33.2f,   -16.6f, 32.4f,   -16.2f, 31.6f,   -15.8f, 30.7f,  -15.8f, 29.7f,  -15.8f,
    28.8f,  -15.8f,  -46.4f, -16.3f,  -47.3f, -15.8f,  -48.1f, -17.4f, -48.8f, -19.1f, -49.4f,
    -19.1f, -50.1f,  -19.1f, -51.0f,  -19.1f, -51.9f,  -19.1f, -73.7f, -19.1f, -74.5f, -17.5f,
    -75.2f, -16.4f,  -76.7f, -16.0f,  -77.6f, -15.8f,  -78.5f, -15.8f, -79.4f, -15.8f, -80.4f,
    -15.8f, -118.2f, -15.8f, -118.2f, -18.3f, -118.2f,
};

constexpr std::array<float, 66 * 2> left_joycon_sideview = {
    -158.8f, -133.5f, -159.8f, -133.5f, -173.5f, -133.3f, -174.5f, -133.0f, -175.4f, -132.6f,
    -176.2f, -132.1f, -177.0f, -131.5f, -177.7f, -130.9f, -178.3f, -130.1f, -179.4f, -128.5f,
    -179.8f, -127.6f, -180.4f, -125.7f, -180.6f, -124.7f, -180.7f, -123.8f, -180.7f, -122.8f,
    -180.0f, 128.8f,  -179.6f, 129.7f,  -179.1f, 130.5f,  -177.9f, 132.1f,  -177.2f, 132.7f,
    -176.4f, 133.3f,  -175.6f, 133.8f,  -174.7f, 134.3f,  -173.8f, 134.6f,  -172.8f, 134.8f,
    -170.9f, 135.0f,  -169.9f, 135.0f,  -156.1f, 134.8f,  -155.2f, 134.6f,  -154.2f, 134.3f,
    -153.3f, 134.0f,  -152.4f, 133.6f,  -151.6f, 133.1f,  -150.7f, 132.6f,  -149.9f, 132.0f,
    -149.2f, 131.4f,  -148.5f, 130.7f,  -147.1f, 129.2f,  -146.5f, 128.5f,  -146.0f, 127.7f,
    -145.5f, 126.8f,  -145.0f, 126.0f,  -144.6f, 125.1f,  -144.2f, 124.1f,  -143.9f, 123.2f,
    -143.7f, 122.2f,  -143.6f, 121.3f,  -143.5f, 120.3f,  -143.5f, 119.3f,  -144.4f, -123.4f,
    -144.8f, -124.3f, -145.3f, -125.1f, -145.8f, -126.0f, -146.3f, -126.8f, -147.0f, -127.5f,
    -147.6f, -128.3f, -148.3f, -129.0f, -149.0f, -129.6f, -149.8f, -130.3f, -150.6f, -130.8f,
    -151.4f, -131.4f, -152.2f, -131.9f, -153.1f, -132.3f, -155.9f, -133.3f, -156.8f, -133.5f,
    -157.8f, -133.5f,
};

constexpr std::array<float, 40 * 2> left_joycon_body_trigger = {
    -146.1f, -124.3f, -146.0f, -122.0f, -145.8f, -119.7f, -145.7f, -117.4f, -145.4f, -112.8f,
    -145.3f, -110.5f, -145.0f, -105.9f, -144.9f, -103.6f, -144.6f, -99.1f,  -144.5f, -96.8f,
    -144.5f, -89.9f,  -144.5f, -87.6f,  -144.5f, -83.0f,  -144.5f, -80.7f,  -144.5f, -80.3f,
    -142.4f, -82.4f,  -141.4f, -84.5f,  -140.2f, -86.4f,  -138.8f, -88.3f,  -137.4f, -90.1f,
    -134.5f, -93.6f,  -133.0f, -95.3f,  -130.0f, -98.8f,  -128.5f, -100.6f, -127.1f, -102.4f,
    -125.8f, -104.3f, -124.7f, -106.3f, -123.9f, -108.4f, -125.1f, -110.2f, -127.4f, -110.3f,
    -129.7f, -110.3f, -134.2f, -110.5f, -136.4f, -111.4f, -138.1f, -112.8f, -139.4f, -114.7f,
    -140.5f, -116.8f, -141.4f, -118.9f, -143.3f, -123.1f, -144.6f, -124.9f, -146.2f, -126.0f,
};

constexpr std::array<float, 49 * 2> left_joycon_topview = {
    -184.8f, -20.8f, -185.6f, -21.1f, -186.4f, -21.5f, -187.1f, -22.1f, -187.8f, -22.6f,
    -188.4f, -23.2f, -189.6f, -24.5f, -190.2f, -25.2f, -190.7f, -25.9f, -191.1f, -26.7f,
    -191.4f, -27.5f, -191.6f, -28.4f, -191.7f, -29.2f, -191.7f, -30.1f, -191.5f, -47.7f,
    -191.2f, -48.5f, -191.0f, -49.4f, -190.7f, -50.2f, -190.3f, -51.0f, -190.0f, -51.8f,
    -189.6f, -52.6f, -189.1f, -53.4f, -188.6f, -54.1f, -187.5f, -55.4f, -186.9f, -56.1f,
    -186.2f, -56.7f, -185.5f, -57.2f, -184.0f, -58.1f, -183.3f, -58.5f, -182.5f, -58.9f,
    -181.6f, -59.2f, -180.8f, -59.5f, -179.9f, -59.7f, -179.1f, -59.9f, -178.2f, -60.0f,
    -174.7f, -60.1f, -168.5f, -60.2f, -162.4f, -60.3f, -156.2f, -60.4f, -149.2f, -60.5f,
    -143.0f, -60.6f, -136.9f, -60.7f, -130.7f, -60.8f, -123.7f, -60.9f, -117.5f, -61.0f,
    -110.5f, -61.1f, -94.4f,  -60.4f, -94.4f,  -59.5f, -94.4f,  -20.6f,
};

constexpr std::array<float, 41 * 2> left_joycon_slider_topview = {
    -95.1f, -51.5f, -95.0f, -51.5f, -91.2f, -51.6f, -91.2f, -51.7f, -91.1f, -52.4f, -91.1f, -52.6f,
    -91.0f, -54.1f, -86.3f, -54.0f, -86.0f, -53.9f, -85.9f, -53.8f, -85.6f, -53.4f, -85.5f, -53.2f,
    -85.5f, -53.1f, -85.4f, -52.9f, -85.4f, -52.8f, -85.3f, -52.4f, -85.3f, -52.3f, -85.4f, -27.2f,
    -85.4f, -27.1f, -85.5f, -27.0f, -85.5f, -26.9f, -85.6f, -26.7f, -85.6f, -26.6f, -85.7f, -26.5f,
    -85.9f, -26.4f, -86.0f, -26.3f, -86.4f, -26.0f, -86.5f, -25.9f, -86.7f, -25.8f, -87.1f, -25.7f,
    -90.4f, -25.8f, -90.7f, -25.9f, -90.8f, -26.0f, -90.9f, -26.3f, -91.0f, -26.4f, -91.0f, -26.5f,
    -91.1f, -26.7f, -91.1f, -26.9f, -91.2f, -28.9f, -95.2f, -29.1f, -95.2f, -29.2f,
};

constexpr std::array<float, 42 * 2> left_joycon_sideview_zl = {
    -148.9f, -128.2f, -148.7f, -126.6f, -148.4f, -124.9f, -148.2f, -123.3f, -147.9f, -121.7f,
    -147.7f, -120.1f, -147.4f, -118.5f, -147.2f, -116.9f, -146.9f, -115.3f, -146.4f, -112.1f,
    -146.1f, -110.5f, -145.9f, -108.9f, -145.6f, -107.3f, -144.2f, -107.3f, -142.6f, -107.5f,
    -141.0f, -107.8f, -137.8f, -108.3f, -136.2f, -108.6f, -131.4f, -109.4f, -129.8f, -109.7f,
    -125.6f, -111.4f, -124.5f, -112.7f, -123.9f, -114.1f, -123.8f, -115.8f, -123.8f, -117.4f,
    -123.9f, -120.6f, -124.5f, -122.1f, -125.8f, -123.1f, -127.4f, -123.4f, -129.0f, -123.6f,
    -130.6f, -124.0f, -132.1f, -124.4f, -133.7f, -124.8f, -135.3f, -125.3f, -136.8f, -125.9f,
    -138.3f, -126.4f, -139.9f, -126.9f, -141.4f, -127.5f, -142.9f, -128.0f, -144.5f, -128.5f,
    -146.0f, -129.0f, -147.6f, -129.4f,
};

constexpr std::array<float, 72 * 2> left_joystick_sideview = {
    -14.7f, -3.8f,  -15.2f, -5.6f,  -15.2f, -7.6f,  -15.5f, -17.6f, -17.4f, -18.3f, -19.4f, -18.2f,
    -21.3f, -17.6f, -22.8f, -16.4f, -23.4f, -14.5f, -23.4f, -12.5f, -24.1f, -8.6f,  -24.8f, -6.7f,
    -25.3f, -4.8f,  -25.7f, -2.8f,  -25.9f, -0.8f,  -26.0f, 1.2f,   -26.0f, 3.2f,   -25.8f, 5.2f,
    -25.5f, 7.2f,   -25.0f, 9.2f,   -24.4f, 11.1f,  -23.7f, 13.0f,  -23.4f, 14.9f,  -23.4f, 16.9f,
    -23.3f, 18.9f,  -22.0f, 20.5f,  -20.2f, 21.3f,  -18.3f, 21.6f,  -16.3f, 21.4f,  -15.3f, 19.9f,
    -15.3f, 17.8f,  -15.2f, 7.8f,   -13.5f, 6.4f,   -12.4f, 7.2f,   -11.4f, 8.9f,   -10.2f, 10.5f,
    -8.7f,  11.8f,  -7.1f,  13.0f,  -5.3f,  14.0f,  -3.5f,  14.7f,  -1.5f,  15.0f,  0.5f,   15.0f,
    2.5f,   14.7f,  4.4f,   14.2f,  6.3f,   13.4f,  8.0f,   12.4f,  9.6f,   11.1f,  10.9f,  9.6f,
    12.0f,  7.9f,   12.7f,  6.0f,   13.2f,  4.1f,   13.3f,  2.1f,   13.2f,  0.1f,   12.9f,  -1.9f,
    12.2f,  -3.8f,  11.3f,  -5.6f,  10.2f,  -7.2f,  8.8f,   -8.6f,  7.1f,   -9.8f,  5.4f,   -10.8f,
    3.5f,   -11.5f, 1.5f,   -11.9f, -0.5f,  -12.0f, -2.5f,  -11.8f, -4.4f,  -11.3f, -6.2f,  -10.4f,
    -8.0f,  -9.4f,  -9.6f,  -8.2f,  -10.9f, -6.7f,  -11.9f, -4.9f,  -12.8f, -3.2f,  -13.5f, -3.8f,
};

constexpr std::array<float, 63 * 2> left_joystick_L_topview = {
    -186.7f, -43.7f, -186.4f, -43.7f, -110.6f, -43.4f, -110.6f, -43.1f, -110.7f, -34.3f,
    -110.7f, -34.0f, -110.8f, -33.7f, -111.1f, -32.9f, -111.2f, -32.6f, -111.4f, -32.3f,
    -111.5f, -32.1f, -111.7f, -31.8f, -111.8f, -31.5f, -112.0f, -31.3f, -112.2f, -31.0f,
    -112.4f, -30.8f, -112.8f, -30.3f, -113.0f, -30.1f, -114.1f, -29.1f, -114.3f, -28.9f,
    -114.6f, -28.7f, -114.8f, -28.6f, -115.1f, -28.4f, -115.3f, -28.3f, -115.6f, -28.1f,
    -115.9f, -28.0f, -116.4f, -27.8f, -116.7f, -27.7f, -117.3f, -27.6f, -117.6f, -27.5f,
    -182.9f, -27.6f, -183.5f, -27.7f, -183.8f, -27.8f, -184.4f, -27.9f, -184.6f, -28.1f,
    -184.9f, -28.2f, -185.4f, -28.5f, -185.7f, -28.7f, -185.9f, -28.8f, -186.2f, -29.0f,
    -186.4f, -29.2f, -187.0f, -29.9f, -187.2f, -30.1f, -187.6f, -30.6f, -187.8f, -30.8f,
    -187.9f, -31.1f, -188.1f, -31.3f, -188.2f, -31.6f, -188.4f, -31.9f, -188.5f, -32.1f,
    -188.6f, -32.4f, -188.8f, -33.3f, -188.9f, -33.6f, -188.9f, -33.9f, -188.8f, -39.9f,
    -188.8f, -40.2f, -188.7f, -41.1f, -188.7f, -41.4f, -188.6f, -41.7f, -188.0f, -43.1f,
    -187.9f, -43.4f, -187.6f, -43.6f, -187.3f, -43.7f,
};

constexpr std::array<float, 44 * 2> left_joystick_ZL_topview = {
    -179.4f, -53.3f, -177.4f, -53.3f, -111.2f, -53.3f, -111.3f, -53.3f, -111.5f, -58.6f,
    -111.8f, -60.5f, -112.2f, -62.4f, -113.1f, -66.1f, -113.8f, -68.0f, -114.5f, -69.8f,
    -115.3f, -71.5f, -116.3f, -73.2f, -117.3f, -74.8f, -118.5f, -76.4f, -119.8f, -77.8f,
    -121.2f, -79.1f, -122.8f, -80.2f, -124.4f, -81.2f, -126.2f, -82.0f, -128.1f, -82.6f,
    -130.0f, -82.9f, -131.9f, -83.0f, -141.5f, -82.9f, -149.3f, -82.8f, -153.1f, -82.6f,
    -155.0f, -82.1f, -156.8f, -81.6f, -158.7f, -80.9f, -160.4f, -80.2f, -162.2f, -79.3f,
    -163.8f, -78.3f, -165.4f, -77.2f, -166.9f, -76.0f, -168.4f, -74.7f, -169.7f, -73.3f,
    -172.1f, -70.3f, -173.2f, -68.7f, -174.2f, -67.1f, -175.2f, -65.4f, -176.1f, -63.7f,
    -178.7f, -58.5f, -179.6f, -56.8f, -180.4f, -55.1f, -181.3f, -53.3f,
};

void PlayerControlPreview::DrawProBody(QPainter& p, const QPointF center) {
    std::array<QPointF, pro_left_handle.size() / 2> qleft_handle;
    std::array<QPointF, pro_left_handle.size() / 2> qright_handle;
    std::array<QPointF, pro_body.size()> qbody;
    constexpr int radius1 = 32;

    for (std::size_t point = 0; point < pro_left_handle.size() / 2; ++point) {
        const float left_x = pro_left_handle[point * 2 + 0];
        const float left_y = pro_left_handle[point * 2 + 1];

        qleft_handle[point] = center + QPointF(left_x, left_y);
        qright_handle[point] = center + QPointF(-left_x, left_y);
    }
    for (std::size_t point = 0; point < pro_body.size() / 2; ++point) {
        const float body_x = pro_body[point * 2 + 0];
        const float body_y = pro_body[point * 2 + 1];

        qbody[point] = center + QPointF(body_x, body_y);
        qbody[pro_body.size() - 1 - point] = center + QPointF(-body_x, body_y);
    }

    // 1. Photorealistic Volumetric Drop Shadow beneath controller
    {
        QRadialGradient shadow_grad(center.x(), center.y() + 50.0, 250.0);
        shadow_grad.setColorAt(0.0, QColor(0, 0, 0, 130));
        shadow_grad.setColorAt(0.55, QColor(0, 0, 0, 50));
        shadow_grad.setColorAt(0.95, QColor(0, 0, 0, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(shadow_grad);
        p.drawEllipse(center + QPointF(0, 50), 245.0, 115.0);
    }

    // 2. Volumetric 3D Left Handle with ergonomic curvature shading
    {
        QLinearGradient left_grad(center.x() - 190.0, center.y(), center.x() - 100.0, center.y());
        left_grad.setColorAt(0.0, colors.grip_left_highlight);
        left_grad.setColorAt(0.35, colors.left);
        left_grad.setColorAt(0.85, colors.left.darker(115));
        left_grad.setColorAt(1.0, colors.grip_left_shadow);

        p.setPen(QPen(colors.outline, 1.2));
        p.setBrush(left_grad);
        DrawPolygon(p, qleft_handle);

        // Tactile micro-dot grip texture on left handle
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 22));
        for (int dy = -25; dy <= 95; dy += 12) {
            for (int dx = -180; dx <= -130; dx += 12) {
                const QPointF dot_pos = center + QPointF(dx + ((dy % 24 == 0) ? 6 : 0), dy);
                p.drawEllipse(dot_pos, 1.2, 1.2);
            }
        }
    }

    // 3. Volumetric 3D Right Handle with ergonomic curvature shading
    {
        QLinearGradient right_grad(center.x() + 190.0, center.y(), center.x() + 100.0, center.y());
        right_grad.setColorAt(0.0, colors.grip_right_highlight);
        right_grad.setColorAt(0.35, colors.right);
        right_grad.setColorAt(0.85, colors.right.darker(115));
        right_grad.setColorAt(1.0, colors.grip_right_shadow);

        p.setPen(QPen(colors.outline, 1.2));
        p.setBrush(right_grad);
        DrawPolygon(p, qright_handle);

        // Tactile micro-dot grip texture on right handle
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 22));
        for (int dy = -25; dy <= 95; dy += 12) {
            for (int dx = 130; dx <= 180; dx += 12) {
                const QPointF dot_pos = center + QPointF(dx + ((dy % 24 == 0) ? 6 : 0), dy);
                p.drawEllipse(dot_pos, 1.2, 1.2);
            }
        }
    }

    // 4. Main Body: Smoky Polycarbonate Chassis with Depth Shading
    {
        QLinearGradient body_grad(center.x(), center.y() - 90.0, center.x(), center.y() + 100.0);
        body_grad.setColorAt(0.0, colors.primary.lighter(125));
        body_grad.setColorAt(0.2, colors.primary);
        body_grad.setColorAt(0.8, colors.primary.darker(110));
        body_grad.setColorAt(1.0, colors.primary.darker(125));

        p.setPen(QPen(colors.outline, 1.2));
        p.setBrush(body_grad);
        DrawPolygon(p, qbody);

        // Subtle internal structural ribbing (visible through smoky translucent plastic)
        p.setPen(QPen(colors.body_inner, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(center.x() - 52, center.y() - 32, 104, 64), 10.0, 10.0);
        p.drawLine(center + QPointF(-40, 0), center + QPointF(40, 0));
        p.drawLine(center + QPointF(0, -32), center + QPointF(0, 32));

        // Polished top rim bevel highlight
        if (colors.body_rim.alpha() > 0) {
            p.setPen(QPen(colors.body_rim, 1.5));
            p.drawArc(QRectF(center.x() - 145, center.y() - 88, 290, 80), 30 * 16, 120 * 16);
        }
    }

    // 5. Authentic Edition Emblems & Signature Graphics
    switch (current_skin) {
    case ControllerSkin::Xenoblade2: {
        // Pyra / Aegis Emerald Core Crystal (Rhombus) + Golden Wings
        p.save();
        p.setPen(QPen(colors.emblem_secondary, 2.0));
        p.setBrush(QColor(colors.emblem_secondary.red(), colors.emblem_secondary.green(), colors.emblem_secondary.blue(), 60));
        const QPointF wing_left[] = {center + QPointF(-24, -18), center + QPointF(-48, -32), center + QPointF(-40, -14), center + QPointF(-24, -8)};
        const QPointF wing_right[] = {center + QPointF(24, -18), center + QPointF(48, -32), center + QPointF(40, -14), center + QPointF(24, -8)};
        p.drawPolygon(wing_left, 4);
        p.drawPolygon(wing_right, 4);

        const QPointF crystal[] = {center + QPointF(0, -34), center + QPointF(16, -16), center + QPointF(0, 2), center + QPointF(-16, -16)};
        p.setPen(QPen(QColor(255, 255, 255, 220), 1.5));
        p.setBrush(colors.emblem);
        p.drawPolygon(crystal, 4);
        p.setPen(QPen(QColor(255, 255, 255, 180), 1.0));
        p.drawLine(center + QPointF(0, -34), center + QPointF(0, 2));
        p.drawLine(center + QPointF(-16, -16), center + QPointF(16, -16));
        p.restore();
        break;
    }
    case ControllerSkin::SmashBrosUltimate: {
        // Super Smash Bros. Signature Off-Center Cross
        p.save();
        p.setPen(QPen(colors.emblem_secondary, 1.5));
        p.setBrush(colors.emblem);
        p.drawRect(QRectF(center.x() - 20, center.y() - 65, 13, 115));
        p.drawRect(QRectF(center.x() - 110, center.y() - 32, 215, 13));
        p.restore();
        break;
    }
    case ControllerSkin::ZeldaTotk: {
        // Zonai Sacred Swirl & Ancient Dragon Runes
        p.save();
        p.setPen(QPen(colors.emblem, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(center.x() - 35, center.y() - 40, 70, 70), 45 * 16, 270 * 16);
        p.drawArc(QRectF(center.x() - 25, center.y() - 30, 50, 50), 180 * 16, 220 * 16);
        p.setPen(QPen(colors.emblem_secondary, 1.8));
        p.drawLine(center + QPointF(40, -45), center + QPointF(65, -30));
        p.drawLine(center + QPointF(65, -30), center + QPointF(55, -15));
        p.drawLine(center + QPointF(55, -15), center + QPointF(75, 5));
        p.restore();
        break;
    }
    case ControllerSkin::Splatoon3: {
        // Splatoon 3 Ink Splatters (Neon Yellow & Violet)
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(colors.emblem);
        p.drawEllipse(center + QPointF(-25, -28), 16.0, 13.0);
        p.drawEllipse(center + QPointF(-38, -20), 9.0, 8.0);
        p.drawEllipse(center + QPointF(-16, -42), 6.5, 6.0);
        p.drawEllipse(center + QPointF(-44, -36), 4.5, 4.5);
        p.setBrush(colors.emblem_secondary);
        p.drawEllipse(center + QPointF(22, 10), 14.0, 12.0);
        p.drawEllipse(center + QPointF(35, 18), 8.0, 7.5);
        p.drawEllipse(center + QPointF(12, 24), 6.0, 5.5);
        p.restore();
        break;
    }
    case ControllerSkin::MonsterHunterRise: {
        // Magnamalo Golden Dragon Armor & Flame Crest
        p.save();
        p.setPen(QPen(colors.emblem, 2.0));
        p.setBrush(QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 50));
        const QPointF blade1[] = {center + QPointF(0, -45), center + QPointF(14, -25), center + QPointF(0, -10)};
        const QPointF blade2[] = {center + QPointF(0, -45), center + QPointF(-14, -25), center + QPointF(0, -10)};
        p.drawPolyline(blade1, 3);
        p.drawPolyline(blade2, 3);
        p.setPen(QPen(colors.emblem_secondary, 1.8));
        p.drawArc(QRectF(center.x() - 40, center.y() - 35, 80, 55), 20 * 16, 140 * 16);
        p.restore();
        break;
    }
    case ControllerSkin::PokemonScarletViolet: {
        // Naranja / Uva Academy Shield Crest
        p.save();
        p.setPen(QPen(colors.emblem, 1.8));
        p.setBrush(QColor(colors.emblem.red(), colors.emblem.green(), colors.emblem.blue(), 40));
        const QPointF shield[] = {
            center + QPointF(0, -38), center + QPointF(20, -32), center + QPointF(20, -10),
            center + QPointF(0, 10), center + QPointF(-20, -10), center + QPointF(-20, -32)
        };
        p.drawPolygon(shield, 6);
        p.setPen(QPen(colors.emblem_secondary, 1.5));
        p.drawLine(center + QPointF(0, -38), center + QPointF(0, 10));
        p.drawLine(center + QPointF(-20, -20), center + QPointF(20, -20));
        p.restore();
        break;
    }
    case ControllerSkin::CyberStorm: {
        // STORM Soft Cyberpunk Vector Grid & Lightning Trace
        p.save();
        p.setPen(QPen(colors.emblem, 1.6));
        p.drawLine(center + QPointF(-70, -25), center + QPointF(-35, -25));
        p.drawLine(center + QPointF(-35, -25), center + QPointF(-15, -45));
        p.drawLine(center + QPointF(-15, -45), center + QPointF(20, -45));
        p.drawLine(center + QPointF(20, -45), center + QPointF(40, -25));
        p.drawLine(center + QPointF(40, -25), center + QPointF(75, -25));
        p.setPen(Qt::NoPen);
        p.setBrush(colors.emblem);
        p.drawEllipse(center + QPointF(-35, -25), 3.0, 3.0);
        p.drawEllipse(center + QPointF(40, -25), 3.0, 3.0);
        p.setPen(QPen(colors.emblem_secondary, 2.0));
        const QPointF bolt[] = {center + QPointF(4, -30), center + QPointF(-4, -16), center + QPointF(2, -16), center + QPointF(-2, -4)};
        p.drawPolyline(bolt, 4);
        p.restore();
        break;
    }
    default:
        break;
    }

    // 6. Joy-Con Stick Wells with Chamfered Friction Ring and Inner Occlusion
    const QPointF left_well = center + QPoint(-111, -55);
    const QPointF right_well = center + QPoint(51, 0);

    for (const QPointF& well_center : {left_well, right_well}) {
        p.setPen(QPen(colors.outline, 1.4));
        QRadialGradient well_grad(well_center, radius1);
        well_grad.setColorAt(0.0, QColor(16, 18, 22));
        well_grad.setColorAt(0.7, QColor(28, 32, 38));
        well_grad.setColorAt(0.95, QColor(48, 52, 60));
        well_grad.setColorAt(1.0, colors.outline);
        p.setBrush(well_grad);
        p.drawEllipse(well_center, radius1, radius1);

        p.setPen(QPen(QColor(70, 75, 85, 120), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(well_center, radius1 - 3.0f, radius1 - 3.0f);
    }
}

void PlayerControlPreview::DrawGCBody(QPainter& p, const QPointF center) {
    std::array<QPointF, gc_left_body.size() / 2> qleft_handle;
    std::array<QPointF, gc_left_body.size() / 2> qright_handle;
    std::array<QPointF, gc_body.size()> qbody;
    std::array<QPointF, 8> left_hex;
    std::array<QPointF, 8> right_hex;
    constexpr float angle = 2.f * float(M_PI) / 8.f;

    for (std::size_t point = 0; point < gc_left_body.size() / 2; ++point) {
        const float body_x = gc_left_body[point * 2 + 0];
        const float body_y = gc_left_body[point * 2 + 1];

        qleft_handle[point] = center + QPointF(body_x, body_y);
        qright_handle[point] = center + QPointF(-body_x, body_y);
    }
    for (std::size_t point = 0; point < gc_body.size() / 2; ++point) {
        const float body_x = gc_body[point * 2 + 0];
        const float body_y = gc_body[point * 2 + 1];

        qbody[point] = center + QPointF(body_x, body_y);
        qbody[gc_body.size() - 1 - point] = center + QPointF(-body_x, body_y);
    }
    for (std::size_t point = 0; point < 8; ++point) {
        const float point_cos = std::cos(point * angle);
        const float point_sin = std::sin(point * angle);

        left_hex[point] = center + QPointF(34 * point_cos - 111, 34 * point_sin - 44);
        right_hex[point] = center + QPointF(26 * point_cos + 61, 26 * point_sin + 37);
    }

    // Draw body
    p.setPen(colors.outline);
    p.setBrush(colors.primary);
    DrawPolygon(p, qbody);

    // Draw left handle body
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_handle);

    // Draw right handle body
    p.setBrush(colors.right);
    DrawPolygon(p, qright_handle);

    DrawText(p, center + QPoint(0, -58), 4.7f, tr("START/PAUSE"));

    // Draw right joystick body
    p.setBrush(colors.button);
    DrawCircle(p, center + QPointF(61, 37), 23.5f);

    // Draw joystick details
    p.setBrush(colors.transparent);
    DrawPolygon(p, left_hex);
    DrawPolygon(p, right_hex);
}

void PlayerControlPreview::DrawHandheldBody(QPainter& p, const QPointF center) {
    const std::size_t body_outline_end = handheld_body.size() / 2 - 6;
    const std::size_t bezel_outline_end = handheld_bezel.size() / 2 - 6;
    const std::size_t bezel_inline_size = 4;
    const std::size_t bezel_inline_start = 35;
    std::array<QPointF, left_joycon_body.size() / 2> left_joycon;
    std::array<QPointF, left_joycon_body.size() / 2> right_joycon;
    std::array<QPointF, handheld_body.size() / 2> qhandheld_body;
    std::array<QPointF, body_outline_end> qhandheld_body_outline;
    std::array<QPointF, handheld_bezel.size() / 2> qhandheld_bezel;
    std::array<QPointF, bezel_inline_size> qhandheld_bezel_inline;
    std::array<QPointF, bezel_outline_end> qhandheld_bezel_outline;
    std::array<QPointF, handheld_buttons.size() / 2> qhandheld_buttons;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        left_joycon[point] =
            center + QPointF(left_joycon_body[point * 2], left_joycon_body[point * 2 + 1]);
        right_joycon[point] =
            center + QPointF(-left_joycon_body[point * 2], left_joycon_body[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < body_outline_end; ++point) {
        qhandheld_body_outline[point] =
            center + QPointF(handheld_body[point * 2], handheld_body[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < handheld_body.size() / 2; ++point) {
        qhandheld_body[point] =
            center + QPointF(handheld_body[point * 2], handheld_body[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < handheld_bezel.size() / 2; ++point) {
        qhandheld_bezel[point] =
            center + QPointF(handheld_bezel[point * 2], handheld_bezel[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < bezel_outline_end; ++point) {
        qhandheld_bezel_outline[point] =
            center + QPointF(handheld_bezel[point * 2], handheld_bezel[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < bezel_inline_size; ++point) {
        qhandheld_bezel_inline[point] =
            center + QPointF(handheld_bezel[(point + bezel_inline_start) * 2],
                             handheld_bezel[(point + bezel_inline_start) * 2 + 1]);
    }
    for (std::size_t point = 0; point < handheld_buttons.size() / 2; ++point) {
        qhandheld_buttons[point] =
            center + QPointF(handheld_buttons[point * 2], handheld_buttons[point * 2 + 1]);
    }

    // Draw left joycon
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, left_joycon);

    // Draw right joycon
    p.setPen(colors.outline);
    p.setBrush(colors.right);
    DrawPolygon(p, right_joycon);

    // Draw Handheld buttons
    p.setPen(colors.outline);
    p.setBrush(colors.button);
    DrawPolygon(p, qhandheld_buttons);

    // Draw handheld body
    p.setPen(colors.transparent);
    p.setBrush(colors.primary);
    DrawPolygon(p, qhandheld_body);
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    DrawPolygon(p, qhandheld_body_outline);

    // Draw Handheld bezel
    p.setPen(colors.transparent);
    p.setBrush(colors.button);
    DrawPolygon(p, qhandheld_bezel);
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    DrawPolygon(p, qhandheld_bezel_outline);
    DrawPolygon(p, qhandheld_bezel_inline);
}

void PlayerControlPreview::DrawDualBody(QPainter& p, const QPointF center) {
    std::array<QPointF, left_joycon_body.size() / 2> left_joycon;
    std::array<QPointF, left_joycon_body.size() / 2> right_joycon;
    std::array<QPointF, left_joycon_slider.size() / 2> qleft_joycon_slider;
    std::array<QPointF, left_joycon_slider.size() / 2> qright_joycon_slider;
    std::array<QPointF, left_joycon_slider_topview.size() / 2> qleft_joycon_slider_topview;
    std::array<QPointF, left_joycon_slider_topview.size() / 2> qright_joycon_slider_topview;
    std::array<QPointF, left_joycon_topview.size() / 2> qleft_joycon_topview;
    std::array<QPointF, left_joycon_topview.size() / 2> qright_joycon_topview;
    constexpr float size = 1.61f;
    constexpr float size2 = 0.9f;
    constexpr float offset = 209.3f;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        const float body_x = left_joycon_body[point * 2 + 0];
        const float body_y = left_joycon_body[point * 2 + 1];

        left_joycon[point] = center + QPointF(body_x * size + offset, body_y * size - 1);
        right_joycon[point] = center + QPointF(-body_x * size - offset, body_y * size - 1);
    }
    for (std::size_t point = 0; point < left_joycon_slider.size() / 2; ++point) {
        const float slider_x = left_joycon_slider[point * 2 + 0];
        const float slider_y = left_joycon_slider[point * 2 + 1];

        qleft_joycon_slider[point] = center + QPointF(slider_x, slider_y);
        qright_joycon_slider[point] = center + QPointF(-slider_x, slider_y);
    }
    for (std::size_t point = 0; point < left_joycon_topview.size() / 2; ++point) {
        const float top_view_x = left_joycon_topview[point * 2 + 0];
        const float top_view_y = left_joycon_topview[point * 2 + 1];

        qleft_joycon_topview[point] =
            center + QPointF(top_view_x * size2 - 52, top_view_y * size2 - 52);
        qright_joycon_topview[point] =
            center + QPointF(-top_view_x * size2 + 52, top_view_y * size2 - 52);
    }
    for (std::size_t point = 0; point < left_joycon_slider_topview.size() / 2; ++point) {
        const float top_view_x = left_joycon_slider_topview[point * 2 + 0];
        const float top_view_y = left_joycon_slider_topview[point * 2 + 1];

        qleft_joycon_slider_topview[point] =
            center + QPointF(top_view_x * size2 - 52, top_view_y * size2 - 52);
        qright_joycon_slider_topview[point] =
            center + QPointF(-top_view_x * size2 + 52, top_view_y * size2 - 52);
    }

    // right joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.right);
    DrawPolygon(p, right_joycon);

    // Left joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, left_joycon);

    // Slider release button top view
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(-149, -108), 12, 11, 2);
    DrawRoundRectangle(p, center + QPoint(149, -108), 12, 11, 2);

    // Joycon slider top view
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider_topview);
    p.drawLine(center + QPointF(-133.8f, -99.0f), center + QPointF(-133.8f, -78.5f));
    DrawPolygon(p, qright_joycon_slider_topview);
    p.drawLine(center + QPointF(133.8f, -99.0f), center + QPointF(133.8f, -78.5f));

    // Joycon body top view
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_joycon_topview);
    p.setBrush(colors.right);
    DrawPolygon(p, qright_joycon_topview);

    // Right Sideview body
    p.setBrush(colors.slider);
    DrawPolygon(p, qright_joycon_slider);

    // Left Sideview body
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider);
}

void PlayerControlPreview::DrawLeftBody(QPainter& p, const QPointF center) {
    std::array<QPointF, left_joycon_body.size() / 2> left_joycon;
    std::array<QPointF, left_joycon_sideview.size() / 2> qleft_joycon_sideview;
    std::array<QPointF, left_joycon_body_trigger.size() / 2> qleft_joycon_trigger;
    std::array<QPointF, left_joycon_slider.size() / 2> qleft_joycon_slider;
    std::array<QPointF, left_joycon_slider_topview.size() / 2> qleft_joycon_slider_topview;
    std::array<QPointF, left_joycon_topview.size() / 2> qleft_joycon_topview;
    constexpr float size = 1.78f;
    constexpr float size2 = 1.1115f;
    constexpr float offset = 312.39f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        left_joycon[point] = center + QPointF(left_joycon_body[point * 2] * size + offset,
                                              left_joycon_body[point * 2 + 1] * size - 1);
    }

    for (std::size_t point = 0; point < left_joycon_sideview.size() / 2; ++point) {
        qleft_joycon_sideview[point] =
            center + QPointF(left_joycon_sideview[point * 2] * size2 + offset2,
                             left_joycon_sideview[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_slider.size() / 2; ++point) {
        qleft_joycon_slider[point] = center + QPointF(left_joycon_slider[point * 2] * size2 + 81,
                                                      left_joycon_slider[point * 2 + 1] * size2);
    }
    for (std::size_t point = 0; point < left_joycon_body_trigger.size() / 2; ++point) {
        qleft_joycon_trigger[point] =
            center + QPointF(left_joycon_body_trigger[point * 2] * size2 + offset2,
                             left_joycon_body_trigger[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_topview.size() / 2; ++point) {
        qleft_joycon_topview[point] =
            center + QPointF(left_joycon_topview[point * 2], left_joycon_topview[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < left_joycon_slider_topview.size() / 2; ++point) {
        qleft_joycon_slider_topview[point] =
            center + QPointF(left_joycon_slider_topview[point * 2],
                             left_joycon_slider_topview[point * 2 + 1]);
    }

    // Joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, left_joycon);
    DrawPolygon(p, qleft_joycon_trigger);

    // Slider release button top view
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(-107, -62), 14, 12, 2);

    // Joycon slider top view
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider_topview);
    p.drawLine(center + QPointF(-91.1f, -51.7f), center + QPointF(-91.1f, -26.5f));

    // Joycon body top view
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_joycon_topview);

    // Slider release button
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(175, -110), 12, 14, 2);

    // Sideview body
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_joycon_sideview);
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider);

    const QPointF sideview_center = QPointF(155, 0) + center;

    // Sideview slider body
    p.setBrush(colors.slider);
    DrawRoundRectangle(p, sideview_center + QPointF(0, -5), 28, 253, 3);
    p.setBrush(colors.button2);
    DrawRoundRectangle(p, sideview_center + QPointF(0, 97), 22.44f, 44.66f, 3);

    // Slider decorations
    p.setPen(colors.outline);
    p.setBrush(colors.slider_arrow);
    DrawArrow(p, sideview_center + QPoint(0, 83), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 96), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 109), Direction::Down, 2.2f);
    DrawCircle(p, sideview_center + QPointF(0, 19), 4.44f);

    // LED indicators
    const float led_size = 5.0f;
    const QPointF led_position = sideview_center + QPointF(0, -36);
    int led_count = 0;
    p.setBrush(led_pattern.position1 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    p.setBrush(led_pattern.position2 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    p.setBrush(led_pattern.position3 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    p.setBrush(led_pattern.position4 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
}

void PlayerControlPreview::DrawRightBody(QPainter& p, const QPointF center) {
    std::array<QPointF, left_joycon_body.size() / 2> right_joycon;
    std::array<QPointF, left_joycon_sideview.size() / 2> qright_joycon_sideview;
    std::array<QPointF, left_joycon_body_trigger.size() / 2> qright_joycon_trigger;
    std::array<QPointF, left_joycon_slider.size() / 2> qright_joycon_slider;
    std::array<QPointF, left_joycon_slider_topview.size() / 2> qright_joycon_slider_topview;
    std::array<QPointF, left_joycon_topview.size() / 2> qright_joycon_topview;
    constexpr float size = 1.78f;
    constexpr float size2 = 1.1115f;
    constexpr float offset = 312.39f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        right_joycon[point] = center + QPointF(-left_joycon_body[point * 2] * size - offset,
                                               left_joycon_body[point * 2 + 1] * size - 1);
    }

    for (std::size_t point = 0; point < left_joycon_sideview.size() / 2; ++point) {
        qright_joycon_sideview[point] =
            center + QPointF(-left_joycon_sideview[point * 2] * size2 - offset2,
                             left_joycon_sideview[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_body_trigger.size() / 2; ++point) {
        qright_joycon_trigger[point] =
            center + QPointF(-left_joycon_body_trigger[point * 2] * size2 - offset2,
                             left_joycon_body_trigger[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_slider.size() / 2; ++point) {
        qright_joycon_slider[point] = center + QPointF(-left_joycon_slider[point * 2] * size2 - 81,
                                                       left_joycon_slider[point * 2 + 1] * size2);
    }
    for (std::size_t point = 0; point < left_joycon_topview.size() / 2; ++point) {
        qright_joycon_topview[point] =
            center + QPointF(-left_joycon_topview[point * 2], left_joycon_topview[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < left_joycon_slider_topview.size() / 2; ++point) {
        qright_joycon_slider_topview[point] =
            center + QPointF(-left_joycon_slider_topview[point * 2],
                             left_joycon_slider_topview[point * 2 + 1]);
    }

    // Joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, right_joycon);
    DrawPolygon(p, qright_joycon_trigger);

    // Slider release button top view
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(107, -62), 14, 12, 2);

    // Joycon slider top view
    p.setBrush(colors.slider);
    DrawPolygon(p, qright_joycon_slider_topview);
    p.drawLine(center + QPointF(91.1f, -51.7f), center + QPointF(91.1f, -26.5f));

    // Joycon body top view
    p.setBrush(colors.left);
    DrawPolygon(p, qright_joycon_topview);

    // Slider release button
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(-175, -110), 12, 14, 2);

    // Sideview body
    p.setBrush(colors.left);
    DrawPolygon(p, qright_joycon_sideview);
    p.setBrush(colors.slider);
    DrawPolygon(p, qright_joycon_slider);

    const QPointF sideview_center = QPointF(-155, 0) + center;

    // Sideview slider body
    p.setBrush(colors.slider);
    DrawRoundRectangle(p, sideview_center + QPointF(0, -5), 28, 253, 3);
    p.setBrush(colors.button2);
    DrawRoundRectangle(p, sideview_center + QPointF(0, 97), 22.44f, 44.66f, 3);

    // Slider decorations
    p.setPen(colors.outline);
    p.setBrush(colors.slider_arrow);
    DrawArrow(p, sideview_center + QPoint(0, 83), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 96), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 109), Direction::Down, 2.2f);
    DrawCircle(p, sideview_center + QPointF(0, 19), 4.44f);

    // LED indicators
    const float led_size = 5.0f;
    const QPointF led_position = sideview_center + QPointF(0, -36);
    int led_count = 0;
    p.setBrush(led_pattern.position1 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    p.setBrush(led_pattern.position2 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    p.setBrush(led_pattern.position3 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    p.setBrush(led_pattern.position4 ? colors.led_on : colors.led_off);
    DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
}

void PlayerControlPreview::DrawProTriggers(QPainter& p, const QPointF center,
                                           const Common::Input::ButtonStatus& left_pressed,
                                           const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, pro_left_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, pro_left_trigger.size() / 2> qright_trigger;
    std::array<QPointF, pro_body_top.size()> qbody_top;

    for (std::size_t point = 0; point < pro_left_trigger.size() / 2; ++point) {
        const float trigger_x = pro_left_trigger[point * 2 + 0];
        const float trigger_y = pro_left_trigger[point * 2 + 1];

        qleft_trigger[point] =
            center + QPointF(trigger_x, trigger_y + (left_pressed.value ? 2 : 0));
        qright_trigger[point] =
            center + QPointF(-trigger_x, trigger_y + (right_pressed.value ? 2 : 0));
    }

    for (std::size_t point = 0; point < pro_body_top.size() / 2; ++point) {
        const float top_x = pro_body_top[point * 2 + 0];
        const float top_y = pro_body_top[point * 2 + 1];

        qbody_top[pro_body_top.size() - 1 - point] = center + QPointF(-top_x, top_y);
        qbody_top[point] = center + QPointF(top_x, top_y);
    }

    // Pro body detail
    p.setPen(colors.outline);
    p.setBrush(colors.primary);
    DrawPolygon(p, qbody_top);

    // Left trigger
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawGCTriggers(QPainter& p, const QPointF center,
                                          Common::Input::TriggerStatus left_trigger,
                                          Common::Input::TriggerStatus right_trigger) {
    std::array<QPointF, left_gc_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, left_gc_trigger.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_gc_trigger.size() / 2; ++point) {
        const float trigger_x = left_gc_trigger[point * 2 + 0];
        const float trigger_y = left_gc_trigger[point * 2 + 1];

        qleft_trigger[point] =
            center + QPointF(trigger_x, trigger_y + (left_trigger.analog.value * 10.0f));
        qright_trigger[point] =
            center + QPointF(-trigger_x, trigger_y + (right_trigger.analog.value * 10.0f));
    }

    // Left trigger
    p.setPen(colors.outline);
    p.setBrush(left_trigger.pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_trigger.pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw L text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, center + QPointF(-132, -119 + (left_trigger.analog.value * 10.0f)), Symbol::L,
               1.7f);

    // Draw R text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, center + QPointF(121.5f, -119 + (right_trigger.analog.value * 10.0f)), Symbol::R,
               1.7f);
}

void PlayerControlPreview::DrawHandheldTriggers(QPainter& p, const QPointF center,
                                                const Common::Input::ButtonStatus& left_pressed,
                                                const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, left_joycon_trigger.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        const float left_trigger_x = left_joycon_trigger[point * 2 + 0];
        const float left_trigger_y = left_joycon_trigger[point * 2 + 1];

        qleft_trigger[point] =
            center + QPointF(left_trigger_x, left_trigger_y + (left_pressed.value ? 0.5f : 0));
        qright_trigger[point] =
            center + QPointF(-left_trigger_x, left_trigger_y + (right_pressed.value ? 0.5f : 0));
    }

    // Left trigger
    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawDualTriggers(QPainter& p, const QPointF center,
                                            const Common::Input::ButtonStatus& left_pressed,
                                            const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, left_joycon_trigger.size() / 2> qright_trigger;
    constexpr float size = 1.62f;
    constexpr float offset = 210.6f;
    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        const float left_trigger_x = left_joycon_trigger[point * 2 + 0];
        const float left_trigger_y = left_joycon_trigger[point * 2 + 1];

        qleft_trigger[point] =
            center + QPointF(left_trigger_x * size + offset,
                             left_trigger_y * size + (left_pressed.value ? 0.5f : 0));
        qright_trigger[point] =
            center + QPointF(-left_trigger_x * size - offset,
                             left_trigger_y * size + (right_pressed.value ? 0.5f : 0));
    }

    // Left trigger
    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawDualTriggersTopView(
    QPainter& p, const QPointF center, const Common::Input::ButtonStatus& left_pressed,
    const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joystick_L_topview.size() / 2> qleft_trigger;
    std::array<QPointF, left_joystick_L_topview.size() / 2> qright_trigger;
    constexpr float size = 0.9f;

    for (std::size_t point = 0; point < left_joystick_L_topview.size() / 2; ++point) {
        const float top_view_x = left_joystick_L_topview[point * 2 + 0];
        const float top_view_y = left_joystick_L_topview[point * 2 + 1];

        qleft_trigger[point] = center + QPointF(top_view_x * size - 50, top_view_y * size - 52);
    }
    for (std::size_t point = 0; point < left_joystick_L_topview.size() / 2; ++point) {
        const float top_view_x = left_joystick_L_topview[point * 2 + 0];
        const float top_view_y = left_joystick_L_topview[point * 2 + 1];

        qright_trigger[point] = center + QPointF(-top_view_x * size + 50, top_view_y * size - 52);
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw L text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(-183, -84), Symbol::L, 1.0f);

    // Draw R text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(177, -84), Symbol::R, 1.0f);
}

void PlayerControlPreview::DrawDualZTriggersTopView(
    QPainter& p, const QPointF center, const Common::Input::ButtonStatus& left_pressed,
    const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joystick_ZL_topview.size() / 2> qleft_trigger;
    std::array<QPointF, left_joystick_ZL_topview.size() / 2> qright_trigger;
    constexpr float size = 0.9f;

    for (std::size_t point = 0; point < left_joystick_ZL_topview.size() / 2; ++point) {
        qleft_trigger[point] =
            center + QPointF(left_joystick_ZL_topview[point * 2] * size - 52,
                             left_joystick_ZL_topview[point * 2 + 1] * size - 52);
    }
    for (std::size_t point = 0; point < left_joystick_ZL_topview.size() / 2; ++point) {
        qright_trigger[point] =
            center + QPointF(-left_joystick_ZL_topview[point * 2] * size + 52,
                             left_joystick_ZL_topview[point * 2 + 1] * size - 52);
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw ZL text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(-180, -113), Symbol::ZL, 1.0f);

    // Draw ZR text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(180, -113), Symbol::ZR, 1.0f);
}

void PlayerControlPreview::DrawLeftTriggers(QPainter& p, const QPointF center,
                                            const Common::Input::ButtonStatus& left_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qleft_trigger;
    constexpr float size = 1.78f;
    constexpr float offset = 311.5f;

    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joycon_trigger[point * 2] * size + offset,
                                                left_joycon_trigger[point * 2 + 1] * size -
                                                    (left_pressed.value ? 0.5f : 1.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
}

void PlayerControlPreview::DrawLeftZTriggers(QPainter& p, const QPointF center,
                                             const Common::Input::ButtonStatus& left_pressed) {
    std::array<QPointF, left_joycon_sideview_zl.size() / 2> qleft_trigger;
    constexpr float size = 1.1115f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_sideview_zl.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joycon_sideview_zl[point * 2] * size + offset2,
                                                left_joycon_sideview_zl[point * 2 + 1] * size +
                                                    (left_pressed.value ? 1.5f : 1.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
    p.drawArc(center.x() + 158, center.y() + (left_pressed.value ? -203.5f : -204.0f), 77, 77,
              225 * 16, 44 * 16);
}

void PlayerControlPreview::DrawLeftTriggersTopView(
    QPainter& p, const QPointF center, const Common::Input::ButtonStatus& left_pressed) {
    std::array<QPointF, left_joystick_L_topview.size() / 2> qleft_trigger;

    for (std::size_t point = 0; point < left_joystick_L_topview.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joystick_L_topview[point * 2],
                                                left_joystick_L_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Draw L text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(-143, -36), Symbol::L, 1.0f);
}

void PlayerControlPreview::DrawLeftZTriggersTopView(
    QPainter& p, const QPointF center, const Common::Input::ButtonStatus& left_pressed) {
    std::array<QPointF, left_joystick_ZL_topview.size() / 2> qleft_trigger;

    for (std::size_t point = 0; point < left_joystick_ZL_topview.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joystick_ZL_topview[point * 2],
                                                left_joystick_ZL_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Draw ZL text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(-140, -68), Symbol::ZL, 1.0f);
}

void PlayerControlPreview::DrawRightTriggers(QPainter& p, const QPointF center,
                                             const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qright_trigger;
    constexpr float size = 1.78f;
    constexpr float offset = 311.5f;

    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        qright_trigger[point] = center + QPointF(-left_joycon_trigger[point * 2] * size - offset,
                                                 left_joycon_trigger[point * 2 + 1] * size -
                                                     (right_pressed.value ? 0.5f : 1.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawRightZTriggers(QPainter& p, const QPointF center,
                                              const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joycon_sideview_zl.size() / 2> qright_trigger;
    constexpr float size = 1.1115f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_sideview_zl.size() / 2; ++point) {
        qright_trigger[point] =
            center + QPointF(-left_joycon_sideview_zl[point * 2] * size - offset2,
                             left_joycon_sideview_zl[point * 2 + 1] * size +
                                 (right_pressed.value ? 0.5f : 0) + 1);
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
    p.drawArc(center.x() - 236, center.y() + (right_pressed.value ? -203.5f : -204.0f), 77, 77,
              271 * 16, 44 * 16);
}

void PlayerControlPreview::DrawRightTriggersTopView(
    QPainter& p, const QPointF center, const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joystick_L_topview.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joystick_L_topview.size() / 2; ++point) {
        qright_trigger[point] = center + QPointF(-left_joystick_L_topview[point * 2],
                                                 left_joystick_L_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw R text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(137, -36), Symbol::R, 1.0f);
}

void PlayerControlPreview::DrawRightZTriggersTopView(
    QPainter& p, const QPointF center, const Common::Input::ButtonStatus& right_pressed) {
    std::array<QPointF, left_joystick_ZL_topview.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joystick_ZL_topview.size() / 2; ++point) {
        qright_trigger[point] = center + QPointF(-left_joystick_ZL_topview[point * 2],
                                                 left_joystick_ZL_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed.value ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw ZR text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(140, -68), Symbol::ZR, 1.0f);
}

void PlayerControlPreview::DrawJoystick(QPainter& p, const QPointF center, float size,
                                        const Common::Input::ButtonStatus& pressed) {
    const float radius1 = 13.0f * size;
    const float radius2 = 9.0f * size;

    // Outer circle
    p.setPen(colors.outline);
    p.setBrush(pressed.value ? colors.highlight : colors.button);
    DrawCircle(p, center, radius1);

    // Cross
    p.drawLine(center - QPoint(radius1, 0), center + QPoint(radius1, 0));
    p.drawLine(center - QPoint(0, radius1), center + QPoint(0, radius1));

    // Inner circle
    p.setBrush(pressed.value ? colors.highlight2 : colors.button2);
    DrawCircle(p, center, radius2);
}

void PlayerControlPreview::DrawJoystickSideview(QPainter& p, const QPointF center, float angle,
                                                float size,
                                                const Common::Input::ButtonStatus& pressed) {
    QVector<QPointF> joystick;
    joystick.reserve(static_cast<int>(left_joystick_sideview.size() / 2));

    for (std::size_t point = 0; point < left_joystick_sideview.size() / 2; ++point) {
        joystick.append(QPointF(left_joystick_sideview[point * 2] * size + (pressed.value ? 1 : 0),
                                left_joystick_sideview[point * 2 + 1] * size - 1));
    }

    // Rotate joystick
    QTransform t;
    t.translate(center.x(), center.y());
    t.rotate(18 * angle);
    QPolygonF p2 = t.map(QPolygonF(joystick));

    // Draw joystick
    p.setPen(colors.outline);
    p.setBrush(pressed.value ? colors.highlight : colors.button);
    p.drawPolygon(p2);
    p.drawLine(p2.at(1), p2.at(30));
    p.drawLine(p2.at(32), p2.at(71));
}

void PlayerControlPreview::DrawProJoystick(QPainter& p, const QPointF center, const QPointF offset,
                                           float offset_scalar,
                                           const Common::Input::ButtonStatus& pressed) {
    const float radius1 = 24.0f;
    const float radius2 = 17.0f;

    const QPointF offset_center = center + offset * offset_scalar;

    const auto amplitude = static_cast<float>(
        1.0 - std::sqrt((offset.x() * offset.x()) + (offset.y() * offset.y())) * 0.1f);

    const float rotation =
        ((offset.x() == 0.f) ? std::atan(1.f) * 2.f : std::atan(offset.y() / offset.x())) *
        (180.f / (std::atan(1.f) * 4.f));

    // 1. Draw metallic stick neck/shaft when tilted
    if (offset.x() != 0.0f || offset.y() != 0.0f) {
        p.save();
        p.setPen(QPen(QColor(30, 32, 36), 1.0));
        QLinearGradient shaft_grad(center, offset_center);
        shaft_grad.setColorAt(0.0, QColor(70, 75, 85));
        shaft_grad.setColorAt(0.5, QColor(145, 150, 165));
        shaft_grad.setColorAt(1.0, QColor(60, 64, 72));
        p.setBrush(shaft_grad);
        const QPointF normal(-offset.y(), offset.x());
        const float len = std::max(0.001f, std::sqrt(float(normal.x() * normal.x() + normal.y() * normal.y())));
        const QPointF unit_norm = normal / len * 6.0f;
        const QPointF shaft_poly[] = {
            center - unit_norm,
            center + unit_norm,
            offset_center + unit_norm * 0.8f,
            offset_center - unit_norm * 0.8f
        };
        p.drawPolygon(shaft_poly, 4);
        p.restore();
    }

    p.save();
    p.translate(offset_center);
    p.rotate(rotation);

    // 2. Outer rubber thumb-pad with spherical lighting
    p.setPen(QPen(colors.outline, 1.2));
    if (pressed.value) {
        p.setBrush(colors.highlight);
    } else {
        QRadialGradient pad_grad(QPointF(-radius1 * 0.3f, -radius1 * 0.3f), radius1 * 1.3f);
        pad_grad.setColorAt(0.0, QColor(75, 80, 90));
        pad_grad.setColorAt(0.5, colors.button);
        pad_grad.setColorAt(1.0, colors.button.darker(130));
        p.setBrush(pad_grad);
    }
    p.drawEllipse(QPointF(0, 0), radius1 * amplitude, radius1);

    // 3. 4 Authentic Tactile Cardinal Notches on the outer rim of thumb-pad
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(18, 20, 24, 180));
    constexpr std::array<float, 4> notch_angles = {0.0f, float(PI_CONST * 0.5f), float(PI_CONST), float(PI_CONST * 1.5f)};
    for (float notch_rad : notch_angles) {
        const QPointF notch_pos(std::cos(notch_rad) * (radius1 - 2.0f) * amplitude,
                                std::sin(notch_rad) * (radius1 - 2.0f));
        p.drawEllipse(notch_pos, 1.6f * amplitude, 1.6f);
    }

    // 4. Inner concave thumb bowl
    const float inner_offset =
        (radius1 - radius2) * 0.4f * ((offset.x() == 0 && offset.y() < 0) ? -1.0f : 1.0f);
    const float offset_factor = (1.0f - amplitude) / 0.1f;
    const QPointF bowl_pos = QPointF((offset.x() < 0) ? -inner_offset : inner_offset, 0) * offset_factor;

    if (pressed.value) {
        p.setBrush(colors.highlight2);
    } else {
        QRadialGradient bowl_grad(bowl_pos, radius2);
        bowl_grad.setColorAt(0.0, colors.button2.darker(120));
        bowl_grad.setColorAt(0.7, colors.button2);
        bowl_grad.setColorAt(1.0, QColor(60, 65, 75));
        p.setBrush(bowl_grad);
    }
    p.setPen(QPen(QColor(15, 17, 20), 0.8));
    p.drawEllipse(bowl_pos, radius2 * amplitude, radius2);

    p.restore();
}

void PlayerControlPreview::DrawGCJoystick(QPainter& p, const QPointF center,
                                          const Common::Input::ButtonStatus& pressed) {
    // Outer circle
    p.setPen(colors.outline);
    p.setBrush(pressed.value ? colors.highlight : colors.button);
    DrawCircle(p, center, 26.0f);

    // Inner circle
    p.setBrush(pressed.value ? colors.highlight2 : colors.button2);
    DrawCircle(p, center, 19.0f);
    p.setBrush(colors.transparent);
    DrawCircle(p, center, 13.5f);
    DrawCircle(p, center, 7.5f);
}

void PlayerControlPreview::DrawRawJoystick(QPainter& p, QPointF center_left, QPointF center_right) {
    using namespace Settings::NativeAnalog;
    if (center_right != QPointF(0, 0)) {
        DrawJoystickProperties(p, center_right, stick_values[RStick].x.properties);
        p.setPen(colors.indicator);
        p.setBrush(colors.indicator);
        DrawJoystickDot(p, center_right, stick_values[RStick], true);
        p.setPen(colors.indicator2);
        p.setBrush(colors.indicator2);
        DrawJoystickDot(p, center_right, stick_values[RStick], false);
    }

    if (center_left != QPointF(0, 0)) {
        DrawJoystickProperties(p, center_left, stick_values[LStick].x.properties);
        p.setPen(colors.indicator);
        p.setBrush(colors.indicator);
        DrawJoystickDot(p, center_left, stick_values[LStick], true);
        p.setPen(colors.indicator2);
        p.setBrush(colors.indicator2);
        DrawJoystickDot(p, center_left, stick_values[LStick], false);
    }
}

void PlayerControlPreview::DrawJoystickProperties(
    QPainter& p, const QPointF center, const Common::Input::AnalogProperties& properties) {
    constexpr float size = 45.0f;
    const float range = size * properties.range;
    const float deadzone = size * properties.deadzone;

    // Max range zone circle
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    QPen pen = p.pen();
    pen.setStyle(Qt::DotLine);
    p.setPen(pen);
    DrawCircle(p, center, range);

    // Deadzone circle
    pen.setColor(colors.deadzone);
    p.setPen(pen);
    DrawCircle(p, center, deadzone);
}

void PlayerControlPreview::DrawJoystickDot(QPainter& p, const QPointF center,
                                           const Common::Input::StickStatus& stick, bool raw) {
    constexpr float size = 45.0f;
    const float range = size * stick.x.properties.range;

    if (raw) {
        const QPointF value = QPointF(stick.x.raw_value, stick.y.raw_value) * size;
        DrawCircle(p, center + value, 2);
        return;
    }

    const QPointF value = QPointF(stick.x.value, stick.y.value) * range;
    DrawCircle(p, center + value, 2);
}

void PlayerControlPreview::DrawRoundButton(QPainter& p, QPointF center,
                                           const Common::Input::ButtonStatus& pressed, float width,
                                           float height, Direction direction, float radius) {
    if (pressed.value) {
        switch (direction) {
        case Direction::Left:
            center.setX(center.x() - 1);
            break;
        case Direction::Right:
            center.setX(center.x() + 1);
            break;
        case Direction::Down:
            center.setY(center.y() + 1);
            break;
        case Direction::Up:
            center.setY(center.y() - 1);
            break;
        case Direction::None:
            break;
        }
    }
    QRectF rect = {center.x() - width, center.y() - height, width * 2.0f, height * 2.0f};
    p.setBrush(GetButtonColor(button_color, pressed.value, pressed.turbo));
    p.drawRoundedRect(rect, radius, radius);
}
void PlayerControlPreview::DrawMinusButton(QPainter& p, const QPointF center,
                                           const Common::Input::ButtonStatus& pressed,
                                           int button_size) {
    p.setPen(colors.outline);
    p.setBrush(GetButtonColor(colors.button, pressed.value, pressed.turbo));
    DrawRectangle(p, center, button_size, button_size / 3.0f);
}
void PlayerControlPreview::DrawPlusButton(QPainter& p, const QPointF center,
                                          const Common::Input::ButtonStatus& pressed,
                                          int button_size) {
    // Draw outer line
    p.setPen(colors.outline);
    p.setBrush(GetButtonColor(colors.button, pressed.value, pressed.turbo));
    DrawRectangle(p, center, button_size, button_size / 3.0f);
    DrawRectangle(p, center, button_size / 3.0f, button_size);

    // Scale down size
    button_size *= 0.88f;

    // Draw inner color
    p.setPen(colors.transparent);
    DrawRectangle(p, center, button_size, button_size / 3.0f);
    DrawRectangle(p, center, button_size / 3.0f, button_size);
}

void PlayerControlPreview::DrawGCButtonX(QPainter& p, const QPointF center,
                                         const Common::Input::ButtonStatus& pressed) {
    std::array<QPointF, gc_button_x.size() / 2> button_x;

    for (std::size_t point = 0; point < gc_button_x.size() / 2; ++point) {
        button_x[point] = center + QPointF(gc_button_x[point * 2], gc_button_x[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(GetButtonColor(colors.button, pressed.value, pressed.turbo));
    DrawPolygon(p, button_x);
}

void PlayerControlPreview::DrawGCButtonY(QPainter& p, const QPointF center,
                                         const Common::Input::ButtonStatus& pressed) {
    std::array<QPointF, gc_button_y.size() / 2> button_x;

    for (std::size_t point = 0; point < gc_button_y.size() / 2; ++point) {
        button_x[point] = center + QPointF(gc_button_y[point * 2], gc_button_y[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(GetButtonColor(colors.button, pressed.value, pressed.turbo));
    DrawPolygon(p, button_x);
}

void PlayerControlPreview::DrawGCButtonZ(QPainter& p, const QPointF center,
                                         const Common::Input::ButtonStatus& pressed) {
    std::array<QPointF, gc_button_z.size() / 2> button_x;

    for (std::size_t point = 0; point < gc_button_z.size() / 2; ++point) {
        button_x[point] = center + QPointF(gc_button_z[point * 2],
                                           gc_button_z[point * 2 + 1] + (pressed.value ? 1 : 0));
    }

    p.setPen(colors.outline);
    p.setBrush(GetButtonColor(colors.button2, pressed.value, pressed.turbo));
    DrawPolygon(p, button_x);
}

void PlayerControlPreview::DrawCircleButton(QPainter& p, const QPointF center,
                                            const Common::Input::ButtonStatus& pressed,
                                            float button_size) {

    p.setBrush(GetButtonColor(button_color, pressed.value, pressed.turbo));
    p.drawEllipse(center, button_size, button_size);
}

void PlayerControlPreview::DrawArrowButtonOutline(QPainter& p, const QPointF center, float size) {
    const std::size_t arrow_points = up_arrow_button.size() / 2;
    std::array<QPointF, (arrow_points - 1) * 4> arrow_button_outline;

    for (std::size_t point = 0; point < arrow_points - 1; ++point) {
        const float up_arrow_x = up_arrow_button[point * 2 + 0];
        const float up_arrow_y = up_arrow_button[point * 2 + 1];

        arrow_button_outline[point] = center + QPointF(up_arrow_x * size, up_arrow_y * size);
        arrow_button_outline[(arrow_points - 1) * 2 - point - 1] =
            center + QPointF(up_arrow_y * size, up_arrow_x * size);
        arrow_button_outline[(arrow_points - 1) * 2 + point] =
            center + QPointF(-up_arrow_x * size, -up_arrow_y * size);
        arrow_button_outline[(arrow_points - 1) * 4 - point - 1] =
            center + QPointF(-up_arrow_y * size, -up_arrow_x * size);
    }
    // Draw arrow button outline
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    DrawPolygon(p, arrow_button_outline);
}

void PlayerControlPreview::DrawArrowButton(QPainter& p, const QPointF center,
                                           const Direction direction,
                                           const Common::Input::ButtonStatus& pressed, float size) {
    std::array<QPointF, up_arrow_button.size() / 2> arrow_button;
    QPoint offset;

    for (std::size_t point = 0; point < up_arrow_button.size() / 2; ++point) {
        const float up_arrow_x = up_arrow_button[point * 2 + 0];
        const float up_arrow_y = up_arrow_button[point * 2 + 1];

        switch (direction) {
        case Direction::Up:
            arrow_button[point] = center + QPointF(up_arrow_x * size, up_arrow_y * size);
            break;
        case Direction::Right:
            arrow_button[point] = center + QPointF(-up_arrow_y * size, up_arrow_x * size);
            break;
        case Direction::Down:
            arrow_button[point] = center + QPointF(up_arrow_x * size, -up_arrow_y * size);
            break;
        case Direction::Left:
            // Compiler doesn't optimize this correctly check why
            arrow_button[point] = center + QPointF(up_arrow_y * size, up_arrow_x * size);
            break;
        case Direction::None:
            break;
        }
    }

    // Draw arrow button
    p.setPen(pressed.value ? colors.highlight : colors.button);
    p.setBrush(GetButtonColor(colors.button, pressed.value, pressed.turbo));
    DrawPolygon(p, arrow_button);

    switch (direction) {
    case Direction::Up:
        offset = QPoint(0, -20 * size);
        break;
    case Direction::Right:
        offset = QPoint(20 * size, 0);
        break;
    case Direction::Down:
        offset = QPoint(0, 20 * size);
        break;
    case Direction::Left:
        offset = QPoint(-20 * size, 0);
        break;
    case Direction::None:
        offset = QPoint(0, 0);
        break;
    }

    // Draw arrow icon
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, center + offset, direction, size);
}

void PlayerControlPreview::DrawTriggerButton(QPainter& p, const QPointF center,
                                             const Direction direction,
                                             const Common::Input::ButtonStatus& pressed) {
    std::array<QPointF, trigger_button.size() / 2> qtrigger_button;

    for (std::size_t point = 0; point < trigger_button.size() / 2; ++point) {
        const float trigger_button_x = trigger_button[point * 2 + 0];
        const float trigger_button_y = trigger_button[point * 2 + 1];

        switch (direction) {
        case Direction::Left:
            qtrigger_button[point] = center + QPointF(-trigger_button_x, trigger_button_y);
            break;
        case Direction::Right:
            qtrigger_button[point] = center + QPointF(trigger_button_x, trigger_button_y);
            break;
        case Direction::Up:
        case Direction::Down:
        case Direction::None:
            break;
        }
    }

    // Draw arrow button
    p.setPen(colors.outline);
    p.setBrush(GetButtonColor(colors.button, pressed.value, pressed.turbo));
    DrawPolygon(p, qtrigger_button);
}

QColor PlayerControlPreview::GetButtonColor(QColor default_color, bool is_pressed, bool turbo) {
    if (is_pressed && turbo) {
        return colors.button_turbo;
    }
    if (is_pressed) {
        return colors.highlight;
    }
    return default_color;
}

void PlayerControlPreview::DrawBattery(QPainter& p, QPointF center,
                                       Common::Input::BatteryLevel battery) {
    if (battery == Common::Input::BatteryLevel::None) {
        return;
    }
    // Draw outline
    p.setPen(QPen(colors.button, 5));
    p.setBrush(colors.transparent);
    p.drawRoundedRect(center.x(), center.y(), 34, 16, 2, 2);

    p.setPen(QPen(colors.button, 3));
    p.drawRect(center.x() + 35, center.y() + 4.5f, 4, 7);

    // Draw Battery shape
    p.setPen(QPen(colors.indicator2, 3));
    p.setBrush(colors.transparent);
    p.drawRoundedRect(center.x(), center.y(), 34, 16, 2, 2);

    p.setPen(QPen(colors.indicator2, 1));
    p.setBrush(colors.indicator2);
    p.drawRect(center.x() + 35, center.y() + 4.5f, 4, 7);
    switch (battery) {
    case Common::Input::BatteryLevel::Charging:
        p.drawRect(center.x(), center.y(), 34, 16);
        p.setPen(colors.slider);
        p.setBrush(colors.charging);
        DrawSymbol(p, center + QPointF(17.0f, 8.0f), Symbol::Charging, 2.1f);
        break;
    case Common::Input::BatteryLevel::Full:
        p.drawRect(center.x(), center.y(), 34, 16);
        break;
    case Common::Input::BatteryLevel::Medium:
        p.drawRect(center.x(), center.y(), 25, 16);
        break;
    case Common::Input::BatteryLevel::Low:
        p.drawRect(center.x(), center.y(), 17, 16);
        break;
    case Common::Input::BatteryLevel::Critical:
        p.drawRect(center.x(), center.y(), 6, 16);
        break;
    case Common::Input::BatteryLevel::Empty:
        p.drawRect(center.x(), center.y(), 3, 16);
        break;
    default:
        break;
    }
}

void PlayerControlPreview::DrawSymbol(QPainter& p, const QPointF center, Symbol symbol,
                                      float icon_size) {
    std::array<QPointF, house.size() / 2> house_icon;
    std::array<QPointF, symbol_a.size() / 2> a_icon;
    std::array<QPointF, symbol_b.size() / 2> b_icon;
    std::array<QPointF, symbol_x.size() / 2> x_icon;
    std::array<QPointF, symbol_y.size() / 2> y_icon;
    std::array<QPointF, symbol_l.size() / 2> l_icon;
    std::array<QPointF, symbol_r.size() / 2> r_icon;
    std::array<QPointF, symbol_c.size() / 2> c_icon;
    std::array<QPointF, symbol_zl.size() / 2> zl_icon;
    std::array<QPointF, symbol_sl.size() / 2> sl_icon;
    std::array<QPointF, symbol_zr.size() / 2> zr_icon;
    std::array<QPointF, symbol_sr.size() / 2> sr_icon;
    std::array<QPointF, symbol_charging.size() / 2> charging_icon;
    switch (symbol) {
    case Symbol::House:
        for (std::size_t point = 0; point < house.size() / 2; ++point) {
            house_icon[point] = center + QPointF(house[point * 2] * icon_size,
                                                 (house[point * 2 + 1] - 0.025f) * icon_size);
        }
        p.drawPolygon(house_icon.data(), static_cast<int>(house_icon.size()));
        break;
    case Symbol::A:
        for (std::size_t point = 0; point < symbol_a.size() / 2; ++point) {
            a_icon[point] = center + QPointF(symbol_a[point * 2] * icon_size,
                                             symbol_a[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(a_icon.data(), static_cast<int>(a_icon.size()));
        break;
    case Symbol::B:
        for (std::size_t point = 0; point < symbol_b.size() / 2; ++point) {
            b_icon[point] = center + QPointF(symbol_b[point * 2] * icon_size,
                                             symbol_b[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(b_icon.data(), static_cast<int>(b_icon.size()));
        break;
    case Symbol::X:
        for (std::size_t point = 0; point < symbol_x.size() / 2; ++point) {
            x_icon[point] = center + QPointF(symbol_x[point * 2] * icon_size,
                                             symbol_x[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(x_icon.data(), static_cast<int>(x_icon.size()));
        break;
    case Symbol::Y:
        for (std::size_t point = 0; point < symbol_y.size() / 2; ++point) {
            y_icon[point] = center + QPointF(symbol_y[point * 2] * icon_size,
                                             (symbol_y[point * 2 + 1] - 1.0f) * icon_size);
        }
        p.drawPolygon(y_icon.data(), static_cast<int>(y_icon.size()));
        break;
    case Symbol::L:
        for (std::size_t point = 0; point < symbol_l.size() / 2; ++point) {
            l_icon[point] = center + QPointF(symbol_l[point * 2] * icon_size,
                                             (symbol_l[point * 2 + 1] - 1.0f) * icon_size);
        }
        p.drawPolygon(l_icon.data(), static_cast<int>(l_icon.size()));
        break;
    case Symbol::R:
        for (std::size_t point = 0; point < symbol_r.size() / 2; ++point) {
            r_icon[point] = center + QPointF(symbol_r[point * 2] * icon_size,
                                             (symbol_r[point * 2 + 1] - 1.0f) * icon_size);
        }
        p.drawPolygon(r_icon.data(), static_cast<int>(r_icon.size()));
        break;
    case Symbol::C:
        for (std::size_t point = 0; point < symbol_c.size() / 2; ++point) {
            c_icon[point] = center + QPointF(symbol_c[point * 2] * icon_size,
                                             (symbol_c[point * 2 + 1] - 1.0f) * icon_size);
        }
        p.drawPolygon(c_icon.data(), static_cast<int>(c_icon.size()));
        break;
    case Symbol::ZL:
        for (std::size_t point = 0; point < symbol_zl.size() / 2; ++point) {
            zl_icon[point] = center + QPointF(symbol_zl[point * 2] * icon_size,
                                              symbol_zl[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(zl_icon.data(), static_cast<int>(zl_icon.size()));
        break;
    case Symbol::SL:
        for (std::size_t point = 0; point < symbol_sl.size() / 2; ++point) {
            sl_icon[point] = center + QPointF(symbol_sl[point * 2] * icon_size,
                                              symbol_sl[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(sl_icon.data(), static_cast<int>(sl_icon.size()));
        break;
    case Symbol::ZR:
        for (std::size_t point = 0; point < symbol_zr.size() / 2; ++point) {
            zr_icon[point] = center + QPointF(symbol_zr[point * 2] * icon_size,
                                              symbol_zr[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(zr_icon.data(), static_cast<int>(zr_icon.size()));
        break;
    case Symbol::SR:
        for (std::size_t point = 0; point < symbol_sr.size() / 2; ++point) {
            sr_icon[point] = center + QPointF(symbol_sr[point * 2] * icon_size,
                                              symbol_sr[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(sr_icon.data(), static_cast<int>(sr_icon.size()));
        break;
    case Symbol::Charging:
        for (std::size_t point = 0; point < symbol_charging.size() / 2; ++point) {
            charging_icon[point] = center + QPointF(symbol_charging[point * 2] * icon_size,
                                                    symbol_charging[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(charging_icon.data(), static_cast<int>(charging_icon.size()));
        break;
    }
}

void PlayerControlPreview::DrawArrow(QPainter& p, const QPointF center, const Direction direction,
                                     float size) {

    std::array<QPointF, up_arrow_symbol.size() / 2> arrow_symbol;

    for (std::size_t point = 0; point < up_arrow_symbol.size() / 2; ++point) {
        const float up_arrow_x = up_arrow_symbol[point * 2 + 0];
        const float up_arrow_y = up_arrow_symbol[point * 2 + 1];

        switch (direction) {
        case Direction::Up:
            arrow_symbol[point] = center + QPointF(up_arrow_x * size, up_arrow_y * size);
            break;
        case Direction::Left:
            arrow_symbol[point] = center + QPointF(up_arrow_y * size, up_arrow_x * size);
            break;
        case Direction::Right:
            arrow_symbol[point] = center + QPointF(-up_arrow_y * size, up_arrow_x * size);
            break;
        case Direction::Down:
            arrow_symbol[point] = center + QPointF(up_arrow_x * size, -up_arrow_y * size);
            break;
        case Direction::None:
            break;
        }
    }

    DrawPolygon(p, arrow_symbol);
}

// Draw motion functions
void PlayerControlPreview::Draw3dCube(QPainter& p, QPointF center, const Common::Vec3f& euler,
                                      float size) {
    std::array<Common::Vec3f, 8> cube{
        Common::Vec3f{-0.7f, -1, -0.5f},
        {-0.7f, 1, -0.5f},
        {0.7f, 1, -0.5f},
        {0.7f, -1, -0.5f},
        {-0.7f, -1, 0.5f},
        {-0.7f, 1, 0.5f},
        {0.7f, 1, 0.5f},
        {0.7f, -1, 0.5f},
    };

    for (Common::Vec3f& point : cube) {
        point.RotateFromOrigin(euler.x, euler.y, euler.z);
        point *= size;
    }

    const std::array<QPointF, 4> front_face{
        center + QPointF{cube[0].x, cube[0].y},
        center + QPointF{cube[1].x, cube[1].y},
        center + QPointF{cube[2].x, cube[2].y},
        center + QPointF{cube[3].x, cube[3].y},
    };
    const std::array<QPointF, 4> back_face{
        center + QPointF{cube[4].x, cube[4].y},
        center + QPointF{cube[5].x, cube[5].y},
        center + QPointF{cube[6].x, cube[6].y},
        center + QPointF{cube[7].x, cube[7].y},
    };

    DrawPolygon(p, front_face);
    DrawPolygon(p, back_face);
    p.drawLine(center + QPointF{cube[0].x, cube[0].y}, center + QPointF{cube[4].x, cube[4].y});
    p.drawLine(center + QPointF{cube[1].x, cube[1].y}, center + QPointF{cube[5].x, cube[5].y});
    p.drawLine(center + QPointF{cube[2].x, cube[2].y}, center + QPointF{cube[6].x, cube[6].y});
    p.drawLine(center + QPointF{cube[3].x, cube[3].y}, center + QPointF{cube[7].x, cube[7].y});
}

template <size_t N>
void PlayerControlPreview::DrawPolygon(QPainter& p, const std::array<QPointF, N>& polygon) {
    p.drawPolygon(polygon.data(), static_cast<int>(polygon.size()));
}

void PlayerControlPreview::DrawCircle(QPainter& p, const QPointF center, float size) {
    p.drawEllipse(center, size, size);
}

void PlayerControlPreview::DrawRectangle(QPainter& p, const QPointF center, float width,
                                         float height) {
    const QRectF rect = QRectF(center.x() - (width / 2), center.y() - (height / 2), width, height);
    p.drawRect(rect);
}
void PlayerControlPreview::DrawRoundRectangle(QPainter& p, const QPointF center, float width,
                                              float height, float round) {
    const QRectF rect = QRectF(center.x() - (width / 2), center.y() - (height / 2), width, height);
    p.drawRoundedRect(rect, round, round);
}

void PlayerControlPreview::DrawText(QPainter& p, const QPointF center, float text_size,
                                    const QString& text) {
    SetTextFont(p, text_size);
    const QFontMetrics fm(p.font());
    const QPointF offset = {fm.horizontalAdvance(text) / 2.0f, -text_size / 2.0f};
    p.drawText(center - offset, text);
}

void PlayerControlPreview::SetTextFont(QPainter& p, float text_size, const QString& font_family) {
    QFont font = p.font();
    font.setPointSizeF(text_size);
    font.setFamily(font_family);
    p.setFont(font);
}
