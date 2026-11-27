// SPDX-FileCopyrightText: Copyright 2026 STORM SWITCH Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "update_dialog.h"

#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMap>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QOperatingSystemVersion>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "common/logging.h"
#include "common/scm_rev.h"
#include "storm_switch/common/storm_design_engine.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

namespace {

constexpr const char* kSettingsOrg = "STORM SOFT";
constexpr const char* kSettingsApp = "STORM SWITCH";
constexpr const char* kSkippedVerKey = "updater/skipped_version";
constexpr const char* kFallbackReleaseUrl = "https://github.com/ReiKatari/STORM_SWITCH/releases/latest";

enum class StormLangId {
    Ru = 0, En, De, Es, Fr, Id, It, Ja, Ko, Pt, Zh, Hi, Count
};

QString NormalizeLocaleCode(const QString& raw) {
    QString clean = raw.trimmed().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (clean == QStringLiteral("in") || clean.startsWith(QStringLiteral("in-"))) {
        clean = QStringLiteral("id") + clean.mid(2);
    } else if (clean == QStringLiteral("iw") || clean.startsWith(QStringLiteral("iw-"))) {
        clean = QStringLiteral("he") + clean.mid(2);
    }
    return clean;
}

StormLangId ResolveActiveLangId() {
    QString raw = QString::fromStdString(UISettings::values.language.GetValue());
    if (raw.trimmed().isEmpty()) {
        raw = QLocale::system().bcp47Name();
    }
    const QString bcp = NormalizeLocaleCode(raw);
    const QString base = bcp.section(QLatin1Char('-'), 0, 0);
    if (base == QStringLiteral("ru") || base == QStringLiteral("be")) return StormLangId::Ru;
    if (base == QStringLiteral("de")) return StormLangId::De;
    if (base == QStringLiteral("es")) return StormLangId::Es;
    if (base == QStringLiteral("fr")) return StormLangId::Fr;
    if (base == QStringLiteral("id")) return StormLangId::Id;
    if (base == QStringLiteral("it")) return StormLangId::It;
    if (base == QStringLiteral("ja")) return StormLangId::Ja;
    if (base == QStringLiteral("ko")) return StormLangId::Ko;
    if (base == QStringLiteral("pt")) return StormLangId::Pt;
    if (base == QStringLiteral("zh")) return StormLangId::Zh;
    if (base == QStringLiteral("hi")) return StormLangId::Hi;
    return StormLangId::En;
}

QString Tr12(const char* ru, const char* en, const char* de, const char* es,
             const char* fr, const char* id, const char* it, const char* ja,
             const char* ko, const char* pt, const char* zh, const char* hi) {
    const char* table[static_cast<int>(StormLangId::Count)] = {
        ru, en, de, es, fr, id, it, ja, ko, pt, zh, hi
    };
    const int idx = static_cast<int>(ResolveActiveLangId());
    const char* chosen = (idx >= 0 && idx < static_cast<int>(StormLangId::Count)) ? table[idx] : en;
    return QString::fromUtf8((chosen && chosen[0] != '\0') ? chosen : en);
}

QString MatchReleaseLanguageHeading(const QString& raw_heading) {
    QString trimmed = raw_heading.trimmed();
    while (trimmed.startsWith(QLatin1Char('#'))) {
        trimmed.remove(0, 1);
    }
    trimmed = trimmed.trimmed();
    if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
        trimmed = trimmed.mid(1, trimmed.size() - 2).trimmed();
    }
    trimmed = trimmed.toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (trimmed == QStringLiteral("ru") || trimmed.contains(QStringLiteral("русский")) ||
        trimmed.contains(QStringLiteral("russian"))) {
        return QStringLiteral("ru");
    }
    if (trimmed == QStringLiteral("en") || trimmed.contains(QStringLiteral("english")) ||
        trimmed.contains(QStringLiteral("английск"))) {
        return QStringLiteral("en");
    }
    if (trimmed == QStringLiteral("de") || trimmed.contains(QStringLiteral("deutsch")) ||
        trimmed.contains(QStringLiteral("german"))) {
        return QStringLiteral("de");
    }
    if (trimmed == QStringLiteral("es") || trimmed.contains(QStringLiteral("español")) ||
        trimmed.contains(QStringLiteral("espanol")) ||
        trimmed.contains(QStringLiteral("spanish"))) {
        return QStringLiteral("es");
    }
    if (trimmed == QStringLiteral("fr") || trimmed.contains(QStringLiteral("français")) ||
        trimmed.contains(QStringLiteral("francais")) ||
        trimmed.contains(QStringLiteral("french"))) {
        return QStringLiteral("fr");
    }
    if (trimmed == QStringLiteral("id") ||
        trimmed.contains(QStringLiteral("bahasa indonesia")) ||
        trimmed.contains(QStringLiteral("indonesian"))) {
        return QStringLiteral("id");
    }
    if (trimmed == QStringLiteral("it") || trimmed.contains(QStringLiteral("italiano")) ||
        trimmed.contains(QStringLiteral("italian"))) {
        return QStringLiteral("it");
    }
    if (trimmed == QStringLiteral("ja") || trimmed.contains(QStringLiteral("日本語")) ||
        trimmed.contains(QStringLiteral("japanese"))) {
        return QStringLiteral("ja");
    }
    if (trimmed == QStringLiteral("ko") || trimmed.contains(QStringLiteral("한국어")) ||
        trimmed.contains(QStringLiteral("korean"))) {
        return QStringLiteral("ko");
    }
    if (trimmed == QStringLiteral("hi") || trimmed.contains(QStringLiteral("हिन्दी")) ||
        trimmed.contains(QStringLiteral("hindi"))) {
        return QStringLiteral("hi");
    }
    if (trimmed == QStringLiteral("pt-br") || trimmed.contains(QStringLiteral("português")) ||
        trimmed.contains(QStringLiteral("portugues")) ||
        trimmed.contains(QStringLiteral("portuguese")) || trimmed == QStringLiteral("pt")) {
        return trimmed.contains(QStringLiteral("br")) ? QStringLiteral("pt-br")
                                                     : QStringLiteral("pt");
    }
    if (trimmed == QStringLiteral("zh-hans") || trimmed == QStringLiteral("zh-cn") ||
        trimmed == QStringLiteral("zh") || trimmed.contains(QStringLiteral("简体中文")) ||
        trimmed.contains(QStringLiteral("中文")) || trimmed.contains(QStringLiteral("chinese"))) {
        return (trimmed.contains(QStringLiteral("hans")) ||
                trimmed.contains(QStringLiteral("cn")) || trimmed.contains(QStringLiteral("简体")))
                   ? QStringLiteral("zh-hans")
                   : QStringLiteral("zh");
    }
    return {};
}

