// SPDX-FileCopyrightText: Copyright 2026 STORM SOFT Project
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <windows.h>
#endif
#include "storm_switch/storm_games_world_dialog.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "storm_switch/storm_catalog_cache.h"
#include "common/scm_rev.h"
#include "storm_switch/common/storm_lang.h"
#include "storm_switch/common/storm_theme.h"
#include "storm_switch/common/storm_design_engine.h"

#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTimer>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGridLayout>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "qt_common/titledb.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/qt_common.h"
#include "storm_switch/configuration/storm_ui_structure_helper.h"


enum class DialogIcon {
    Refresh,
    Search,
    Folder,
    FolderOpen,
    Download,
    Upload,
    Pause,
    Play,
    Cancel,
    Check,
    Copy,
    Network,
    Link,
    Cloud,
    TimeMachine,
    Plus,
    Restore,
    Sync,
    Warning,
    Package,
};

static QIcon MakeDialogVectorIcon(DialogIcon kind, const QColor& color, int size = 16) {
    const int px_size = size * 2;
    QPixmap pix(px_size, px_size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPen pen(color);
    pen.setWidthF(qMax(1.6, 1.8 * (px_size / 16.0)));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const qreal s = px_size;

    switch (kind) {
    case DialogIcon::Refresh:
    case DialogIcon::Sync: {
        QRectF arc_rect(s * 0.16, s * 0.16, s * 0.68, s * 0.68);
        p.drawArc(arc_rect, 40 * 16, 270 * 16);
        QPointF head_pt(s * 0.76, s * 0.32);
        p.drawLine(head_pt, QPointF(s * 0.88, s * 0.18));
        p.drawLine(head_pt, QPointF(s * 0.62, s * 0.18));
        break;
    }
    case DialogIcon::Search: {
        p.drawEllipse(QRectF(s * 0.16, s * 0.16, s * 0.44, s * 0.44));
        p.drawLine(QPointF(s * 0.50, s * 0.50), QPointF(s * 0.84, s * 0.84));
        break;
    }
    case DialogIcon::Folder:
    case DialogIcon::FolderOpen: {
        QPainterPath path;
        path.moveTo(s * 0.14, s * 0.28);
        path.lineTo(s * 0.40, s * 0.28);
        path.lineTo(s * 0.48, s * 0.38);
        path.lineTo(s * 0.86, s * 0.38);
        path.lineTo(s * 0.86, s * 0.76);
        path.lineTo(s * 0.14, s * 0.76);
        path.closeSubpath();
        p.drawPath(path);
        break;
    }
    case DialogIcon::Download: {
        p.drawLine(QPointF(s * 0.50, s * 0.16), QPointF(s * 0.50, s * 0.62));
        p.drawLine(QPointF(s * 0.30, s * 0.44), QPointF(s * 0.50, s * 0.62));
        p.drawLine(QPointF(s * 0.70, s * 0.44), QPointF(s * 0.50, s * 0.62));
        p.drawLine(QPointF(s * 0.20, s * 0.68), QPointF(s * 0.20, s * 0.82));
        p.drawLine(QPointF(s * 0.20, s * 0.82), QPointF(s * 0.80, s * 0.82));
        p.drawLine(QPointF(s * 0.80, s * 0.82), QPointF(s * 0.80, s * 0.68));
        break;
    }
    case DialogIcon::Upload: {
        p.drawLine(QPointF(s * 0.50, s * 0.62), QPointF(s * 0.50, s * 0.18));
        p.drawLine(QPointF(s * 0.30, s * 0.36), QPointF(s * 0.50, s * 0.18));
        p.drawLine(QPointF(s * 0.70, s * 0.36), QPointF(s * 0.50, s * 0.18));
        p.drawLine(QPointF(s * 0.20, s * 0.68), QPointF(s * 0.20, s * 0.82));
        p.drawLine(QPointF(s * 0.20, s * 0.82), QPointF(s * 0.80, s * 0.82));
        p.drawLine(QPointF(s * 0.80, s * 0.82), QPointF(s * 0.80, s * 0.68));
        break;
    }
    case DialogIcon::Pause: {
        p.drawLine(QPointF(s * 0.34, s * 0.20), QPointF(s * 0.34, s * 0.80));
        p.drawLine(QPointF(s * 0.66, s * 0.20), QPointF(s * 0.66, s * 0.80));
        break;
    }
    case DialogIcon::Play: {
        QPainterPath path;
        path.moveTo(s * 0.28, s * 0.20);
        path.lineTo(s * 0.80, s * 0.50);
        path.lineTo(s * 0.28, s * 0.80);
        path.closeSubpath();
        p.fillPath(path, color);
        break;
    }
    case DialogIcon::Cancel: {
        p.drawLine(QPointF(s * 0.24, s * 0.24), QPointF(s * 0.76, s * 0.76));
        p.drawLine(QPointF(s * 0.76, s * 0.24), QPointF(s * 0.24, s * 0.76));
        break;
    }
    case DialogIcon::Check: {
        p.drawLine(QPointF(s * 0.18, s * 0.50), QPointF(s * 0.40, s * 0.76));
        p.drawLine(QPointF(s * 0.40, s * 0.76), QPointF(s * 0.82, s * 0.24));
        break;
    }
    case DialogIcon::Copy: {
        p.drawRoundedRect(QRectF(s * 0.32, s * 0.16, s * 0.52, s * 0.52), s * 0.08, s * 0.08);
        p.drawRoundedRect(QRectF(s * 0.16, s * 0.32, s * 0.52, s * 0.52), s * 0.08, s * 0.08);
        break;
    }
    case DialogIcon::Network: {
        p.drawEllipse(QRectF(s * 0.16, s * 0.16, s * 0.68, s * 0.68));
        p.drawLine(QPointF(s * 0.16, s * 0.50), QPointF(s * 0.84, s * 0.50));
        p.drawEllipse(QRectF(s * 0.32, s * 0.16, s * 0.36, s * 0.68));
        break;
    }
    case DialogIcon::Link: {
        p.drawRoundedRect(QRectF(s * 0.16, s * 0.30, s * 0.38, s * 0.40), s * 0.14, s * 0.14);
        p.drawRoundedRect(QRectF(s * 0.46, s * 0.30, s * 0.38, s * 0.40), s * 0.14, s * 0.14);
        break;
    }
    case DialogIcon::Cloud: {
        QPainterPath path;
        path.moveTo(s * 0.24, s * 0.68);
        path.lineTo(s * 0.76, s * 0.68);
        path.arcTo(QRectF(s * 0.62, s * 0.42, s * 0.26, s * 0.26), -90, 180);
        path.arcTo(QRectF(s * 0.36, s * 0.24, s * 0.36, s * 0.36), 0, 180);
        path.arcTo(QRectF(s * 0.14, s * 0.44, s * 0.24, s * 0.24), 90, 180);
        path.closeSubpath();
        p.drawPath(path);
        break;
    }
    case DialogIcon::TimeMachine: {
        p.drawEllipse(QRectF(s * 0.16, s * 0.16, s * 0.68, s * 0.68));
        p.drawLine(QPointF(s * 0.50, s * 0.30), QPointF(s * 0.50, s * 0.50));
        p.drawLine(QPointF(s * 0.50, s * 0.50), QPointF(s * 0.68, s * 0.50));
        break;
    }
    case DialogIcon::Plus: {
        p.drawLine(QPointF(s * 0.20, s * 0.50), QPointF(s * 0.80, s * 0.50));
        p.drawLine(QPointF(s * 0.50, s * 0.20), QPointF(s * 0.50, s * 0.80));
        break;
    }
    case DialogIcon::Restore: {
        p.drawLine(QPointF(s * 0.50, s * 0.24), QPointF(s * 0.24, s * 0.50));
        p.drawLine(QPointF(s * 0.24, s * 0.50), QPointF(s * 0.50, s * 0.76));
        p.drawLine(QPointF(s * 0.76, s * 0.24), QPointF(s * 0.50, s * 0.50));
        p.drawLine(QPointF(s * 0.50, s * 0.50), QPointF(s * 0.76, s * 0.76));
        break;
    }
    case DialogIcon::Warning: {
        QPainterPath path;
        path.moveTo(s * 0.50, s * 0.16);
        path.lineTo(s * 0.86, s * 0.80);
        path.lineTo(s * 0.14, s * 0.80);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(s * 0.50, s * 0.38), QPointF(s * 0.50, s * 0.58));
        p.drawPoint(QPointF(s * 0.50, s * 0.70));
        break;
    }
    case DialogIcon::Package: {
        QPainterPath path;
        path.moveTo(s * 0.50, s * 0.16);
        path.lineTo(s * 0.84, s * 0.34);
        path.lineTo(s * 0.84, s * 0.68);
        path.lineTo(s * 0.50, s * 0.86);
        path.lineTo(s * 0.16, s * 0.68);
        path.lineTo(s * 0.16, s * 0.34);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(s * 0.50, s * 0.86), QPointF(s * 0.50, s * 0.50));
        p.drawLine(QPointF(s * 0.16, s * 0.34), QPointF(s * 0.50, s * 0.50));
        p.drawLine(QPointF(s * 0.84, s * 0.34), QPointF(s * 0.50, s * 0.50));
        break;
    }
    }

    p.end();
    return QIcon(pix);
}

static QString SanitizeFileName(QString name) {
    static const QString forbidden = QStringLiteral("<>:\"/\\|?*");
    for (const auto& ch : forbidden) {
        name.replace(ch, QLatin1Char('_'));
    }
    return name.trimmed();
}

static QString SanitizeLibretroName(QString name) {
    static const QString forbidden = QStringLiteral("&*/:`<>?\\|\"");
    for (const auto& ch : forbidden) {
        name.replace(ch, QLatin1Char('_'));
    }
    return name.trimmed();
}

static QString GetVerifiedCoverUrlForTid(const QString& tid) {
    static const QHash<QString, QString> s_verified_covers = {
        {QStringLiteral("01007F600B134000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_AssassinsCreedIIIDefinitiveEdition_image500w.jpg")},
        {QStringLiteral("01009B90006DC000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMarioMaker2_image500w.jpg")},
        {QStringLiteral("0100F3E024DFC000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/pnikpcs0uhydw9lw8672")},
        {QStringLiteral("0100FD8022DAA000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/supermariogalaxy2/1x1_NSwitch_SuperMarioGalaxy2_image500w.jpg")},
        {QStringLiteral("058E630A38C70000"), QStringLiteral("https://upload.wikimedia.org/wikipedia/en/d/d9/HellfireCoverSmall.jpg")},
        {QStringLiteral("0100C69018E4A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_TheLastFaith_image500w.jpg")},
        {QStringLiteral("0100CEA007D08000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_CryptOfTheNecroDancerNintendoSwitchEdition_image500w.jpg")},
        {QStringLiteral("0100B36008F90000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/inazumaelevenvictoryroad/1x1_InazumaElevenVictoryRoad_EN_image500w.jpg")},
        {QStringLiteral("0100CA400E300000"), QStringLiteral("https://img-eshop.cdn.nintendo.net/i/77cf808ec79894dca98a64d9c0b281f3b513992f84897967f8e115891d4cd91f.jpg")},
        {QStringLiteral("0100000000010000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMarioOdyssey_Alt01_image500w.jpg")},
        {QStringLiteral("01008CF01BAAC000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TheLegendofZeldaEchoesOfWisdom_image500w.jpg")},
        {QStringLiteral("01005EC01E6A4000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_DaveTheDiver_image500w.jpg")},
        {QStringLiteral("01001BB01E8E2000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/fantasianneodimension/1x1_FantasianNeoDimension_image500w.jpg")},
        {QStringLiteral("010042D00D900000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_LegoStarWarsTheSkywalkerSaga_image500w.jpg")},
        {QStringLiteral("01007EF00011E000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/wii_u_20/SQ_WiiU_TheLegendOfZeldaBreathOfTheWild_image500w.jpg")},
        {QStringLiteral("0100E65002BB8000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_StardewValley_image500w.jpg")},
        {QStringLiteral("0100EAE010560000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_CaptainTsubasaRiseOfNewChampions_image500w.jpg")},
        {QStringLiteral("0100152000022000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioKart8Deluxe_image500w.jpg")},
        {QStringLiteral("010013F009B88000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_XenoCrisis_image500w.jpg")},
        {QStringLiteral("010075D026910000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter5/1x1_PoppyPlaytimeChapter5_image500w.jpg")},
        {QStringLiteral("0100BC0018138000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_SuperMarioRPG_image500w.jpg")},
        {QStringLiteral("0100F7901971C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/dungeons4nintendoswitchedition/1x1_Dungeons4NintendoSwitchEdition_image500w.jpg")},
        {QStringLiteral("01009D6022DC2000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/reus2/1x1_Reus2_image500w.jpg")},
        {QStringLiteral("019232F2781D0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/virtual_console_nintendo_3ds_8/SQ_3DSVC_DrMario_image500w.jpg")},
        {QStringLiteral("01008E20257E0000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/zcxmw0qbm6ytnvgf69ey")},
        {QStringLiteral("010015100B514000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_SuperMarioBrosWonder_image500w.jpg")},
        {QStringLiteral("0100F8F00C4F2000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_DCSuperHeroGirlsTeenPower_image500w.jpg")},
        {QStringLiteral("010093801237C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MetroidDread_image500w.jpg")},
        {QStringLiteral("0100F2200C984000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MortalKombat11_image500w.jpg")},
        {QStringLiteral("0100C2801F22C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/talesofberseriaremastered/1x1_TalesOfBerseriaRemastered_image500w.jpg")},
        {QStringLiteral("01006560184E6000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MortalKombat1_image500w.jpg")},
        {QStringLiteral("010034B00E14C000"), QStringLiteral("https://upload.wikimedia.org/wikipedia/en/8/80/Tokyo_2020_game_cover.png")},
        {QStringLiteral("010092A0172E4000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_ItTakesTwo_image500w.jpg")},
        {QStringLiteral("010022201229A000"), QStringLiteral("https://img-eshop.cdn.nintendo.net/i/2035f3f0fc956dc61d691a2d3234974d6416f8175a9fc0989bfb8e6cf9477c92.jpg")},
        {QStringLiteral("01002FC00412C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_LittleNightmaresCompleteEdition_image500w.jpg")},
        {QStringLiteral("0100BDA01AABC000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/riftofthenecrodancer/1x1_RiftOfTheNecroDancer_image500w.jpg")},
        {QStringLiteral("0100ECD018EBE000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_PaperMarioTheThousandYearDoor_GB_en_image500w.jpg")},
        {QStringLiteral("0100FA501AF90000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_CastlevaniaDominusCollection_image500w.jpg")},
        {QStringLiteral("010033100691A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_TheComaRecut_image500w.jpg")},
        {QStringLiteral("01849C8CA8080000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_9/SQ_N64_MarioKart64_image500w.jpg")},
        {QStringLiteral("0100FF100FB68000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_FindingTeddyTwoDefinitiveEdition_image500w.jpg")},
        {QStringLiteral("01001AA022B66000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/tentacletango/1x1_TentacleTango_image500w.jpg")},
        {QStringLiteral("0100A31020078000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/thecoma2bcatacomb/1x1_TheComa2BCatacomb_image500w.jpg")},
        {QStringLiteral("0100F43008C44000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/pokemonlegendsza/1x1_NSwitch2_PokemonLegendsZA_KV_GB_en_image500w.jpg")},
        {QStringLiteral("010003000E146000"), QStringLiteral("https://upload.wikimedia.org/wikipedia/en/1/1a/Mario_%26_Sonic_at_the_Olympic_Games_Tokyo_2020_box_art.jpg")},
        {QStringLiteral("0100C3801C786000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_PoppyPlaytimeChapter1_image500w.jpg")},
        {QStringLiteral("010019401051C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MarioStrikersBattleLeagueFootball_image500w.jpg")},
        {QStringLiteral("010033001F050000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/ysvstrailsintheskyalternativesaga/1x1_YsVsTrailsInTheSkyAlternativeSaga_image500w.jpg")},
        {QStringLiteral("0100BDE00862A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioTennisAces_image500w.jpg")},
        {QStringLiteral("01006C900CC60000"), QStringLiteral("https://img-eshop.cdn.nintendo.net/i/fc9a60cfb3a86cc0fbb45cc5377cd3d4ae1fa5dd05ebd8be991480992c809d46.jpg")},
        {QStringLiteral("010028600EBDA000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMario3DWorldAndBowsersFury_image500w.jpg")},
        {QStringLiteral("01000B900D8B0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_CadenceOfHyruleCryptOfTheNecroDancerFeaturingTheLegendOfZelda_v2_image500w.jpg")},
        {QStringLiteral("01006DD02868A000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/mv8kxcjjajbjx536qx0q")},
        {QStringLiteral("0100FC001ACE0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/anvilsaga/1x1_AnvilSaga_image500w.jpg")},
        {QStringLiteral("01008F1008DA6000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_DarkestDungeon_image500w.jpg")},
        {QStringLiteral("010020D01AD24000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_AnimalWell_V3_image500w.jpg")},
        {QStringLiteral("010059D020C26000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/marvelcosmicinvasion_1/1x1_MarvelCosmicInvasion_image500w.jpg")},
        {QStringLiteral("01007A2027548000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/fg0htmfgjrnu87m4m9oo")},
        {QStringLiteral("0100E1C0252F8000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/fj3inknfvf8hvgfjerci")},
        {QStringLiteral("0100EC9010258000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_StreetsOfRage4_image500w.jpg")},
        {QStringLiteral("010040502453E000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/vampirecrawlerstheturbowildcardfromvampiresurvivors/1x1_VampireCrawlersTheTurboWildcardFromVampireSurvivors_image500w.jpg")},
        {QStringLiteral("0100BB901FA12000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/littlebigadventuretwinsensquest/1x1_LittleBigAdventureTwinsensQuest_image500w.jpg")},
        {QStringLiteral("01007DE013A48000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_GoldenForce_image500w.jpg")},
        {QStringLiteral("01052BA7AC450000"), QStringLiteral("https://www.nintendo.com/eu/media/images/03_teaser_module_1_square/games_3/wiiu_download_software_1/TM_WiiUDS_DuckTalesRemastered_image500w.png")},
        {QStringLiteral("01542031DCEC0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/virtual_console_nintendo_3ds_8/SQ_3DSVC_SuperMarioBros_image500w.jpg")},
        {QStringLiteral("01006BB00C6F0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_TheLegendOfZeldaLinksAwakening_image500w.jpg")},
        {QStringLiteral("0100217023F6C000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/wk30tmoq2t6mvfhlo4ee")},
        {QStringLiteral("0100317013770000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioAndRabbidsSparksOfHope_enGB_image500w.jpg")},
        {QStringLiteral("010036B0034E4000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_SuperMarioParty_image500w.jpg")},
        {QStringLiteral("0100D3801E6CE000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter2/1x1_PoppyPlaytimeChapter2_image500w.jpg")},
        {QStringLiteral("01001B300B9BE000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_DiabloIIIEternalCollection_image500w.jpg")},
        {QStringLiteral("010026800E304000"), QStringLiteral("https://img-eshop.cdn.nintendo.net/i/7a48b73ce4dbcacbf25a4103ffad0f4414bcefc0a39d94ebcdc28f66e2e09f7d.jpg")},
        {QStringLiteral("010099C022B96000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/supermariogalaxy/1x1_NSwitch_SuperMarioGalaxy_image500w.jpg")},
        {QStringLiteral("0100BAC01E57E000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_YsXNordics_image500w.jpg")},
        {QStringLiteral("010094D023A28000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/drillcore/1x1_DrillCore_image500w.jpg")},
        {QStringLiteral("0100B7C01169C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_TheComa2ViciousSisters_image500w.jpg")},
        {QStringLiteral("0100307018934000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_Signalis_image500w.jpg")},
        {QStringLiteral("01009720213B0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/captaintsubasa2worldfighters/1x1_CaptainTsubasa2WorldFighters_image500w.jpg")},
        {QStringLiteral("01005CF01E784000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_TeenageMutantNinjaTurtlesSplinteredFate_new_image500w.jpg")},
        {QStringLiteral("01006D0017F7A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MarioLuigiBrothership_Base_GB_en_image500w.jpg")},
        {QStringLiteral("010063301BD50000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/superrobotwarsy/1x1_SuperRobotWarsY_image500w.jpg")},
        {QStringLiteral("0132B3143DF50000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/virtual_console_nintendo_3ds_8/SQ_3DSVC_SuperMarioBros_image500w.jpg")},
        {QStringLiteral("01002C0008E52000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_TalesOfVesperiaDefinitiveEdition_image500w.jpg")},
        {QStringLiteral("0100A3900C3E2000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_PaperMarioTheOrigamiKing_image500w.jpg")},
        {QStringLiteral("0100FBE015910000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TheLegendOfHeroesTrailsToAzure_image500w.jpg")},
        {QStringLiteral("0100C6A0235D4000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/deviljam/1x1_DevilJam_image500w.jpg")},
        {QStringLiteral("0100EA80032EA000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_NewSuperMarioBrosUDeluxe_image500w.jpg")},
        {QStringLiteral("0100A4601ECA8000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_CryptCustodian_image500w.jpg")},
        {QStringLiteral("01005E701D168000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/talesofgracesfremastered/1x1_TalesOfGracesFRemastered_image500w.jpg")},
        {QStringLiteral("010044700DEB0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_AssassinsCreedTheRebelCollection_image500w.jpg")},
        {QStringLiteral("010066101A55A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/littlenightmaresiii_1/1x1_LittleNightmaresIII_image500w.jpg")},
        {QStringLiteral("01008BA02525A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_2_games/dispatchns2e/1x1_DispatchNS2E_image500w.jpg")},
        {QStringLiteral("9B485EB8"), QStringLiteral("https://upload.wikimedia.org/wikipedia/en/a/a5/Grand_Theft_Auto_V.png")},
        {QStringLiteral("0100852026502000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/pgsmphppjax0yb5aydgk")},
        {QStringLiteral("010097100EDD6000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_LittleNightmaresII_image500w.jpg")},
        {QStringLiteral("0100622020F5A000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/ufxlo3aficvsyiamjzg7")},
        {QStringLiteral("0100E5E01C098000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_DarkestDungeonII_image500w.jpg")},
        {QStringLiteral("01006FE013472000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioPartySuperStars_image500w.jpg")},
        {QStringLiteral("010057901E9E6000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/underlinguprising/1x1_UnderlingUprising_image500w.jpg")},
        {QStringLiteral("0100304027592000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/urbanjungle/1x1_UrbanJungle_image500w.jpg")},
        {QStringLiteral("0100B11027658000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/xnznx3wpdmxfyrjxwfq6")},
        {QStringLiteral("0100965017338000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_SuperMarioPartyJamboree_BASE_image500w.jpg")},
        {QStringLiteral("0100646009FBE000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_DeadCells_image500w.jpg")},
        {QStringLiteral("01009A5009A9E000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_ShiningResonanceRefrain_image500w.jpg")},
        {QStringLiteral("010040F01EC60000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/uuov0vj1qffnjd27os5d")},
        {QStringLiteral("0100726014352000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_DiabloIIResurrected_image500w.jpg")},
        {QStringLiteral("010067300059A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioAndRabbidsKingdomBattle_EU_image500w.jpg")},
        {QStringLiteral("0100D59022590000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/scottpilgrimex/1x1_ScottPilgrimEx_image500w.jpg")},
        {QStringLiteral("010004D00A9C0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_Aggelos_image500w.jpg")},
        {QStringLiteral("01002DA013484000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_TheLegendOfZeldaSkywardSwordHD_image500w.jpg")},
        {QStringLiteral("0100BD601EC3E000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter3/1x1_PoppyPlaytimeChapter3_image500w.jpg")},
        {QStringLiteral("010089A0197E4000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_VampireSurvivors_image500w.jpg")},
        {QStringLiteral("01002EF01A316000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_Brotato_image500w.jpg")},
        {QStringLiteral("010058C017024000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_Dungeon3NintendoSwitchEdition_image500w.jpg")},
        {QStringLiteral("0100A2902051A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/poppyplaytimechapter4/1x1_PoppyPlaytimeChapter4_image500w.jpg")},
        {QStringLiteral("0100670014482000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_AssassinsCreedTheEzioCollection_image500w.jpg")},
        {QStringLiteral("0100AC300919A000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/SQ_NSwitchDS_Firewatch_image500w.jpg")},
        {QStringLiteral("0100F1101BB9E000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/talesofxilliaremastered/1x1_TalesOfXilliaRemastered_image500w.jpg")},
        {QStringLiteral("0100919027DBE000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/thecoma3bloodlines/1x1_TheComa3Bloodlines_image500w.jpg")},
        {QStringLiteral("0100B51020B68000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/terminator2dnofate/1x1_Terminator2DNoFate_image500w.jpg")},
        {QStringLiteral("010097F018538000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_download_software/1x1_NSwitchDS_DaveTheDiver_image500w.jpg")},
        {QStringLiteral("01001B90277BE000"), QStringLiteral("https://assets.nintendo.eu/image/private/f_auto,q_auto,w_500/sm89rnvoxaacheso2a9b")},
        {QStringLiteral("0100EB60202C8000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/kalanoro/1x1_Kalanoro_image500w.jpg")},
        {QStringLiteral("01008970149B0000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_RabbidsPartyOfLegends_EN_image500w.jpg")},
        {QStringLiteral("0100A410169A4000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TalesOfSymphoniaRemastered_image500w.jpg")},
        {QStringLiteral("0100F2C0115B6000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_TloZTearsOfTheKingdom_BASE_image500w.jpg")},
        {QStringLiteral("010027901C89C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/hellokittyislandadventure/1x1_HelloKittyIslandAdventure_image500w.jpg")},
        {QStringLiteral("01000EB0276F2000"), QStringLiteral("https://www.nintendo.com/eu/media/images/assets/nintendo_switch_games/garfieldescapefrommonday/1x1_GarfieldEscapeFromMonday_image500w.jpg")},
        {QStringLiteral("0100B99019412000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/1x1_NSwitch_MarioVsDonkeyKong_image500w.jpg")},
        {QStringLiteral("0100C9C00E25C000"), QStringLiteral("https://www.nintendo.com/eu/media/images/11_square_images/games_18/nintendo_switch_5/SQ_NSwitch_MarioGolfSuperRush_image500w.jpg")}
    };
    return s_verified_covers.value(tid.toUpper());
}

