/*
 * Copyright 2018  Michail Vourlakos <mvourlakos@gmail.com>
 *
 * This file is part of Latte-Dock
 *
 * Latte-Dock is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Latte-Dock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "theme.h"

// local
#include "lattecorona.h"
#include "panelbackground.h"
#include "../../layouts/importer.h"
#include "../../view/panelshadows_p.h"
#include "../../wm/schemecolors.h"
#include "../../tools/commontools.h"

// Qt
#include <QDebug>
#include <QDir>
#include <QProcess>

// KDE
#include <KDirWatch>
#include <KConfigGroup>
#include <KSharedConfig>

// X11
#include <KWindowSystem>

#define DEFAULTCOLORSCHEME "default.colors"
#define REVERSEDCOLORSCHEME "reversed.colors"

namespace Latte {
namespace PlasmaExtended {

Theme::Theme(KSharedConfig::Ptr config, QObject *parent) :
    QObject(parent),
    m_themeGroup(KConfigGroup(config, QStringLiteral("PlasmaThemeExtended"))),
    m_backgroundTopEdge(new PanelBackground(Plasma::Types::TopEdge, this)),
    m_backgroundLeftEdge(new PanelBackground(Plasma::Types::LeftEdge, this)),
    m_backgroundBottomEdge(new PanelBackground(Plasma::Types::BottomEdge, this)),
    m_backgroundRightEdge(new PanelBackground(Plasma::Types::RightEdge, this))
{
    qmlRegisterTypes();

    m_corona = qobject_cast<Latte::Corona *>(parent);

    //! compositing tracking
    if (KWindowSystem::isPlatformWayland()) {
        //! TODO: Wayland compositing active
        m_compositing = true;
    } else {
        connect(KWindowSystem::self(), &KWindowSystem::compositingChanged
                , this, [&](bool enabled) {
            if (m_compositing == enabled)
                return;

            m_compositing = enabled;
            emit compositingChanged();
        });

        m_compositing = KWindowSystem::compositingActive();
    }
    //!

    loadConfig();

    connect(this, &Theme::compositingChanged, this, &Theme::updateBackgrounds);
    connect(this, &Theme::outlineWidthChanged, this, &Theme::saveConfig);

    connect(&m_theme, &Plasma::Theme::themeChanged, this, &Theme::hasShadowChanged);
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &Theme::load);
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &Theme::themeChanged);
}

void Theme::load()
{
    loadThemePaths();
    updateBackgrounds();
}

Theme::~Theme()
{
    saveConfig();

    m_defaultScheme->deleteLater();
    m_reversedScheme->deleteLater();
}

bool Theme::hasShadow() const
{
    return PanelShadows::self()->hasShadows();
}

bool Theme::isLightTheme() const
{
    return m_isLightTheme;
}

bool Theme::isDarkTheme() const
{
    return !m_isLightTheme;
}

int Theme::outlineWidth() const
{
    return m_outlineWidth;
}

void Theme::setOutlineWidth(int width)
{
    if (m_outlineWidth == width) {
        return;
    }

    m_outlineWidth = width;
    emit outlineWidthChanged();
}

PanelBackground *Theme::backgroundTopEdge() const
{
    return m_backgroundTopEdge;
}

PanelBackground *Theme::backgroundLeftEdge() const
{
    return m_backgroundLeftEdge;
}

PanelBackground *Theme::backgroundBottomEdge() const
{
    return m_backgroundBottomEdge;
}

PanelBackground *Theme::backgroundRightEdge() const
{
    return m_backgroundRightEdge;
}

WindowSystem::SchemeColors *Theme::defaultTheme() const
{
    return m_defaultScheme;
}

WindowSystem::SchemeColors *Theme::lightTheme() const
{
    return m_isLightTheme ? m_defaultScheme : m_reversedScheme;
}

WindowSystem::SchemeColors *Theme::darkTheme() const
{
    return !m_isLightTheme ? m_defaultScheme : m_reversedScheme;
}


void Theme::setOriginalSchemeFile(const QString &file)
{
    if (m_originalSchemePath == file) {
        return;
    }

    m_originalSchemePath = file;

    qDebug() << "plasma theme original colors ::: " << m_originalSchemePath;

    updateDefaultScheme();
    updateReversedScheme();

    loadThemeLightness();

    emit themeChanged();
}

//! WM records need to be updated based on the colors that
//! plasma will use in order to be consistent. Such an example
//! are the Breeze color schemes that have different values for
//! WM and the plasma theme records
void Theme::updateDefaultScheme()
{
    QString defaultFilePath = m_extendedThemeDir.path() + "/" + DEFAULTCOLORSCHEME;
    if (QFileInfo(defaultFilePath).exists()) {
        QFile(defaultFilePath).remove();
    }

    QFile(m_originalSchemePath).copy(defaultFilePath);
    m_defaultSchemePath = defaultFilePath;

    updateDefaultSchemeValues();

    if (m_defaultScheme) {
        disconnect(m_defaultScheme, &WindowSystem::SchemeColors::colorsChanged, this, &Theme::loadThemeLightness);
        m_defaultScheme->deleteLater();
    }

    m_defaultScheme = new WindowSystem::SchemeColors(this, m_defaultSchemePath, true);
    connect(m_defaultScheme, &WindowSystem::SchemeColors::colorsChanged, this, &Theme::loadThemeLightness);

    qDebug() << "plasma theme default colors ::: " << m_defaultSchemePath;
}

void Theme::updateDefaultSchemeValues()
{
    //! update WM values based on original scheme
    KSharedConfigPtr originalPtr = KSharedConfig::openConfig(m_originalSchemePath);
    KSharedConfigPtr defaultPtr = KSharedConfig::openConfig(m_defaultSchemePath);

    if (originalPtr && defaultPtr) {
        KConfigGroup normalWindowGroup(originalPtr, "Colors:Window");
        KConfigGroup defaultWMGroup(defaultPtr, "WM");

        defaultWMGroup.writeEntry("activeBackground", normalWindowGroup.readEntry("BackgroundNormal", QColor()));
        defaultWMGroup.writeEntry("activeForeground", normalWindowGroup.readEntry("ForegroundNormal", QColor()));

        defaultWMGroup.sync();
    }
}

void Theme::updateReversedScheme()
{
    QString reversedFilePath = m_extendedThemeDir.path() + "/" + REVERSEDCOLORSCHEME;

    if (QFileInfo(reversedFilePath).exists()) {
        QFile(reversedFilePath).remove();
    }

    QFile(m_originalSchemePath).copy(reversedFilePath);
    m_reversedSchemePath = reversedFilePath;

    updateReversedSchemeValues();

    if (m_reversedScheme) {
        m_reversedScheme->deleteLater();
    }

    m_reversedScheme = new WindowSystem::SchemeColors(this, m_reversedSchemePath, true);

    qDebug() << "plasma theme reversed colors ::: " << m_reversedSchemePath;
}

void Theme::updateReversedSchemeValues()
{
    //! reverse values based on original scheme
    KSharedConfigPtr originalPtr = KSharedConfig::openConfig(m_originalSchemePath);
    KSharedConfigPtr reversedPtr = KSharedConfig::openConfig(m_reversedSchemePath);

    if (originalPtr && reversedPtr) {
        for (const auto &groupName : reversedPtr->groupList()) {
            if (groupName != "Colors:Button" && groupName != "Colors:Selection") {
                KConfigGroup reversedGroup(reversedPtr, groupName);

                if (reversedGroup.keyList().contains("BackgroundNormal")
                        && reversedGroup.keyList().contains("ForegroundNormal")) {
                    //! reverse usual text/background values
                    KConfigGroup originalGroup(originalPtr, groupName);

                    reversedGroup.writeEntry("BackgroundNormal", originalGroup.readEntry("ForegroundNormal", QColor()));
                    reversedGroup.writeEntry("ForegroundNormal", originalGroup.readEntry("BackgroundNormal", QColor()));

                    reversedGroup.sync();
                }
            }
        }

        //! update WM group
        KConfigGroup reversedWMGroup(reversedPtr, "WM");
        KConfigGroup normalWindowGroup(originalPtr, "Colors:Window");

        if (reversedWMGroup.keyList().contains("activeBackground")
                && reversedWMGroup.keyList().contains("activeForeground")
                && reversedWMGroup.keyList().contains("inactiveBackground")
                && reversedWMGroup.keyList().contains("inactiveForeground")) {
            //! reverse usual wm titlebar values
            KConfigGroup originalGroup(originalPtr, "WM");
            reversedWMGroup.writeEntry("activeBackground", normalWindowGroup.readEntry("ForegroundNormal", QColor()));
            reversedWMGroup.writeEntry("activeForeground", normalWindowGroup.readEntry("BackgroundNormal", QColor()));
            reversedWMGroup.writeEntry("inactiveBackground", originalGroup.readEntry("inactiveForeground", QColor()));
            reversedWMGroup.writeEntry("inactiveForeground", originalGroup.readEntry("inactiveBackground", QColor()));
            reversedWMGroup.sync();
        }

        if (reversedWMGroup.keyList().contains("activeBlend")
                && reversedWMGroup.keyList().contains("inactiveBlend")) {
            KConfigGroup originalGroup(originalPtr, "WM");
            reversedWMGroup.writeEntry("activeBlend", originalGroup.readEntry("inactiveBlend", QColor()));
            reversedWMGroup.writeEntry("inactiveBlend", originalGroup.readEntry("activeBlend", QColor()));
            reversedWMGroup.sync();
        }

        //! update scheme name
        QString originalSchemeName = WindowSystem::SchemeColors::schemeName(m_originalSchemePath);
        KConfigGroup generalGroup(reversedPtr, "General");
        generalGroup.writeEntry("Name", originalSchemeName + "_reversed");
        generalGroup.sync();
    }
}

void Theme::updateBackgrounds()
{
    m_backgroundTopEdge->update();
    m_backgroundLeftEdge->update();
    m_backgroundBottomEdge->update();
    m_backgroundRightEdge->update();
}

void Theme::loadThemePaths()
{
    m_themePath = Layouts::Importer::standardPath("plasma/desktoptheme/" + m_theme.themeName());

    if (QDir(m_themePath+"/widgets").exists()) {
        m_themeWidgetsPath = m_themePath + "/widgets";
    } else {
        m_themeWidgetsPath = Layouts::Importer::standardPath("plasma/desktoptheme/default/widgets");
    }

    qDebug() << "current plasma theme ::: " << m_theme.themeName();
    qDebug() << "theme path ::: " << m_themePath;
    qDebug() << "theme widgets path ::: " << m_themeWidgetsPath;

    //! clear kde connections
    for (auto &c : m_kdeConnections) {
        disconnect(c);
    }

    //! assign color schemes
    QString themeColorScheme = m_themePath + "/colors";

    if (QFileInfo(themeColorScheme).exists()) {
        setOriginalSchemeFile(themeColorScheme);
    } else {
        //! when plasma theme uses the kde colors
        //! we track when kde color scheme is changing
        QString kdeSettingsFile = QDir::homePath() + "/.config/kdeglobals";

        KDirWatch::self()->addFile(kdeSettingsFile);

        m_kdeConnections[0] = connect(KDirWatch::self(), &KDirWatch::dirty, this, [ &, kdeSettingsFile](const QString & path) {
            if (path == kdeSettingsFile) {
                this->setOriginalSchemeFile(WindowSystem::SchemeColors::possibleSchemeFile("kdeglobals"));
            }
        });

        m_kdeConnections[1] = connect(KDirWatch::self(), &KDirWatch::created, this, [ &, kdeSettingsFile](const QString & path) {
            if (path == kdeSettingsFile) {
                this->setOriginalSchemeFile(WindowSystem::SchemeColors::possibleSchemeFile("kdeglobals"));
            }
        });

        setOriginalSchemeFile(WindowSystem::SchemeColors::possibleSchemeFile("kdeglobals"));
    }
}

void Theme::loadThemeLightness()
{
    float textColorLum = Latte::colorLumina(m_defaultScheme->textColor());
    float backColorLum = Latte::colorLumina(m_defaultScheme->backgroundColor());

    if (backColorLum > textColorLum) {
        m_isLightTheme = true;
    } else {
        m_isLightTheme = false;
    }

    if (m_isLightTheme) {
        qDebug() << "Plasma theme is light...";
    } else {
        qDebug() << "Plasma theme is dark...";
    }
}

void Theme::loadConfig()
{
    setOutlineWidth(m_themeGroup.readEntry("outlineWidth", 1));
}

void Theme::saveConfig()
{
    m_themeGroup.writeEntry("outlineWidth", m_outlineWidth);
}

void Theme::qmlRegisterTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    qmlRegisterType<Latte::PlasmaExtended::Theme>();
    qmlRegisterType<Latte::PlasmaExtended::PanelBackground>();
#else
    qmlRegisterAnonymousType<Latte::PlasmaExtended::Theme>("latte-dock", 1);
    qmlRegisterAnonymousType<Latte::PlasmaExtended::PanelBackground>("latte-dock", 1);
#endif
}

}
}