QString SelectLocalizedReleaseNotes(const QString& raw, const QString& ui_language,
                                    const QLocale& locale) {
    QString normalized = raw;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const QStringList lines = normalized.split(QLatin1Char('\n'));

    static const QRegularExpression kCommentTagRe(
        QStringLiteral("<!--\\s*lang\\s*:\\s*([a-zA-Z_-]+)\\s*-->"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression kCommentEndRe(QStringLiteral("<!--\\s*/lang\\s*-->"),
                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression kHtmlCommentLineRe(QStringLiteral("^\\s*<!--.*-->\\s*$"));

    QMap<QString, QStringList> marker_sections;
    QStringList marker_order;
    QString active_marker;
    for (const QString& line : lines) {
        const auto m = kCommentTagRe.match(line);
        if (m.hasMatch()) {
            active_marker = NormalizeLocaleCode(m.captured(1));
            if (!marker_sections.contains(active_marker)) {
                marker_order.append(active_marker);
            }
            QString stripped = line;
            stripped.remove(kCommentTagRe);
            stripped.remove(kCommentEndRe);
            stripped = stripped.trimmed();
            if (!stripped.isEmpty()) {
                marker_sections[active_marker].append(stripped);
            }
            if (kCommentEndRe.match(line).hasMatch()) {
                active_marker.clear();
            }
            continue;
        }
        if (kCommentEndRe.match(line).hasMatch()) {
            active_marker.clear();
            continue;
        }
        if (!active_marker.isEmpty() && !kHtmlCommentLineRe.match(line).hasMatch()) {
            marker_sections[active_marker].append(line);
        }
    }

    QMap<QString, QStringList> heading_sections;
    QStringList heading_order;
    if (marker_sections.size() < 2) {
        QString active_heading;
        for (const QString& line : lines) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith(QStringLiteral("## ")) ||
                trimmed.startsWith(QStringLiteral("### "))) {
                const QString detected = MatchReleaseLanguageHeading(trimmed);
                if (!detected.isEmpty()) {
                    active_heading = detected;
                    if (!heading_sections.contains(detected)) {
                        heading_order.append(detected);
                    }
                    continue;
                }
            }
            if (!active_heading.isEmpty() && !kHtmlCommentLineRe.match(line).hasMatch()) {
                heading_sections[active_heading].append(line);
            }
        }
    }

    const bool use_markers = marker_sections.size() >= 2;
    const bool use_headings = !use_markers && heading_sections.size() >= 2;
    if (!use_markers && !use_headings) {
        QStringList clean;
        for (const QString& line : lines) {
            if (!kHtmlCommentLineRe.match(line).hasMatch()) {
                clean.append(line);
            }
        }
        return clean.join(QLatin1Char('\n')).trimmed();
    }

    const auto& chosen_map = use_markers ? marker_sections : heading_sections;
    const auto& chosen_order = use_markers ? marker_order : heading_order;

    QStringList candidates;
    const QString pref_bcp = NormalizeLocaleCode(ui_language);
    if (!pref_bcp.isEmpty()) {
        const QString pref_base = pref_bcp.section(QLatin1Char('-'), 0, 0);
        candidates.append(pref_bcp);
        if (pref_base == QStringLiteral("zh")) {
            candidates.append(QStringLiteral("zh-hans"));
        }
        if (!pref_base.isEmpty()) {
            candidates.append(pref_base);
        }
    }

    const QString bcp = NormalizeLocaleCode(locale.bcp47Name());
    const QString base_lang = bcp.section(QLatin1Char('-'), 0, 0);
    if (!bcp.isEmpty()) {
        candidates.append(bcp);
    }
    if (base_lang == QStringLiteral("zh")) {
        candidates.append(QStringLiteral("zh-hans"));
    }
    if (!base_lang.isEmpty()) {
        candidates.append(base_lang);
    }
    candidates.append(QStringLiteral("en"));
    candidates.append(QStringLiteral("ru"));

    for (const QString& key : candidates) {
        const auto it = chosen_map.constFind(key);
        if (it != chosen_map.constEnd() && !it.value().join(QString()).trimmed().isEmpty()) {
            return it.value().join(QLatin1Char('\n')).trimmed();
        }
    }
    for (const QString& key : chosen_order) {
        const auto it = chosen_map.constFind(key);
        if (it != chosen_map.constEnd() && !it.value().join(QString()).trimmed().isEmpty()) {
            return it.value().join(QLatin1Char('\n')).trimmed();
        }
    }
    return normalized.trimmed();
}

QString ColorRgba(const QColor& c, int alpha) {
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(qBound(0, alpha, 255));
}

QString ContrastTextOnAccent(const QColor& accent) {
    const double lum = 0.299 * accent.redF() + 0.587 * accent.greenF() + 0.114 * accent.blueF();
    return lum > 0.62 ? QStringLiteral("#0B1120") : QStringLiteral("#FFFFFF");
}

QString SanitizeText(QString text) {
    text.replace(QChar(0x2014), QLatin1Char('-'));
    text.replace(QChar(0x2013), QLatin1Char('-'));
    text.replace(QChar(0x2015), QLatin1Char('-'));
    text.replace(QChar(0x2012), QLatin1Char('-'));
    return text;
}

QString BuildInlineRichHtml(const QString& text, const QString& base_color,
                            const QString& bold_color, const QString& code_color,
                            bool default_bold) {
    QString out;
    out.reserve(text.size() * 2);

    const QString mono_css = QStringLiteral("font-family:'Cascadia Mono','Consolas','Courier New',monospace;");
    int i = 0;
    const int n = text.size();

    while (i < n) {
        if (i + 1 < n && text[i] == QLatin1Char('*') && text[i + 1] == QLatin1Char('*')) {
            const int end = text.indexOf(QStringLiteral("**"), i + 2);
            if (end > i + 2) {
                const QString inner = text.mid(i + 2, end - (i + 2)).toHtmlEscaped();
                out += QStringLiteral("<b style='color:%1;'>%2</b>").arg(bold_color, inner);
                i = end + 2;
                continue;
            }
            out += QStringLiteral("**");
            i += 2;
            continue;
        }

        if (text[i] == QLatin1Char('`')) {
            const int end = text.indexOf(QLatin1Char('`'), i + 1);
            if (end > i + 1) {
                const QString inner = text.mid(i + 1, end - (i + 1)).toHtmlEscaped();
                out += QStringLiteral("<span style='%1font-weight:600;color:%2;'>%3</span>")
                           .arg(mono_css, code_color, inner);
                i = end + 1;
                continue;
            }
            out += QLatin1Char('`');
            ++i;
            continue;
        }

        int next_bold = text.indexOf(QStringLiteral("**"), i);
        int next_code = text.indexOf(QLatin1Char('`'), i);
        int next = n;
        if (next_bold >= 0 && next_bold < next) next = next_bold;
        if (next_code >= 0 && next_code < next) next = next_code;
        if (next == i) next = i + 1;

        const QString chunk = text.mid(i, next - i).toHtmlEscaped();
        if (default_bold) {
            out += QStringLiteral("<b style='color:%1;'>%2</b>").arg(base_color, chunk);
        } else {
            out += QStringLiteral("<span style='color:%1;'>%2</span>").arg(base_color, chunk);
        }
        i = next;
    }

    return out;
}

class BadgeIconWidget final : public QWidget {
public:
    enum class Kind { Update, UpToDate, Error, Arrow };

    explicit BadgeIconWidget(Kind kind, QWidget* parent = nullptr)
        : QWidget(parent), m_kind(kind) {
        if (kind == Kind::Arrow) {
            setFixedSize(30, 30);
        } else {
            setFixedSize(40, 40);
        }
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void SetKind(Kind kind) {
        m_kind = kind;
        update();
    }

    void SetColors(const QColor& bg, const QColor& border, const QColor& fg) {
        m_bg = bg;
        m_border = border;
        m_fg = fg;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal radius = (m_kind == Kind::Arrow) ? 8.0 : 12.0;
        const QRectF box = QRectF(rect()).adjusted(0.75, 0.75, -0.75, -0.75);
        p.setBrush(m_bg);
        p.setPen(QPen(m_border, 1.1));
        p.drawRoundedRect(box, radius, radius);

        QPen fg_pen(m_fg, (m_kind == Kind::Arrow) ? 1.8 : 2.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(fg_pen);
        p.setBrush(Qt::NoBrush);

        const qreal cx = width() * 0.5;
        const qreal cy = height() * 0.5;

        switch (m_kind) {
        case Kind::Update: {
            // Clean downward arrow into tray + top sparkle feel
            p.drawLine(QPointF(cx, cy - 7.5), QPointF(cx, cy + 4.0));
            p.drawLine(QPointF(cx - 4.5, cy - 0.5), QPointF(cx, cy + 4.2));
            p.drawLine(QPointF(cx + 4.5, cy - 0.5), QPointF(cx, cy + 4.2));
            QPainterPath tray;
            tray.moveTo(cx - 7.5, cy + 4.5);
            tray.lineTo(cx - 7.5, cy + 7.5);
            tray.lineTo(cx + 7.5, cy + 7.5);
            tray.lineTo(cx + 7.5, cy + 4.5);
            p.drawPath(tray);
            break;
        }
        case Kind::UpToDate: {
            QPainterPath check;
            check.moveTo(cx - 6.5, cy + 0.5);
            check.lineTo(cx - 1.5, cy + 5.5);
            check.lineTo(cx + 7.0, cy - 5.0);
            p.drawPath(check);
            break;
        }
        case Kind::Error: {
            p.drawLine(QPointF(cx, cy - 6.0), QPointF(cx, cy + 1.5));
            p.setBrush(m_fg);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(cx, cy + 5.5), 1.4, 1.4);
            break;
        }
        case Kind::Arrow: {
            p.drawLine(QPointF(cx - 5.5, cy), QPointF(cx + 5.0, cy));
            p.drawLine(QPointF(cx + 1.0, cy - 4.0), QPointF(cx + 5.2, cy));
            p.drawLine(QPointF(cx + 1.0, cy + 4.0), QPointF(cx + 5.2, cy));
            break;
        }
        }
    }

private:
    Kind m_kind{Kind::Update};
    QColor m_bg{QColor(34, 211, 238, 36)};
    QColor m_border{QColor(34, 211, 238, 120)};
    QColor m_fg{QColor(34, 211, 238)};
};

class RotatingSpinnerWidget final : public QWidget {
public:
    explicit RotatingSpinnerWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(40, 40);
        m_timer.setInterval(16);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_angle = (m_angle + 8) % 360;
            update();
        });
    }

    void SetAccentColor(const QColor& c) {
        m_accent = c;
        update();
    }

    void SetActive(bool active) {
        if (active && !m_timer.isActive()) {
            m_timer.start();
        } else if (!active && m_timer.isActive()) {
            m_timer.stop();
        }
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF arc_rect = QRectF(rect()).adjusted(5.0, 5.0, -5.0, -5.0);

        QColor track = m_accent;
        track.setAlpha(45);
        p.setPen(QPen(track, 3.2, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(arc_rect);

        p.setPen(QPen(m_accent, 3.2, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arc_rect, (90 - m_angle) * 16, -110 * 16);
    }

private:
    QTimer m_timer;
    int m_angle{0};
    QColor m_accent{QColor(34, 211, 238)};
};

class CloseIconButton final : public QPushButton {
public:
    explicit CloseIconButton(QWidget* parent = nullptr) : QPushButton(parent) {
        setFixedSize(32, 32);
        setCursor(Qt::PointingHandCursor);
        setFlat(true);
    }

    void SetIconColor(const QColor& fg) {
        m_fg = fg;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPushButton::paintEvent(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(m_fg, 1.8, Qt::SolidLine, Qt::RoundCap));
        const qreal cx = width() * 0.5;
        const qreal cy = height() * 0.5;
        p.drawLine(QPointF(cx - 4.5, cy - 4.5), QPointF(cx + 4.5, cy + 4.5));
        p.drawLine(QPointF(cx + 4.5, cy - 4.5), QPointF(cx - 4.5, cy + 4.5));
    }

private:
    QColor m_fg{QColor(148, 163, 184)};
};

StormUpdateInfo ConvertLegacyRelease(const Common::Net::Release& release) {
    StormUpdateInfo info;
    info.current_version = UpdateDialog::CleanVersion(QString::fromUtf8(Common::g_build_version));
    info.new_version = UpdateDialog::CleanVersion(QString::fromStdString(release.tag));
    info.release_name = SanitizeText(QString::fromStdString(release.title));
    info.release_notes = SanitizeText(QString::fromStdString(release.body));
    info.published_date = UpdateDialog::FormatEpochToRu(static_cast<qint64>(release.published));

    const bool is_sgw = QString::fromStdString(release.host).contains(QStringLiteral("stormgamesworld.ru"), Qt::CaseInsensitive) ||
                        QString::fromStdString(release.html_url).contains(QStringLiteral("stormgamesworld.ru"), Qt::CaseInsensitive);
    info.source_mirror = is_sgw ? QStringLiteral("STORM GAMES WORLD") : QStringLiteral("GitHub");
    info.release_page_url = QString::fromStdString(release.html_url);
    if (info.release_page_url.isEmpty()) {
        info.release_page_url = QString::fromLatin1(kFallbackReleaseUrl);
    }

    const bool is_win11 = QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 22000);
    const QString preferred_zip = is_win11 ? QStringLiteral("Windows11.zip") : QStringLiteral("Windows10.zip");
    const QString preferred_setup = QStringLiteral("_Setup.exe");

    QString best_url;
    qint64 best_size = 0;
    QStringList all_urls;
    for (size_t i = 0; i < release.assets.size(); ++i) {
        QString url = QString::fromStdString(release.assets[i]);
        if (!url.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)) {
            const QString host = QString::fromStdString(release.host);
            if (!host.isEmpty()) {
                url = host + url;
            }
        }
        const qint64 sz = (i < release.asset_sizes.size()) ? static_cast<qint64>(release.asset_sizes[i]) : 0;
        if (url.endsWith(preferred_zip, Qt::CaseInsensitive)) {
            best_url = url;
            best_size = sz;
            break;
        }
        if (best_url.isEmpty() && url.endsWith(preferred_setup, Qt::CaseInsensitive)) {
            best_url = url;
            best_size = sz;
        }
        if (url.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive) ||
            url.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
            all_urls.push_back(url);
            if (best_url.isEmpty()) {
                best_url = url;
                best_size = sz;
            }
        }
    }

    if (!best_url.isEmpty()) {
        info.download_urls.push_back(best_url);
    }
    for (const QString& u : all_urls) {
        if (!info.download_urls.contains(u)) {
            info.download_urls.push_back(u);
        }
    }
    info.package_size_bytes = best_size;
    return info;
}

} // namespace