struct KnownCloudVariant {
    QString ext;
    QString size;
    qint64 file_size_bytes{0};
};

static QString LocalizeSizeString(QString size_str) {
    if (size_str.isEmpty()) return size_str;
    std::string lang = UISettings::values.language.GetValue();
    if (lang.empty()) {
        lang = QLocale::system().name().toStdString();
    }
    const QString gb_str = StormLang11(
        QStringLiteral("ГБ"), QStringLiteral("GB"), QStringLiteral("GB"), QStringLiteral("Go"),
        QStringLiteral("GB"), QStringLiteral("GB"), QStringLiteral("GB"), QStringLiteral("GB"),
        QStringLiteral("GB"), QStringLiteral("GB"), QStringLiteral("GB"), QStringLiteral("जीबी"));
    const QString mb_str = StormLang11(
        QStringLiteral("МБ"), QStringLiteral("MB"), QStringLiteral("MB"), QStringLiteral("Mo"),
        QStringLiteral("MB"), QStringLiteral("MB"), QStringLiteral("MB"), QStringLiteral("MB"),
        QStringLiteral("MB"), QStringLiteral("MB"), QStringLiteral("MB"), QStringLiteral("एमबी"));
    const QString kb_str = StormLang11(
        QStringLiteral("КБ"), QStringLiteral("KB"), QStringLiteral("KB"), QStringLiteral("ko"),
        QStringLiteral("KB"), QStringLiteral("KB"), QStringLiteral("KB"), QStringLiteral("KB"),
        QStringLiteral("KB"), QStringLiteral("KB"), QStringLiteral("KB"), QStringLiteral("केबी"));
    const QString b_str = StormLang11(
        QStringLiteral("Б"), QStringLiteral("B"), QStringLiteral("B"), QStringLiteral("o"),
        QStringLiteral("B"), QStringLiteral("B"), QStringLiteral("B"), QStringLiteral("B"),
        QStringLiteral("B"), QStringLiteral("B"), QStringLiteral("B"), QStringLiteral("बी"));

    static const QRegularExpression gb_rx(QStringLiteral(R"(\b(ГБ|GB|Go|जीबी)\b)"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression mb_rx(QStringLiteral(R"(\b(МБ|MB|Mo|एमबी)\b)"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression kb_rx(QStringLiteral(R"(\b(КБ|KB|ko|केबी)\b)"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression b_rx(QStringLiteral(R"((?<=\s|^)(Б|B|o|बी)(?=\s|$))"), QRegularExpression::CaseInsensitiveOption);

    size_str.replace(gb_rx, gb_str);
    size_str.replace(mb_rx, mb_str);
    size_str.replace(kb_rx, kb_str);
    size_str.replace(b_rx, b_str);

    if (lang.rfind("ru", 0) == 0 || lang.rfind("fr", 0) == 0) {
        size_str.replace(QLatin1Char('.'), QLatin1Char(','));
    } else {
        size_str.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    return size_str;
}

static const QMap<QString, QVector<KnownCloudVariant>>& GetKnownCloudVariants() {
    static const QMap<QString, QVector<KnownCloudVariant>> s_map = {
        {QStringLiteral("0100FC001ACE0000"), { // Anvil Saga
            {QStringLiteral(".nsp"), QStringLiteral("694,29 МБ"), 728016736LL},
            {QStringLiteral(".nsz"), QStringLiteral("308,51 МБ"), 323499452LL}
        }},
        {QStringLiteral("010004D00A9C0000"), { // Aggelos
            {QStringLiteral(".nsp"), QStringLiteral("112,93 МБ"), 118411040LL},
            {QStringLiteral(".nsz"), QStringLiteral("105,46 МБ"), 110581026LL}
        }},
        {QStringLiteral("010040F01EC60000"), { // Aggelos 2
            {QStringLiteral(".nsp"), QStringLiteral("107,12 МБ"), 112319264LL},
            {QStringLiteral(".nsz"), QStringLiteral("89,56 МБ"), 93908837LL}
        }},
        {QStringLiteral("010020D01AD24000"), { // Animal Well
            {QStringLiteral(".nsp"), QStringLiteral("37,63 МБ"), 39457568LL},
            {QStringLiteral(".nsz"), QStringLiteral("34,83 МБ"), 36522797LL}
        }},
        {QStringLiteral("0100F3E024DFC000"), { // Another Eden Begins
            {QStringLiteral(".nsp"), QStringLiteral("3,61 ГБ"), 3876480288LL},
            {QStringLiteral(".nsz"), QStringLiteral("3,29 ГБ"), 3535621250LL}
        }},
        {QStringLiteral("01006DD02868A000"), { // Artis Impact
            {QStringLiteral(".nsp"), QStringLiteral("1,39 ГБ"), 1496537888LL},
            {QStringLiteral(".nsz"), QStringLiteral("970,82 МБ"), 1017977718LL}
        }},
        {QStringLiteral("01008F1008DA6000"), { // Darkest Dungeon [Ancestral Edition]
            {QStringLiteral(".nsp"), QStringLiteral("3,26 ГБ"), 3497294656LL},
            {QStringLiteral(".nsz"), QStringLiteral("1,16 ГБ"), 1243524535LL}
        }},
        {QStringLiteral("0100E5E01C098000"), { // Darkest Dungeon II
            {QStringLiteral(".nsp"), QStringLiteral("4,00 ГБ"), 4298739840LL},
            {QStringLiteral(".nsz"), QStringLiteral("1,72 ГБ"), 1844242747LL}
        }},
        {QStringLiteral("0100F2C0115B6000"), { // The Legend of Zelda: Tears of the Kingdom (ONLY NSZ)
            {QStringLiteral(".nsz"), QStringLiteral("15,45 ГБ"), 16591941089LL}
        }}
    };
    return s_map;
}

static int ExtractModCount(const QString& title, const QString& final_title, const QStringList& text_langs) {
    static const QRegularExpression mod_regex(
        QStringLiteral(R"((?:\+|[\(\[])(\d+)\s*(?:M\b|MOD\b))"),
        QRegularExpression::CaseInsensitiveOption);

    auto match = mod_regex.match(final_title);
    if (match.hasMatch()) {
        bool ok = false;
        const int count = match.captured(1).toInt(&ok);
        if (ok && count > 0) return count;
    }

    match = mod_regex.match(title);
    if (match.hasMatch()) {
        bool ok = false;
        const int count = match.captured(1).toInt(&ok);
        if (ok && count > 0) return count;
    }

    const QString combined = (title + QLatin1Char(' ') + final_title).toLower();
    if (combined.contains(QStringLiteral("mod")) ||
        combined.contains(QStringLiteral("русификатор")) ||
        combined.contains(QStringLiteral("озвучка"))) {
        return 1;
    }

    for (const auto& l : text_langs) {
        const QString lower_l = l.toLower();
        if (lower_l.contains(QStringLiteral("mod")) ||
            lower_l.contains(QStringLiteral("русификатор")) ||
            lower_l.contains(QStringLiteral("озвучка"))) {
            return 1;
        }
    }

    return 0;
}

class StormWorldDlcListDialog : public QDialog {
public:
    StormWorldDlcListDialog(QWidget* parent, const StormWorldGame& game)
        : QDialog(parent) {
        const QString disp_title = game.final_title.isEmpty() ? game.title : game.final_title;
        setWindowTitle(tr("STORM SWITCH — Дополнения: %1").arg(disp_title));
        resize(740, 500);
        setMinimumSize(620, 380);

        const StormTheme::Palette pal = StormTheme::GetThemePalette();
        setStyleSheet(QStringLiteral(
            "QDialog {"
            "    background-color: %1;"
            "    color: %2;"
            "    font-family: 'Segoe UI', sans-serif;"
            "}"
            "QFrame#HeaderBox {"
            "    background: %3;"
            "    border: none;"
            "    border-radius: 10px;"
            "    padding: 10px;"
            "}"
            "QTreeWidget {"
            "    background-color: %5;"
            "    border: none;"
            "    border-radius: 8px;"
            "    color: %2;"
            "    alternate-background-color: %1;"
            "    outline: none;"
            "}"
            "QTreeWidget::item {"
            "    height: 32px;"
            "    padding: 4px;"
            "}"
            "QTreeWidget::item:hover {"
            "    background: %6;"
            "}"
            "QTreeWidget::item:selected {"
            "    background: %7;"
            "    color: %8;"
            "}"
            "QHeaderView::section {"
            "    background-color: %3;"
            "    color: %8;"
            "    font-weight: bold;"
            "    padding: 6px;"
            "    border: none;"
            "    border-bottom: 1px solid %4;"
            "}"
            "QPushButton {"
            "    background-color: %8;"
            "    border: none;"
            "    border-radius: 6px;"
            "    color: %9;"
            "    font-weight: bold;"
            "    font-size: 12px;"
            "    padding: 8px 26px;"
            "    min-height: 24px;"
            "}"
            "QPushButton:hover {"
            "    background-color: %10;"
            "    border: none;"
            "    color: %9;"
            "}"
            "QPushButton:pressed {"
            "    background-color: %8;"
            "    border: none;"
            "    padding: 8px 26px;"
            "    color: %9;"
            "}"
        ).arg(
            pal.bg,
            pal.textPrimary,
            pal.surface,
            pal.border,
            pal.inputBg,
            pal.itemHover,
            pal.itemSelected,
            pal.accent,
            pal.is_light ? QStringLiteral("#FFFFFF") : QStringLiteral("#000000"),
            pal.accentHover
        ));

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(12);

        // Header Box
        auto* header_box = new QFrame(this);
        header_box->setObjectName(QStringLiteral("HeaderBox"));
        auto* header_layout = new QHBoxLayout(header_box);
        header_layout->setContentsMargins(14, 10, 14, 10);
        header_layout->setSpacing(14);

        auto* icon_label = new QLabel(header_box);
        icon_label->setPixmap(MakeDialogVectorIcon(DialogIcon::Package, QColor(pal.accent), 24).pixmap(24, 24));
        icon_label->setStyleSheet(QStringLiteral("font-size: 28px; background: transparent;"));
        header_layout->addWidget(icon_label);

        auto* header_text_layout = new QVBoxLayout();
        header_text_layout->setSpacing(3);

        auto* title_lbl = new QLabel(disp_title, header_box);
        title_lbl->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: #FFFFFF;"));
        title_lbl->setWordWrap(true);
        header_text_layout->addWidget(title_lbl);

        auto* meta_lbl = new QLabel(
            tr("Title ID: %1  •  Всего дополнений (DLC): %2")
                .arg(game.serial_id.isEmpty() ? tr("—") : game.serial_id)
                .arg(game.dlc_count),
            header_box);
        meta_lbl->setStyleSheet(QStringLiteral("font-size: 12px; color: %1; font-family: monospace; font-weight: bold;").arg(StormTheme::GetThemePalette().accent));
        header_text_layout->addWidget(meta_lbl);

        header_layout->addLayout(header_text_layout, 1);
        layout->addWidget(header_box);

        // Tree
        auto* tree = new QTreeWidget(this);
        tree->setHeaderLabels({
            tr("№"),
            tr("Title ID"),
            tr("Название дополнения"),
            tr("Версия"),
            tr("Статус")
        });
        tree->setRootIsDecorated(false);
        tree->setAlternatingRowColors(true);

        tree->headerItem()->setTextAlignment(0, Qt::AlignCenter);
        tree->headerItem()->setTextAlignment(1, Qt::AlignCenter);
        tree->headerItem()->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
        tree->headerItem()->setTextAlignment(3, Qt::AlignCenter);
        tree->headerItem()->setTextAlignment(4, Qt::AlignCenter);

        tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
        tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

        bool ok = false;
        const u64 tid = game.serial_id.toULongLong(&ok, 16);
        std::vector<TitleDB::Entry> dlcs;

        if (!game.dlcs.isEmpty()) {
            for (int i = 0; i < game.dlcs.size(); ++i) {
                const auto& d = game.dlcs[i];
                auto* item = new QTreeWidgetItem(tree);
                item->setText(0, QString::number(i + 1));
                item->setTextAlignment(0, Qt::AlignCenter);
                item->setText(1, d.id.isEmpty() ? (ok ? QStringLiteral("%1").arg(tid + 0x1000ULL + static_cast<u64>(i + 1), 16, 16, QLatin1Char('0')).toUpper() : QStringLiteral("—")) : d.id.toUpper());
                item->setText(2, d.name.isEmpty() ? tr("Официальное дополнение (DLC #%1)").arg(i + 1) : d.name);
                item->setText(3, d.version.isEmpty() ? (game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version) : d.version);
                item->setText(4, tr("Доступно в пакете"));

                item->setTextAlignment(1, Qt::AlignCenter);
                item->setTextAlignment(3, Qt::AlignCenter);
                item->setTextAlignment(4, Qt::AlignCenter);
                item->setForeground(1, QBrush(QColor(StormTheme::GetThemePalette().accent)));
                item->setForeground(2, QBrush(QColor(QStringLiteral("#FFFFFF"))));
                item->setForeground(4, QBrush(QColor(QStringLiteral("#00FF66"))));
            }
        } else {
            if (ok && tid != 0) {
                TitleDB::TitleDatabase::Instance().WaitLoaded(std::chrono::milliseconds(1500));
                dlcs = TitleDB::TitleDatabase::Instance().GetDlcs(tid);
            }

            const int total_items = std::max(static_cast<int>(dlcs.size()), game.dlc_count);
            if (total_items > 0) {
                for (int i = 0; i < total_items; ++i) {
                    auto* item = new QTreeWidgetItem(tree);
                    item->setText(0, QString::number(i + 1));
                    item->setTextAlignment(0, Qt::AlignCenter);

                    if (i < static_cast<int>(dlcs.size())) {
                        const auto& d = dlcs[i];
                        item->setText(1, QString::fromStdString(d.id).toUpper());
                        item->setText(2, QString::fromStdString(d.name.empty() ? ("DLC Pack " + std::to_string(i + 1)) : d.name));
                        item->setText(3, QString::fromStdString(d.version.empty() ? "1.0.0" : d.version));
                        item->setText(4, tr("Доступно в пакете"));
                    } else {
                        const u64 dlc_tid = ok ? (tid + 0x1000ULL + static_cast<u64>(i)) : 0ULL;
                        item->setText(1, dlc_tid != 0 ? QStringLiteral("%1").arg(dlc_tid, 16, 16, QLatin1Char('0')).toUpper() : QStringLiteral("—"));
                        item->setText(2, tr("Официальный контент (DLC #%1)").arg(i + 1));
                        item->setText(3, game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version);
                        item->setText(4, tr("Включено в релиз"));
                    }

                    item->setTextAlignment(1, Qt::AlignCenter);
                    item->setTextAlignment(3, Qt::AlignCenter);
                    item->setTextAlignment(4, Qt::AlignCenter);
                    item->setForeground(1, QBrush(QColor(StormTheme::GetThemePalette().accent)));
                    item->setForeground(2, QBrush(QColor(QStringLiteral("#FFFFFF"))));
                    item->setForeground(4, QBrush(QColor(QStringLiteral("#00FF66"))));
                }
            } else {
                auto* item = new QTreeWidgetItem(tree);
                item->setText(0, QStringLiteral("—"));
                item->setText(1, QStringLiteral("—"));
                item->setText(2, tr("Для данной игры официальные дополнения не обнаружены"));
                item->setText(3, QStringLiteral("—"));
                item->setText(4, tr("Нет данных"));
                item->setForeground(2, QBrush(QColor(QStringLiteral("#94a3b8"))));
            }
        }

        layout->addWidget(tree, 1);

        // Close Button
        auto* btn_layout = new QHBoxLayout();
        btn_layout->addStretch(1);
        auto* close_btn = new QPushButton(tr("Закрыть"), this);
        close_btn->setCursor(Qt::PointingHandCursor);
        connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
        btn_layout->addWidget(close_btn);
        layout->addLayout(btn_layout);
    }
};

static int ExtractModCountFast(const QString bitand str) {
    static const QRegularExpression m_regex(QStringLiteral(R"((?:[+([{\s]|^)(\d+)m(?:[+)\]}\s]|$))"), QRegularExpression::CaseInsensitiveOption);
    const auto match = m_regex.match(str);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    static const QRegularExpression word_regex(QStringLiteral(R"((?:\bmod\b)|(?:\bмод\b)|русификатор|озвучка)"), QRegularExpression::CaseInsensitiveOption);
    if (str.contains(word_regex)) {
        return 1;
    }
    return 0;
}

static int ExtractDlcCountFast(const QString bitand str) {
    static const QRegularExpression d_regex(QStringLiteral(R"((?:[+([{\s]|^)(\d+)d(?:[+)\]}\s]|$))"), QRegularExpression::CaseInsensitiveOption);
    const auto match = d_regex.match(str);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    if (str.contains(QStringLiteral("dlc"), Qt::CaseInsensitive)) {
        return 1;
    }
    return 0;
}

using LocalPackageVersionInfo = StormGamesWorldDialog::LocalPackageVersionInfo;

static std::mutex s_global_cache_mutex;
static std::vector<StormGamesWorldDialog::DownloadedFileInfo> s_global_cached_dir_files;
static std::map<u64, std::set<QString>> s_global_installed_title_id_formats;
static std::map<QString, std::set<QString>> s_global_installed_title_formats;
static std::unordered_map<std::string, LocalPackageVersionInfo> s_global_pkg_ver_cache;
static bool s_global_cache_ready = false;
static QJsonArray s_cached_catalog_games;
static std::atomic<bool> s_scan_in_progress{false};
static StormGamesWorldDialog* s_active_dialog{nullptr};

static LocalPackageVersionInfo ExtractLocalPackageVersionInfo(
    const QFileInfo& fi,
    const QMap<QString, QJsonObject>& meta_by_path,
    const QMap<QString, QJsonObject>& meta_by_name) {

    LocalPackageVersionInfo pkg;
    const QString lower_file = fi.fileName().toLower();
    pkg.path = fi.absoluteFilePath();
    pkg.size = fi.size();
    pkg.mtime = fi.lastModified().toMSecsSinceEpoch();
    pkg.mod_count = ExtractModCountFast(lower_file);
    pkg.dlc_count = ExtractDlcCountFast(lower_file);
    pkg.has_rus = lower_file.contains(QStringLiteral("rus")) || lower_file.contains(QStringLiteral("рус"));

    const QString clean_path = QDir::cleanPath(fi.absoluteFilePath()).toLower();
    QJsonObject meta_obj;
    if (meta_by_path.contains(clean_path)) {
        meta_obj = meta_by_path.value(clean_path);
    } else if (meta_by_name.contains(lower_file)) {
        meta_obj = meta_by_name.value(lower_file);
    }

    if (!meta_obj.isEmpty()) {
        pkg.tid = meta_obj[QStringLiteral("id")].toString().trimmed().toUpper();
        pkg.title = meta_obj[QStringLiteral("name")].toString().trimmed();
        pkg.display_ver = meta_obj[QStringLiteral("version")].toString().trimmed();
        pkg.internal_ver = meta_obj[QStringLiteral("internal_version")].toString().trimmed();
    }

    static const QRegularExpression tid_rx(QStringLiteral(R"(\b(01[0-9A-Fa-f]{14})\b)"));
    static const QRegularExpression tid_bracket_rx(QStringLiteral(R"(\[([0-9A-Fa-f]{16})\])"));
    static const QRegularExpression ver_rx(QStringLiteral(R"(\(\s*([^-\)]+?)\s*-\s*(\d+)\s*-\s*([0-9A-Fa-f]{16})\s*\))"));

    if (pkg.tid.isEmpty()) {
        auto m_v = ver_rx.match(fi.fileName());
        if (m_v.hasMatch()) {
            pkg.display_ver = m_v.captured(1).trimmed();
            pkg.internal_ver = m_v.captured(2).trimmed();
            pkg.tid = m_v.captured(3).toUpper();
        } else {
            auto m_b = tid_bracket_rx.match(fi.fileName());
            if (m_b.hasMatch()) {
                pkg.tid = m_b.captured(1).toUpper();
            } else {
                auto m_tid = tid_rx.match(fi.fileName());
                if (m_tid.hasMatch()) {
                    pkg.tid = m_tid.captured(1).toUpper();
                }
            }
        }
    }

    if (pkg.title.isEmpty()) {
        bool ok_tid = false;
        const u64 tid_val = pkg.tid.toULongLong(&ok_tid, 16);
        if (ok_tid && tid_val != 0 && TitleDB::TitleDatabase::Instance().IsLoaded()) {
            auto entry = TitleDB::TitleDatabase::Instance().Lookup(tid_val);
            if (!entry || entry->name.empty()) {
                entry = TitleDB::TitleDatabase::Instance().Lookup(tid_val & ~0x1FFFULL);
            }
            if (entry && !entry->name.empty()) {
                pkg.title = QString::fromStdString(entry->name);
            }
        }
        if (pkg.title.isEmpty()) {
            QString clean = fi.completeBaseName();
            static const QRegularExpression strip_rx(QStringLiteral(R"(\s*(\[[^\]]*\]|\([^\)]*\)).*$)"));
            clean = clean.remove(strip_rx).trimmed();
            pkg.title = clean.isEmpty() ? fi.completeBaseName() : clean;
        }
    }

    if (pkg.display_ver.isEmpty()) {
        static const QRegularExpression fn_ver_regex(QStringLiteral(R"((?:[\(\[\s]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))"));
        auto m_ver = fn_ver_regex.match(fi.fileName());
        if (m_ver.hasMatch()) {
            pkg.display_ver = m_ver.captured(1).trimmed();
        } else {
            pkg.display_ver = QStringLiteral("1.0.0");
        }
    }

    while (pkg.display_ver.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        pkg.display_ver.remove(0, 1);
    }
    pkg.display_ver = pkg.display_ver.trimmed();

    return pkg;
}

static void LoadDownloadedIndexDiskCache() {
    const auto index_path = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "sgw_downloaded_index_v2.json";
    const QString qindex_path = QString::fromStdString(Common::FS::PathToUTF8String(index_path));
    QFile file(qindex_path);
    if (!file.open(QFile::ReadOnly)) {
        return;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return;
    }
    const auto arr = doc.array();
    s_global_cached_dir_files.clear();
    s_global_pkg_ver_cache.clear();
    s_global_installed_title_id_formats.clear();
    s_global_installed_title_formats.clear();

    for (const auto& val : arr) {
        if (!val.isObject()) continue;
        const auto obj = val.toObject();

        LocalPackageVersionInfo pkg;
        pkg.path = obj[QStringLiteral("path")].toString();
        pkg.size = obj[QStringLiteral("size")].toVariant().toLongLong();
        pkg.mtime = obj[QStringLiteral("mtime")].toVariant().toLongLong();
        pkg.tid = obj[QStringLiteral("tid")].toString();
        pkg.title = obj[QStringLiteral("title")].toString();
        pkg.display_ver = obj[QStringLiteral("display_ver")].toString();
        pkg.internal_ver = obj[QStringLiteral("internal_ver")].toString();
        pkg.has_rus = obj[QStringLiteral("has_rus")].toBool();
        pkg.mod_count = obj[QStringLiteral("mod_count")].toInt();
        pkg.dlc_count = obj[QStringLiteral("dlc_count")].toInt();

        const std::string cache_key = pkg.path.toStdString() + "|" +
                                      std::to_string(pkg.size) + "|" +
                                      std::to_string(pkg.mtime);
        s_global_pkg_ver_cache[cache_key] = pkg;

        QFileInfo fi(pkg.path);
        const QString lower_file = fi.fileName().toLower();

        StormGamesWorldDialog::DownloadedFileInfo info;
        info.file_path = pkg.path;
        info.file_name = fi.fileName();
        info.extension = fi.suffix().toLower();
        info.complete_base_name = fi.completeBaseName().toLower();
        info.lower_name = lower_file;
        info.size = pkg.size;
        info.tid = pkg.tid;
        info.title = pkg.title;
        info.version = pkg.display_ver;
        info.internal_version = pkg.internal_ver;
        info.has_rus = pkg.has_rus;
        info.mod_count = pkg.mod_count;
        info.dlc_count = pkg.dlc_count;
        info.has_mod = (pkg.mod_count > 0);
        info.has_dlc = (pkg.dlc_count > 0);

        const QString ext = info.extension.trimmed().toLower();
        bool ok_fid = false;
        const u64 fid_val = info.tid.toULongLong(&ok_fid, 16);
        if (ok_fid && fid_val != 0) {
            s_global_installed_title_id_formats[fid_val].insert(ext);
            s_global_installed_title_id_formats[fid_val & ~0x1FFFULL].insert(ext);
        }
        const QString n_title = SanitizeFileName(info.title).trimmed().toLower();
        if (!n_title.isEmpty()) {
            s_global_installed_title_formats[n_title].insert(ext);
        }
        const QString n_base = fi.completeBaseName().trimmed().toLower();
        if (!n_base.isEmpty()) {
            s_global_installed_title_formats[n_base].insert(ext);
        }

        s_global_cached_dir_files.push_back(std::move(info));
    }
    s_global_cache_ready = true;
}

static void SaveDownloadedIndexDiskCache() {
    const auto index_path = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "sgw_downloaded_index_v2.json";
    const QString qindex_path = QString::fromStdString(Common::FS::PathToUTF8String(index_path));
    const QString tmp_path = qindex_path + QStringLiteral(".tmp");

    QJsonArray arr;
    {
        std::lock_guard<std::mutex> lock(s_global_cache_mutex);
        for (const auto& [key, pkg] : s_global_pkg_ver_cache) {
            QJsonObject obj;
            obj[QStringLiteral("path")] = pkg.path;
            obj[QStringLiteral("size")] = pkg.size;
            obj[QStringLiteral("mtime")] = pkg.mtime;
            obj[QStringLiteral("tid")] = pkg.tid;
            obj[QStringLiteral("title")] = pkg.title;
            obj[QStringLiteral("display_ver")] = pkg.display_ver;
            obj[QStringLiteral("internal_ver")] = pkg.internal_ver;
            obj[QStringLiteral("has_rus")] = pkg.has_rus;
            obj[QStringLiteral("mod_count")] = pkg.mod_count;
            obj[QStringLiteral("dlc_count")] = pkg.dlc_count;
            arr.append(obj);
        }
    }

    QFile file(tmp_path);
    if (file.open(QFile::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        file.close();
        QFile::remove(qindex_path);
        file.rename(qindex_path);
    }
}

void StormGamesWorldDialog::StartBackgroundScanner() {
    if (s_scan_in_progress.exchange(true)) {
        return; // Scan already in progress
    }

    std::thread scanner([]() {
#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
        {
            std::lock_guard<std::mutex> lock(s_global_cache_mutex);
            if (!s_global_cache_ready) {
                LoadDownloadedIndexDiskCache();
            }
        }

        // Collect scan dirs
        QStringList dirs_to_scan;
        for (const auto& gd : UISettings::values.game_dirs) {
            const QString gpath = QString::fromStdString(gd.path).trimmed();
            if (gpath.isEmpty()) continue;
            if (gpath == QStringLiteral("SDMC") || gpath == QStringLiteral("UserNAND") ||
                gpath == QStringLiteral("SysNAND")) {
                continue;
            }
            if (!QDir(gpath).exists()) continue;
            const QString clean = QDir::cleanPath(gpath);
            if (!dirs_to_scan.contains(clean)) {
                dirs_to_scan.append(clean);
            }
        }

        // Extra standard directories
        for (const auto& extra : {
            QStringLiteral("Q:/CONSOLES/Nintendo Switch/GAMES"),
            QStringLiteral("Q:/CONSOLES/Nintendo Switch 2/GAMES")
        }) {
            if (QDir(extra).exists()) {
                const QString clean = QDir::cleanPath(extra);
                if (!dirs_to_scan.contains(clean)) {
                    dirs_to_scan.append(clean);
                }
            }
        }

        // Default download dir
        QSettings settings;
        QString dl_dir = settings.value(QStringLiteral("StormGamesWorld/download_dir")).toString().trimmed();
        if (dl_dir.isEmpty() || !QDir(dl_dir).exists()) {
            dl_dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/STORM_SWITCH_GAMES");
        }
        if (QDir(dl_dir).exists()) {
            const QString clean = QDir::cleanPath(dl_dir);
            if (!dirs_to_scan.contains(clean)) {
                dirs_to_scan.append(clean);
            }
        }

        // Load file_metadata_cache.json if available
        QMap<QString, QJsonObject> meta_by_name;
        QMap<QString, QJsonObject> meta_by_path;
        const auto meta_cache_path = Common::FS::PathToUTF8String(
            Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list" / "file_metadata_cache.json");
        QFile meta_file(QString::fromStdString(meta_cache_path));
        if (meta_file.open(QFile::ReadOnly)) {
            const auto doc = QJsonDocument::fromJson(meta_file.readAll());
            if (doc.isObject()) {
                const auto root = doc.object();
                for (auto it = root.begin(); it != root.end(); ++it) {
                    const auto obj = it.value().toObject();
                    const QString key = it.key();
                    meta_by_path.insert(QDir::cleanPath(key).toLower(), obj);
                    const QString base_fn = QFileInfo(key).fileName().toLower();
                    meta_by_name.insert(base_fn, obj);
                }
            }
        }

        static const QStringList filters{
            QStringLiteral("*.nsp"), QStringLiteral("*.xci"), QStringLiteral("*.nsz"),
            QStringLiteral("*.xcz"), QStringLiteral("*.nca"), QStringLiteral("*.nro")
        };

        std::vector<StormGamesWorldDialog::DownloadedFileInfo> new_files;
        std::map<u64, std::set<QString>> new_installed_title_id_formats;
        std::map<QString, std::set<QString>> new_installed_title_formats;
        QSet<QString> seen_paths;

        for (const auto& scan_dir : dirs_to_scan) {
            QDirIterator it(scan_dir, filters, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                const QFileInfo fi = it.fileInfo();
                const QString rel = QDir(scan_dir).relativeFilePath(fi.absoluteFilePath());
                if (rel.count(QLatin1Char('/')) > 2 || rel.count(QLatin1Char('\\')) > 2) {
                    continue;
                }
                const QString lower_file = fi.fileName().toLower();
                const bool is_nro = lower_file.endsWith(QStringLiteral(".nro"));
                const qint64 min_size = is_nro ? (32 * 1024) : (1024 * 1024);
                if (fi.size() < min_size) continue;

                const QString clean_path = QDir::cleanPath(fi.absoluteFilePath()).toLower();
                if (seen_paths.contains(clean_path)) continue;
                seen_paths.insert(clean_path);

                const qint64 mtime = fi.lastModified().toMSecsSinceEpoch();
                const std::string cache_key = fi.absoluteFilePath().toStdString() + "|" +
                                              std::to_string(fi.size()) + "|" +
                                              std::to_string(mtime);

                LocalPackageVersionInfo pkg_info;
                bool found_in_cache = false;
                {
                    std::lock_guard<std::mutex> lock(s_global_cache_mutex);
                    auto it_c = s_global_pkg_ver_cache.find(cache_key);
                    if (it_c != s_global_pkg_ver_cache.end()) {
                        pkg_info = it_c->second;
                        found_in_cache = true;
                    }
                }

                if (!found_in_cache) {
                    pkg_info = ExtractLocalPackageVersionInfo(fi, meta_by_path, meta_by_name);
                    pkg_info.path = fi.absoluteFilePath();
                    pkg_info.size = fi.size();
                    pkg_info.mtime = mtime;
                    std::lock_guard<std::mutex> lock(s_global_cache_mutex);
                    s_global_pkg_ver_cache[cache_key] = pkg_info;
                }

                StormGamesWorldDialog::DownloadedFileInfo info;
                info.file_path = fi.absoluteFilePath();
                info.file_name = fi.fileName();
                info.extension = fi.suffix().toLower();
                info.complete_base_name = fi.completeBaseName().toLower();
                info.lower_name = lower_file;
                info.size = fi.size();
                info.tid = pkg_info.tid;
                info.title = pkg_info.title;
                info.version = pkg_info.display_ver;
                info.internal_version = pkg_info.internal_ver;
                info.has_rus = pkg_info.has_rus;
                info.mod_count = pkg_info.mod_count;
                info.dlc_count = pkg_info.dlc_count;
                info.has_mod = (info.mod_count > 0);
                info.has_dlc = (info.dlc_count > 0);

                const QString ext = info.extension.trimmed().toLower();
                bool ok_fid = false;
                const u64 fid_val = info.tid.toULongLong(&ok_fid, 16);
                if (ok_fid && fid_val != 0) {
                    new_installed_title_id_formats[fid_val].insert(ext);
                    new_installed_title_id_formats[fid_val & ~0x1FFFULL].insert(ext);
                }
                const QString n_title = SanitizeFileName(info.title).trimmed().toLower();
                if (!n_title.isEmpty()) {
                    new_installed_title_formats[n_title].insert(ext);
                }
                const QString n_base = fi.completeBaseName().trimmed().toLower();
                if (!n_base.isEmpty()) {
                    new_installed_title_formats[n_base].insert(ext);
                }

                new_files.push_back(std::move(info));
            }
        }

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(s_global_cache_mutex);
            if (s_global_cached_dir_files.size() != new_files.size()) {
                changed = true;
            } else {
                for (size_t i = 0; i < new_files.size(); ++i) {
                    if (new_files[i].file_path != s_global_cached_dir_files[i].file_path ||
                        new_files[i].size != s_global_cached_dir_files[i].size) {
                        changed = true;
                        break;
                    }
                }
            }
            s_global_cached_dir_files = std::move(new_files);
            s_global_installed_title_id_formats = std::move(new_installed_title_id_formats);
            s_global_installed_title_formats = std::move(new_installed_title_formats);
            s_global_cache_ready = true;
        }

        SaveDownloadedIndexDiskCache();

        if (changed) {
            std::lock_guard<std::mutex> lock(s_global_cache_mutex);
            if (s_active_dialog) {
                QMetaObject::invokeMethod(s_active_dialog, [dlg = s_active_dialog]() {
                    if (dlg) {
                        dlg->RefreshDownloadedFilesCache(dlg->GetDefaultDownloadDir());
                        dlg->PopulateGameList();
                    }
                }, Qt::QueuedConnection);
            }
        }

        s_scan_in_progress = false;
    });
    scanner.detach();
}

StormGamesWorldDialog::StormGamesWorldDialog(QWidget* parent)
    : QDialog(parent) {
    {
        std::lock_guard<std::mutex> lock(s_global_cache_mutex);
        s_active_dialog = this;
    }
    // Warm up downloaded cache synchronously in 0 ms
    RefreshDownloadedFilesCache(GetDefaultDownloadDir());

    SetupUI();
    ApplyStormStyles();
    LoadCachedCatalog();
    OnFetchCatalog();

    // Trigger async background scanner to discover any newly added files
    StartBackgroundScanner();
}

void StormGamesWorldDialog::CancelAllNetworkRequests() {
    is_closing = true;

    if (catalog_reply) {
        disconnect(catalog_reply, nullptr, nullptr, nullptr);
        catalog_reply->abort();
        catalog_reply->deleteLater();
        catalog_reply = nullptr;
    }
    if (download_reply) {
        disconnect(download_reply, nullptr, nullptr, nullptr);
        download_reply->abort();
        download_reply->deleteLater();
        download_reply = nullptr;
    }
    if (head_reply) {
        disconnect(head_reply, nullptr, nullptr, nullptr);
        head_reply->abort();
        head_reply->deleteLater();
        head_reply = nullptr;
    }
    if (cover_reply) {
        disconnect(cover_reply, nullptr, nullptr, nullptr);
        cover_reply->abort();
        cover_reply->deleteLater();
        cover_reply = nullptr;
    }
    if (details_reply) {
        disconnect(details_reply, nullptr, nullptr, nullptr);
        details_reply->abort();
        details_reply->deleteLater();
        details_reply = nullptr;
    }
    if (output_file && output_file->isOpen()) {
        output_file->close();
    }
    StormUI::StructureEngine::ModernizeForm(this);
    StormUI::StructureEngine::InstallEscDismiss(this);
}

StormGamesWorldDialog::~StormGamesWorldDialog() {
    {
        std::lock_guard<std::mutex> lock(s_global_cache_mutex);
        if (s_active_dialog == this) {
            s_active_dialog = nullptr;
        }
    }
    CancelAllNetworkRequests();
}

void StormGamesWorldDialog::closeEvent(QCloseEvent* event) {
    CancelAllNetworkRequests();
    QSettings settings;
    settings.setValue(QStringLiteral("StormGamesWorld/geometry"), saveGeometry());
    QDialog::closeEvent(event);
}

void StormGamesWorldDialog::LoadCachedCatalog() {
    bool loaded = false;
    {
        std::lock_guard<std::mutex> lock(s_global_cache_mutex);
        if (!s_cached_catalog_games.isEmpty()) {
            ParseCatalogArray(s_cached_catalog_games);
            loaded = true;
        }
    }
    if (!loaded) {
        const QByteArray cached_data = StormCatalogCache::Instance().GetRawCacheData();
        if (!cached_data.isEmpty()) {
            ParseCatalogData(cached_data);
            loaded = true;
        }
    }
    if (status_label && !all_games.empty()) {
        status_label->setText(tr("Каталог загружен из кэша (доступно: %1)").arg(all_games.size()));
    }
}

static bool IsAuxiliarySubfolder(const QString& path_str) {
    QString norm = QDir::fromNativeSeparators(path_str).trimmed();
    while (norm.endsWith(QLatin1Char('/'))) {
        norm.chop(1);
    }
    const QString lower = norm.toLower();
    if (lower.isEmpty() || lower == QStringLiteral("sdmc") ||
        lower == QStringLiteral("usernand") || lower == QStringLiteral("sysnand")) {
        return true;
    }
    if (lower.endsWith(QStringLiteral("/cheats")) || lower.contains(QStringLiteral("/cheats/")) ||
        lower.endsWith(QStringLiteral("/mods")) || lower.contains(QStringLiteral("/mods/")) ||
        lower.endsWith(QStringLiteral("/patch")) || lower.contains(QStringLiteral("/patch/")) ||
        lower.endsWith(QStringLiteral("/60fps")) || lower.contains(QStringLiteral("/60fps/")) ||
        lower.endsWith(QStringLiteral("/savestates")) || lower.contains(QStringLiteral("/load/"))) {
        return true;
    }
    return false;
}

QString StormGamesWorldDialog::GetDefaultDownloadDir() const {
    QSettings settings;
    QString dir = settings.value(QStringLiteral("StormGamesWorld/download_dir")).toString().trimmed();
    if (!dir.isEmpty() && IsAuxiliarySubfolder(dir)) {
        QDir parent_dir(dir);
        if (parent_dir.cdUp() && !IsAuxiliarySubfolder(parent_dir.absolutePath()) && parent_dir.exists()) {
            dir = parent_dir.absolutePath();
            settings.setValue(QStringLiteral("StormGamesWorld/download_dir"), dir);
        } else {
            dir.clear();
        }
    }
    if (dir.isEmpty() or !QDir(dir).exists()) {
        for (const auto bitand gd : UISettings::values.game_dirs) {
            const QString p = QString::fromStdString(gd.path).trimmed();
            if (!p.isEmpty() and !IsAuxiliarySubfolder(p) and QDir(p).exists()) {
                dir = p;
                break;
            }
        }
    }
    if (dir.isEmpty() or !QDir(dir).exists()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/STORM_SWITCH_GAMES");
        QDir().mkpath(dir);
    }
    return dir;
}

void StormGamesWorldDialog::SaveDownloadDir(const QString bitand dir) {
    QSettings settings;
    settings.setValue(QStringLiteral("StormGamesWorld/download_dir"), dir);
}

void StormGamesWorldDialog::RefreshDownloadedFilesCache(const QString bitand dir_path) const {
    // 0 ms Fast Path: if global cache is ready, immediately copy
    {
        std::lock_guard<std::mutex> lock(s_global_cache_mutex);
        if (!s_global_cache_ready) {
            LoadDownloadedIndexDiskCache();
        }
        if (s_global_cache_ready && !s_global_cached_dir_files.empty()) {
            cached_dir_files = s_global_cached_dir_files;
            installed_title_id_formats_ = s_global_installed_title_id_formats;
            installed_title_formats_ = s_global_installed_title_formats;
            cached_dir_path = dir_path;
            return;
        }
    }
    // Collect all unique directories to scan: download dir + real UISettings::values.game_dirs
    QStringList dirs_to_scan;
    QString key_builder = dir_path.trimmed();

    if (!dir_path.isEmpty() and QDir(dir_path).exists()) {
        dirs_to_scan.append(QDir::cleanPath(dir_path));
    }

    for (const auto bitand gd : UISettings::values.game_dirs) {
        const QString gpath = QString::fromStdString(gd.path).trimmed();
        if (gpath.isEmpty()) continue;
        if (gpath == QStringLiteral("SDMC") or gpath == QStringLiteral("UserNAND") or
            gpath == QStringLiteral("SysNAND")) {
            continue;
        }
        if (!QDir(gpath).exists()) continue;
        const QString clean = QDir::cleanPath(gpath);
        if (!dirs_to_scan.contains(clean)) {
            dirs_to_scan.append(clean);
        }
        key_builder += QLatin1Char(';') + clean;
    }

    if (dirs_to_scan.isEmpty()) {
        cached_dir_files.clear();
        installed_title_id_formats_.clear();
        installed_title_formats_.clear();
        cached_dir_path = key_builder;
        return;
    }

    if (cached_dir_path == key_builder and !cached_dir_files.empty()) {
        return;
    }

    cached_dir_files.clear();
    installed_title_id_formats_.clear();
    installed_title_formats_.clear();
    cached_dir_path = key_builder;

    // Load file_metadata_cache.json if available
    QMap<QString, QJsonObject> meta_by_name;
    QMap<QString, QJsonObject> meta_by_path;
    const auto meta_cache_path = Common::FS::PathToUTF8String(
        Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir) / "game_list" / "file_metadata_cache.json");
    QFile meta_file(QString::fromStdString(meta_cache_path));
    if (meta_file.open(QFile::ReadOnly)) {
        const auto doc = QJsonDocument::fromJson(meta_file.readAll());
        if (doc.isObject()) {
            const auto root = doc.object();
            for (auto it = root.begin(); it != root.end(); ++it) {
                const auto obj = it.value().toObject();
                const QString key = it.key();
                meta_by_path.insert(QDir::cleanPath(key).toLower(), obj);
                const QString base_fn = QFileInfo(key).fileName().toLower();
                meta_by_name.insert(base_fn, obj);
            }
        }
    }

    static const QRegularExpression tid_rx(QStringLiteral(R"(\b(01[0-9A-Fa-f]{14})\b)"));
    static const QRegularExpression tid_bracket_rx(QStringLiteral(R"(\[([0-9A-Fa-f]{16})\])"));
    static const QRegularExpression ver_rx(QStringLiteral(R"(\(\s*([^-\)]+?)\s*-\s*(\d+)\s*-\s*([0-9A-Fa-f]{16})\s*\))"));
    static const QStringList filters{
        QStringLiteral("*.nsp"), QStringLiteral("*.xci"), QStringLiteral("*.nsz"),
        QStringLiteral("*.xcz"), QStringLiteral("*.nca"), QStringLiteral("*.nro")
    };

    QSet<QString> seen_paths;

    for (const auto bitand scan_dir : dirs_to_scan) {
        QDirIterator it(scan_dir, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fi = it.fileInfo();

            // Safe non-blocking depth check: limit to 2 levels of subdirectories
            const QString rel = QDir(scan_dir).relativeFilePath(fi.absoluteFilePath());
            if (rel.count(QLatin1Char('/')) > 2 or rel.count(QLatin1Char('\\')) > 2) {
                continue;
            }

            const QString lower_file = fi.fileName().toLower();
            const bool is_nro = lower_file.endsWith(QStringLiteral(".nro"));
            const qint64 min_size = is_nro ? (32 * 1024) : (1024 * 1024);
            if (fi.size() < min_size) continue;

            const QString clean_path = QDir::cleanPath(fi.absoluteFilePath()).toLower();
            if (seen_paths.contains(clean_path)) continue;
            seen_paths.insert(clean_path);

            DownloadedFileInfo info;
            info.file_path = fi.absoluteFilePath();
            info.file_name = fi.fileName();
            info.extension = fi.suffix().toLower();
            info.complete_base_name = fi.completeBaseName().toLower();
            info.lower_name = lower_file;
            info.size = fi.size();
            info.mod_count = ExtractModCountFast(lower_file);
            info.dlc_count = ExtractDlcCountFast(lower_file);
            info.has_rus = lower_file.contains(QStringLiteral("rus")) or lower_file.contains(QStringLiteral("рус"));
            info.has_mod = (info.mod_count > 0);
            info.has_dlc = (info.dlc_count > 0);

            // Try to obtain metadata from cache
            QJsonObject meta_obj;
            if (meta_by_path.contains(clean_path)) {
                meta_obj = meta_by_path.value(clean_path);
            } else if (meta_by_name.contains(lower_file)) {
                meta_obj = meta_by_name.value(lower_file);
            }

            if (!meta_obj.isEmpty()) {
                info.tid = meta_obj[QStringLiteral("id")].toString().trimmed().toUpper();
                info.title = meta_obj[QStringLiteral("name")].toString().trimmed();
                info.version = meta_obj[QStringLiteral("version")].toString().trimmed();
                info.internal_version = meta_obj[QStringLiteral("internal_version")].toString().trimmed();
            }

            // Fallbacks if not in metadata cache
            if (info.tid.isEmpty()) {
                auto m_v = ver_rx.match(fi.fileName());
                if (m_v.hasMatch()) {
                    info.version = m_v.captured(1).trimmed();
                    info.tid = m_v.captured(3).toUpper();
                } else {
                    auto m_b = tid_bracket_rx.match(fi.fileName());
                    if (m_b.hasMatch()) {
                        info.tid = m_b.captured(1).toUpper();
                    } else {
                        auto m_tid = tid_rx.match(fi.fileName());
                        if (m_tid.hasMatch()) {
                            info.tid = m_tid.captured(1).toUpper();
                        }
                    }
                }
            }

            if (info.title.isEmpty()) {
                bool ok_tid = false;
                const u64 tid_val = info.tid.toULongLong(bitand ok_tid, 16);
                if (ok_tid and tid_val != 0 and TitleDB::TitleDatabase::Instance().IsLoaded()) {
                    auto entry = TitleDB::TitleDatabase::Instance().Lookup(tid_val);
                    if (!entry or entry->name.empty()) {
                        entry = TitleDB::TitleDatabase::Instance().Lookup(tid_val & ~0x1FFFULL);
                    }
                    if (entry and !entry->name.empty()) {
                        info.title = QString::fromStdString(entry->name);
                    }
                }
                if (info.title.isEmpty()) {
                    QString clean = fi.completeBaseName();
                    static const QRegularExpression strip_rx(QStringLiteral(R"(\s*(\[[^\]]*\]|\([^\)]*\)).*$)"));
                    clean = clean.remove(strip_rx).trimmed();
                    info.title = clean.isEmpty() ? fi.completeBaseName() : clean;
                }
            }

            if (info.version.isEmpty()) {
                static const QRegularExpression fn_ver_regex(QStringLiteral(R"((?:[\(\[\s]v?|\b)([0-9]+\.[0-9]+(?:\.[0-9]+)*)(?!\s*(?:GB|MB|KB|TB|ГБ|МБ|КБ|Б|B)\b))"));
                auto m_ver = fn_ver_regex.match(fi.fileName());
                if (m_ver.hasMatch()) {
                    info.version = m_ver.captured(1).trimmed();
                } else {
                    info.version = QStringLiteral("1.0.0");
                }
            }

            while (info.version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
                info.version.remove(0, 1);
            }
            info.version = info.version.trimmed();

            const QString ext = info.extension.trimmed().toLower();
            bool ok_fid = false;
            const u64 fid_val = info.tid.toULongLong(&ok_fid, 16);
            if (ok_fid && fid_val != 0) {
                installed_title_id_formats_[fid_val].insert(ext);
                installed_title_id_formats_[fid_val & ~0x1FFFULL].insert(ext);
            }
            const QString n_title = SanitizeFileName(info.title).trimmed().toLower();
            if (!n_title.isEmpty()) {
                installed_title_formats_[n_title].insert(ext);
            }
            const QString n_base = fi.completeBaseName().trimmed().toLower();
            if (!n_base.isEmpty()) {
                installed_title_formats_[n_base].insert(ext);
            }

            cached_dir_files.push_back(std::move(info));
        }
    }
}

void StormGamesWorldDialog::MergeLocalGamesIntoCatalog(const QString bitand dir_path) {
    RefreshDownloadedFilesCache(dir_path);
    if (cached_dir_files.empty()) return;

    // Remove previously merged local games (id < 0) so we never duplicate
    all_games.erase(
        std::remove_if(all_games.begin(), all_games.end(), [](const StormWorldGame bitand g) {
            return g.id < 0;
        }),
        all_games.end()
    );

    // Build sets of already cataloged Title IDs and names
    QSet<QString> catalog_tids;
    QSet<u64> catalog_base_tids;
    QSet<QString> catalog_names;
    for (const auto bitand g : all_games) {
        const QString c_tid = g.serial_id.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
        if (!c_tid.isEmpty() and c_tid != QStringLiteral("—")) {
            catalog_tids.insert(c_tid);
            bool ok = false;
            const u64 val = c_tid.toULongLong(bitand ok, 16);
            if (ok and val != 0) {
                catalog_base_tids.insert(val & ~0x1FFFULL);
            }
        }
        const QString clean_t = SanitizeFileName(g.final_title.isEmpty() ? g.title : g.final_title).toLower();
        if (!clean_t.isEmpty()) {
            catalog_names.insert(clean_t);
        }
        const QString orig_t = SanitizeFileName(g.title).toLower();
        if (!orig_t.isEmpty()) {
            catalog_names.insert(orig_t);
        }
    }

    int local_idx = 0;
    for (const auto bitand file : cached_dir_files) {
        const QString ftid = file.tid.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
        const QString fname = SanitizeFileName(file.title).toLower();
        const QString fbase = file.complete_base_name;

        // Check if catalog already contains this game
        bool already_in_catalog = false;
        if (!ftid.isEmpty()) {
            if (catalog_tids.contains(ftid)) {
                already_in_catalog = true;
            } else {
                bool ok = false;
                const u64 f_val = ftid.toULongLong(bitand ok, 16);
                if (ok and f_val != 0 and catalog_base_tids.contains(f_val & ~0x1FFFULL)) {
                    already_in_catalog = true;
                }
            }
        }
        if (!already_in_catalog and !fname.isEmpty() and catalog_names.contains(fname)) {
            already_in_catalog = true;
        }
        if (!already_in_catalog and !fbase.isEmpty() and catalog_names.contains(fbase)) {
            already_in_catalog = true;
        }

        if (!already_in_catalog) {
            StormWorldGame local_g;
            local_g.id = -1000 - (++local_idx);
            local_g.title = file.title.isEmpty() ? file.file_name : file.title;
            local_g.final_title = local_g.title;
            local_g.serial_id = ftid.isEmpty() ? QStringLiteral("—") : ftid;
            local_g.version = file.version.isEmpty() ? QStringLiteral("1.0.0") : file.version;
            local_g.size = QtCommon::ReadableByteSize(file.size);
            local_g.file_exists = true;
            local_g.has_file = true;
            local_g.dlc_count = file.dlc_count;
            local_g.mod_count = file.mod_count;
            if (file.has_rus) {
                local_g.text_langs.append(QStringLiteral("RUS"));
            } else {
                local_g.text_langs.append(QStringLiteral("Multi"));
            }
            local_g.real_extension = file.extension.isEmpty() ? QStringLiteral(".nsp") : (QLatin1Char('.') + file.extension);
            local_g.is_recommended = false;

            all_games.push_back(std::move(local_g));

            if (!ftid.isEmpty()) {
                catalog_tids.insert(ftid);
                bool ok = false;
                const u64 f_val = ftid.toULongLong(bitand ok, 16);
                if (ok and f_val != 0) {
                    catalog_base_tids.insert(f_val & ~0x1FFFULL);
                }
            }
            if (!fname.isEmpty()) {
                catalog_names.insert(fname);
            }
        }
    }
}

bool StormGamesWorldDialog::IsGameDownloaded(const StormWorldGame bitand game, const QString bitand dir_path, const QMap<QString, int> bitand group_counts) const {
    if (game.id < 0) {
        return true;
    }

    RefreshDownloadedFilesCache(dir_path);
    if (cached_dir_files.empty()) return false;

    QString expected_ext = game.real_extension.trimmed().remove(QLatin1Char('.')).toLower();
    if (expected_ext.isEmpty()) {
        static const QRegularExpression ext_rx(QStringLiteral(R"(\b(nsp|nsz|xci|xcz)\b)"), QRegularExpression::CaseInsensitiveOption);
        auto m_ext = ext_rx.match(game.final_title);
        if (!m_ext.hasMatch()) m_ext = ext_rx.match(game.title);
        if (m_ext.hasMatch()) {
            expected_ext = m_ext.captured(1).toLower();
        } else {
            expected_ext = QStringLiteral("nsp");
        }
    }

    const QString clean_final = SanitizeFileName(game.final_title.isEmpty() ? game.title : game.final_title).toLower();
    const QString clean_title = SanitizeFileName(game.title).toLower();
    const QString tid = game.serial_id.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
    const QString ver = game.version.trimmed().toLower();
    const QString int_ver = game.internal_version.trimmed();

    const QString full_game_name = (game.final_title + QLatin1Char(' ') + game.title).toLower();
    const bool game_is_rus = full_game_name.contains(QStringLiteral("rus")) or full_game_name.contains(QStringLiteral("рус"));
    const int game_mod_count = std::max(game.mod_count, ExtractModCountFast(full_game_name));
    const int game_dlc_count = std::max(game.dlc_count, ExtractDlcCountFast(full_game_name));
    const bool game_is_mod = (game_mod_count > 0);
    const bool game_has_dlc = (game_dlc_count > 0);

    bool ok_gtid = false;
    const u64 game_tid_val = tid.toULongLong(bitand ok_gtid, 16);
    const u64 game_base_tid = (ok_gtid and game_tid_val != 0) ? (game_tid_val & ~0x1FFFULL) : 0;

    // Strict format matching: if the library has this game but NOT in this exact format, reject early
    bool format_matched = false;
    if (ok_gtid && game_tid_val != 0) {
        auto it = installed_title_id_formats_.find(game_tid_val);
        if (it != installed_title_id_formats_.end() && it->second.count(expected_ext) > 0) {
            format_matched = true;
        } else if (game_base_tid != 0) {
            auto it2 = installed_title_id_formats_.find(game_base_tid);
            if (it2 != installed_title_id_formats_.end() && it2->second.count(expected_ext) > 0) {
                format_matched = true;
            }
        }
    }
    if (!format_matched && !clean_final.isEmpty()) {
        auto it = installed_title_formats_.find(clean_final);
        if (it != installed_title_formats_.end() && it->second.count(expected_ext) > 0) {
            format_matched = true;
        }
    }
    if (!format_matched && !clean_title.isEmpty()) {
        auto it = installed_title_formats_.find(clean_title);
        if (it != installed_title_formats_.end() && it->second.count(expected_ext) > 0) {
            format_matched = true;
        }
    }
    if (!format_matched) {
        bool any_with_ext = false;
        for (const auto& f : cached_dir_files) {
            if (f.extension == expected_ext) {
                any_with_ext = true;
                break;
            }
        }
        if (!any_with_ext) {
            return false;
        }
    }

    for (const auto bitand file : cached_dir_files) {
        // [Rule 1: MANDATORY FORMAT MATCH]
        // An NSP file NEVER matches an NSZ entry; an NSZ file NEVER matches an NSP entry!
        const QString file_ext = file.extension.trimmed().remove(QLatin1Char('.')).toUpper();
        if (!file_ext.isEmpty() && !expected_ext.isEmpty() && file_ext != expected_ext) {
            continue;
        }

        const QString ftid = file.tid.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
        bool ok_ftid = false;
        const u64 file_tid_val = ftid.toULongLong(bitand ok_ftid, 16);

        // [Rule 2: GAME IDENTITY MATCH]
        bool id_match = false;
        if (!tid.isEmpty() and tid != QStringLiteral("—") and !ftid.isEmpty() and ftid == tid) {
            id_match = true;
        } else if (game_base_tid != 0 and ok_ftid and file_tid_val != 0 and (file_tid_val & ~0x1FFFULL) == game_base_tid) {
            id_match = true;
        } else if (!clean_final.isEmpty() and (file.complete_base_name == clean_final or file.lower_name.contains(clean_final))) {
            id_match = true;
        } else if (!clean_title.isEmpty() and (file.complete_base_name == clean_title or (clean_title.length() >= 4 and file.lower_name.contains(clean_title)))) {
            id_match = true;
        } else if (!tid.isEmpty() and tid != QStringLiteral("—") and file.lower_name.contains(tid.toLower())) {
            id_match = true;
        }

        if (!id_match) {
            continue;
        }

        // [Rule 3: FULL-SET MATCHING - MODS]
        if (game_is_mod != file.has_mod) {
            continue;
        }
        if (game_is_mod and game_mod_count > 0 and file.mod_count > 0 and file.mod_count < game_mod_count) {
            continue;
        }

        // [Rule 4: FULL-SET MATCHING - DLC]
        if (game_has_dlc != file.has_dlc) {
            if (file.dlc_count == 0 && !file.lower_name.contains(QStringLiteral("(1g"))) {
                // allow match if user filename does not use (1g full-set notation
            } else {
                continue;
            }
        }
        if (game_has_dlc and game_dlc_count > 0 and file.dlc_count > 0 and file.dlc_count < game_dlc_count) {
            continue;
        }

        // [Rule 5: FULL-SET MATCHING - RUSSIAN]
        if (game_is_mod and game_is_rus != file.has_rus) {
            continue;
        }

        // [Rule 6: FULL-SET MATCHING - VERSION]
        QString eff_int_ver = int_ver;
        if (eff_int_ver.isEmpty() || eff_int_ver == QStringLiteral("0")) {
            static const QRegularExpression title_intver_rx(QStringLiteral(R"(\(([0-9][0-9A-Za-z._-]*)\s*-\s*([0-9]{4,})\s*-\s*([0-9A-Fa-f]{16})\))"));
            auto m_int = title_intver_rx.match(game.final_title);
            if (!m_int.hasMatch()) {
                m_int = title_intver_rx.match(game.title);
            }
            if (m_int.hasMatch()) {
                eff_int_ver = m_int.captured(2).trimmed();
            }
        }

        const bool has_ver = (!ver.isEmpty() and ver != QStringLiteral("1.0.0") and ver != QStringLiteral("—") and ver != QStringLiteral("-"));
        const bool has_int_ver = (!eff_int_ver.isEmpty() and eff_int_ver != QStringLiteral("0"));
        if (has_ver or has_int_ver) {
            bool ver_matched = false;
            if (has_ver and (!file.version.isEmpty() and (file.version.toLower() == ver or file.version == eff_int_ver))) ver_matched = true;
            if (has_int_ver and (!file.internal_version.isEmpty() and file.internal_version == eff_int_ver)) ver_matched = true;
            if (has_ver and (file.lower_name.contains(ver) or file.lower_name.contains(QStringLiteral("v%1").arg(ver)))) ver_matched = true;
            if (has_int_ver and (file.lower_name.contains(eff_int_ver) or file.lower_name.contains(QStringLiteral("v%1").arg(eff_int_ver)))) ver_matched = true;
            if (file.version.isEmpty() or (file.version == QStringLiteral("1.0.0") and !file.lower_name.contains(QStringLiteral("(1g")))) ver_matched = true;
            if (!ver_matched) {
                continue;
            }
        } else if (ver == QStringLiteral("1.0.0") or eff_int_ver == QStringLiteral("0")) {
            if (!file.version.isEmpty() and file.version != QStringLiteral("1.0.0") and file.version != QStringLiteral("0")) {
                if (file.lower_name.contains(QStringLiteral("(1g"))) {
                    continue;
                }
            }
        }

        return true;
    }

    return false;
}

void StormGamesWorldDialog::SetupUI() {
    setWindowTitle(tr("STORM SWITCH — Каталог и менеджер игр STORM GAMES WORLD"));

    const auto pal = StormTheme::GetThemePalette();

    QSettings settings;
    const QByteArray saved_geo = settings.value(QStringLiteral("StormGamesWorld/geometry")).toByteArray();
    if (!saved_geo.isEmpty()) {
        restoreGeometry(saved_geo);
    } else if (parentWidget()) {
        const QRect parent_geom = parentWidget()->geometry();
        setGeometry(parent_geom);
    } else {
        resize(1200, 740);
    }
    setMinimumSize(960, 620);

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(14, 14, 14, 14);
    root_layout->setSpacing(10);

    // --- Top Bar: Search, Pills and Refresh ---
    auto* top_bar = new QHBoxLayout();
    top_bar->setSpacing(8);

    search_edit = new QLineEdit(this);
    search_edit->setPlaceholderText(tr("Поиск доступных игр по названию или Title ID..."));
    search_edit->setClearButtonEnabled(true);
    top_bar->addWidget(search_edit, 1);

    auto make_pill = [this](const QString& label, const QString& filter_token) {
        auto* btn = new QPushButton(label, this);
        btn->setObjectName(QStringLiteral("FilterPill"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(28);
        connect(btn, &QPushButton::clicked, this, [this, filter_token]() {
            if (!search_edit) return;
            if (search_edit->text() == filter_token) {
                search_edit->clear();
            } else {
                search_edit->setText(filter_token);
            }
        });
        return btn;
    };
    top_bar->addWidget(make_pill(tr("Все"), QString()));
    top_bar->addWidget(make_pill(tr("Имеющиеся"), QStringLiteral("installed")));
    top_bar->addWidget(make_pill(tr("RUS"), QStringLiteral("ru")));
    top_bar->addWidget(make_pill(tr("DLC"), QStringLiteral("dlc")));
    top_bar->addWidget(make_pill(tr("MOD"), QStringLiteral("mod")));

    refresh_btn = new QPushButton(tr("Обновить каталог"), this);
    refresh_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Refresh, QColor(pal.accent), 16));
    refresh_btn->setCursor(Qt::PointingHandCursor);
    refresh_btn->setMinimumWidth(150);
    refresh_btn->setMinimumHeight(30);
    top_bar->addWidget(refresh_btn);

    root_layout->addLayout(top_bar);

    // --- Download Directory Bar ---
    auto* dir_bar = new QHBoxLayout();
    dir_bar->setSpacing(8);

    auto* dir_label = new QLabel(tr("Папка для сохранения:"), this);
    dir_label->setStyleSheet(QStringLiteral("font-weight: bold; color: %1; font-size: 12px;").arg(pal.accent));
    dir_bar->addWidget(dir_label);

    download_dir_edit = new QLineEdit(GetDefaultDownloadDir(), this);
    download_dir_edit->setReadOnly(true);
    dir_bar->addWidget(download_dir_edit, 1);

    browse_dir_btn = new QPushButton(tr("Обзор..."), this);
    browse_dir_btn->setIcon(MakeDialogVectorIcon(DialogIcon::FolderOpen, QColor(pal.textPrimary), 16));
    browse_dir_btn->setCursor(Qt::PointingHandCursor);
    browse_dir_btn->setMinimumHeight(28);
    dir_bar->addWidget(browse_dir_btn);

    open_folder_btn = new QPushButton(tr("Открыть папку"), this);
    open_folder_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Folder, QColor(pal.textPrimary), 16));
    open_folder_btn->setCursor(Qt::PointingHandCursor);
    open_folder_btn->setMinimumHeight(28);
    dir_bar->addWidget(open_folder_btn);

    root_layout->addLayout(dir_bar);

    // --- Main Splitter (Left: Games Tree, Right: Game Card) ---
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // Left Panel: Games Tree
    games_tree = new QTreeWidget(splitter);
    games_tree->setHeaderLabels({
        tr("Название игры"),
        tr("Версия"),
        tr("Размер"),
        tr("Дополнения и моды"),
        tr("Язык"),
        tr("Title ID"),
        tr("Статус")
    });
    games_tree->setRootIsDecorated(false);
    games_tree->setAlternatingRowColors(true);
    games_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    games_tree->setIconSize(QSize(28, 28));

    for (int col = 0; col < games_tree->columnCount(); ++col) {
        games_tree->headerItem()->setTextAlignment(col, Qt::AlignCenter);
    }
    games_tree->header()->setDefaultAlignment(Qt::AlignCenter);

    games_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    games_tree->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    games_tree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    games_tree->header()->setSectionResizeMode(3, QHeaderView::Interactive);
    games_tree->header()->setSectionResizeMode(4, QHeaderView::Interactive);
    games_tree->header()->setSectionResizeMode(5, QHeaderView::Interactive);
    games_tree->header()->setSectionResizeMode(6, QHeaderView::Interactive);

    games_tree->setColumnWidth(1, 110);
    games_tree->setColumnWidth(2, 95);
    games_tree->setColumnWidth(3, 140);
    games_tree->setColumnWidth(4, 90);
    games_tree->setColumnWidth(5, 145);
    games_tree->setColumnWidth(6, 110);

    connect(games_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        if (column == 3) {
            ShowDlcListForCurrentGame();
        }
    });
    splitter->addWidget(games_tree);

    // Right Panel: Game Details & Download Controls
    auto* details_panel = new QFrame(splitter);
    details_panel->setObjectName(QStringLiteral("details_panel"));
    auto* details_layout = new QVBoxLayout(details_panel);
    details_layout->setContentsMargins(14, 14, 14, 14);
    details_layout->setSpacing(10);

    // Header Card (Cover + Titles + Badges)
    auto* card_header = new QHBoxLayout();
    card_header->setSpacing(14);

    cover_label = new QLabel(details_panel);
    cover_label->setFixedSize(148, 148);
    cover_label->setAlignment(Qt::AlignCenter);
    cover_label->setStyleSheet(QStringLiteral(
        "background: transparent;"
        "border: none;"
        "color: %1;"
        "font-size: 11px;"
    ).arg(pal.textSecondary));
    card_header->addWidget(cover_label, 0, Qt::AlignTop);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setSpacing(6);

    title_label = new QLabel(tr("Выберите игру из списка"), details_panel);
    title_label->setWordWrap(true);
    title_label->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: %1;").arg(pal.textPrimary));
    meta_layout->addWidget(title_label);

    tid_label = new QLabel(tr("Title ID: —"), details_panel);
    tid_label->setStyleSheet(QStringLiteral("font-family: monospace; color: %1; font-size: 12px; font-weight: bold;").arg(pal.accent));
    meta_layout->addWidget(tid_label);

    // Multi-row grid layout for metadata badges that NEVER clip
    auto* badges_grid = new QGridLayout();
    badges_grid->setHorizontalSpacing(6);
    badges_grid->setVerticalSpacing(6);

    version_badge = new QLabel(tr("Версия: —"), details_panel);
    internal_version_badge = new QLabel(tr("Сборка: —"), details_panel);
    format_badge = new QLabel(tr("Формат: NSP"), details_panel);
    size_badge = new QLabel(tr("Размер: —"), details_panel);

    dlc_badge = new QLabel(tr("Дополнений: —"), details_panel);
    dlc_badge->setCursor(Qt::PointingHandCursor);
    dlc_badge->installEventFilter(this);

    mod_badge = new QLabel(tr("Модификации: —"), details_panel);
    mod_badge->setVisible(false);

    lang_badge = new QLabel(tr("Язык: —"), details_panel);

    badges_grid->addWidget(version_badge, 0, 0);
    badges_grid->addWidget(internal_version_badge, 0, 1);
    badges_grid->addWidget(format_badge, 0, 2);
    badges_grid->addWidget(size_badge, 0, 3);
    badges_grid->addWidget(dlc_badge, 1, 0);
    badges_grid->addWidget(mod_badge, 1, 1);
    badges_grid->addWidget(lang_badge, 1, 2, 1, 2);

    meta_layout->addLayout(badges_grid);

    // Version selector dropdown
    auto* ver_select_layout = new QHBoxLayout();
    ver_select_layout->setSpacing(6);
    auto* ver_title = new QLabel(tr("Доступная версия:"), details_panel);
    ver_title->setStyleSheet(QStringLiteral("color: #FFFFFF; font-weight: bold;"));
    ver_select_layout->addWidget(ver_title);

    version_combo = new QComboBox(details_panel);
    version_combo->setMinimumWidth(220);
    ver_select_layout->addWidget(version_combo, 1);
    meta_layout->addLayout(ver_select_layout);

    meta_layout->addStretch(1);
    card_header->addLayout(meta_layout, 1);

    details_layout->addLayout(card_header);

    // Description text browser
    auto* desc_title = new QLabel(tr("Описание и состав релиза:"), details_panel);
    desc_title->setStyleSheet(QStringLiteral("font-weight: bold; color: #FFFFFF; font-size: 13px;"));
    details_layout->addWidget(desc_title);

    description_browser = new QTextBrowser(details_panel);
    description_browser->setOpenExternalLinks(true);
    description_browser->setPlaceholderText(tr("Выберите игру, чтобы увидеть описание сюжета и состав пакета..."));
    details_layout->addWidget(description_browser, 1);

    // Download Box
    auto* download_box = new QGroupBox(tr("Менеджер загрузки"), details_panel);
    download_box->setStyleSheet(QStringLiteral("QGroupBox { font-weight: bold; color: #00F0FF; border: none; border-radius: 8px; margin-top: 10px; padding-top: 10px; }"));
    auto* dl_box_layout = new QVBoxLayout(download_box);
    dl_box_layout->setSpacing(8);

    progress_bar = new QProgressBar(download_box);
    progress_bar->setRange(0, 100);
    progress_bar->setValue(0);
    progress_bar->setTextVisible(true);
    progress_bar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: rgba(0,0,0,0.5); border: none; border-radius: 5px; text-align: center; color: #FFFFFF; font-weight: bold; height: 22px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0072ff, stop:1 #00f2fe); border-radius: 4px; }"
    ));
    dl_box_layout->addWidget(progress_bar);

    download_status_label = new QLabel(tr("Готов к скачиванию"), download_box);
    download_status_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 12px; font-weight: 500;"));
    dl_box_layout->addWidget(download_status_label);

    auto* dl_buttons = new QHBoxLayout();
    dl_buttons->setSpacing(10);

    download_btn = new QPushButton(tr("Скачать игру"), download_box);
    download_btn->setCursor(Qt::PointingHandCursor);
    download_btn->setEnabled(false);
    download_btn->setMinimumWidth(130);
    download_btn->setMinimumHeight(32);
    download_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Download, Qt::white, 16));
    dl_buttons->addWidget(download_btn, 1);

    pause_btn = new QPushButton(tr("Пауза"), download_box);
    pause_btn->setCursor(Qt::PointingHandCursor);
    pause_btn->setEnabled(false);
    pause_btn->setMinimumWidth(124);
    pause_btn->setMinimumHeight(30);
    pause_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Pause, QColor(245, 158, 11), 16));
    dl_buttons->addWidget(pause_btn);

    cancel_btn = new QPushButton(tr("Отмена"), download_box);
    cancel_btn->setCursor(Qt::PointingHandCursor);
    cancel_btn->setEnabled(false);
    cancel_btn->setMinimumWidth(124);
    cancel_btn->setMinimumHeight(30);
    cancel_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Cancel, QColor(239, 68, 68), 16));
    dl_buttons->addWidget(cancel_btn);

    dl_box_layout->addLayout(dl_buttons);
    details_layout->addWidget(download_box);

    splitter->addWidget(details_panel);
    splitter->setStretchFactor(0, 62);
    splitter->setStretchFactor(1, 38);
    splitter->setSizes({620, 380});

    root_layout->addWidget(splitter, 1);

    // --- Bottom Status Bar ---
    auto* bottom_bar = new QHBoxLayout();
    status_label = new QLabel(tr("Подключение к порталу stormgamesworld.ru..."), this);
    status_label->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 12px;"));
    bottom_bar->addWidget(status_label, 1);

    auto* close_btn = new QPushButton(tr("Закрыть"), this);
    close_btn->setStyleSheet(QStringLiteral("padding: 6px 18px; border-radius: 5px; font-weight: bold;"));
    bottom_bar->addWidget(close_btn);

    root_layout->addLayout(bottom_bar);

    // Connections
    connect(search_edit, &QLineEdit::textChanged, this, &StormGamesWorldDialog::OnSearchFilterChanged);
    connect(refresh_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnFetchCatalog);
    connect(browse_dir_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnBrowseDownloadFolder);
    connect(open_folder_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnOpenDownloadFolder);
    connect(games_tree, &QTreeWidget::itemSelectionChanged, this, &StormGamesWorldDialog::OnGameSelectionChanged);
    connect(download_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnStartDownload);
    connect(pause_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnPauseResumeDownload);
    connect(cancel_btn, &QPushButton::clicked, this, &StormGamesWorldDialog::OnCancelDownload);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);
}

