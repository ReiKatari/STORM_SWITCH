// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storm_switch/log_viewer_dialog.h"

#include <QButtonGroup>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QUrl>
#include <QVBoxLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"

class LogSyntaxHighlighter : public QSyntaxHighlighter {
public:
    explicit LogSyntaxHighlighter(QTextDocument* parent = nullptr)
        : QSyntaxHighlighter(parent) {
        fmt_error.setForeground(QColor(QStringLiteral("#FF5555")));
        fmt_error.setFontWeight(QFont::Bold);

        fmt_critical.setForeground(QColor(QStringLiteral("#FF3366")));
        fmt_critical.setFontWeight(QFont::Bold);

        fmt_warning.setForeground(QColor(QStringLiteral("#FFAA00")));
        fmt_warning.setFontWeight(QFont::DemiBold);

        fmt_info.setForeground(QColor(QStringLiteral("#00F0FF")));

        fmt_debug.setForeground(QColor(QStringLiteral("#6B7D96")));

        fmt_component.setForeground(QColor(QStringLiteral("#C084FC")));
        fmt_component.setFontWeight(QFont::Medium);
    }

protected:
    void highlightBlock(const QString& text) override {
        if (text.contains(QLatin1String("<Critical>"), Qt::CaseInsensitive)) {
            setFormat(0, text.length(), fmt_critical);
        } else if (text.contains(QLatin1String("<Error>"), Qt::CaseInsensitive)) {
            setFormat(0, text.length(), fmt_error);
        } else if (text.contains(QLatin1String("<Warning>"), Qt::CaseInsensitive)) {
            setFormat(0, text.length(), fmt_warning);
        } else if (text.contains(QLatin1String("<Info>"), Qt::CaseInsensitive)) {
            setFormat(0, text.length(), fmt_info);
        } else if (text.contains(QLatin1String("<Debug>"), Qt::CaseInsensitive) ||
                   text.contains(QLatin1String("<Trace>"), Qt::CaseInsensitive)) {
            setFormat(0, text.length(), fmt_debug);
        }

        // Highlight component name if present: [Component.Sub]
        int comp_start = text.indexOf(QLatin1Char('['));
        if (comp_start != -1) {
            // Find second opening bracket if first is timestamp
            int second_bracket = text.indexOf(QLatin1Char('['), comp_start + 1);
            if (second_bracket != -1) {
                int comp_end = text.indexOf(QLatin1Char(']'), second_bracket);
                if (comp_end != -1 && comp_end > second_bracket) {
                    setFormat(second_bracket, comp_end - second_bracket + 1, fmt_component);
                }
            }
        }
    }

private:
    QTextCharFormat fmt_error;
    QTextCharFormat fmt_critical;
    QTextCharFormat fmt_warning;
    QTextCharFormat fmt_info;
    QTextCharFormat fmt_debug;
    QTextCharFormat fmt_component;
};

LogViewerDialog::LogViewerDialog(QWidget* parent)
    : QDialog(parent) {
    SetupUI();
    ApplyStormStyles();
    LoadLogFile();
}

LogViewerDialog::~LogViewerDialog() = default;

QString LogViewerDialog::FindActiveLogPath() const {
    const auto log_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LogDir);
    const std::vector<std::string> names = {
        "storm_switch.txt",
        "storm_switch_log.txt",
        "eden_log.txt",
        "yuzu_log.txt",
    };

    for (const auto& name : names) {
        const auto path = log_dir / name;
        if (std::filesystem::exists(path) && std::filesystem::file_size(path) > 0) {
            return QString::fromStdString(Common::FS::PathToUTF8String(path));
        }
    }

    return QString::fromStdString(Common::FS::PathToUTF8String(log_dir / "yuzu_log.txt"));
}