UpdateDialog::UpdateDialog(QWidget* parent) : QDialog(parent) {
    BuildUi();
    ApplyThemeAndFont();
    SetChecking();
}

UpdateDialog::UpdateDialog(const StormUpdateInfo& info, QWidget* parent) : QDialog(parent) {
    BuildUi();
    ApplyThemeAndFont();
    SetUpdateAvailable(info);
}

UpdateDialog::UpdateDialog(const Common::Net::Release& release, QWidget* parent)
    : QDialog(parent) {
    BuildUi();
    ApplyThemeAndFont();
    SetUpdateAvailable(ConvertLegacyRelease(release));
}

UpdateDialog::~UpdateDialog() {
    AbortActiveDownload();
}

QString UpdateDialog::CleanVersion(const QString& raw) {
    QString clean = raw.trimmed();
    while (clean.startsWith(QLatin1Char('v'), Qt::CaseInsensitive) ||
           clean.startsWith(QLatin1Char(' '))) {
        clean.remove(0, 1);
    }
    return clean.trimmed();
}

QString UpdateDialog::FormatIsoDateToRu(const QString& iso_date) {
    const QString s = iso_date.trimmed();
    if (s.isEmpty()) {
        return {};
    }
    const QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    if (dt.isValid()) {
        return dt.toLocalTime().date().toString(QStringLiteral("dd.MM.yyyy"));
    }
    if (s.size() >= 10) {
        const QDate d = QDate::fromString(s.left(10), QStringLiteral("yyyy-MM-dd"));
        if (d.isValid()) {
            return d.toString(QStringLiteral("dd.MM.yyyy"));
        }
    }
    return {};
}

QString UpdateDialog::FormatEpochToRu(qint64 epoch_seconds) {
    if (epoch_seconds <= 0) {
        return {};
    }
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(epoch_seconds, Qt::UTC).toLocalTime();
    return dt.isValid() ? dt.date().toString(QStringLiteral("dd.MM.yyyy")) : QString{};
}

QString UpdateDialog::FormatBytes(qint64 bytes) {
    if (bytes <= 0) {
        return {};
    }
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mb >= 0.1) {
        return QStringLiteral("%1 %2")
            .arg(QString::number(mb, 'f', 1),
                 Tr12("МБ", "MB", "MB", "MB", "Mo", "MB", "MB", "MB", "MB", "MB", "MB", "MB"));
    }
    const qint64 kb = std::max<qint64>(1, bytes / 1024);
    return QStringLiteral("%1 %2")
        .arg(QString::number(kb),
             Tr12("КБ", "KB", "KB", "KB", "Ko", "KB", "KB", "KB", "KB", "KB", "KB", "KB"));
}

QString UpdateDialog::GetSkippedVersion() {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    return CleanVersion(settings.value(QString::fromLatin1(kSkippedVerKey)).toString());
}

void UpdateDialog::SetSkippedVersion(const QString& version) {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(QString::fromLatin1(kSkippedVerKey), CleanVersion(version));
}

void UpdateDialog::ClearSkippedVersion() {
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.remove(QString::fromLatin1(kSkippedVerKey));
}