void StormGamesWorldDialog::ApplyStormStyles() {
    const auto pal = StormTheme::GetThemePalette();
    const QString base_style = StormDesign::BuildDialogStylesheet(pal);

    const QColor acc(pal.accent);
    const QString acc_15 = QStringLiteral("rgba(%1, %2, %3, 0.15)").arg(acc.red()).arg(acc.green()).arg(acc.blue());
    const QString acc_25 = QStringLiteral("rgba(%1, %2, %3, 0.25)").arg(acc.red()).arg(acc.green()).arg(acc.blue());
    const QString acc_35 = QStringLiteral("rgba(%1, %2, %3, 0.35)").arg(acc.red()).arg(acc.green()).arg(acc.blue());

    const QString custom_style = QStringLiteral(
        "QDialog {"
        "    background-color: %1;"
        "    color: %2;"
        "    font-family: 'Segoe UI', sans-serif;"
        "}"
        "QFrame#details_panel {"
        "    background: %3;"
        "    border: none;"
        "    border-radius: 10px;"
        "}"
        "QFrame#HeaderCard, QFrame#DownloadBox {"
        "    background: %5;"
        "    border: none;"
        "    border-radius: 8px;"
        "}"
        "QLineEdit {"
        "    background: %6;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 6px 12px;"
        "    color: %2;"
        "    font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "    border: none;"
        "    background: %8;"
        "}"
        "QTreeWidget {"
        "    background-color: %6;"
        "    border: none;"
        "    border-radius: 8px;"
        "    color: %2;"
        "    alternate-background-color: %5;"
        "    selection-background-color: %9;"
        "    selection-color: #FFFFFF;"
        "}"
        "QTreeWidget::item {"
        "    height: 34px;"
        "    min-height: 34px;"
        "    padding: 2px 4px;"
        "    border-right: 1px solid %10;"
        "}"
        "QTreeWidget::item:hover {"
        "    background-color: %8;"
        "}"
        "QTreeWidget::item:selected {"
        "    background-color: %9;"
        "}"
        "QHeaderView::section {"
        "    background-color: %11;"
        "    color: %7;"
        "    font-weight: bold;"
        "    border: none;"
        "    border-right: 1px solid %4;"
        "    border-bottom: 1px solid %4;"
        "    padding: 7px 8px;"
        "}"
        "QTextBrowser {"
        "    background-color: %6;"
        "    border: none;"
        "    border-radius: 8px;"
        "    color: %12;"
        "    font-size: 12px;"
        "    padding: 8px;"
        "}"
        "QComboBox {"
        "    background: %6;"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: %2;"
        "    padding: 5px 10px;"
        "    font-weight: bold;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: %11;"
        "    color: %2;"
        "    selection-background-color: %7;"
        "    selection-color: %13;"
        "    border: none;"
        "}"
        "QPushButton {"
        "    background: %14;"
        "    border: none;"
        "    color: %2;"
        "    padding: 6px 14px;"
        "    border-radius: 6px;"
        "    font-weight: 600;"
        "    font-size: 12px;"
        "    min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "    background: %16;"
        "    border: none;"
        "    color: %7;"
        "}"
        "QPushButton:pressed {"
        "    background: %6;"
        "    color: %2;"
        "    border: none;"
        "}"
        "QPushButton:disabled {"
        "    background: transparent;"
        "    color: %17;"
        "    border: none;"
        "}"
        "QPushButton#FilterPill {"
        "    background: %5;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 4px 14px;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    min-height: 28px;"
        "}"
        "QPushButton#FilterPill:hover, QPushButton#FilterPill:checked {"
        "    background: %8;"
        "    border: none;"
        "    color: %7;"
        "}"
        "QProgressBar {"
        "    border: none;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "    background: %6;"
        "    color: #FFFFFF;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %7, stop:1 %18);"
        "    border-radius: 4px;"
        "}"
        "QScrollBar:vertical {"
        "    background: %1;"
        "    width: 12px;"
        "    margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: %4;"
        "    min-height: 24px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: %7;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ).arg(
        pal.bg,
        pal.textPrimary,
        pal.card,
        pal.border,
        pal.cardAlt,
        pal.inputBg,
        pal.accent,
        acc_15,
        acc_35,
        acc_15,
        pal.surface,
        pal.textSecondary,
        pal.is_light ? QStringLiteral("#FFFFFF") : QStringLiteral("#000000"),
        pal.buttonBg,
        pal.buttonBorder,
        pal.itemHover,
        pal.textSecondary,
        pal.accentHover
    );

    setStyleSheet(base_style + QStringLiteral("\n") + custom_style);
}