void LogViewerDialog::SetupUI() {
    setWindowTitle(tr("📜 Журнал работы (Логи) — STORM SWITCH"));
    resize(1100, 720);
    setMinimumSize(860, 560);

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(14, 14, 14, 14);
    root_layout->setSpacing(10);

    // --- Header Section ---
    auto* header_layout = new QHBoxLayout();
    header_layout->setSpacing(12);

    auto* icon_label = new QLabel(QStringLiteral("📋"), this);
    icon_label->setStyleSheet(QStringLiteral("font-size: 26px;"));
    header_layout->addWidget(icon_label);

    auto* title_box = new QVBoxLayout();
    title_box->setSpacing(2);

    title_label = new QLabel(tr("Журнал работы и диагностики эмулятора"), this);
    title_label->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold; color: #FFFFFF;"));
    title_box->addWidget(title_label);

    stats_label = new QLabel(tr("Загрузка записей журнала..."), this);
    stats_label->setStyleSheet(QStringLiteral("color: #7090B0; font-size: 11px;"));
    title_box->addWidget(stats_label);

    header_layout->addLayout(title_box, 1);
    root_layout->addLayout(header_layout);

    // --- Filters and Search Bar ---
    auto* filter_bar = new QHBoxLayout();
    filter_bar->setSpacing(10);

    // Radio button filter group
    filter_group = new QButtonGroup(this);

    radio_all = new QRadioButton(tr("Все"), this);
    radio_all->setChecked(true);
    filter_group->addButton(radio_all, FilterAll);
    filter_bar->addWidget(radio_all);

    radio_errors = new QRadioButton(tr("Ошибки"), this);
    filter_group->addButton(radio_errors, FilterErrors);
    filter_bar->addWidget(radio_errors);

    radio_warnings = new QRadioButton(tr("Предупреждения"), this);
    filter_group->addButton(radio_warnings, FilterWarnings);
    filter_bar->addWidget(radio_warnings);

    radio_info = new QRadioButton(tr("Инфо"), this);
    filter_group->addButton(radio_info, FilterInfo);
    filter_bar->addWidget(radio_info);

    filter_bar->addSpacing(14);

    search_edit = new QLineEdit(this);
    search_edit->setPlaceholderText(tr("🔍 Поиск по тексту журнала..."));
    search_edit->setClearButtonEnabled(true);
    filter_bar->addWidget(search_edit, 1);

    root_layout->addLayout(filter_bar);

    // --- Log Text View ---
    log_view = new QPlainTextEdit(this);
    log_view->setReadOnly(true);
    log_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont font(QStringLiteral("Consolas, Courier New, monospace"));
    font.setPointSize(10);
    log_view->setFont(font);

    new LogSyntaxHighlighter(log_view->document());
    root_layout->addWidget(log_view, 1);

    // --- Action Buttons Bar ---
    auto* bottom_bar = new QHBoxLayout();
    bottom_bar->setSpacing(10);

    refresh_btn = new QPushButton(tr("🔄 Обновить"), this);
    refresh_btn->setCursor(Qt::PointingHandCursor);
    bottom_bar->addWidget(refresh_btn);

    copy_btn = new QPushButton(tr("📋 Скопировать"), this);
    copy_btn->setCursor(Qt::PointingHandCursor);
    bottom_bar->addWidget(copy_btn);

    export_btn = new QPushButton(tr("💾 Экспорт в файл..."), this);
    export_btn->setCursor(Qt::PointingHandCursor);
    bottom_bar->addWidget(export_btn);

    open_folder_btn = new QPushButton(tr("📁 Открыть папку логов"), this);
    open_folder_btn->setCursor(Qt::PointingHandCursor);
    bottom_bar->addWidget(open_folder_btn);

    bottom_bar->addStretch(1);

    close_btn = new QPushButton(tr("Закрыть"), this);
    close_btn->setCursor(Qt::PointingHandCursor);
    bottom_bar->addWidget(close_btn);

    root_layout->addLayout(bottom_bar);

    // Connections
    connect(filter_group, &QButtonGroup::idClicked, this, &LogViewerDialog::OnFilterChanged);
    connect(search_edit, &QLineEdit::textChanged, this, &LogViewerDialog::OnSearchTextChanged);
    connect(refresh_btn, &QPushButton::clicked, this, &LogViewerDialog::OnRefreshLog);
    connect(copy_btn, &QPushButton::clicked, this, &LogViewerDialog::OnCopyLog);
    connect(export_btn, &QPushButton::clicked, this, &LogViewerDialog::OnExportLog);
    connect(open_folder_btn, &QPushButton::clicked, this, &LogViewerDialog::OnOpenLogFolder);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
}

void LogViewerDialog::ApplyStormStyles() {
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0A0E17; color: #E0E8F0; font-family: 'Segoe UI', sans-serif; }"
        "QRadioButton { color: #B0C4DE; font-size: 12px; font-weight: bold; spacing: 6px; }"
        "QRadioButton:hover { color: #00F0FF; }"
        "QRadioButton:checked { color: #00F0FF; }"
        "QRadioButton::indicator { width: 14px; height: 14px; border-radius: 7px; border: 2px solid #20354E; background: #0E1422; }"
        "QRadioButton::indicator:checked { border-color: #00D2FF; background: #00D2FF; }"
        "QLineEdit { background: rgba(255, 255, 255, 0.05); border: 1px solid #20354E; border-radius: 6px; padding: 6px 12px; color: #FFFFFF; font-size: 12px; }"
        "QLineEdit:focus { border: 1px solid #00D2FF; background: rgba(0, 210, 255, 0.08); }"
        "QPlainTextEdit { background-color: #050811; border: 1px solid #1A2638; border-radius: 8px; color: #E0E8F0; padding: 8px; }"
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1A2434, stop:1 #131A26); border: 1px solid #2A3B52; color: #E0E8F0; padding: 7px 16px; border-radius: 6px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #25344C, stop:1 #1A2638); border-color: #00D2FF; color: #FFFFFF; }"
        "QPushButton:pressed { background: #00D2FF; color: #000000; border-color: #38BDF8; }"
        "QScrollBar:vertical { background: #0A0E17; width: 12px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #1E2D42; min-height: 24px; border-radius: 6px; }"
        "QScrollBar::handle:vertical:hover { background: #00D2FF; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: #0A0E17; height: 12px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: #1E2D42; min-width: 24px; border-radius: 6px; }"
        "QScrollBar::handle:horizontal:hover { background: #00D2FF; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
    ));

    // Volumetric shadow for action buttons
    auto* refresh_shadow = new QGraphicsDropShadowEffect(refresh_btn);
    refresh_shadow->setBlurRadius(8);
    refresh_shadow->setColor(QColor(0, 210, 255, 80));
    refresh_shadow->setOffset(0, 2);
    refresh_btn->setGraphicsEffect(refresh_shadow);
}