void UpdateDialog::BuildUi() {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setModal(true);
    setFixedWidth(576);
    setMaximumHeight(680);

    StormUI::StructureEngine::InstallEscDismiss(this);

    auto* outer_layout = new QVBoxLayout(this);
    outer_layout->setContentsMargins(10, 10, 10, 10);
    outer_layout->setSpacing(0);

    m_card = new QFrame(this);
    m_card->setObjectName(QStringLiteral("stormUpdateCard"));
    outer_layout->addWidget(m_card);

    auto* shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(22);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 145));
    m_card->setGraphicsEffect(shadow);

    auto* card_layout = new QVBoxLayout(m_card);
    card_layout->setContentsMargins(20, 18, 20, 18);
    card_layout->setSpacing(14);

    // Header
    m_header_widget = new QWidget(m_card);
    auto* header_layout = new QHBoxLayout(m_header_widget);
    header_layout->setContentsMargins(0, 0, 0, 0);
    header_layout->setSpacing(12);

    m_header_badge = new BadgeIconWidget(BadgeIconWidget::Kind::Update, m_header_widget);
    header_layout->addWidget(m_header_badge, 0, Qt::AlignVCenter);

    auto* title_col = new QVBoxLayout();
    title_col->setContentsMargins(0, 0, 0, 0);
    title_col->setSpacing(2);

    m_title_label = new QLabel(m_header_widget);
    m_title_label->setWordWrap(true);
    m_subtitle_label = new QLabel(QStringLiteral("STORM SWITCH"), m_header_widget);

    title_col->addWidget(m_title_label);
    title_col->addWidget(m_subtitle_label);
    header_layout->addLayout(title_col, 1);

    auto* close_btn = new CloseIconButton(m_header_widget);
    close_btn->setObjectName(QStringLiteral("stormUpdateCloseBtn"));
    m_close_btn = close_btn;
    connect(m_close_btn, &QPushButton::clicked, this, [this]() {
        if (m_installing) {
            return;
        }
        AbortActiveDownload();
        reject();
    });
    header_layout->addWidget(m_close_btn, 0, Qt::AlignVCenter);

    card_layout->addWidget(m_header_widget);

    m_header_divider = new QFrame(m_card);
    m_header_divider->setFrameShape(QFrame::NoFrame);
    m_header_divider->setFixedHeight(1);
    card_layout->addWidget(m_header_divider);

    m_stack = new QStackedWidget(m_card);
    card_layout->addWidget(m_stack, 1);

    // Page 0: Checking
    auto* page_checking = new QWidget(m_stack);
    auto* chk_layout = new QVBoxLayout(page_checking);
    chk_layout->setContentsMargins(16, 26, 16, 26);
    chk_layout->setSpacing(14);
    chk_layout->setAlignment(Qt::AlignCenter);

    m_checking_spinner = new RotatingSpinnerWidget(page_checking);
    chk_layout->addWidget(m_checking_spinner, 0, Qt::AlignHCenter);

    m_checking_label = new QLabel(
        Tr12("Поиск новой версии на STORM GAMES WORLD и GitHub...",
             "Searching for a new version on STORM GAMES WORLD and GitHub...",
             "Suche nach neuer Version auf STORM GAMES WORLD und GitHub...",
             "Buscando una nueva versión en STORM GAMES WORLD y GitHub...",
             "Recherche d'une nouvelle version sur STORM GAMES WORLD et GitHub...",
             "Mencari versi terbaru di STORM GAMES WORLD dan GitHub...",
             "Ricerca della nuova versione su STORM GAMES WORLD e GitHub...",
             "STORM GAMES WORLD と GitHub で新バージョンを検索中...",
             "STORM GAMES WORLD 및 GitHub에서 새 버전을 검색 중...",
             "Procurando nova versão em STORM GAMES WORLD e GitHub...",
             "正在 STORM GAMES WORLD 和 GitHub 上搜索新版本...",
             "STORM GAMES WORLD और GitHub पर नया संस्करण खोजा जा रहा है..."),
        page_checking);
    m_checking_label->setAlignment(Qt::AlignCenter);
    m_checking_label->setWordWrap(true);
    chk_layout->addWidget(m_checking_label);
    m_stack->addWidget(page_checking);

    // Page 1: UpdateAvailable
    auto* page_avail = new QWidget(m_stack);
    auto* avail_layout = new QVBoxLayout(page_avail);
    avail_layout->setContentsMargins(0, 0, 0, 0);
    avail_layout->setSpacing(12);

    // Version Transition Card
    m_version_card = new QFrame(page_avail);
    m_version_card->setObjectName(QStringLiteral("stormVersionCard"));
    auto* ver_layout = new QHBoxLayout(m_version_card);
    ver_layout->setContentsMargins(14, 11, 14, 11);
    ver_layout->setSpacing(12);

    auto* cur_col = new QVBoxLayout();
    cur_col->setContentsMargins(12, 6, 12, 6);
    cur_col->setSpacing(2);
    m_cur_ver_title = new QLabel(Tr12("Текущая", "Current", "Aktuell", "Actual", "Actuelle", "Saat ini", "Attuale", "現在", "현재", "Atual", "当前版本", "वर्तमान"), m_version_card);
    m_cur_ver_value = new QLabel(QStringLiteral("-"), m_version_card);
    cur_col->addWidget(m_cur_ver_title);
    cur_col->addWidget(m_cur_ver_value);

    m_arrow_badge = new BadgeIconWidget(BadgeIconWidget::Kind::Arrow, m_version_card);

    m_new_ver_pill = new QFrame(m_version_card);
    m_new_ver_pill->setObjectName(QStringLiteral("stormNewVersionPill"));
    auto* new_col = new QVBoxLayout(m_new_ver_pill);
    new_col->setContentsMargins(12, 6, 12, 6);
    new_col->setSpacing(2);
    m_new_ver_title = new QLabel(Tr12("Новая", "New", "Neu", "Nueva", "Nouvelle", "Baru", "Nuova", "新規", "새 버전", "Nova", "最新版本", "नया"), m_new_ver_pill);
    m_new_ver_value = new QLabel(QStringLiteral("-"), m_new_ver_pill);
    new_col->addWidget(m_new_ver_title);
    new_col->addWidget(m_new_ver_value);

    ver_layout->addLayout(cur_col, 1);
    ver_layout->addWidget(m_arrow_badge, 0, Qt::AlignVCenter);
    ver_layout->addWidget(m_new_ver_pill, 1);
    avail_layout->addWidget(m_version_card);

    // Metadata Chips Row
    m_chips_row = new QWidget(page_avail);
    auto* chips_layout = new QHBoxLayout(m_chips_row);
    chips_layout->setContentsMargins(0, 0, 0, 0);
    chips_layout->setSpacing(8);

    m_mirror_chip = new QLabel(m_chips_row);
    m_mirror_chip->setObjectName(QStringLiteral("stormMirrorChip"));
    m_size_chip = new QLabel(m_chips_row);
    m_size_chip->setObjectName(QStringLiteral("stormMetaChip"));
    m_date_chip = new QLabel(m_chips_row);
    m_date_chip->setObjectName(QStringLiteral("stormMetaChip"));

    chips_layout->addWidget(m_mirror_chip);
    chips_layout->addWidget(m_size_chip);
    chips_layout->addWidget(m_date_chip);
    chips_layout->addStretch(1);
    avail_layout->addWidget(m_chips_row);

    // Release Notes Section
    m_notes_header = new QLabel(Tr12("ЧТО НОВОГО:", "WHAT'S NEW:", "WAS IST NEU:", "NOVEDADES:", "NOUVEAUTÉS :", "APA YANG BARU:", "NOVITÀ:", "更新内容:", "변경 사항:", "NOVIDADES:", "更新内容:", "नया क्या है:"), page_avail);
    avail_layout->addWidget(m_notes_header);

    m_notes_frame = new QFrame(page_avail);
    m_notes_frame->setObjectName(QStringLiteral("stormNotesFrame"));
    m_notes_frame->setMinimumHeight(130);
    m_notes_frame->setMaximumHeight(240);
    auto* notes_frame_layout = new QVBoxLayout(m_notes_frame);
    notes_frame_layout->setContentsMargins(4, 4, 4, 4);
    notes_frame_layout->setSpacing(0);

    m_notes_scroll = new QScrollArea(m_notes_frame);
    m_notes_scroll->setWidgetResizable(true);
    m_notes_scroll->setFrameShape(QFrame::NoFrame);
    m_notes_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_notes_scroll->viewport()->setAutoFillBackground(false);
    m_notes_scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));

    m_notes_container = new QWidget(m_notes_scroll);
    m_notes_container->setObjectName(QStringLiteral("stormNotesViewport"));
    m_notes_layout = new QVBoxLayout(m_notes_container);
    m_notes_layout->setContentsMargins(12, 10, 12, 10);
    m_notes_layout->setSpacing(6);
    m_notes_scroll->setWidget(m_notes_container);

    notes_frame_layout->addWidget(m_notes_scroll);
    avail_layout->addWidget(m_notes_frame);

    // Inline Download Progress Card
    m_progress_card = new QFrame(page_avail);
    m_progress_card->setObjectName(QStringLiteral("stormProgressCard"));
    auto* prog_layout = new QVBoxLayout(m_progress_card);
    prog_layout->setContentsMargins(14, 12, 14, 12);
    prog_layout->setSpacing(8);

    m_progress_status = new QLabel(Tr12("Загрузка обновления...", "Downloading update...", "Update wird heruntergeladen...", "Descargando actualización...", "Téléchargement de la mise à jour...", "Mengunduh pembaruan...", "Download dell'aggiornamento...", "アップデートをダウンロード中...", "업데이트 다운로드 중...", "Baixando atualização...", "正在下载更新...", "अपडेट डाउनलोड हो रहा है..."),
                                   m_progress_card);
    m_progress_status->setWordWrap(true);
    prog_layout->addWidget(m_progress_status);

    m_progress_bar = new QProgressBar(m_progress_card);
    m_progress_bar->setObjectName(QStringLiteral("stormUpdateProgressBar"));
    m_progress_bar->setTextVisible(false);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(0);
    m_progress_bar->setFixedHeight(8);
    prog_layout->addWidget(m_progress_bar);

    auto* stats_row = new QHBoxLayout();
    stats_row->setContentsMargins(0, 0, 0, 0);
    stats_row->setSpacing(8);
    m_progress_bytes = new QLabel(m_progress_card);
    m_progress_speed = new QLabel(m_progress_card);
    m_progress_percent = new QLabel(QStringLiteral("0%"), m_progress_card);
    stats_row->addWidget(m_progress_bytes);
    stats_row->addStretch(1);
    stats_row->addWidget(m_progress_speed);
    stats_row->addWidget(m_progress_percent);
    prog_layout->addLayout(stats_row);

    m_cancel_dl_btn = new QPushButton(Tr12("Отмена", "Cancel", "Abbrechen", "Cancelar", "Annuler", "Batal", "Annulla", "キャンセル", "취소", "Cancelar", "取消", "रद्द करें"), m_progress_card);
    m_cancel_dl_btn->setObjectName(QStringLiteral("stormSecondaryBtn"));
    m_cancel_dl_btn->setFixedHeight(34);
    m_cancel_dl_btn->setCursor(Qt::PointingHandCursor);
    connect(m_cancel_dl_btn, &QPushButton::clicked, this, &UpdateDialog::CancelDownload);
    prog_layout->addWidget(m_cancel_dl_btn);

    m_progress_card->setVisible(false);
    avail_layout->addWidget(m_progress_card);

    // Actions & Footer
    m_actions_container = new QWidget(page_avail);
    auto* act_layout = new QVBoxLayout(m_actions_container);
    act_layout->setContentsMargins(0, 2, 0, 0);
    act_layout->setSpacing(8);

    m_update_btn = new QPushButton(Tr12("Обновить сейчас", "Update now", "Jetzt aktualisieren", "Actualizar ahora", "Mettre à jour", "Perbarui sekarang", "Aggiorna ora", "今すぐ更新", "지금 업데이트", "Atualizar agora", "立即更新", "अभी अपडेट करें"), m_actions_container);
    m_update_btn->setObjectName(QStringLiteral("stormPrimaryBtn"));
    m_update_btn->setFixedHeight(42);
    m_update_btn->setCursor(Qt::PointingHandCursor);
    connect(m_update_btn, &QPushButton::clicked, this, &UpdateDialog::StartDownload);
    act_layout->addWidget(m_update_btn);

    auto* sec_row = new QHBoxLayout();
    sec_row->setContentsMargins(0, 0, 0, 0);
    sec_row->setSpacing(8);
    m_later_btn = new QPushButton(Tr12("Напомнить позже", "Remind later", "Später erinnern", "Recordar más tarde", "Plus tard", "Ingatkan nanti", "Ricorda più tardi", "後で通知", "나중에 알림", "Lembrar mais tarde", "稍后提醒", "बाद में याद दिलाएं"), m_actions_container);
    m_later_btn->setObjectName(QStringLiteral("stormSecondaryBtn"));
    m_later_btn->setFixedHeight(38);
    m_later_btn->setCursor(Qt::PointingHandCursor);
    connect(m_later_btn, &QPushButton::clicked, this, &QDialog::reject);

    m_skip_btn = new QPushButton(Tr12("Пропустить версию", "Skip version", "Version überspringen", "Omitir versión", "Ignorer la version", "Lewati versi ini", "Salta versione", "このバージョンをスキップ", "이 버전 건너뛰기", "Pular versão", "跳过此版本", "यह संस्करण छोड़ें"), m_actions_container);
    m_skip_btn->setObjectName(QStringLiteral("stormSecondaryBtn"));
    m_skip_btn->setFixedHeight(38);
    m_skip_btn->setCursor(Qt::PointingHandCursor);
    connect(m_skip_btn, &QPushButton::clicked, this, [this]() {
        if (!m_info.new_version.isEmpty()) {
            SetSkippedVersion(m_info.new_version);
        }
        reject();
    });

    sec_row->addWidget(m_later_btn, 1);
    sec_row->addWidget(m_skip_btn, 1);
    act_layout->addLayout(sec_row);

    m_release_link_btn = new QPushButton(Tr12("Открыть страницу релиза", "Open release page", "Release-Seite öffnen", "Abrir página del lanzamiento", "Ouvrir la page de publication", "Buka halaman rilis", "Apri pagina di rilascio", "リリースページを開く", "릴리스 페이지 열기", "Abrir página do lançamento", "打开发布页面", "रिलीज़ पृष्ठ खोलें"), m_actions_container);
    m_release_link_btn->setObjectName(QStringLiteral("stormLinkBtn"));
    m_release_link_btn->setFlat(true);
    m_release_link_btn->setCursor(Qt::PointingHandCursor);
    connect(m_release_link_btn, &QPushButton::clicked, this, [this]() {
        const QString target = m_info.release_page_url.isEmpty()
            ? QString::fromLatin1(kFallbackReleaseUrl)
            : m_info.release_page_url;
        QDesktopServices::openUrl(QUrl(target));
    });
    act_layout->addWidget(m_release_link_btn, 0, Qt::AlignHCenter);

    avail_layout->addWidget(m_actions_container);
    m_stack->addWidget(page_avail);

    // Page 2: UpToDate
    auto* page_uptodate = new QWidget(m_stack);
    auto* utd_layout = new QVBoxLayout(page_uptodate);
    utd_layout->setContentsMargins(0, 6, 0, 0);
    utd_layout->setSpacing(12);

    m_uptodate_desc = new QLabel(
        Tr12("У вас установлена последняя актуальная версия приложения.",
             "You are running the latest version of the application.",
             "Sie verwenden bereits die neueste Version der Anwendung.",
             "Ya tienes instalada la última versión de la aplicación.",
             "Vous utilisez déjà la dernière version de l'application.",
             "Anda sedang menggunakan versi terbaru aplikasi ini.",
             "Stai utilizzando l'ultima versione dell'applicazione.",
             "最新バージョンのアプリケーションを使用しています。",
             "현재 최신 버전의 애플리케이션을 사용 중입니다.",
             "Você já está usando a versão mais recente do aplicativo.",
             "您当前使用的已是最新版本。",
             "आप एप्लिकेशन का नवीनतम संस्करण उपयोग कर रहे हैं।"),
        page_uptodate);
    m_uptodate_desc->setWordWrap(true);
    utd_layout->addWidget(m_uptodate_desc);

    m_uptodate_badge = new QLabel(page_uptodate);
    m_uptodate_badge->setObjectName(QStringLiteral("stormMirrorChip"));
    m_uptodate_badge->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    utd_layout->addWidget(m_uptodate_badge);

    m_uptodate_mirror = new QLabel(page_uptodate);
    m_uptodate_mirror->setWordWrap(true);
    m_uptodate_mirror->setVisible(false);
    utd_layout->addWidget(m_uptodate_mirror);

    m_uptodate_ok_btn = new QPushButton(Tr12("ОК", "OK", "OK", "Aceptar", "OK", "OK", "OK", "OK", "확인", "OK", "确定", "ठीक है"), page_uptodate);
    m_uptodate_ok_btn->setObjectName(QStringLiteral("stormPrimaryBtn"));
    m_uptodate_ok_btn->setFixedHeight(40);
    m_uptodate_ok_btn->setCursor(Qt::PointingHandCursor);
    connect(m_uptodate_ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    utd_layout->addWidget(m_uptodate_ok_btn);

    m_stack->addWidget(page_uptodate);

    // Page 3: Error
    auto* page_err = new QWidget(m_stack);
    auto* err_layout = new QVBoxLayout(page_err);
    err_layout->setContentsMargins(0, 4, 0, 0);
    err_layout->setSpacing(12);

    m_error_card = new QFrame(page_err);
    m_error_card->setObjectName(QStringLiteral("stormErrorCard"));
    auto* err_card_layout = new QVBoxLayout(m_error_card);
    err_card_layout->setContentsMargins(14, 12, 14, 12);
    m_error_label = new QLabel(m_error_card);
    m_error_label->setWordWrap(true);
    err_card_layout->addWidget(m_error_label);
    err_layout->addWidget(m_error_card);

    auto* err_btn_row = new QHBoxLayout();
    err_btn_row->setContentsMargins(0, 0, 0, 0);
    err_btn_row->setSpacing(8);

    m_error_retry_btn = new QPushButton(Tr12("Повторить", "Retry", "Wiederholen", "Reintentar", "Réessayer", "Coba lagi", "Riprova", "再試行", "다시 시도", "Tentar novamente", "重试", "पुनः प्रयास करें"), page_err);
    m_error_retry_btn->setObjectName(QStringLiteral("stormPrimaryBtn"));
    m_error_retry_btn->setFixedHeight(40);
    m_error_retry_btn->setCursor(Qt::PointingHandCursor);
    connect(m_error_retry_btn, &QPushButton::clicked, this, [this]() {
        SetChecking();
        emit RetryRequested();
    });

    m_error_ok_btn = new QPushButton(Tr12("ОК", "OK", "OK", "Aceptar", "OK", "OK", "OK", "OK", "확인", "OK", "确定", "ठीक है"), page_err);
    m_error_ok_btn->setObjectName(QStringLiteral("stormSecondaryBtn"));
    m_error_ok_btn->setFixedHeight(40);
    m_error_ok_btn->setCursor(Qt::PointingHandCursor);
    connect(m_error_ok_btn, &QPushButton::clicked, this, &QDialog::reject);

    err_btn_row->addWidget(m_error_retry_btn, 1);
    err_btn_row->addWidget(m_error_ok_btn, 1);
    err_layout->addLayout(err_btn_row);

    m_error_link_btn = new QPushButton(Tr12("Открыть страницу релиза", "Open release page", "Release-Seite öffnen", "Abrir página del lanzamiento", "Ouvrir la page de publication", "Buka halaman rilis", "Apri pagina di rilascio", "リリースページを開く", "릴리스 페이지 열기", "Abrir página do lançamento", "打开发布页面", "रिलीज़ पृष्ठ खोलें"), page_err);
    m_error_link_btn->setObjectName(QStringLiteral("stormLinkBtn"));
    m_error_link_btn->setFlat(true);
    m_error_link_btn->setCursor(Qt::PointingHandCursor);
    connect(m_error_link_btn, &QPushButton::clicked, this, [this]() {
        const QString target = m_info.release_page_url.isEmpty()
            ? QString::fromLatin1(kFallbackReleaseUrl)
            : m_info.release_page_url;
        QDesktopServices::openUrl(QUrl(target));
    });
    err_layout->addWidget(m_error_link_btn, 0, Qt::AlignHCenter);

    m_stack->addWidget(page_err);
}