void StormGamesWorldDialog::OnFetchCatalog() {
    status_label->setText(tr("Загрузка каталога с stormgamesworld.ru..."));
    refresh_btn->setEnabled(false);

    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games/index")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/8.7.5 (Windows x64)"));

    if (catalog_reply) {
        catalog_reply->abort();
        catalog_reply->deleteLater();
    }
    catalog_reply = network_mgr.get(req);
    connect(catalog_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnCatalogReplyFinished);
}

void StormGamesWorldDialog::OnCatalogReplyFinished() {
    if (is_closing) return;
    refresh_btn->setEnabled(true);
    if (!catalog_reply) return;

    if (catalog_reply->error() != QNetworkReply::NoError) {
        if (all_games.empty()) {
            status_label->setText(tr("Ошибка подключения: %1").arg(catalog_reply->errorString()));
        }
        catalog_reply->deleteLater();
        catalog_reply = nullptr;
        return;
    }

    const QByteArray raw_data = catalog_reply->readAll();
    catalog_reply->deleteLater();
    catalog_reply = nullptr;

    auto parse_bool_val = [](const QJsonValue bitand v) -> bool {
        if (v.isBool()) return v.toBool();
        if (v.isDouble()) return v.toInt() > 0;
        if (v.isString()) {
            const QString s = v.toString().trimmed().toLower();
            return s == QStringLiteral("true") or s == QStringLiteral("1") or s == QStringLiteral("yes");
        }
        return false;
    };

    // Filter incoming catalog strictly so only Nintendo Switch games with real existing files are kept
    QJsonArray filtered_arr;
    const QJsonDocument incoming_doc = QJsonDocument::fromJson(raw_data);
    if (incoming_doc.isArray()) {
        for (const auto bitand val : incoming_doc.array()) {
            if (!val.isObject()) continue;
            const QJsonObject obj = val.toObject();
            const QString platform = obj[QStringLiteral("platformName")].toString();
            const QString platform_type = obj[QStringLiteral("platformTypeName")].toString();
            const bool fe = parse_bool_val(obj[QStringLiteral("fileExists")]);
            const bool hf = parse_bool_val(obj[QStringLiteral("hasFile")]);
            const QString sz = obj[QStringLiteral("size")].toString().trimmed();
            const bool sz_ok = !sz.isEmpty() and
                               sz != QStringLiteral("—") and
                               sz != QStringLiteral("null") and
                               sz != QStringLiteral("0") and
                               sz != QStringLiteral("0 B") and
                               sz != QStringLiteral("0,00 B") and
                               sz != QStringLiteral("0.00 B") and
                               sz != QStringLiteral("0,00 MB") and
                               sz != QStringLiteral("0.00 MB") and
                               !sz.startsWith(QStringLiteral("0 B")) and
                               !sz.startsWith(QStringLiteral("0.00")) and
                               !sz.startsWith(QStringLiteral("0,00"));

            bool pos_bytes = true;
            if (obj.contains(QStringLiteral("fileSizeBytes")) and !obj[QStringLiteral("fileSizeBytes")].isNull()) {
                pos_bytes = (obj[QStringLiteral("fileSizeBytes")].toVariant().toLongLong() > 0);
            }

            if (platform == QStringLiteral("Nintendo Switch") and
                platform_type == QStringLiteral("CONSOLES") and
                fe and hf and sz_ok and pos_bytes) {
                filtered_arr.append(obj);
            }
        }
    }

    const QByteArray filtered_bytes = QJsonDocument(filtered_arr).toJson(QJsonDocument::Compact);
    ParseCatalogData(filtered_bytes);
    StormCatalogCache::Instance().SaveCache(filtered_bytes);
    status_label->setText(tr("Каталог обновлен. Доступно для загрузки игр Nintendo Switch: %1").arg(all_games.size()));
}