void LogViewerDialog::LoadLogFile() {
    active_log_path = FindActiveLogPath();
    raw_lines.clear();

    QFile file(active_log_path);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        while (!in.atEnd()) {
            raw_lines.append(in.readLine());
        }
        file.close();

        // Limit to last 3500 lines for maximum responsiveness
        if (raw_lines.size() > 3500) {
            raw_lines = raw_lines.mid(raw_lines.size() - 3500);
        }
    }

    ApplyFilterAndSearch();
}

void LogViewerDialog::ApplyFilterAndSearch() {
    const QString query = search_edit ? search_edit->text().trimmed() : QString();
    QStringList filtered;
    filtered.reserve(raw_lines.size());

    for (const auto& line : raw_lines) {
        bool matches_category = true;
        switch (current_filter) {
        case FilterErrors:
            matches_category = line.contains(QLatin1String("<Error>"), Qt::CaseInsensitive) ||
                               line.contains(QLatin1String("<Critical>"), Qt::CaseInsensitive);
            break;
        case FilterWarnings:
            matches_category = line.contains(QLatin1String("<Warning>"), Qt::CaseInsensitive);
            break;
        case FilterInfo:
            matches_category = line.contains(QLatin1String("<Info>"), Qt::CaseInsensitive);
            break;
        case FilterAll:
        default:
            matches_category = true;
            break;
        }

        if (!matches_category) continue;

        if (!query.isEmpty() && !line.contains(query, Qt::CaseInsensitive)) {
            continue;
        }

        filtered.append(line);
    }

    log_view->setPlainText(filtered.join(QLatin1Char('\n')));

    // Scroll to end
    log_view->verticalScrollBar()->setValue(log_view->verticalScrollBar()->maximum());

    QFileInfo fi(active_log_path);
    const qint64 size_kb = fi.exists() ? (fi.size() / 1024) : 0;

    stats_label->setText(tr("Файл: %1 | Строк: %2 (отфильтровано: %3) | Размер: %4 КБ")
        .arg(fi.fileName())
        .arg(raw_lines.size())
        .arg(filtered.size())
        .arg(size_kb));
}

void LogViewerDialog::OnFilterChanged(int id) {
    current_filter = static_cast<FilterType>(id);
    ApplyFilterAndSearch();
}

void LogViewerDialog::OnSearchTextChanged(const QString& /*text*/) {
    ApplyFilterAndSearch();
}

void LogViewerDialog::OnRefreshLog() {
    LoadLogFile();
}

void LogViewerDialog::OnCopyLog() {
    const QString text = log_view->toPlainText();
    if (text.isEmpty()) return;

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text);
        stats_label->setText(tr("✅ Содержимое журнала скопировано в буфер обмена!"));
    }
}

void LogViewerDialog::OnExportLog() {
    const QString dest = QFileDialog::getSaveFileName(
        this,
        tr("Экспорт журнала работы"),
        QDir::homePath() + QStringLiteral("/storm_switch_log.txt"),
        tr("Текстовые файлы (*.txt);;Все файлы (*.*)")
    );

    if (dest.isEmpty()) return;

    QFile file(dest);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << log_view->toPlainText();
        file.close();

        QMessageBox::information(this, tr("Экспорт завершен"),
            tr("Журнал успешно экспортирован в файл:\n%1").arg(dest));
    } else {
        QMessageBox::warning(this, tr("Ошибка экспорта"),
            tr("Не удалось сохранить файл журнала:\n%1").arg(dest));
    }
}

void LogViewerDialog::OnOpenLogFolder() {
    const auto log_dir = Common::FS::GetEdenPath(Common::FS::EdenPath::LogDir);
    const QString dir_path = QString::fromStdString(Common::FS::PathToUTF8String(log_dir));
    if (QDir(dir_path).exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir_path));
    }
}