void UpdateDialog::ApplyThemeAndFont() {
    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    QString active_font = StormDesign::ResolveActiveUiFontFamily();
    active_font.replace(QLatin1Char('"'), QString());
    active_font.replace(QLatin1Char('\''), QString());

    const QColor accent_c(pal.accent);
    const QColor border_c(pal.border);
    const QColor surface_c(pal.surface);
    const QColor card_c(pal.card);
    const QColor input_c(pal.inputBg);
    const QColor text_pri(pal.textPrimary);
    const QColor text_sec(pal.textSecondary);
    const QColor btn_bg(pal.buttonBg);
    const QColor btn_border(pal.buttonBorder);

    const QString primary_fg = ContrastTextOnAccent(accent_c);
    const QString card_border = pal.is_light ? pal.border : ColorRgba(accent_c, 85);

    const QString qss = QStringLiteral(
        "QDialog { background: transparent; font-family: \"%1\", \"Segoe UI\", sans-serif; }"
        "QWidget { font-family: \"%1\", \"Segoe UI\", sans-serif; color: %2; }"
        "QFrame#stormUpdateCard {"
        "  background-color: %3;"
        "  border: 1.5px solid %4;"
        "  border-radius: 18px;"
        "}"
        "QFrame#stormUpdateCard > QWidget, QStackedWidget, QStackedWidget > QWidget {"
        "  background: transparent;"
        "}"
        "QFrame#stormVersionCard {"
        "  background-color: %5;"
        "  border: 1px solid %6;"
        "  border-radius: 14px;"
        "}"
        "QFrame#stormNewVersionPill {"
        "  background-color: %7;"
        "  border: 1px solid %8;"
        "  border-radius: 10px;"
        "}"
        "QLabel#stormMirrorChip {"
        "  background-color: %7;"
        "  border: 1px solid %8;"
        "  border-radius: 9px;"
        "  color: %9;"
        "  padding: 5px 10px;"
        "  font-family: \"Cascadia Mono\", \"Consolas\", monospace;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "}"
        "QLabel#stormMetaChip {"
        "  background-color: %5;"
        "  border: 1px solid %6;"
        "  border-radius: 9px;"
        "  color: %10;"
        "  padding: 5px 10px;"
        "  font-family: \"Cascadia Mono\", \"Consolas\", monospace;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "}"
        "QFrame#stormNotesFrame {"
        "  background-color: %11;"
        "  border: 1px solid %6;"
        "  border-radius: 14px;"
        "}"
        "QWidget#stormNotesViewport, QScrollArea, QFrame#stormNotesFrame QLabel, QFrame#stormNotesFrame QWidget {"
        "  background: transparent;"
        "}"
        "QFrame#stormProgressCard {"
        "  background-color: %5;"
        "  border: 1px solid %8;"
        "  border-radius: 14px;"
        "}"
        "QFrame#stormErrorCard {"
        "  background-color: rgba(239, 68, 68, 22);"
        "  border: 1px solid rgba(239, 68, 68, 105);"
        "  border-radius: 12px;"
        "}"
        "QProgressBar#stormUpdateProgressBar {"
        "  background-color: %12;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar#stormUpdateProgressBar::chunk {"
        "  background-color: %9;"
        "  border-radius: 4px;"
        "}"
        "QPushButton#stormPrimaryBtn {"
        "  background-color: %9;"
        "  color: %13;"
        "  border: 1px solid %9;"
        "  border-radius: 12px;"
        "  font-size: 13px;"
        "  font-weight: 700;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton#stormPrimaryBtn:hover {"
        "  background-color: %14;"
        "  border-color: %14;"
        "}"
        "QPushButton#stormPrimaryBtn:disabled {"
        "  opacity: 0.6;"
        "}"
        "QPushButton#stormSecondaryBtn {"
        "  background-color: %15;"
        "  color: %2;"
        "  border: 1px solid %16;"
        "  border-radius: 11px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 6px 14px;"
        "}"
        "QPushButton#stormSecondaryBtn:hover {"
        "  background-color: %17;"
        "  border-color: %8;"
        "}"
        "QPushButton#stormUpdateCloseBtn {"
        "  background-color: %5;"
        "  border: 1px solid %6;"
        "  border-radius: 9px;"
        "}"
        "QPushButton#stormUpdateCloseBtn:hover {"
        "  background-color: %17;"
        "  border-color: %8;"
        "}"
        "QPushButton#stormLinkBtn {"
        "  background: transparent;"
        "  border: none;"
        "  color: %9;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "  padding: 4px 10px;"
        "}"
        "QPushButton#stormLinkBtn:hover {"
        "  color: %14;"
        "  text-decoration: underline;"
        "}"
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 7px;"
        "  margin: 6px 2px 6px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %8;"
        "  border-radius: 3px;"
        "  min-height: 24px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    )
    .arg(active_font, pal.textPrimary, pal.bg, card_border, pal.card, pal.border,
         ColorRgba(accent_c, 36), ColorRgba(accent_c, 115), pal.accent)
    .arg(pal.textSecondary, pal.inputBg, ColorRgba(border_c, 150), primary_fg,
         pal.accentHover, pal.buttonBg, pal.buttonBorder, pal.itemHover);

    setStyleSheet(qss);

    QFont title_font(active_font);
    title_font.setPixelSize(16);
    title_font.setBold(true);
    m_title_label->setFont(title_font);

    QFont mono_sub(QStringLiteral("Cascadia Mono"));
    mono_sub.setStyleHint(QFont::Monospace);
    mono_sub.setPixelSize(11);
    m_subtitle_label->setFont(mono_sub);
    m_subtitle_label->setStyleSheet(QStringLiteral("color:%1;").arg(pal.textSecondary));

    m_header_divider->setStyleSheet(QStringLiteral("background-color:%1;").arg(pal.border));

    if (auto* arrow = static_cast<BadgeIconWidget*>(m_arrow_badge)) {
        arrow->SetColors(QColor(accent_c.red(), accent_c.green(), accent_c.blue(), 36),
                         QColor(accent_c.red(), accent_c.green(), accent_c.blue(), 95),
                         accent_c);
    }
    if (auto* spinner = static_cast<RotatingSpinnerWidget*>(m_checking_spinner)) {
        spinner->SetAccentColor(accent_c);
    }
    if (auto* close_ic = static_cast<CloseIconButton*>(m_close_btn)) {
        close_ic->SetIconColor(text_sec);
    }

    QFont ver_caption(active_font);
    ver_caption.setPixelSize(11);
    m_cur_ver_title->setFont(ver_caption);
    m_cur_ver_title->setStyleSheet(QStringLiteral("color:%1;").arg(pal.textSecondary));
    m_new_ver_title->setFont(ver_caption);
    m_new_ver_title->setStyleSheet(QStringLiteral("color:%1;font-weight:600;").arg(pal.accent));

    QFont ver_val(QStringLiteral("Cascadia Mono"));
    ver_val.setStyleHint(QFont::Monospace);
    ver_val.setPixelSize(15);
    ver_val.setBold(true);
    m_cur_ver_value->setFont(ver_val);
    m_cur_ver_value->setStyleSheet(QStringLiteral("color:%1;").arg(pal.textPrimary));
    m_new_ver_value->setFont(ver_val);
    m_new_ver_value->setStyleSheet(QStringLiteral("color:%1;").arg(pal.accent));

    QFont notes_hdr_f(QStringLiteral("Cascadia Mono"));
    notes_hdr_f.setStyleHint(QFont::Monospace);
    notes_hdr_f.setPixelSize(11);
    notes_hdr_f.setBold(true);
    m_notes_header->setFont(notes_hdr_f);
    m_notes_header->setStyleSheet(QStringLiteral("color:%1;letter-spacing:0.6px;").arg(pal.textSecondary));

    QFont prog_stat_f(QStringLiteral("Cascadia Mono"));
    prog_stat_f.setStyleHint(QFont::Monospace);
    prog_stat_f.setPixelSize(11);
    m_progress_bytes->setFont(prog_stat_f);
    m_progress_bytes->setStyleSheet(QStringLiteral("color:%1;").arg(pal.textSecondary));
    m_progress_speed->setFont(prog_stat_f);
    m_progress_speed->setStyleSheet(QStringLiteral("color:%1;").arg(pal.textSecondary));
    prog_stat_f.setBold(true);
    m_progress_percent->setFont(prog_stat_f);
    m_progress_percent->setStyleSheet(QStringLiteral("color:%1;").arg(pal.accent));

    m_checking_label->setStyleSheet(QStringLiteral("color:%1;font-size:13px;").arg(pal.textSecondary));
    m_uptodate_desc->setStyleSheet(QStringLiteral("color:%1;font-size:13px;").arg(pal.textSecondary));
    m_uptodate_mirror->setStyleSheet(QStringLiteral("color:%1;font-size:12px;").arg(pal.textSecondary));
    m_error_label->setStyleSheet(QStringLiteral("color:%1;font-size:12.5px;").arg(pal.textPrimary));

    UpdateHeaderAppearance(m_mode);
}