void StormGamesWorldDialog::ParseCatalogData(const QByteArray bitand raw_data) {
    if (is_closing) return;
    const QJsonDocument doc = QJsonDocument::fromJson(raw_data);
    if (!doc.isArray()) {
        if (status_label) {
            status_label->setText(tr("Некорректный формат ответа сервера"));
        }
        return;
    }

    const QJsonArray arr = doc.array();
    {
        std::lock_guard<std::mutex> lock(s_global_cache_mutex);
        s_cached_catalog_games = arr;
    }
    ParseCatalogArray(arr);
}

void StormGamesWorldDialog::ParseCatalogArray(const QJsonArray bitand arr) {
    if (is_closing) return;
    all_games.clear();

    auto parse_bool_val = [](const QJsonValue bitand v) -> bool {
        if (v.isBool()) return v.toBool();
        if (v.isDouble()) return v.toInt() > 0;
        if (v.isString()) {
            const QString s = v.toString().trimmed().toLower();
            return s == QStringLiteral("true") or s == QStringLiteral("1") or s == QStringLiteral("yes");
        }
        return false;
    };

    for (const auto bitand val : arr) {
        if (!val.isObject()) continue;
        const QJsonObject obj = val.toObject();

        const QString platform = obj[QStringLiteral("platformName")].toString();
        const QString platform_type = obj[QStringLiteral("platformTypeName")].toString();
        const bool file_exists = parse_bool_val(obj[QStringLiteral("fileExists")]);
        const bool has_file = parse_bool_val(obj[QStringLiteral("hasFile")]);
        const QString size_str = obj[QStringLiteral("size")].toString().trimmed();

        const bool size_valid = !size_str.isEmpty() and
                                size_str != QStringLiteral("—") and
                                size_str != QStringLiteral("null") and
                                size_str != QStringLiteral("0") and
                                size_str != QStringLiteral("0 B") and
                                size_str != QStringLiteral("0,00 B") and
                                size_str != QStringLiteral("0.00 B") and
                                size_str != QStringLiteral("0,00 MB") and
                                size_str != QStringLiteral("0.00 MB") and
                                !size_str.startsWith(QStringLiteral("0 B")) and
                                !size_str.startsWith(QStringLiteral("0.00")) and
                                !size_str.startsWith(QStringLiteral("0,00"));

        bool has_positive_bytes = true;
        if (obj.contains(QStringLiteral("fileSizeBytes")) and !obj[QStringLiteral("fileSizeBytes")].isNull()) {
            has_positive_bytes = (obj[QStringLiteral("fileSizeBytes")].toVariant().toLongLong() > 0);
        }

        // Strict requirement: Nintendo Switch CONSOLES only, and ONLY games with verified existing files!
        if (platform == QStringLiteral("Nintendo Switch") and
            platform_type == QStringLiteral("CONSOLES") and
            file_exists and
            has_file and
            size_valid and
            has_positive_bytes) {

            StormWorldGame g;
            g.id = obj[QStringLiteral("id")].toInt();
            if (g.id == 1305 || g.id == 1306 || g.id == 1342 || g.id == 1343 ||
                g.id == 1344 || g.id == 1345 || g.id == 1349) {
                continue;
            }
            g.title = obj[QStringLiteral("title")].toString();
            g.final_title = obj[QStringLiteral("finalTitle")].toString();
            g.version = obj[QStringLiteral("version")].toString();
            g.serial_id = obj[QStringLiteral("serialId")].toString();
            g.size = LocalizeSizeString(obj[QStringLiteral("size")].toString());
            g.cover_url = obj[QStringLiteral("cover")].toString();
            g.file_exists = file_exists;
            g.has_file = has_file;

            QString fmt_field = obj[QStringLiteral("format")].toString().trimmed().remove(QLatin1Char('.')).toUpper();
            if (fmt_field.isEmpty()) {
                const QString ft = (g.final_title + QLatin1Char(' ') + g.title).toUpper();
                if (ft.contains(QStringLiteral(".NSZ")) || ft.contains(QStringLiteral("[NSZ]")) || ft.contains(QStringLiteral(" NSZ"))) {
                    fmt_field = QStringLiteral("NSZ");
                } else if (ft.contains(QStringLiteral(".XCZ")) || ft.contains(QStringLiteral("[XCZ]")) || ft.contains(QStringLiteral(" XCZ"))) {
                    fmt_field = QStringLiteral("XCZ");
                } else if (ft.contains(QStringLiteral(".XCI")) || ft.contains(QStringLiteral("[XCI]")) || ft.contains(QStringLiteral(" XCI"))) {
                    fmt_field = QStringLiteral("XCI");
                } else {
                    fmt_field = QStringLiteral("NSP");
                }
            }
            g.real_extension = QLatin1Char('.') + fmt_field.toLower();

            QString detected_ext;
            if (obj.contains(QStringLiteral("format"))) {
                detected_ext = obj[QStringLiteral("format")].toString().trimmed().remove(QLatin1Char('.')).toLower();
            } else if (obj.contains(QStringLiteral("extension"))) {
                detected_ext = obj[QStringLiteral("extension")].toString().trimmed().remove(QLatin1Char('.')).toLower();
            } else if (obj.contains(QStringLiteral("fileFormat"))) {
                detected_ext = obj[QStringLiteral("fileFormat")].toString().trimmed().remove(QLatin1Char('.')).toLower();
            } else if (obj.contains(QStringLiteral("fileName"))) {
                detected_ext = QFileInfo(obj[QStringLiteral("fileName")].toString()).suffix().trimmed().toLower();
            }
            if (detected_ext.isEmpty()) {
                static const QRegularExpression ext_rx(QStringLiteral(R"(\b(nsp|nsz|xci|xcz)\b)"), QRegularExpression::CaseInsensitiveOption);
                auto m_ext = ext_rx.match(g.final_title);
                if (!m_ext.hasMatch()) m_ext = ext_rx.match(g.title);
                if (m_ext.hasMatch()) {
                    detected_ext = m_ext.captured(1).toLower();
                }
            }
            if (!detected_ext.isEmpty()) {
                g.real_extension = QLatin1Char('.') + detected_ext;
            }

            static const QRegularExpression ver_rx(QStringLiteral(R"(\(\s*([^-\)]+?)\s*-\s*(\d+)\s*-\s*([0-9A-Fa-f]{16})\s*\))"));
            auto m = ver_rx.match(g.final_title);
            if (!m.hasMatch()) {
                m = ver_rx.match(g.title);
            }
            if (m.hasMatch()) {
                QString v_str = m.captured(1).trimmed();
                if (v_str.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
                    v_str = v_str.mid(1).trimmed();
                }
                if (!v_str.isEmpty()) {
                    g.version = v_str;
                }
                g.internal_version = m.captured(2).trimmed();
                if (g.serial_id.isEmpty()) {
                    g.serial_id = m.captured(3).trimmed();
                }
            }

            if (g.cover_url.isEmpty() && !g.serial_id.isEmpty()) {
                const QString verified_cover = GetVerifiedCoverUrlForTid(g.serial_id.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper());
                if (!verified_cover.isEmpty()) {
                    g.cover_url = verified_cover;
                }
            }

            bool ok_tid = false;
            const u64 num_tid = g.serial_id.toULongLong(&ok_tid, 16);
            g.dlc_count = ExtractDlcCountFast(g.title);
            if (g.dlc_count == 0 && !g.final_title.isEmpty()) {
                g.dlc_count = ExtractDlcCountFast(g.final_title);
            }
            if (g.dlc_count == 0 && ok_tid && num_tid != 0 && TitleDB::TitleDatabase::Instance().IsLoaded()) {
                g.dlc_count = TitleDB::TitleDatabase::Instance().GetDlcCount(num_tid);
            }

            const QJsonArray reg_arr = obj[QStringLiteral("regions")].toArray();
            for (const auto& r : reg_arr) g.regions.append(r.toString());

            const QJsonArray lang_arr = obj[QStringLiteral("textLangs")].toArray();
            for (const auto& l : lang_arr) g.text_langs.append(l.toString());

            g.mod_count = ExtractModCount(g.title, g.final_title, g.text_langs);

            const QString clean_tid = g.serial_id.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
            const auto& known_variants = GetKnownCloudVariants();
            if (known_variants.contains(clean_tid)) {
                const auto& variants = known_variants.value(clean_tid);
                for (const auto& v : variants) {
                    StormWorldGame vg = g;
                    vg.real_extension = v.ext;
                    vg.size = v.size;
                    vg.file_size_bytes = v.file_size_bytes;
                    if (v.ext.compare(QStringLiteral(".nsz"), Qt::CaseInsensitive) == 0 && variants.size() > 1) {
                        vg.id = 10000000 + g.id;
                    }
                    all_games.push_back(std::move(vg));
                }
            } else {
                all_games.push_back(g);
            }
        }
    }

    // Group games by key to determine recommendations
    QMap<QString, QVector<int>> game_groups;
    for (int i = 0; i < static_cast<int>(all_games.size()); ++i) {
        const auto& g = all_games[i];
        const QString key = !g.serial_id.trimmed().isEmpty() ? g.serial_id.trimmed().toUpper()
            : (g.final_title.isEmpty() ? g.title.trimmed().toLower() : g.final_title.trimmed().toLower());
        game_groups[key].push_back(i);
    }

    auto calc_priority = [](const StormWorldGame& g) -> int {
        const QString t = (g.final_title + QLatin1Char(' ') + g.title).toUpper();
        if (t.contains(QStringLiteral("MOD - RUS")) || t.contains(QStringLiteral("MOD - M. RUS")) ||
            t.contains(QStringLiteral("MOD-RUS")) || t.contains(QStringLiteral("MOD - M.RUS"))) {
            return 300;
        }
        if (t.contains(QStringLiteral("[RUS]")) || t.contains(QStringLiteral("(RUS)")) ||
            t.contains(QStringLiteral(" RUS ")) || t.endsWith(QStringLiteral(" RUS"))) {
            return 200;
        }
        for (const auto& l : g.text_langs) {
            const QString lu = l.toUpper();
            if (lu == QStringLiteral("RUS") || lu.contains(QStringLiteral("RUSSIAN")) || lu.contains(QStringLiteral("РУССКИЙ"))) {
                return 200;
            }
        }
        return 100;
    };

    for (auto it = game_groups.begin(); it != game_groups.end(); ++it) {
        const auto& indices = it.value();
        // If only 1 version exists: do NOT display Recommended badge
        if (indices.size() <= 1) {
            for (int idx : indices) {
                all_games[idx].is_recommended = false;
            }
            continue;
        }

        int best_idx = -1;
        int best_priority = -1;
        QString best_version;
        int best_id = -1;

        for (int idx : indices) {
            all_games[idx].is_recommended = false;
            const auto& g = all_games[idx];
            const int prio = calc_priority(g);

            bool is_better = false;
            if (best_idx == -1) {
                is_better = true;
            } else if (prio > best_priority) {
                is_better = true;
            } else if (prio == best_priority) {
                const int cmp = StormCatalogCache::CompareVersions(g.version, best_version);
                if (cmp > 0) {
                    is_better = true;
                } else if (cmp == 0 && g.id > best_id) {
                    is_better = true;
                }
            }

            if (is_better) {
                best_idx = idx;
                best_priority = prio;
                best_version = g.version;
                best_id = g.id;
            }
        }

        if (best_idx >= 0) {
            all_games[best_idx].is_recommended = true;
        }
    }

    // Sort all_games so recommended version appears at the top within its group, followed by higher versions
    std::stable_sort(all_games.begin(), all_games.end(), [&calc_priority](const StormWorldGame& a, const StormWorldGame& b) {
        const QString key_a = !a.serial_id.trimmed().isEmpty() ? a.serial_id.trimmed().toUpper() : (a.final_title.isEmpty() ? a.title.trimmed().toLower() : a.final_title.trimmed().toLower());
        const QString key_b = !b.serial_id.trimmed().isEmpty() ? b.serial_id.trimmed().toUpper() : (b.final_title.isEmpty() ? b.title.trimmed().toLower() : b.final_title.trimmed().toLower());
        if (key_a == key_b) {
            if (a.is_recommended != b.is_recommended) {
                return a.is_recommended;
            }
            const int prio_a = calc_priority(a);
            const int prio_b = calc_priority(b);
            if (prio_a != prio_b) {
                return prio_a > prio_b;
            }
            return StormCatalogCache::CompareVersions(a.version, b.version) > 0;
        }
        return false;
    });

    PopulateGameList(search_edit ? search_edit->text() : QString());
}

