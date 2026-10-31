// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIcon>
#include <fmt/ranges.h>
#include "common/scm_rev.h"
#include "ui_aboutdialog.h"
#include "storm_switch/about_dialog.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()} {
    static const std::string build_id = std::string{Common::g_build_id};
    static const std::string yuzu_build =
        fmt::format("{} {}", std::string{Common::g_build_name}, std::string{Common::g_build_version});

    const auto override_build =
        fmt::format(fmt::runtime(std::string(Common::g_title_bar_format_idle)), build_id);
    const auto yuzu_build_version = override_build.empty() ? yuzu_build : override_build;

    ui->setupUi(this);
    const QIcon app_logo = QApplication::windowIcon();
    if (!app_logo.isNull()) {
        ui->labelLogo->setPixmap(app_logo.pixmap(180, 180));
    }

    const StormTheme::Palette pal = StormTheme::GetThemePalette();
    setStyleSheet(StormTheme::GetDialogStyle(pal));

    setWindowTitle(StormLangStorm11(
        QStringLiteral("О программе — STORM SWITCH"),
        QStringLiteral("About — STORM SWITCH"),
        QStringLiteral("Über — STORM SWITCH"),
        QStringLiteral("À propos — STORM SWITCH"),
        QStringLiteral("关于 — STORM SWITCH"),
        QStringLiteral("정보 — STORM SWITCH"),
        QStringLiteral("バージョン情報 — STORM SWITCH"),
        QStringLiteral("Acerca de — STORM SWITCH"),
        QStringLiteral("Informazioni — STORM SWITCH"),
        QStringLiteral("Sobre — STORM SWITCH"),
        QStringLiteral("के बारे में — STORM SWITCH")
    ));

    ui->labelBuildInfo->setText(
        ui->labelBuildInfo->text().arg(QString::fromStdString(yuzu_build_version),
                                       QString::fromUtf8(Common::g_build_date).left(10)));

    const QString about_body = StormLangStorm11(
        QStringLiteral("STORM SWITCH — это передовой высокопроизводительный форк эмулятора Eden (Nintendo Switch), созданный для максимальной производительности, глубокой кастомизации и стабильности.<br/><br/>"
                       "STORM SWITCH распространяется под лицензией GPLv3.0+ и включает расширенные режимы графики, ускоренный запуск игр, многофункциональный подвал управления и полную поддержку DLC и обновлений.<br/><br/>"
                       "Данное программное обеспечение предназначено исключительно для запуска легально полученных копий игр."),
        QStringLiteral("STORM SWITCH is an advanced high-performance fork of the Eden emulator (Nintendo Switch), built for maximum performance, deep customization, and stability.<br/><br/>"
                       "STORM SWITCH is distributed under the GPLv3.0+ license and includes enhanced graphics modes, accelerated game launch, a multi-functional control dock, and full DLC and update support.<br/><br/>"
                       "This software is intended exclusively for running legally obtained copies of games."),
        QStringLiteral("STORM SWITCH ist ein fortschrittlicher Hochleistungs-Fork des Eden-Emulators (Nintendo Switch), der für maximale Performance, tiefe Anpassbarkeit und Stabilität entwickelt wurde.<br/><br/>"
                       "STORM SWITCH steht unter der GPLv3.0+-Lizenz und bietet erweiterte Grafikmodi, beschleunigten Spielstart, eine multifunktionale Steuerungsleiste sowie volle DLC- und Update-Unterstützung.<br/><br/>"
                       "Diese Software ist ausschließlich für den Betrieb legal erworbener Spielkopien bestimmt."),
        QStringLiteral("STORM SWITCH est un fork haute performance avancé de l'émulateur Eden (Nintendo Switch), conçu pour des performances maximales, une personnalisation poussée et une stabilité accrue.<br/><br/>"
                       "STORM SWITCH est distribué sous licence GPLv3.0+ et inclut des modes graphiques étendus, un lancement accéléré des jeux, un dock de contrôle multifonction et une prise en charge complète des DLC et mises à jour.<br/><br/>"
                       "Ce logiciel est destiné exclusivement à l'exécution de copies de jeux légalement acquises."),
        QStringLiteral("STORM SWITCH 是 Eden 模拟器（Nintendo Switch）的高级高性能分支，专为极致性能、深度自定义和卓越稳定性而打造。<br/><br/>"
                       "STORM SWITCH 基于 GPLv3.0+ 许可证发布，包含增强图形模式、加速游戏启动、多功能控制底栏以及对 DLC 和更新的全面支持。<br/><br/>"
                       "本软件仅可用于运行合法获得的游戏副本。"),
        QStringLiteral("STORM SWITCH는 최고 성능, 세부 사용자 정의 및 안정성을 위해 제작된 Eden 에뮬레이터(Nintendo Switch)의 고급 고성능 포크입니다.<br/><br/>"
                       "STORM SWITCH는 GPLv3.0+ 라이선스에 따라 배포되며 확장 그래픽 모드, 빠른 게임 시작, 다기능 제어 독, DLC 및 업데이트 완벽 지원을 포함합니다.<br/><br/>"
                       "본 소프트웨어는 합법적으로 취득한 게임 사본을 실행하기 위한 용도로만 제공됩니다."),
        QStringLiteral("STORM SWITCH は、最高性能、高度なカスタマイズ性、および安定性を追求して設計された Eden エミュレータ（Nintendo Switch）の先進的な高性能フォークです。<br/><br/>"
                       "STORM SWITCH は GPLv3.0+ ライセンスの下で配布され、拡張グラフィックスモード、高速ゲーム起動、多機能コントロール下部バー、DLC およびアップデートの完全サポートを備えています。<br/><br/>"
                       "本ソフトウェアは、合法的に取得したゲームのバックアップを実行することのみを目的としています。"),
        QStringLiteral("STORM SWITCH es una bifurcación avanzada de alto rendimiento del emulador Eden (Nintendo Switch), diseñada para ofrecer el máximo rendimiento, personalización profunda y estabilidad.<br/><br/>"
                       "STORM SWITCH se distribuye bajo la licencia GPLv3.0+ e incluye modos gráficos avanzados, inicio acelerado de juegos, barra de control multifuncional y compatibilidad total con DLC y actualizaciones.<br/><br/>"
                       "Este software está destinado exclusivamente a la ejecución de copias de juegos obtenidas legalmente."),
        QStringLiteral("STORM SWITCH è un fork avanzato ad alte prestazioni dell'emulatore Eden (Nintendo Switch), progettato per offrire massime prestazioni, personalizzazione approfondita e stabilità.<br/><br/>"
                       "STORM SWITCH è distribuito con licenza GPLv3.0+ e include modalità grafiche avanzate, avvio accelerato dei giochi, dock di controllo multifunzionale e supporto completo per DLC e aggiornamenti.<br/><br/>"
                       "Questo software è destinato esclusivamente all'esecuzione di copie di giochi legalmente ottenute."),
        QStringLiteral("STORM SWITCH é uma bifurcação avançada de alto desempenho do emulador Eden (Nintendo Switch), desenvolvida para desempenho máximo, personalização profunda e estabilidade.<br/><br/>"
                       "STORM SWITCH é distribuído sob a licença GPLv3.0+ e inclui modos gráficos aprimorados, inicialização acelerada de jogos, dock de controle multifuncional e suporte completo a DLCs e atualizações.<br/><br/>"
                       "Este software destina-se exclusivamente à execução de cópias legalmente obtidas de jogos."),
        QStringLiteral("STORM SWITCH ईडन एमुलेटर (Nintendo Switch) का एक उन्नत उच्च-प्रदर्शन फ़ॉर्क है, जिसे अधिकतम प्रदर्शन, गहन अनुकूलन और स्थिरता के लिए बनाया गया है।<br/><br/>"
                       "STORM SWITCH को GPLv3.0+ लाइसेंस के तहत वितरित किया जाता है और इसमें उन्नत ग्राफिक्स मोड, त्वरित गेम लॉन्च, बहु-कार्यात्मक नियंत्रण डॉक और DLC व अपडेट का पूर्ण समर्थन शामिल है।<br/><br/>"
                       "यह सॉफ़्टवेयर केवल कानूनी रूप से प्राप्त गेम प्रतियों को चलाने के लिए है।")
    );

    ui->labelAbout->setText(QStringLiteral(
        "<html><head/><body><p><span style='font-size:10pt; color:%1;'>%2</span></p></body></html>")
        .arg(pal.textPrimary, about_body));

    const QString link_source = StormLangStorm11(
        QStringLiteral("Исходный код"), QStringLiteral("Source Code"), QStringLiteral("Quellcode"),
        QStringLiteral("Code source"), QStringLiteral("源代码"), QStringLiteral("소스 코드"),
        QStringLiteral("ソースコード"), QStringLiteral("Código fuente"), QStringLiteral("Codice sorgente"),
        QStringLiteral("Código-fonte"), QStringLiteral("स्रोत कोड"));

    const QString link_license = StormLangStorm11(
        QStringLiteral("Лицензия (GPLv3)"), QStringLiteral("License (GPLv3)"), QStringLiteral("Lizenz (GPLv3)"),
        QStringLiteral("Licence (GPLv3)"), QStringLiteral("许可证 (GPLv3)"), QStringLiteral("라이선스 (GPLv3)"),
        QStringLiteral("ライセンス (GPLv3)"), QStringLiteral("Licencia (GPLv3)"), QStringLiteral("Licenza (GPLv3)"),
        QStringLiteral("Licença (GPLv3)"), QStringLiteral("लाइसेंस (GPLv3)"));

    ui->labelLinks->setText(QStringLiteral(
        "<html><head/><body><p><a href='https://eden-emulator.github.io/'><span style='text-decoration: underline; color:%1;'>Eden Project</span></a> | "
        "<a href='https://git.eden-emu.dev'><span style='text-decoration: underline; color:%1;'>%2</span></a> | "
        "<a href='https://git.eden-emu.dev/eden-emu/eden/src/branch/master/LICENSE.txt'><span style='text-decoration: underline; color:%1;'>%3</span></a></p></body></html>")
        .arg(pal.accent, link_source, link_license));

    const QString liability_text = StormLangStorm11(
        QStringLiteral("&quot;Nintendo Switch&quot; является зарегистрированным товарным знаком Nintendo. STORM SWITCH не связан с Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; is a registered trademark of Nintendo. STORM SWITCH is not affiliated with Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; ist eine eingetragene Marke von Nintendo. STORM SWITCH steht in keiner Verbindung zu Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; est une marque déposée de Nintendo. STORM SWITCH n'est pas affilié à Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; 是任天堂的注册商标。STORM SWITCH 与任天堂无任何关联。"),
        QStringLiteral("&quot;Nintendo Switch&quot;는 Nintendo의 등록 상표입니다. STORM SWITCH는 Nintendo와 관련이 없습니다."),
        QStringLiteral("&quot;Nintendo Switch&quot; は任天堂の登録商標です。STORM SWITCH は任天堂とは一切関係ありません。"),
        QStringLiteral("&quot;Nintendo Switch&quot; es una marca registrada de Nintendo. STORM SWITCH no está afiliado a Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; è un marchio registrato di Nintendo. STORM SWITCH non è affiliato a Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; é uma marca registrada da Nintendo. STORM SWITCH não é afiliado à Nintendo."),
        QStringLiteral("&quot;Nintendo Switch&quot; निन्टेंडो का पंजीकृत ट्रेडमार्क है। STORM SWITCH निन्टेंडो से संबद्ध नहीं है।")
    );

    ui->labelLiability->setText(QStringLiteral(
        "<html><head/><body><p><span style='font-size:7.5pt; color:%1;'>%2</span></p></body></html>")
        .arg(pal.textSecondary, liability_text));

    StormUI::StructureEngine::ModernizeForm(this);
    StormUI::StructureEngine::InstallEscDismiss(this);
}

AboutDialog::~AboutDialog() = default;