void UpdateDialog::UpdateHeaderAppearance(Mode mode) {
    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    const QColor accent_c(pal.accent);
    auto* badge = static_cast<BadgeIconWidget*>(m_header_badge);

    switch (mode) {
    case Mode::Checking:
        m_title_label->setText(Tr12("Проверка обновлений...", "Checking for updates...", "Suche nach Updates...", "Buscando actualizaciones...", "Recherche de mises à jour...", "Memeriksa pembaruan...", "Verifica aggiornamenti...", "アップデートを確認中...", "업데이트 확인 중...", "Verificando atualizações...", "正在检查更新...", "अपडेट की जांच हो रही है..."));
        if (badge) {
            badge->SetKind(BadgeIconWidget::Kind::Update);
            badge->SetColors(QColor(accent_c.red(), accent_c.green(), accent_c.blue(), 36),
                             QColor(accent_c.red(), accent_c.green(), accent_c.blue(), 115),
                             accent_c);
        }
        break;
    case Mode::UpdateAvailable:
        m_title_label->setText(Tr12("Доступно обновление", "Update available", "Update verfügbar", "Actualización disponible", "Mise à jour disponible", "Pembaruan tersedia", "Aggiornamento disponibile", "アップデートがあります", "업데이트 사용 가능", "Atualização disponível", "发现新版本", "नया अपडेट उपलब्ध है"));
        if (badge) {
            badge->SetKind(BadgeIconWidget::Kind::Update);
            badge->SetColors(QColor(accent_c.red(), accent_c.green(), accent_c.blue(), 36),
                             QColor(accent_c.red(), accent_c.green(), accent_c.blue(), 115),
                             accent_c);
        }
        break;
    case Mode::UpToDate:
        m_title_label->setText(Tr12("Установлена актуальная версия", "Up to date", "Aktuelle Version installiert", "Versión actualizada", "Application à jour", "Sudah versi terbaru", "Versione aggiornata", "最新バージョンです", "최신 버전입니다", "Versão atualizada", "已是最新版本", "नवीनतम संस्करण इंस्टॉल है"));
        if (badge) {
            const QColor green(16, 185, 129);
            badge->SetKind(BadgeIconWidget::Kind::UpToDate);
            badge->SetColors(QColor(16, 185, 129, 36), QColor(16, 185, 129, 125), green);
        }
        break;
    case Mode::Error:
        m_title_label->setText(Tr12("Ошибка проверки обновлений", "Update check failed", "Fehler bei der Update-Suche", "Error al buscar actualizaciones", "Échec de vérification des mises à jour", "Gagal memeriksa pembaruan", "Errore verifica aggiornamenti", "アップデート確認エラー", "업데이트 확인 오류", "Erro ao verificar atualizações", "检查更新失败", "अपडेट जांच विफल रही"));
        if (badge) {
            const QColor red(239, 68, 68);
            badge->SetKind(BadgeIconWidget::Kind::Error);
            badge->SetColors(QColor(239, 68, 68, 36), QColor(239, 68, 68, 125), red);
        }
        break;
    }
}

void UpdateDialog::ActivatePage(int index) {
    for (int i = 0; i < m_stack->count(); ++i) {
        if (QWidget* page = m_stack->widget(i)) {
            page->setSizePolicy(QSizePolicy::Preferred,
                                i == index ? QSizePolicy::Preferred : QSizePolicy::Ignored);
            page->updateGeometry();
        }
    }
    m_stack->setCurrentIndex(index);
    m_stack->updateGeometry();
    if (m_card && m_card->layout()) {
        m_card->layout()->activate();
    }
    if (layout()) {
        layout()->activate();
    }
    const QSize target = sizeHint();
    resize(width(), target.height());
    CenterOverParent();
}

void UpdateDialog::SetChecking() {
    AbortActiveDownload();
    m_mode = Mode::Checking;
    UpdateHeaderAppearance(m_mode);
    SetDownloadUiActive(false);
    if (auto* spinner = static_cast<RotatingSpinnerWidget*>(m_checking_spinner)) {
        spinner->SetActive(true);
    }
    ActivatePage(0);
}

void UpdateDialog::SetUpdateAvailable(const StormUpdateInfo& info) {
    AbortActiveDownload();
    m_info = info;
    m_info.current_version = CleanVersion(info.current_version);
    if (m_info.current_version.isEmpty()) {
        m_info.current_version = CleanVersion(QString::fromUtf8(Common::g_build_version));
    }
    m_info.new_version = CleanVersion(info.new_version);

    if (auto* spinner = static_cast<RotatingSpinnerWidget*>(m_checking_spinner)) {
        spinner->SetActive(false);
    }

    m_mode = Mode::UpdateAvailable;
    UpdateHeaderAppearance(m_mode);
    SetDownloadUiActive(false);

    m_cur_ver_value->setText(m_info.current_version.isEmpty() ? QStringLiteral("-") : m_info.current_version);
    m_new_ver_value->setText(m_info.new_version.isEmpty() ? QStringLiteral("-") : m_info.new_version);

    const QString mirror = m_info.source_mirror.isEmpty() ? QStringLiteral("GitHub") : m_info.source_mirror;
    m_mirror_chip->setText(mirror);

    const QString size_str = FormatBytes(m_info.package_size_bytes);
    m_size_chip->setText(size_str);
    m_size_chip->setVisible(!size_str.isEmpty());

    m_date_chip->setText(m_info.published_date);
    m_date_chip->setVisible(!m_info.published_date.isEmpty());

    PopulateReleaseNotes(m_info.release_notes);

    ActivatePage(1);
}

void UpdateDialog::SetUpToDate(const QString& current_version, const QString& source_mirror) {
    AbortActiveDownload();
    if (auto* spinner = static_cast<RotatingSpinnerWidget*>(m_checking_spinner)) {
        spinner->SetActive(false);
    }

    m_mode = Mode::UpToDate;
    UpdateHeaderAppearance(m_mode);

    const QString clean_cur = CleanVersion(current_version.isEmpty() ? QString::fromUtf8(Common::g_build_version) : current_version);
    m_uptodate_badge->setText(QStringLiteral("STORM SWITCH %1").arg(clean_cur));

    if (!source_mirror.isEmpty()) {
        m_uptodate_mirror->setText(
            Tr12("Источник проверки: %1", "Checked via: %1", "Geprüft über: %1", "Fuente: %1", "Source : %1", "Sumber: %1", "Fonte: %1", "確認元: %1", "확인 출처: %1", "Fonte: %1", "检查来源: %1", "स्रोत: %1").arg(source_mirror));
        m_uptodate_mirror->setVisible(true);
    } else {
        m_uptodate_mirror->setVisible(false);
    }

    ActivatePage(2);
}

void UpdateDialog::SetError(const QString& error_message, const QString& release_page_url) {
    AbortActiveDownload();
    if (auto* spinner = static_cast<RotatingSpinnerWidget*>(m_checking_spinner)) {
        spinner->SetActive(false);
    }

    m_mode = Mode::Error;
    UpdateHeaderAppearance(m_mode);

    if (!release_page_url.isEmpty()) {
        m_info.release_page_url = release_page_url;
    }

    const QString clean_err = SanitizeText(error_message).trimmed();
    m_error_label->setText(clean_err.isEmpty()
        ? Tr12("Не удалось подключиться к серверам обновлений.", "Could not reach update servers.", "Update-Server nicht erreichbar.", "No se pudo conectar con los servidores de actualización.", "Impossible de joindre les serveurs de mise à jour.", "Tidak dapat terhubung ke server pembaruan.", "Impossibile raggiungere i server di aggiornamento.", "アップデートサーバーに接続できませんでした。", "업데이트 서버에 연결할 수 없습니다.", "Não foi possível conectar aos servidores de atualização.", "无法连接到更新服务器。", "अपडेट सर्वर से कनेक्ट नहीं हो सका।")
        : clean_err);

    ActivatePage(3);
}