void StormGamesWorldDialog::PopulateGameList(const QString bitand filter) {
    const auto& pal = StormTheme::GetThemePalette();
    int prev_selected_game_id = -1;
    if (selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
        prev_selected_game_id = filtered_games[selected_game_index].id;
    }
    if ((is_downloading || is_paused) && current_download_game_id != -1) {
        prev_selected_game_id = current_download_game_id;
    }
    games_tree->setUpdatesEnabled(false);
    games_tree->clear();
    filtered_games.clear();

    const QString lower_filter = filter.trimmed().toLower();
    const QString current_dir = download_dir_edit ? download_dir_edit->text() : QString();

    MergeLocalGamesIntoCatalog(current_dir);

    const QString eden_cache_path = QString::fromStdString(Common::FS::PathToUTF8String(Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir)));
    const QString gl_dir = QDir(eden_cache_path).filePath(QStringLiteral("game_list"));

    // Precalculate group counts in O(N)
    QMap<QString, int> group_counts;
    for (const auto bitand g : all_games) {
        const QString tid = g.serial_id.trimmed().toLower();
        const QString clean_title = SanitizeFileName(g.title).toLower();
        const QString key = (!tid.isEmpty() and tid != QStringLiteral("—")) ? tid : clean_title;
        group_counts[key]++;
    }

    QList<QTreeWidgetItem*> items_to_add;
    items_to_add.reserve(all_games.size());

    for (const auto bitand g : all_games) {
        if (g.id >= 0 and (!g.file_exists or !g.has_file or g.size.trimmed().isEmpty() or g.size.trimmed() == QStringLiteral("—"))) {
            continue;
        }

        if (!lower_filter.isEmpty()) {
            if (lower_filter == QStringLiteral("dlc")) {
                if (g.dlc_count <= 0) continue;
            } else if (lower_filter == QStringLiteral("mod")) {
                if (g.mod_count <= 0) continue;
            } else if (lower_filter == QStringLiteral("ru")) {
                const bool has_ru = g.text_langs.contains(QStringLiteral("RU"), Qt::CaseInsensitive) ||
                                    g.title.contains(QStringLiteral("рус"), Qt::CaseInsensitive) ||
                                    g.final_title.contains(QStringLiteral("рус"), Qt::CaseInsensitive);
                if (!has_ru) continue;
            } else if (lower_filter == QStringLiteral("installed") || lower_filter == QStringLiteral("скачано") ||
                       lower_filter == QStringLiteral("имеющиеся") || lower_filter == QStringLiteral("установленные")) {
                if (!IsGameDownloaded(g, current_dir, group_counts)) continue;
            } else {
                const bool match_title = g.title.toLower().contains(lower_filter);
                const bool match_final = g.final_title.toLower().contains(lower_filter);
                const bool match_tid = g.serial_id.toLower().contains(lower_filter);
                QString ext_clean = g.real_extension;
                ext_clean.remove(QLatin1Char('.'));
                ext_clean = ext_clean.toLower();
                const bool match_ext = ext_clean.contains(lower_filter) || g.real_extension.toLower().contains(lower_filter);
                if (!match_title and !match_final and !match_tid and !match_ext) {
                    continue;
                }
            }
        }

        filtered_games.push_back(g);

        const QString disp_title = g.final_title.isEmpty() ? g.title : g.final_title;
        const QString langs = g.text_langs.isEmpty() ? tr("Multi") : g.text_langs.join(QStringLiteral(", "));

        auto* item = new QTreeWidgetItem();
        item->setText(0, disp_title);
        item->setForeground(0, QBrush(QColor(QStringLiteral("#FFFFFF"))));

        const QString item_tid = g.serial_id.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
        if (!item_tid.isEmpty() and item_tid != QStringLiteral("—")) {
            const QString icon_file = QDir(gl_dir).filePath(item_tid + QStringLiteral(".jpeg"));
            if (QFile::exists(icon_file)) {
                item->setIcon(0, QIcon(icon_file));
            }
        }

        QString ext_tag = g.real_extension.isEmpty() ? QStringLiteral("NSP") : g.real_extension;
        ext_tag.remove(QLatin1Char('.'));
        ext_tag = ext_tag.toUpper();
        const QString clean_ver = StormFormat::CleanVersion(g.version);

        if (g.is_recommended) {
            item->setText(1, QStringLiteral("%1 [%2]").arg(clean_ver, ext_tag));
            item->setForeground(1, QBrush(QColor(QStringLiteral("#10B981"))));
            item->setToolTip(1, tr("Рекомендуемая новейшая версия игры (%1)").arg(ext_tag));
        } else {
            item->setText(1, QStringLiteral("%1 [%2]").arg(clean_ver, ext_tag));
            item->setForeground(1, QBrush(QColor(pal.textPrimary)));
        }

        const QString sz_str = (g.file_size_bytes > 0)
            ? StormFormat::ReadableByteSize(static_cast<qulonglong>(g.file_size_bytes))
            : (g.size.isEmpty() ? tr("—") : LocalizeSizeString(g.size));
        item->setText(2, sz_str);
        item->setForeground(2, QBrush(QColor(pal.textPrimary)));

        if (g.dlc_count > 0 && g.mod_count > 0) {
            item->setText(3, tr("%1 DLC  •  +%2 MOD").arg(g.dlc_count).arg(g.mod_count));
            item->setForeground(3, QBrush(QColor(pal.accent)));
            item->setToolTip(3, tr("Дополнений: %1, Модификаций: %2 (нажмите для списка дополнений)").arg(g.dlc_count).arg(g.mod_count));
        } else if (g.dlc_count > 0) {
            item->setText(3, tr("%1 DLC").arg(g.dlc_count));
            item->setForeground(3, QBrush(QColor(pal.accent)));
            item->setToolTip(3, tr("Нажмите для просмотра списка дополнений"));
        } else if (g.mod_count > 0) {
            item->setText(3, tr("+%1 MOD").arg(g.mod_count));
            item->setForeground(3, QBrush(QColor(QStringLiteral("#C084FC"))));
            item->setToolTip(3, tr("Вшита модификация / русификатор (%1 MOD)").arg(g.mod_count));
        } else {
            item->setText(3, QStringLiteral("—"));
            item->setForeground(3, QBrush(QColor(pal.textSecondary)));
        }

        item->setText(4, langs);
        item->setForeground(4, QBrush(QColor(pal.textPrimary)));

        item->setText(5, g.serial_id.isEmpty() ? tr("—") : g.serial_id);
        item->setForeground(5, QBrush(QColor(pal.accent)));

        // Center align columns 1 to 6
        item->setTextAlignment(1, Qt::AlignCenter);
        item->setTextAlignment(2, Qt::AlignCenter);
        item->setTextAlignment(3, Qt::AlignCenter);
        item->setTextAlignment(4, Qt::AlignCenter);
        item->setTextAlignment(5, Qt::AlignCenter);
        item->setTextAlignment(6, Qt::AlignCenter);

        // Check download status fast
        if (IsGameDownloaded(g, current_dir, group_counts)) {
            item->setText(6, tr("Скачано"));
            item->setForeground(6, QBrush(QColor(QStringLiteral("#10B981"))));
        } else if (is_downloading && current_download_game_id == g.id) {
            item->setText(6, tr("Загрузка"));
            item->setForeground(6, QBrush(QColor(QStringLiteral("#F59E0B"))));
        } else {
            item->setText(6, tr("Доступно"));
            item->setForeground(6, QBrush(QColor(pal.textSecondary)));
        }

        item->setData(0, Qt::UserRole, static_cast<int>(filtered_games.size() - 1));
        items_to_add.append(item);
    }

    games_tree->blockSignals(true);
    games_tree->addTopLevelItems(items_to_add);
    int restore_row = 0;
    if (prev_selected_game_id != -1) {
        for (int i = 0; i < static_cast<int>(filtered_games.size()); ++i) {
            if (filtered_games[i].id == prev_selected_game_id) {
                restore_row = i;
                break;
            }
        }
    }
    if (!filtered_games.empty() && restore_row < games_tree->topLevelItemCount()) {
        games_tree->setCurrentItem(games_tree->topLevelItem(restore_row));
        selected_game_index = restore_row;
    }
    games_tree->blockSignals(false);
    games_tree->setUpdatesEnabled(true);

    if (!filtered_games.empty()) {
        OnGameSelectionChanged();
    } else {
        selected_game_index = -1;
        download_btn->setEnabled(false);
        title_label->setText(tr("Игры не найдены"));
        tid_label->setText(tr("Title ID: —"));
        description_browser->clear();
    }
}

