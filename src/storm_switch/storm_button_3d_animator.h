// SPDX-FileCopyrightText: 2026 STORM SOFT
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QHash>
#include <QMouseEvent>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>
#include <QVariantAnimation>
#include <QEasingCurve>

/**
 * @brief StormButton3DAnimator
 *
 * Lightweight, zero-overhead global Qt event filter that enhances QPushButton
 * and QToolButton controls with tactile press depression, smooth hover micro-animations,
 * and 3D faceted depth across the entire desktop interface (Law 3 & STORM Standard).
 *
 * Designed to guarantee zero crashes on widget destruction via QPointer tracking,
 * and zero high-DPI / layout artifacts (no QGraphicsDropShadowEffect).
 */
class StormButton3DAnimator : public QObject {
public:
    explicit StormButton3DAnimator(QObject* parent = nullptr) : QObject(parent) {}

    ~StormButton3DAnimator() override {
        // Stop and clean up any ongoing animations
        for (auto anim : m_active_anims) {
            if (anim) {
                anim->stop();
                anim->deleteLater();
            }
        }
        m_active_anims.clear();
    }

    /**
     * @brief Singleton accessor
     */
    static StormButton3DAnimator* Instance() {
        static StormButton3DAnimator* s_instance = new StormButton3DAnimator(qApp);
        return s_instance;
    }

    /**
     * @brief Install the global event filter on QApplication
     */
    static void Install(QApplication* app = nullptr) {
        if (!app) {
            app = qobject_cast<QApplication*>(QCoreApplication::instance());
        }
        if (app) {
            app->installEventFilter(Instance());
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (!watched || !watched->isWidgetType()) {
            return false;
        }

        auto* btn = qobject_cast<QAbstractButton*>(watched);
        if (!btn) {
            return false;
        }

        // Preserve Law 11: Do not interfere with custom titlebar caption buttons (Min/Max/Close)
        const QString name = btn->objectName();
        if (name.contains(QLatin1String("Caption"), Qt::CaseInsensitive) ||
            name.contains(QLatin1String("TitleBar"), Qt::CaseInsensitive) ||
            name == QLatin1String("CloseButton") ||
            name == QLatin1String("MinimizeButton") ||
            name == QLatin1String("MaximizeButton") ||
            name.startsWith(QLatin1String("qt_tabwidget_closebutton"))) {
            return false;
        }

        const QEvent::Type type = event->type();
        switch (type) {
        case QEvent::Enter: {
            btn->setProperty("stormHover", true);
            AnimateHoverGlow(btn, true);
            break;
        }
        case QEvent::Leave: {
            btn->setProperty("stormHover", false);
            AnimateHoverGlow(btn, false);
            break;
        }
        case QEvent::MouseButtonPress: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                btn->setProperty("stormPressed", true);
                btn->update();
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                btn->setProperty("stormPressed", false);
                btn->update();
            }
            break;
        }
        default:
            break;
        }

        // Always return false so standard Qt button event dispatching & signal emission continue normally
        return false;
    }

private:
    void AnimateHoverGlow(QAbstractButton* btn, bool hover) {
        if (!btn) return;

        QPointer<QAbstractButton> safe_btn(btn);
        const quintptr btn_key = reinterpret_cast<quintptr>(btn);

        // Cancel existing animation on this button if running
        if (m_active_anims.contains(btn_key)) {
            QVariantAnimation* old_anim = m_active_anims.value(btn_key).data();
            if (old_anim) {
                old_anim->stop();
                old_anim->deleteLater();
            }
            m_active_anims.remove(btn_key);
        }

        const float start_val = btn->property("stormGlow").isValid()
                                    ? btn->property("stormGlow").toFloat()
                                    : (hover ? 0.0f : 1.0f);
        const float end_val = hover ? 1.0f : 0.0f;

        auto* anim = new QVariantAnimation(this);
        anim->setDuration(120); // Smooth 120ms micro-transition
        anim->setStartValue(start_val);
        anim->setEndValue(end_val);
        anim->setEasingCurve(QEasingCurve::OutCubic);

        connect(anim, &QVariantAnimation::valueChanged, this, [safe_btn](const QVariant& val) {
            if (safe_btn) {
                safe_btn->setProperty("stormGlow", val);
                safe_btn->update();
            }
        });

        connect(anim, &QVariantAnimation::finished, this, [this, btn_key, anim]() {
            if (m_active_anims.value(btn_key).data() == anim) {
                m_active_anims.remove(btn_key);
            }
            anim->deleteLater();
        });

        // Ensure zero crashes on button destruction
        connect(btn, &QObject::destroyed, this, [this, btn_key]() {
            if (m_active_anims.contains(btn_key)) {
                QVariantAnimation* a = m_active_anims.value(btn_key).data();
                if (a) {
                    a->stop();
                    a->deleteLater();
                }
                m_active_anims.remove(btn_key);
            }
        });

        m_active_anims.insert(btn_key, anim);
        anim->start();
    }

    QHash<quintptr, QPointer<QVariantAnimation>> m_active_anims;
};