void UpdateDialog::PopulateReleaseNotes(const QString& markdown) {
    while (QLayoutItem* item = m_notes_layout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    const QColor border_c(pal.border);
    QString active_font = StormDesign::ResolveActiveUiFontFamily();
    active_font.replace(QLatin1Char('"'), QString());

    const QString active_ui_lang = QString::fromStdString(UISettings::values.language.GetValue());
    QString normalized = SelectLocalizedReleaseNotes(SanitizeText(markdown), active_ui_lang, QLocale());
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    static const QRegularExpression html_br(QStringLiteral("<br\\s*/?>"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression html_tags(QStringLiteral("</?(details|summary|b|i|strong|em|p)>"),
                                              QRegularExpression::CaseInsensitiveOption);
    normalized.replace(html_br, QStringLiteral("\n"));
    normalized.remove(html_tags);

    QStringList lines = normalized.split(QLatin1Char('\n'));
    bool has_content = false;

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) {
            if (has_content) {
                auto* sp = new QWidget(m_notes_container);
                sp->setFixedHeight(4);
                m_notes_layout->addWidget(sp);
            }
            continue;
        }

        if (line.startsWith(QLatin1Char('>'))) {
            line = line.mid(1).trimmed();
            if (line.isEmpty()) {
                continue;
            }
        }

        if (line == QStringLiteral("---") || line == QStringLiteral("***") || line == QStringLiteral("___")) {
            auto* rule = new QFrame(m_notes_container);
            rule->setFrameShape(QFrame::NoFrame);
            rule->setFixedHeight(1);
            rule->setStyleSheet(QStringLiteral("background-color:%1;").arg(pal.border));
            m_notes_layout->addWidget(rule);
            has_content = true;
            continue;
        }

        if (line.startsWith(QLatin1Char('#'))) {
            int level = 0;
            while (level < line.size() && line[level] == QLatin1Char('#')) {
                ++level;
            }
            const QString heading = line.mid(level).trimmed();
            if (!heading.isEmpty()) {
                auto* row_w = new QWidget(m_notes_container);
                row_w->setStyleSheet(QStringLiteral("background:transparent;"));
                auto* row = new QHBoxLayout(row_w);
                row->setContentsMargins(0, 3, 4, 1);
                row->setSpacing(8);

                auto* bar = new QFrame(row_w);
                bar->setFixedSize(3, 14);
                bar->setStyleSheet(QStringLiteral("background-color:%1;border-radius:2px;").arg(pal.accent));

                auto* lbl = new QLabel(row_w);
                lbl->setTextFormat(Qt::RichText);
                lbl->setWordWrap(true);
                lbl->setText(BuildInlineRichHtml(heading, pal.textPrimary, pal.textPrimary, pal.accent, true));
                lbl->setStyleSheet(QStringLiteral("background:transparent;font-family:\"%1\",sans-serif;font-size:%2px;")
                                       .arg(active_font)
                                       .arg(level <= 2 ? 13 : 12));

                row->addWidget(bar, 0, Qt::AlignVCenter);
                row->addWidget(lbl, 1);
                m_notes_layout->addWidget(row_w);
                has_content = true;
            }
            continue;
        }

        if (line.startsWith(QStringLiteral("- ")) || line.startsWith(QStringLiteral("* "))) {
            const QString content = line.mid(2).trimmed();
            if (!content.isEmpty()) {
                auto* row_w = new QWidget(m_notes_container);
                row_w->setStyleSheet(QStringLiteral("background:transparent;"));
                auto* row = new QHBoxLayout(row_w);
                row->setContentsMargins(2, 0, 4, 0);
                row->setSpacing(8);

                auto* bullet = new QLabel(QStringLiteral("-"), row_w);
                bullet->setFixedWidth(10);
                bullet->setStyleSheet(QStringLiteral("background:transparent;font-family:'Cascadia Mono','Consolas',monospace;font-weight:700;font-size:12px;color:%1;")
                                          .arg(pal.accent));

                auto* lbl = new QLabel(row_w);
                lbl->setTextFormat(Qt::RichText);
                lbl->setWordWrap(true);
                lbl->setText(BuildInlineRichHtml(content, pal.textSecondary, pal.textPrimary, pal.accent, false));
                lbl->setStyleSheet(QStringLiteral("background:transparent;font-family:\"%1\",sans-serif;font-size:12px;").arg(active_font));

                row->addWidget(bullet, 0, Qt::AlignTop);
                row->addWidget(lbl, 1);
                m_notes_layout->addWidget(row_w);
                has_content = true;
            }
            continue;
        }

        if (line.startsWith(QLatin1Char('|')) && line.endsWith(QLatin1Char('|'))) {
            const QString inner = line.mid(1, line.size() - 2);
            QStringList cells;
            bool is_align_row = true;
            for (const QString& raw_cell : inner.split(QLatin1Char('|'))) {
                const QString c = raw_cell.trimmed();
                if (!c.isEmpty()) {
                    cells.push_back(c);
                }
                if (c.isEmpty()) {
                    is_align_row = false;
                } else {
                    for (QChar ch : c) {
                        if (ch != QLatin1Char('-') && ch != QLatin1Char(':') && ch != QLatin1Char(' ')) {
                            is_align_row = false;
                            break;
                        }
                    }
                }
            }
            if (!is_align_row && !cells.isEmpty()) {
                auto* pill = new QLabel(cells.join(QStringLiteral(" | ")), m_notes_container);
                pill->setWordWrap(true);
                pill->setStyleSheet(QStringLiteral(
                    "background-color:%1;border-radius:7px;padding:4px 8px;"
                    "font-family:'Cascadia Mono','Consolas',monospace;font-size:11px;color:%2;")
                    .arg(ColorRgba(border_c, 65), pal.textSecondary));
                m_notes_layout->addWidget(pill);
                has_content = true;
            }
            continue;
        }

        auto* para = new QLabel(m_notes_container);
        para->setTextFormat(Qt::RichText);
        para->setWordWrap(true);
        para->setText(BuildInlineRichHtml(line, pal.textSecondary, pal.textPrimary, pal.accent, false));
        para->setStyleSheet(QStringLiteral("background:transparent;font-family:\"%1\",sans-serif;font-size:12px;").arg(active_font));
        m_notes_layout->addWidget(para);
        has_content = true;
    }

    if (!has_content) {
        auto* empty_lbl = new QLabel(
            Tr12("Описание изменений для этого релиза не указано.", "No changelog provided for this release.", "Keine Änderungsbeschreibung für dieses Release angegeben.", "No se proporcionaron notas de cambios para esta versión.", "Aucune note de version fournie.", "Tidak ada catatan perubahan untuk rilis ini.", "Nessuna nota di rilascio specificata.", "このリリースの変更履歴はありません。", "이 릴리스에 대한 변경 내역이 없습니다.", "Nenhuma nota de lançamento informada.", "此版本未提供更新说明。", "इस रिलीज़ के लिए कोई परिवर्तन विवरण नहीं दिया गया है।"),
            m_notes_container);
        empty_lbl->setWordWrap(true);
        empty_lbl->setStyleSheet(QStringLiteral("color:%1;font-size:12px;").arg(pal.textSecondary));
        m_notes_layout->addWidget(empty_lbl);
    }

    m_notes_layout->addStretch(1);
}

void UpdateDialog::SetDownloadUiActive(bool active) {
    m_progress_card->setVisible(active);
    m_actions_container->setVisible(!active);
    if (active) {
        m_progress_bar->setValue(0);
        m_progress_status->setText(Tr12("Загрузка обновления...", "Downloading update...", "Update wird heruntergeladen...", "Descargando actualización...", "Téléchargement de la mise à jour...", "Mengunduh pembaruan...", "Download dell'aggiornamento...", "アップデートをダウンロード中...", "업데이트 다운로드 중...", "Baixando atualização...", "正在下载更新...", "अपडेट डाउनलोड हो रहा है..."));
        m_progress_bytes->setText(Tr12("Подготовка загрузки...", "Preparing download...", "Download wird vorbereitet...", "Preparando descarga...", "Préparation du téléchargement...", "Menyiapkan unduhan...", "Preparazione download...", "ダウンロードを準備中...", "다운로드 준비 중...", "Preparando download...", "正在准备下载...", "डाउनलोड तैयार हो रहा है..."));
        m_progress_speed->setText(QString());
        m_progress_percent->setText(QStringLiteral("0%"));
        m_cancel_dl_btn->setEnabled(true);
    }
    ActivatePage(m_stack->currentIndex());
}

void UpdateDialog::StartDownload() {
    if (m_info.download_urls.isEmpty()) {
        const QString target = m_info.release_page_url.isEmpty()
            ? QString::fromLatin1(kFallbackReleaseUrl)
            : m_info.release_page_url;
        QDesktopServices::openUrl(QUrl(target));
        accept();
        return;
    }

    if (!m_info.new_version.isEmpty() && GetSkippedVersion() == m_info.new_version) {
        ClearSkippedVersion();
    }

    m_download_cancelled = false;
    m_installing = false;
    SetDownloadUiActive(true);
    DownloadFromMirrorIndex(0);
}

void UpdateDialog::CancelDownload() {
    if (m_installing) {
        return;
    }
    m_download_cancelled = true;
    AbortActiveDownload();
    SetDownloadUiActive(false);
}

void UpdateDialog::AbortActiveDownload() {
    if (m_active_reply) {
        QNetworkReply* reply = m_active_reply.data();
        m_active_reply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}

void UpdateDialog::DownloadFromMirrorIndex(int mirror_index) {
    if (m_download_cancelled) {
        return;
    }
    if (mirror_index >= m_info.download_urls.size()) {
        SetDownloadUiActive(false);
        SetError(Tr12("Не удалось скачать или проверить целостность пакета обновления ни с одного зеркала.",
                      "Failed to download or verify update package from all mirrors.",
                      "Update-Paket konnte von keinem Mirror heruntergeladen oder verifiziert werden.",
                      "No se pudo descargar ni verificar el paquete de actualización desde ningún espejo.",
                      "Impossible de télécharger ou vérifier le paquet depuis les miroirs.",
                      "Gagal mengunduh atau memverifikasi paket pembaruan dari semua cermin.",
                      "Impossibile scaricare o verificare il pacchetto di aggiornamento dai mirror.",
                      "すべてのミラーからアップデートをダウンロードまたは検証できませんでした。",
                      "모든 미러에서 업데이트 패키지를 다운로드하거나 검증하지 못했습니다.",
                      "Falha ao baixar ou verificar o pacote de atualização dos espelhos.",
                      "无法从任何镜像下载或验证更新包。",
                      "किसी भी मिरर से अपडेट पैकेज डाउनलोड या सत्यापित नहीं किया जा सका।"),
            m_info.release_page_url);
        return;
    }

    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }

    const QString url = m_info.download_urls.at(mirror_index);
    const QString mirror_name = url.contains(QStringLiteral("stormgamesworld.ru"), Qt::CaseInsensitive)
        ? QStringLiteral("STORM GAMES WORLD")
        : QStringLiteral("GitHub");

    m_progress_status->setText(Tr12("Загрузка обновления (%1)...", "Downloading update (%1)...", "Update wird heruntergeladen (%1)...", "Descargando actualización (%1)...", "Téléchargement (%1)...", "Mengunduh pembaruan (%1)...", "Download (%1)...", "ダウンロード中 (%1)...", "업데이트 다운로드 중 (%1)...", "Baixando atualização (%1)...", "正在下载更新 (%1)...", "अपडेट डाउनलोड (%1)...").arg(mirror_name));

    const QString temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                             QStringLiteral("/storm_switch_update_") +
                             (m_info.new_version.isEmpty() ? QStringLiteral("latest") : m_info.new_version);
    QDir(temp_dir).removeRecursively();
    QDir().mkpath(temp_dir);

    const bool expects_exe = url.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive);
    const QString pkg_path = temp_dir + (expects_exe ? QStringLiteral("/update_setup.exe")
                                                     : QStringLiteral("/update.zip"));

    auto* out_file = new QFile(pkg_path, this);
    if (!out_file->open(QIODevice::WriteOnly)) {
        out_file->deleteLater();
        DownloadFromMirrorIndex(mirror_index + 1);
        return;
    }

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH-Updater"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_nam->get(req);
    m_active_reply = reply;

    struct SpeedState {
        QElapsedTimer timer;
        qint64 last_ms{0};
        qint64 last_bytes{0};
        double smoothed_mb_s{0.0};
    };
    auto speed = std::make_shared<SpeedState>();
    speed->timer.start();

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, out_file]() {
        if (m_download_cancelled) {
            return;
        }
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty()) {
            out_file->write(chunk);
        }
    });

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, speed](qint64 received, qint64 total) {
        if (m_download_cancelled) {
            return;
        }
        const qint64 effective_total = total > 0 ? total : m_info.package_size_bytes;
        int pct = 0;
        if (effective_total > 0) {
            pct = static_cast<int>(qBound(qint64{0}, (received * 100) / effective_total, qint64{100}));
        }
        m_progress_bar->setValue(pct);
        m_progress_percent->setText(QStringLiteral("%1%").arg(pct));

        const QString rec_str = FormatBytes(received);
        if (effective_total > 0) {
            m_progress_bytes->setText(QStringLiteral("%1 / %2").arg(rec_str, FormatBytes(effective_total)));
        } else if (!rec_str.isEmpty()) {
            m_progress_bytes->setText(rec_str);
        }

        const qint64 now_ms = speed->timer.elapsed();
        const qint64 dt = now_ms - speed->last_ms;
        if (dt >= 180) {
            const qint64 db = received - speed->last_bytes;
            if (db >= 0) {
                const double inst_mb_s = (static_cast<double>(db) / (1024.0 * 1024.0)) /
                                         (static_cast<double>(dt) / 1000.0);
                speed->smoothed_mb_s = (speed->smoothed_mb_s <= 0.01)
                    ? inst_mb_s
                    : (speed->smoothed_mb_s * 0.65 + inst_mb_s * 0.35);
                if (speed->smoothed_mb_s > 0.01) {
                    m_progress_speed->setText(QStringLiteral("%1 %2")
                        .arg(QString::number(speed->smoothed_mb_s, 'f', 1),
                             Tr12("МБ/с", "MB/s", "MB/s", "MB/s", "Mo/s", "MB/s", "MB/s", "MB/s",
                                  "MB/s", "MB/s", "MB/s", "MB/s")));
                }
            }
            speed->last_ms = now_ms;
            speed->last_bytes = received;
        }
    });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, out_file, temp_dir, pkg_path, url, mirror_index]() {
        if (m_active_reply == reply) {
            m_active_reply = nullptr;
        }

        if (out_file->isOpen()) {
            const QByteArray rem = reply->readAll();
            if (!rem.isEmpty()) {
                out_file->write(rem);
            }
            out_file->flush();
            out_file->close();
        }
        out_file->deleteLater();

        const auto net_err = reply->error();
        reply->deleteLater();

        if (m_download_cancelled) {
            QDir(temp_dir).removeRecursively();
            return;
        }

        if (net_err != QNetworkReply::NoError || QFileInfo(pkg_path).size() < 1024) {
            QDir(temp_dir).removeRecursively();
            DownloadFromMirrorIndex(mirror_index + 1);
            return;
        }

        FinalizeDownloadedPackage(temp_dir, pkg_path, url, mirror_index);
    });
}