void StormGamesWorldDialog::OnSearchFilterChanged(const QString bitand query) {
    PopulateGameList(query);
}

void StormGamesWorldDialog::OnGameSelectionChanged() {
    const auto items = games_tree->selectedItems();
    if (items.isEmpty()) {
        selected_game_index = -1;
        download_btn->setEnabled(false);
        return;
    }

    const int idx = items.first()->data(0, Qt::UserRole).toInt();
    if (idx >= 0 and idx < static_cast<int>(filtered_games.size())) {
        selected_game_index = idx;
        const auto bitand game = filtered_games[idx];
        DisplayGameDetails(game);
        if (game.id < 0) {
            LoadCover(game);
            download_btn->setEnabled(false);
            download_btn->setText(tr("Уже скачано"));
            download_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Check, QColor(QStringLiteral("#10B981")), 16));
            if (!is_downloading) {
                download_status_label->setText(tr("Локальная игра (уже сохранена в папке)"));
            }
        } else {
            LoadCover(game);
            if (!is_downloading) {
                FetchGameDetails(game.id);
                FetchRealExtension(game.id);
            }
            if (is_downloading && game.id == current_download_game_id) {
                download_btn->setEnabled(false);
                download_btn->setText(tr("Идет скачивание..."));
                download_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Download, Qt::white, 16));
            } else {
                const QString dir_path = download_dir_edit ? download_dir_edit->text().trimmed() : QString();
                QString base_fn = SanitizeFileName(game.final_title.isEmpty() ? game.title : game.final_title);
                if (base_fn.endsWith(QStringLiteral(".nsp"), Qt::CaseInsensitive) ||
                    base_fn.endsWith(QStringLiteral(".nsz"), Qt::CaseInsensitive) ||
                    base_fn.endsWith(QStringLiteral(".xci"), Qt::CaseInsensitive) ||
                    base_fn.endsWith(QStringLiteral(".xcz"), Qt::CaseInsensitive)) {
                    base_fn.chop(4);
                }
                base_fn += (game.real_extension.isEmpty() ? QStringLiteral(".nsp") : game.real_extension);
                const QString part_p = QDir(dir_path).filePath(base_fn + QStringLiteral(".part"));
                const QFileInfo part_fi(part_p);
                if (part_fi.exists() && part_fi.size() > 0) {
                    download_btn->setText(tr("Докачать игру (%1)").arg(StormFormat::ReadableByteSize(part_fi.size())));
                    download_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Play, Qt::white, 16));
                    if (!is_downloading) {
                        download_status_label->setText(tr("Найден незавершенный файл (%1) — доступна докачка").arg(StormFormat::ReadableByteSize(part_fi.size())));
                        if (game.file_size_bytes > 0) {
                            progress_bar->setValue(static_cast<int>(std::min<qint64>(99, (part_fi.size() * 100) / game.file_size_bytes)));
                        }
                    }
                } else {
                    download_btn->setText(tr("Скачать игру"));
                    download_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Download, Qt::white, 16));
                    if (!is_downloading) {
                        progress_bar->setValue(0);
                        download_status_label->setText(tr("Готов к скачиванию"));
                    }
                }
                download_btn->setEnabled(!is_downloading);
            }
        }
    }
}

static QString ExtractInternalVersion(const QString& title, const QString& version_str) {
    // 1. Look for explicit [v655360] or (655360) or [655360] in title
    static const QRegularExpression re_title(QStringLiteral(R"([\[\(vV]?(\d{5,8})[\]\)]?)"));
    QRegularExpressionMatchIterator it = re_title.globalMatch(title);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString num = match.captured(1);
        if (num.length() >= 5 && num.length() <= 8) {
            return num;
        }
    }

    // 2. Compute from version_str e.g. "1.0.10" or "v1.2.3"
    QString clean_v = version_str;
    if (clean_v.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        clean_v.remove(0, 1);
    }
    const QStringList parts = clean_v.trimmed().split(QLatin1Char('.'));
    if (parts.size() >= 3) {
        bool ok1 = false, ok2 = false, ok3 = false;
        const int major = parts[0].toInt(&ok1);
        const int minor = parts[1].toInt(&ok2);
        const int patch = parts[2].toInt(&ok3);
        if (ok1 && ok2 && ok3) {
            quint32 internal_ver = ((quint32)std::max(0, major - 1) * 655360) +
                                   ((quint32)minor * 655360) +
                                   ((quint32)patch * 65536);
            if (internal_ver == 0 && (major > 1 || minor > 0 || patch > 0)) {
                internal_ver = static_cast<quint32>(patch) * 65536;
            }
            return QString::number(internal_ver);
        }
    } else if (parts.size() == 2) {
        bool ok1 = false, ok2 = false;
        const int major = parts[0].toInt(&ok1);
        const int minor = parts[1].toInt(&ok2);
        if (ok1 && ok2) {
            const quint32 internal_ver = ((quint32)std::max(0, major - 1) * 655360) + ((quint32)minor * 65536);
            return QString::number(internal_ver);
        }
    }
    return QStringLiteral("0");
}

void StormGamesWorldDialog::DisplayGameDetails(const StormWorldGame& game) {
    const auto pal = StormTheme::GetThemePalette();
    title_label->setText(game.final_title.isEmpty() ? game.title : game.final_title);
    tid_label->setText(tr("Title ID: %1").arg(game.serial_id.isEmpty() ? tr("Не указан") : game.serial_id));

    const QString clean_gv = StormFormat::CleanVersion(game.version);
    const QColor acc_col(pal.accent);
    const QString acc_15 = QStringLiteral("rgba(%1, %2, %3, 0.15)").arg(acc_col.red()).arg(acc_col.green()).arg(acc_col.blue());

    if (game.is_recommended) {
        version_badge->setText(tr("Версия: %1 (Рекомендуемая)").arg(clean_gv));
        version_badge->setStyleSheet(QStringLiteral("background: rgba(16, 185, 129, 0.18); border: none; border-radius: 4px; padding: 2px 8px; color: #10B981; font-size: 11px; font-weight: bold;"));
    } else {
        version_badge->setText(tr("Версия: %1").arg(clean_gv));
        version_badge->setStyleSheet(QStringLiteral("background: %1; border: none; border-radius: 4px; padding: 2px 8px; color: %2; font-size: 11px; font-weight: bold;").arg(acc_15, pal.accent));
    }
    const QString int_ver = ExtractInternalVersion(game.title, game.version);
    internal_version_badge->setText(tr("Сборка: %1").arg(int_ver));
    internal_version_badge->setStyleSheet(QStringLiteral("background: rgba(168, 85, 247, 0.15); border: none; border-radius: 4px; padding: 2px 8px; color: #C084FC; font-size: 11px; font-weight: bold; font-family: monospace;"));

    QString ext_tag = game.real_extension.isEmpty() ? QStringLiteral("NSP") : game.real_extension;
    ext_tag.remove(QLatin1Char('.'));
    ext_tag = ext_tag.toUpper();
    if (format_badge) {
        format_badge->setText(tr("Формат: %1").arg(ext_tag));
        format_badge->setStyleSheet(QStringLiteral("background: rgba(59, 130, 246, 0.15); border: none; border-radius: 4px; padding: 2px 8px; color: #60A5FA; font-size: 11px; font-weight: bold;"));
    }

    const QString disp_size = (game.file_size_bytes > 0)
        ? StormFormat::ReadableByteSize(static_cast<qulonglong>(game.file_size_bytes))
        : (game.size.isEmpty() ? tr("Неизвестно") : LocalizeSizeString(game.size));
    size_badge->setText(tr("Размер: %1").arg(disp_size));
    size_badge->setStyleSheet(QStringLiteral("background: rgba(16, 185, 129, 0.15); border: none; border-radius: 4px; padding: 2px 8px; color: #10B981; font-size: 11px; font-weight: bold;"));

    if (game.dlc_count > 0) {
        dlc_badge->setText(tr("Дополнений: %1").arg(game.dlc_count));
        dlc_badge->setStyleSheet(QStringLiteral(
            "background: %1; border: none; "
            "border-radius: 4px; padding: 2px 8px; color: %2; font-size: 11px; font-weight: bold;").arg(acc_15, pal.accent));
        dlc_badge->setToolTip(tr("Нажмите для просмотра списка дополнений"));
    } else {
        dlc_badge->setText(tr("Дополнений: 0"));
        dlc_badge->setStyleSheet(QStringLiteral(
            "background: %1; border: none; "
            "border-radius: 4px; padding: 2px 8px; color: %3; font-size: 11px; font-weight: bold;").arg(pal.surface, pal.border, pal.textSecondary));
        dlc_badge->setToolTip(QString());
    }

    if (game.mod_count > 0) {
        mod_badge->setText(tr("+%1 MOD").arg(game.mod_count));
        mod_badge->setStyleSheet(QStringLiteral("background: rgba(168, 85, 247, 0.18); border: none; border-radius: 4px; padding: 2px 8px; color: #C084FC; font-size: 11px; font-weight: bold;"));
        mod_badge->setToolTip(tr("В сборку вшиты модификации / русификатор (%1 MOD)").arg(game.mod_count));
        mod_badge->setVisible(true);
    } else {
        mod_badge->setVisible(false);
    }

    lang_badge->setText(tr("Язык: %1").arg(game.text_langs.isEmpty() ? tr("Multi") : game.text_langs.join(QStringLiteral(", "))));

    version_combo->clear();
    const QString ver_item_text = game.is_recommended
        ? tr("Основная версия (%1) [%2] — Рекомендуемая").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(ext_tag)
        : tr("Основная версия (%1) [%2]").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(ext_tag);
    version_combo->addItem(ver_item_text);

    // Fallback description from TitleDB if site description is not yet fetched
    if (!game.serial_id.isEmpty()) {
        bool ok = false;
        const u64 tid = game.serial_id.toULongLong(&ok, 16);
        if (ok) {
            const auto entry = TitleDB::TitleDatabase::Instance().Lookup(tid);
            if (entry && !entry->description.empty()) {
                description_browser->setHtml(QStringLiteral("<p style='line-height: 1.4; color: #FFFFFF;'>%1</p>")
                    .arg(QString::fromStdString(entry->description)));
                return;
            }
        }
    }

    description_browser->setHtml(tr("<p style='color: #FFFFFF;'>%1<br><br><b>Платформа:</b> Nintendo Switch (CONSOLES)<br><b>Идентификатор:</b> %2<br><b>Размер пакета:</b> %3</p>")
        .arg(tr("Информация о пакете и файлах получена с сервера stormgamesworld.ru."))
        .arg(game.serial_id)
        .arg(LocalizeSizeString(game.size)));
}

bool StormGamesWorldDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == dlc_badge && event->type() == QEvent::MouseButtonRelease) {
        ShowDlcListForCurrentGame();
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

void StormGamesWorldDialog::ShowDlcListForCurrentGame() {
    if (selected_game_index < 0 || selected_game_index >= static_cast<int>(filtered_games.size())) {
        return;
    }
    const auto& game = filtered_games[selected_game_index];
    StormWorldDlcListDialog dlg(this, game);
    dlg.exec();
}

void StormGamesWorldDialog::FetchGameDetails(int game_id) {
    if (is_closing) return;
    if (details_reply) {
        disconnect(details_reply, nullptr, nullptr, nullptr);
        details_reply->abort();
        details_reply->deleteLater();
        details_reply = nullptr;
    }

    const int raw_id = (game_id >= 10000000) ? (game_id - 10000000) : game_id;
    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games?id=%1").arg(raw_id)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/9.5.0 (Windows x64)"));
    details_reply = network_mgr.get(req);
    connect(details_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnGameDetailsReplyFinished);
}

void StormGamesWorldDialog::OnGameDetailsReplyFinished() {
    if (is_closing || !details_reply) return;
    if (details_reply->error() == QNetworkReply::NoError) {
        const QByteArray raw_data = details_reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw_data);
        QJsonObject obj;
        if (doc.isObject()) {
            obj = doc.object();
        } else if (doc.isArray() && !doc.array().isEmpty()) {
            obj = doc.array().first().toObject();
        }

        if (!obj.isEmpty()) {
            const QString desc = obj[QStringLiteral("description")].toString();
            if (!desc.isEmpty()) {
                description_browser->setHtml(QStringLiteral("<p style='line-height: 1.4; color: #FFFFFF;'>%1</p>").arg(desc));
            }
            const qint64 bytes = obj[QStringLiteral("fileSizeBytes")].toVariant().toLongLong();
            if (bytes > 0 && selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
                filtered_games[selected_game_index].file_size_bytes = bytes;
            }

            const QJsonArray dlcs_arr = obj[QStringLiteral("dlcs")].toArray();
            if (!dlcs_arr.isEmpty() && selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
                filtered_games[selected_game_index].dlcs.clear();
                for (const auto& d_val : dlcs_arr) {
                    const QJsonObject d_obj = d_val.toObject();
                    StormWorldDlc dlc;
                    dlc.id = d_obj[QStringLiteral("id")].toString();
                    dlc.name = d_obj[QStringLiteral("name")].toString().trimmed();
                    dlc.description = d_obj[QStringLiteral("description")].toString().trimmed();
                    if (!dlc.name.isEmpty()) {
                        filtered_games[selected_game_index].dlcs.push_back(dlc);
                    }
                }
                if (!filtered_games[selected_game_index].dlcs.isEmpty()) {
                    filtered_games[selected_game_index].dlc_count = filtered_games[selected_game_index].dlcs.size();
                    dlc_badge->setText(tr("Дополнений: %1").arg(filtered_games[selected_game_index].dlc_count));
                }
            }

            const QJsonArray mods_arr = obj[QStringLiteral("mods")].toArray();
            if (!mods_arr.isEmpty() && selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
                filtered_games[selected_game_index].mod_count = std::max(filtered_games[selected_game_index].mod_count, static_cast<int>(mods_arr.size()));
                mod_badge->setText(tr("+%1 MOD").arg(filtered_games[selected_game_index].mod_count));
                mod_badge->setToolTip(tr("В сборку вшиты модификации / русификатор (%1 MOD)").arg(filtered_games[selected_game_index].mod_count));
                mod_badge->setVisible(true);
            }
        }
    }
    if (details_reply) {
        details_reply->deleteLater();
        details_reply = nullptr;
    }
}