void UpdateDialog::FinalizeDownloadedPackage(const QString& temp_dir, const QString& package_path,
                                             const QString& source_url, int mirror_index) {
    QFile pkg_file(package_path);
    bool is_mz = false;
    bool is_pk = false;
    if (pkg_file.open(QIODevice::ReadOnly)) {
        const QByteArray magic = pkg_file.read(4);
        is_mz = (magic.size() >= 2 && magic[0] == 'M' && magic[1] == 'Z');
        is_pk = (magic.size() >= 4 && magic[0] == 'P' && magic[1] == 'K' && magic[2] == 0x03 && magic[3] == 0x04);
        pkg_file.close();
    }

    if (!is_mz && !is_pk) {
        QDir(temp_dir).removeRecursively();
        DownloadFromMirrorIndex(mirror_index + 1);
        return;
    }

    m_installing = true;
    m_cancel_dl_btn->setEnabled(false);
    m_progress_bar->setValue(96);
    m_progress_percent->setText(QStringLiteral("96%"));
    m_progress_status->setText(Tr12("Распаковка и проверка цифровой подписи PE (MZ)...", "Extracting and verifying PE (MZ) signature...", "Entpacken und Prüfen der PE (MZ)-Signatur...", "Extrayendo y verificando firma PE (MZ)...", "Extraction et vérification de la signature PE (MZ)...", "Mengekstrak dan memverifikasi tanda tangan PE (MZ)...", "Estrazione e verifica firma PE (MZ)...", "展開と PE (MZ) 署名を検証中...", "압축 해제 및 PE (MZ) 서명 검증 중...", "Extraindo e verificando assinatura PE (MZ)...", "正在解压并验证 PE (MZ) 签名...", "PE (MZ) हस्ताक्षर निकाला और सत्यापित किया जा रहा है..."));
    QCoreApplication::processEvents();

    const bool is_installer = is_mz ||
        source_url.contains(QStringLiteral("_Setup.exe"), Qt::CaseInsensitive) ||
        source_url.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive);

    if (is_installer && is_mz) {
        m_progress_bar->setValue(100);
        m_progress_percent->setText(QStringLiteral("100%"));
        m_progress_status->setText(Tr12("Запуск установщика...", "Launching installer...", "Installer wird gestartet...", "Iniciando instalador...", "Lancement de l'installateur...", "Menjalankan pemasang...", "Avvio programma di installazione...", "インストーラーを起動中...", "설치 관리자 실행 중...", "Iniciando instalador...", "正在启动安装程序...", "इंस्टॉलर शुरू हो रहा है..."));
        QProcess::startDetached(package_path, {});
        emit InstallReadyToQuit();
        accept();
        return;
    }

    auto run_proc_async = [](QProcess& proc, int timeout_ms) -> bool {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(timeout_ms);
        loop.exec();
        return proc.state() == QProcess::NotRunning;
    };

    const QString extract_dir = temp_dir + QStringLiteral("/extracted");
    QDir().mkpath(extract_dir);

    QProcess tar_proc;
    tar_proc.start(QStringLiteral("tar.exe"), {QStringLiteral("-xf"), package_path, QStringLiteral("-C"), extract_dir});
    if (!run_proc_async(tar_proc, 60000) || tar_proc.exitCode() != 0) {
        QProcess ps_proc;
        ps_proc.start(QStringLiteral("powershell.exe"),
                      {QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                       QStringLiteral("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
                           .arg(package_path, extract_dir)});
        run_proc_async(ps_proc, 90000);
    }

    QString staging_root = extract_dir;
    QString exe_name = QStringLiteral("STORM_SWITCH.exe");
    if (QFileInfo::exists(staging_root + QStringLiteral("/STORM SWITCH.exe"))) {
        exe_name = QStringLiteral("STORM SWITCH.exe");
    } else if (!QFileInfo::exists(staging_root + QStringLiteral("/STORM_SWITCH.exe"))) {
        QDirIterator it(extract_dir,
                        QStringList() << QStringLiteral("STORM_SWITCH.exe") << QStringLiteral("STORM SWITCH.exe"),
                        QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext()) {
            const QString found = it.next();
            staging_root = QFileInfo(found).absolutePath();
            exe_name = QFileInfo(found).fileName();
        }
    }

    QFile pe_check(staging_root + QStringLiteral("/") + exe_name);
    bool valid_pe = false;
    if (pe_check.open(QIODevice::ReadOnly)) {
        const QByteArray magic = pe_check.read(2);
        valid_pe = (magic.size() == 2 && magic[0] == 'M' && magic[1] == 'Z');
        pe_check.close();
    }

    if (!valid_pe) {
        m_installing = false;
        QDir(temp_dir).removeRecursively();
        DownloadFromMirrorIndex(mirror_index + 1);
        return;
    }

    const qint64 app_pid = QCoreApplication::applicationPid();
    const QString app_dir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    const QString app_exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString native_stage = QDir::toNativeSeparators(staging_root);
    const QString native_temp_dir = QDir::toNativeSeparators(temp_dir);
    const QString bat_path = QDir::toNativeSeparators(temp_dir + QStringLiteral("/apply_update.bat"));

    QFile bat_file(bat_path);
    if (bat_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&bat_file);
        ts << "@echo off\n";
        ts << "chcp 65001 >nul\n";
        ts << "timeout /t 2 /nobreak >nul\n";
        ts << "taskkill /F /PID " << app_pid << " >nul 2>&1\n";
        ts << "set RETRY_COUNT=0\n";
        ts << ":RETRY_COPY\n";
        ts << "xcopy /E /Y /I /R \"" << native_stage << "\\*\" \"" << app_dir << "\\\" >nul 2>&1\n";
        ts << "if errorlevel 1 (\n";
        ts << "    set /A RETRY_COUNT+=1\n";
        ts << "    if %RETRY_COUNT% LEQ 10 (\n";
        ts << "        timeout /t 1 /nobreak >nul\n";
        ts << "        goto RETRY_COPY\n";
        ts << "    )\n";
        ts << ")\n";
        ts << "start \"\" \"" << app_exe << "\"\n";
        ts << "rmdir /S /Q \"" << native_temp_dir << "\" >nul 2>&1\n";
        ts << "exit\n";
        bat_file.close();
    }

    m_progress_bar->setValue(100);
    m_progress_percent->setText(QStringLiteral("100%"));
    QProcess::startDetached(QStringLiteral("cmd.exe"), {QStringLiteral("/c"), bat_path});
    emit InstallReadyToQuit();
    accept();
}

void UpdateDialog::CenterOverParent() {
    QWidget* p = parentWidget();
    QRect target_rect;
    if (p && p->isVisible()) {
        target_rect = p->frameGeometry();
    } else if (QScreen* scr = QApplication::screenAt(QCursor::pos())) {
        target_rect = scr->availableGeometry();
    } else if (QScreen* primary = QApplication::primaryScreen()) {
        target_rect = primary->availableGeometry();
    }
    if (!target_rect.isValid()) {
        return;
    }
    const QSize sz = sizeHint().expandedTo(size());
    const int x = target_rect.x() + (target_rect.width() - sz.width()) / 2;
    const int y = target_rect.y() + (target_rect.height() - sz.height()) / 2;
    move(x, y);
}

void UpdateDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    ActivatePage(m_stack->currentIndex());
}

void UpdateDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_header_widget &&
        m_header_widget->geometry().contains(event->position().toPoint())) {
        m_dragging = true;
        m_drag_offset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void UpdateDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_drag_offset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void UpdateDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

void UpdateDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_installing) {
            event->accept();
            return;
        }
        AbortActiveDownload();
    }
    QDialog::keyPressEvent(event);
}