void StormGamesWorldDialog::FetchRealExtension(int game_id) {
    if (is_closing) return;
    if (head_reply) {
        disconnect(head_reply, nullptr, nullptr, nullptr);
        head_reply->abort();
        head_reply->deleteLater();
        head_reply = nullptr;
    }

    const int raw_id = (game_id >= 10000000) ? (game_id - 10000000) : game_id;
    QNetworkRequest req(QUrl(QStringLiteral("https://stormgamesworld.ru/api/games/%1/download").arg(raw_id)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("STORM_SWITCH/9.5.0 (Windows x64)"));
    head_reply = network_mgr.head(req);
    connect(head_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnHeadReplyFinished);
}

void StormGamesWorldDialog::OnHeadReplyFinished() {
    if (is_closing || !head_reply) return;

    if (head_reply->error() == QNetworkReply::NoError) {
        const QString disp = QString::fromUtf8(head_reply->rawHeader("Content-Disposition"));
        QString real_ext = QStringLiteral(".nsp");
        QString real_tag = QStringLiteral("NSP");

        if (disp.contains(QLatin1String(".nsz"), Qt::CaseInsensitive)) {
            real_ext = QStringLiteral(".nsz");
            real_tag = QStringLiteral("NSZ");
        } else if (disp.contains(QLatin1String(".xci"), Qt::CaseInsensitive)) {
            real_ext = QStringLiteral(".xci");
            real_tag = QStringLiteral("XCI");
        } else if (disp.contains(QLatin1String(".nsp"), Qt::CaseInsensitive)) {
            real_ext = QStringLiteral(".nsp");
            real_tag = QStringLiteral("NSP");
        }

        if (selected_game_index >= 0 && selected_game_index < static_cast<int>(filtered_games.size())) {
            const QString current_ext = filtered_games[selected_game_index].real_extension;
            if (current_ext.compare(QStringLiteral(".nsz"), Qt::CaseInsensitive) == 0 &&
                real_ext.compare(QStringLiteral(".nsp"), Qt::CaseInsensitive) == 0) {
                // Preserve verified NSZ variant
            } else {
                filtered_games[selected_game_index].real_extension = real_ext;
            }
            const auto& game = filtered_games[selected_game_index];
            QString actual_tag = game.real_extension.isEmpty() ? QStringLiteral("NSP") : game.real_extension;
            actual_tag.remove(QLatin1Char('.'));
            actual_tag = actual_tag.toUpper();
            version_combo->clear();
            version_combo->addItem(tr("Основная версия (%1) [%2]").arg(game.version.isEmpty() ? QStringLiteral("1.0.0") : game.version).arg(actual_tag));
        }
    }

    if (head_reply) {
        head_reply->deleteLater();
        head_reply = nullptr;
    }
}

void StormGamesWorldDialog::LoadCover(const StormWorldGame bitand game) {
    if (is_closing) return;
    if (cover_reply) {
        disconnect(cover_reply, nullptr, nullptr, nullptr);
        cover_reply->abort();
        cover_reply->deleteLater();
        cover_reply = nullptr;
    }

    // Default stylized icon placeholder
    const auto pal = StormTheme::GetThemePalette();
    QPixmap placeholder(148, 148);
    placeholder.fill(QColor(pal.surface));
    QPainter p(bitand placeholder);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(pal.accent));
    p.drawRoundedRect(1, 1, 146, 146, 8, 8);
    p.setPen(QColor(pal.textPrimary));
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(9);
    p.setFont(f);
    p.drawText(QRect(6, 25, 136, 75), Qt::AlignCenter | Qt::TextWordWrap, game.final_title.isEmpty() ? game.title : game.final_title);
    p.setPen(QColor(pal.accent));
    f.setPointSize(8);
    p.setFont(f);
    p.drawText(QRect(6, 115, 136, 25), Qt::AlignCenter, game.serial_id);
    p.end();

    cover_label->setPixmap(placeholder);

    // Normalize Title ID
    QString tid = game.serial_id.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
    if (tid.isEmpty() or tid == QStringLiteral("—")) {
        for (const auto bitand file_item : cached_dir_files) {
            if (!file_item.tid.isEmpty() and (file_item.title.compare(game.title, Qt::CaseInsensitive) == 0 or
                                              file_item.title.compare(game.final_title, Qt::CaseInsensitive) == 0 or
                                              file_item.file_name.compare(game.title, Qt::CaseInsensitive) == 0)) {
                tid = file_item.tid.trimmed().remove(QStringLiteral("0x"), Qt::CaseInsensitive).toUpper();
                break;
            }
        }
    }

    // 1. Check local emulator game_list icon cache FIRST (genuine icon extracted directly from ROM/NCA)
    const auto eden_cache = Common::FS::GetEdenPath(Common::FS::EdenPath::CacheDir);
    const QString game_list_dir = QString::fromStdString(Common::FS::PathToUTF8String(eden_cache / "game_list"));

    if (!tid.isEmpty() and tid != QStringLiteral("—")) {
        const QStringList local_icon_candidates = {
            QDir(game_list_dir).filePath(tid + QStringLiteral(".jpeg")),
            QDir(game_list_dir).filePath(tid.toLower() + QStringLiteral(".jpeg")),
            QDir(game_list_dir).filePath(tid + QStringLiteral(".jpg")),
            QDir(game_list_dir).filePath(tid + QStringLiteral(".png")),
        };
        for (const auto bitand path : local_icon_candidates) {
            if (QFile::exists(path)) {
                QFileInfo fi(path);
                if (fi.size() > 100) {
                    QPixmap local_pix;
                    if (local_pix.load(path)) {
                        cover_label->setPixmap(local_pix.scaled(148, 148, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        return;
                    }
                }
            }
        }
    }

    // 2. Cache directory: covers/switch
    const auto cache_dir = eden_cache / "covers" / "switch";
    const QString cache_dir_q = QString::fromStdString(Common::FS::PathToUTF8String(cache_dir));
    QDir().mkpath(cache_dir_q);

    current_cover_cache_file = QDir(cache_dir_q).filePath(tid.isEmpty() ? QStringLiteral("game_%1.jpg").arg(game.id) : QStringLiteral("%1.jpg").arg(tid));

    // Check local covers cache
    if (QFile::exists(current_cover_cache_file)) {
        QFileInfo fi(current_cover_cache_file);
        if (fi.size() > 500) {
            QPixmap cached_pix;
            if (cached_pix.load(current_cover_cache_file)) {
                cover_label->setPixmap(cached_pix.scaled(148, 148, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                return;
            }
        }
    }

    // If local / homebrew game not in online store: do not query remote CDNs (avoid mismatched covers)
    if (game.id < 0) {
        return;
    }

    // 3. Build multi-source candidate URLs matching STORM SWITCH BOX CoverCacheService
    current_cover_candidates.clear();
    current_cover_candidate_index = 0;

    // S. Storm Switch Verified High-Res CDN Icon (Exact 1:1 match for all known games)
    if (!tid.isEmpty() and tid != QStringLiteral("—")) {
        const QString verified_url = GetVerifiedCoverUrlForTid(tid);
        if (!verified_url.isEmpty()) {
            current_cover_candidates.append(verified_url);
        }
    }

    // A. TitleDB Official Nintendo eShop Icon
    if (!tid.isEmpty() and tid != QStringLiteral("—")) {
        bool ok = false;
        const u64 tid_val = tid.toULongLong(bitand ok, 16);
        if (ok) {
            const auto entry = TitleDB::TitleDatabase::Instance().Lookup(tid_val);
            if (entry and !entry->icon_url.empty()) {
                current_cover_candidates.append(QString::fromStdString(entry->icon_url));
            }
        }
    }

    // B. Storm Games World Cover URL
    if (!game.cover_url.isEmpty()) {
        current_cover_candidates.append(game.cover_url.startsWith(QLatin1String("http"))
            ? game.cover_url
            : QStringLiteral("https://stormgamesworld.ru") + game.cover_url);
    }

    // C. LibRetro Switch Boxart CDN
    const QString raw_title = game.final_title.isEmpty() ? game.title : game.final_title;
    const QString clean_title = SanitizeLibretroName(raw_title);
    if (!clean_title.isEmpty()) {
        const QString esc_title = QString::fromUtf8(QUrl::toPercentEncoding(clean_title));
        current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1.png").arg(esc_title));

        // Base title without parenthesis
        static const QRegularExpression reg(QStringLiteral("\\s*\\([^)]*\\)"));
        const QString base_title = clean_title.split(reg).first().trimmed();
        if (!base_title.isEmpty() and base_title != clean_title) {
            const QString esc_base = QString::fromUtf8(QUrl::toPercentEncoding(base_title));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1%20(USA).png").arg(esc_base));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1%20(World).png").arg(esc_base));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1%20(Europe).png").arg(esc_base));
            current_cover_candidates.append(QStringLiteral("https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_Switch/master/Named_Boxarts/%1.png").arg(esc_base));
        }
    }

    // D. Tinfoil and GameTDB CDNs
    if (!tid.isEmpty() and tid != QStringLiteral("—")) {
        current_cover_candidates.append(QStringLiteral("https://tinfoil.media/repo/db/icons/%1.jpg").arg(tid));
        current_cover_candidates.append(QStringLiteral("https://art.gametdb.com/switch/coverM/US/%1.jpg").arg(tid));
    }

    TryNextCoverCandidate();
}

void StormGamesWorldDialog::TryNextCoverCandidate() {
    if (is_closing) return;
    if (current_cover_candidate_index >= current_cover_candidates.size()) {
        return;
    }

    const QString url_str = current_cover_candidates[current_cover_candidate_index++];
    QNetworkRequest req{QUrl(url_str)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/122.0.0.0 Safari/537.36"));

    cover_reply = network_mgr.get(req);
    connect(cover_reply, &QNetworkReply::finished, this, [this]() {
        if (is_closing || !cover_reply) return;

        if (cover_reply->error() == QNetworkReply::NoError) {
            const QByteArray data = cover_reply->readAll();
            if (data.size() > 300) {
                QPixmap pix;
                if (pix.loadFromData(data)) {
                    QFile out(current_cover_cache_file);
                    if (out.open(QIODevice::WriteOnly)) {
                        out.write(data);
                        out.close();
                    }

                    if (!is_closing && cover_label) {
                        cover_label->setPixmap(pix.scaled(148, 148, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    }
                    cover_reply->deleteLater();
                    cover_reply = nullptr;
                    return;
                }
            }
        }

        cover_reply->deleteLater();
        cover_reply = nullptr;

        if (!is_closing) {
            TryNextCoverCandidate();
        }
    });
}

void StormGamesWorldDialog::OnBrowseDownloadFolder() {
    const QString current = download_dir_edit->text();
    const QString selected = QFileDialog::getExistingDirectory(
        this,
        tr("Выберите папку для сохранения игр"),
        current.isEmpty() ? GetDefaultDownloadDir() : current,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!selected.isEmpty()) {
        download_dir_edit->setText(selected);
        SaveDownloadDir(selected);
        PopulateGameList(search_edit->text());
    }
}

void StormGamesWorldDialog::OnOpenDownloadFolder() {
    const QString dir = download_dir_edit->text();
    if (QDir(dir).exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(GetDefaultDownloadDir()));
    }
}

void StormGamesWorldDialog::OnStartDownload() {
    if (selected_game_index < 0 or selected_game_index >= static_cast<int>(filtered_games.size())) {
        return;
    }

    const auto bitand game = filtered_games[selected_game_index];
    if (game.id < 0) {
        return;
    }

    QString dir_path = download_dir_edit->text().trimmed();
    if (IsAuxiliarySubfolder(dir_path)) {
        dir_path = GetDefaultDownloadDir();
        download_dir_edit->setText(dir_path);
    }
    QDir dir(dir_path);
    if (!dir.exists()) {
        dir.mkpath(dir_path);
    }

    QString base_filename = SanitizeFileName(game.final_title.isEmpty() ? game.title : game.final_title);
    if (base_filename.endsWith(QStringLiteral(".nsp"), Qt::CaseInsensitive) ||
        base_filename.endsWith(QStringLiteral(".nsz"), Qt::CaseInsensitive) ||
        base_filename.endsWith(QStringLiteral(".xci"), Qt::CaseInsensitive) ||
        base_filename.endsWith(QStringLiteral(".xcz"), Qt::CaseInsensitive)) {
        base_filename.chop(4);
    }
    base_filename += (game.real_extension.isEmpty() ? QStringLiteral(".nsp") : game.real_extension);

    current_download_path = dir.filePath(base_filename);
    current_part_path = current_download_path + QStringLiteral(".part");

    // Clean up any 0-byte leftover target file from older versions
    QFileInfo target_fi(current_download_path);
    if (target_fi.exists() && target_fi.size() == 0) {
        QFile::remove(current_download_path);
    }

    QFileInfo part_fi(current_part_path);
    resume_offset_bytes = (part_fi.exists() && part_fi.size() > 0) ? part_fi.size() : 0;
    expected_total_bytes = game.file_size_bytes;

    output_file = std::make_unique<QFile>(current_part_path);
    const QIODevice::OpenMode open_mode =
        (resume_offset_bytes > 0) ? (QIODevice::WriteOnly | QIODevice::Append)
                                  : (QIODevice::WriteOnly | QIODevice::Truncate);
    if (!output_file->open(open_mode)) {
        QMessageBox::warning(this, tr("Ошибка записи файла"),
            tr("Не удалось создать файл для сохранения:\n%1").arg(current_part_path));
        return;
    }

    const int raw_id = (game.id >= 10000000) ? (game.id - 10000000) : game.id;
    QString ext_param = game.real_extension.isEmpty() ? QStringLiteral("nsp") : game.real_extension;
    ext_param.remove(QLatin1Char('.'));
    ext_param = ext_param.toLower();
    const QUrl download_url(QStringLiteral("https://stormgamesworld.ru/api/games/%1/download?format=%2").arg(raw_id).arg(ext_param));
    QNetworkRequest req(download_url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("STORM_SWITCH/%1 (Windows x64)")
                      .arg(QString::fromUtf8(Common::g_build_version)));
    // Use HTTP/1.1 for rock-solid multi-GB binary streaming without HTTP/2 window stalls
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("Connection", "keep-alive");
    req.setRawHeader("Accept-Encoding", "identity");
    if (resume_offset_bytes > 0) {
        req.setRawHeader("Range", QByteArray("bytes=") + QByteArray::number(resume_offset_bytes) + QByteArray("-"));
    }

    QSslConfiguration ssl_cfg = req.sslConfiguration();
    ssl_cfg.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl_cfg);

    is_downloading = true;
    is_paused = false;
    headers_inspected = false;
    current_download_game_id = game.id;
    download_write_buffer.clear();
    download_btn->setEnabled(false);
    pause_btn->setEnabled(true);
    pause_btn->setText(tr("Пауза"));
    pause_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Pause, QColor(245, 158, 11), 16));
    cancel_btn->setEnabled(true);
    refresh_btn->setEnabled(false);
    if (resume_offset_bytes > 0 && expected_total_bytes > 0) {
        progress_bar->setValue(static_cast<int>(std::min<qint64>(99, (resume_offset_bytes * 100) / expected_total_bytes)));
        download_status_label->setText(tr("Возобновление загрузки с %1...").arg(StormFormat::ReadableByteSize(resume_offset_bytes)));
    } else {
        progress_bar->setValue(0);
        download_status_label->setText(tr("Подключение к серверу загрузки..."));
    }

    PopulateGameList(search_edit->text());

    download_timer.start();
    last_received_bytes = 0;
    last_speed_time = 0;
    current_speed_mbps = 0.0;

    download_reply = network_mgr.get(req);
    download_reply->setReadBufferSize(0);
    connect(download_reply, &QNetworkReply::sslErrors, download_reply, [reply = download_reply](const QList<QSslError>&) {
        reply->ignoreSslErrors();
    });
    connect(download_reply, &QNetworkReply::readyRead, this, &StormGamesWorldDialog::OnDownloadDataReady);
    connect(download_reply, &QNetworkReply::downloadProgress, this, &StormGamesWorldDialog::OnDownloadProgress);
    connect(download_reply, &QNetworkReply::finished, this, &StormGamesWorldDialog::OnDownloadReplyFinished);
}

void StormGamesWorldDialog::OnPauseResumeDownload() {
    if (is_downloading && download_reply) {
        is_paused = true;
        download_reply->abort();
    } else if (is_paused) {
        download_retry_count = 0;
        OnStartDownload();
    }
}

void StormGamesWorldDialog::OnDownloadDataReady() {
    if (is_closing || !download_reply || !output_file || !output_file->isOpen()) return;

    if (!headers_inspected) {
        headers_inspected = true;
        const int http_code = download_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (http_code == 200 && resume_offset_bytes > 0) {
            resume_offset_bytes = 0;
            output_file->close();
            std::ignore = output_file->open(QIODevice::WriteOnly | QIODevice::Truncate);
        }
        const QString disp = QString::fromUtf8(download_reply->rawHeader("Content-Disposition")).toLower();
        QString real_ext;
        if (disp.contains(QStringLiteral(".nsz"))) real_ext = QStringLiteral(".nsz");
        else if (disp.contains(QStringLiteral(".xcz"))) real_ext = QStringLiteral(".xcz");
        else if (disp.contains(QStringLiteral(".xci"))) real_ext = QStringLiteral(".xci");
        else if (disp.contains(QStringLiteral(".nsp"))) real_ext = QStringLiteral(".nsp");
        if (!real_ext.isEmpty() && !current_download_path.endsWith(real_ext, Qt::CaseInsensitive)) {
            if (current_download_path.endsWith(QStringLiteral(".nsp"), Qt::CaseInsensitive) ||
                current_download_path.endsWith(QStringLiteral(".nsz"), Qt::CaseInsensitive) ||
                current_download_path.endsWith(QStringLiteral(".xci"), Qt::CaseInsensitive) ||
                current_download_path.endsWith(QStringLiteral(".xcz"), Qt::CaseInsensitive)) {
                current_download_path.chop(4);
            }
            current_download_path += real_ext;
        }
    }

    download_write_buffer.append(download_reply->readAll());
    if (download_write_buffer.size() >= 1024 * 1024) {
        output_file->write(download_write_buffer);
        download_write_buffer.clear();
    }
}

static QString FormatRussianNumber(double value, int precision = 1) {
    QLocale ru(QLocale::Russian, QLocale::Russia);
    ru.setNumberOptions(QLocale::DefaultNumberOptions);
    QString str = ru.toString(value, 'f', precision);
    str.replace(QChar(0x00A0), QLatin1Char(' '));
    str.replace(QChar(0x202F), QLatin1Char(' '));
    return str;
}

void StormGamesWorldDialog::OnDownloadProgress(qint64 received, qint64 total) {
    if (is_closing) return;

    const qint64 full_received = resume_offset_bytes + received;
    const qint64 full_total = (total > 0) ? (resume_offset_bytes + total) : expected_total_bytes;
    if (full_total > expected_total_bytes) {
        expected_total_bytes = full_total;
    }

    int pct = 0;
    if (full_total > 0) {
        pct = static_cast<int>(std::clamp<qint64>((full_received * 100) / full_total, 0, 100));
        progress_bar->setValue(pct);
    }

    const qint64 elapsed = download_timer.elapsed();
    if (elapsed - last_speed_time > 400) {
        const qint64 bytes_delta = received - last_received_bytes;
        const qint64 time_delta = elapsed - last_speed_time;
        if (time_delta > 0 && bytes_delta >= 0) {
            current_speed_mbps = (static_cast<double>(bytes_delta) / (1024.0 * 1024.0)) / (static_cast<double>(time_delta) / 1000.0);
        }
        last_received_bytes = received;
        last_speed_time = elapsed;
    }

    const double rec_mb = static_cast<double>(full_received) / (1024.0 * 1024.0);
    const double tot_mb = full_total > 0 ? (static_cast<double>(full_total) / (1024.0 * 1024.0)) : 0.0;

    QString status;
    if (full_total > 0) {
        const double rem_mb = std::max(0.0, tot_mb - rec_mb);
        const int eta_sec = (current_speed_mbps > 0.05) ? static_cast<int>(rem_mb / current_speed_mbps) : 0;
        const int eta_min = eta_sec / 60;
        const int eta_rem = eta_sec % 60;

        const QString line1 = tr("Скачано: %1 МБ из %2 МБ (%3%)")
            .arg(FormatRussianNumber(rec_mb, 1))
            .arg(FormatRussianNumber(tot_mb, 1))
            .arg(pct);
        const QString line2 = tr("Скорость: %1 МБ/с • Осталось: %2 мин %3 сек")
            .arg(FormatRussianNumber(current_speed_mbps, 1))
            .arg(eta_min)
            .arg(eta_rem);
        status = QStringLiteral("%1\n%2").arg(line1, line2);
    } else {
        const QString line1 = tr("Скачано: %1 МБ").arg(FormatRussianNumber(rec_mb, 1));
        const QString line2 = tr("Скорость: %1 МБ/с").arg(FormatRussianNumber(current_speed_mbps, 1));
        status = QStringLiteral("%1\n%2").arg(line1, line2);
    }

    download_status_label->setText(status);
}

void StormGamesWorldDialog::OnCancelDownload() {
    is_paused = false;
    download_retry_count = 0;
    if (download_reply) {
        download_reply->abort();
    } else {
        if (!current_part_path.isEmpty() && QFile::exists(current_part_path)) {
            QFile::remove(current_part_path);
        }
        pause_btn->setEnabled(false);
        pause_btn->setText(tr("Пауза"));
    pause_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Pause, QColor(245, 158, 11), 16));
        cancel_btn->setEnabled(false);
        progress_bar->setValue(0);
        download_status_label->setText(tr("Загрузка отменена и временный файл удален"));
        OnGameSelectionChanged();
    }
    download_write_buffer.clear();
}

void StormGamesWorldDialog::OnDownloadReplyFinished() {
    is_downloading = false;

    if (output_file && output_file->isOpen()) {
        if (!download_write_buffer.isEmpty()) {
            output_file->write(download_write_buffer);
            download_write_buffer.clear();
        }
        output_file->flush();
        output_file->close();
    }

    if (is_closing) {
        if (download_reply) {
            disconnect(download_reply, nullptr, nullptr, nullptr);
            download_reply->deleteLater();
            download_reply = nullptr;
        }
        return;
    }

    if (!download_reply) return;

    const auto err = download_reply->error();
    const auto err_str = download_reply->errorString();
    const int http_code = download_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    download_reply->deleteLater();
    download_reply = nullptr;

    const qint64 part_size = QFileInfo(current_part_path).exists() ? QFileInfo(current_part_path).size() : 0;

    if (is_paused) {
        pause_btn->setEnabled(true);
        pause_btn->setText(tr("Продолжить"));
        pause_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Play, QColor(16, 185, 129), 16));
        download_btn->setEnabled(true);
        download_btn->setText(tr("Докачать игру"));
        download_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Play, Qt::white, 16));
        cancel_btn->setEnabled(true);
        refresh_btn->setEnabled(true);
        const double part_mb = static_cast<double>(part_size) / (1024.0 * 1024.0);
        download_status_label->setText(tr("Загрузка на паузе (сохранено %1). Нажмите «Продолжить» для докачки.").arg(StormFormat::ReadableByteSize(part_size)));
        PopulateGameList(search_edit->text());
        return;
    }

    current_download_game_id = -1;
    pause_btn->setEnabled(false);
    pause_btn->setText(tr("Пауза"));
    pause_btn->setIcon(MakeDialogVectorIcon(DialogIcon::Pause, QColor(245, 158, 11), 16));
    cancel_btn->setEnabled(false);
    download_btn->setEnabled(true);
    refresh_btn->setEnabled(true);

    if (err == QNetworkReply::OperationCanceledError) {
        download_status_label->setText(tr("Загрузка отменена пользователем"));
        if (output_file) {
            output_file->remove();
        }
        if (!current_part_path.isEmpty() && QFile::exists(current_part_path)) {
            QFile::remove(current_part_path);
        }
        progress_bar->setValue(0);
    } else if ((err != QNetworkReply::NoError && http_code != 416) ||
               (expected_total_bytes > 0 && part_size < expected_total_bytes && http_code != 416)) {
        if (http_code != 404 && download_retry_count < 5 && part_size > 0) {
            ++download_retry_count;
            const int delay_sec = std::min(10, download_retry_count * 2);
            download_status_label->setText(
                tr("Сбой соединения (%1). Авто-докачка (попытка %2/5) через %3 сек...")
                    .arg(err_str).arg(download_retry_count).arg(delay_sec));
            QTimer::singleShot(delay_sec * 1000, this, [this]() {
                if (!is_closing && !is_downloading && !is_paused) {
                    OnStartDownload();
                }
            });
            return;
        }
        download_retry_count = 0;
        download_status_label->setText(tr("Ошибка загрузки: %1 (прогресс сохранен для докачки)").arg(err_str));
        QMessageBox::critical(this, tr("Сбой скачивания"),
            tr("Произошла ошибка при загрузке игры с сервера:\n%1\n\nСкачанная часть сохранена — вы можете продолжить докачку в любой момент.").arg(err_str));
    } else {
        download_retry_count = 0;
        if (QFile::exists(current_download_path)) {
            QFile::remove(current_download_path);
        }
        QFile::rename(current_part_path, current_download_path);
        cached_dir_scan_time = 0;
        progress_bar->setValue(100);
        download_status_label->setText(tr("Загрузка успешно завершена! Файл сохранен в: %1").arg(current_download_path));

        emit GameDownloaded(current_download_path);

        QMessageBox::information(this, tr("Загрузка завершена"),
            tr("Игра успешно скачана в папку:\n%1\n\nВы можете запустить её прямо сейчас или открыть папку с файлом.")
            .arg(current_download_path));
    }

    PopulateGameList(search_edit->text());
}
