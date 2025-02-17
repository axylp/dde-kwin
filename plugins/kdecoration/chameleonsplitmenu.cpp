// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chameleonsplitmenu.h"
#include <QEventLoop>
#include <QVBoxLayout>
#include <QDebug>
#include <QPainter>
#include <QBitmap>
#include <QPainterPath>
#include <QtMath>
#include <QTimer>
#include "kwinutils.h"
#include <QToolTip>
#include <QTranslator>
#include <QX11Info>

Q_LOGGING_CATEGORY(SPLIT_MENU, "kwin.splitmenu", QtCriticalMsg);

ChameleonSplitMenu::ChameleonSplitMenu(QWidget *parent) : QWidget (parent)
{
    if (QX11Info::isPlatformX11()) {
        setWindowFlags(Qt::X11BypassWindowManagerHint);
    } else {
        setWindowFlags(Qt::Popup | Qt::X11BypassWindowManagerHint);
    }
    setAttribute(Qt::WA_TranslucentBackground);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(5);

    llabel = new QLabel(this);
    llabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/left_split_normal.svg); background-repeat:no-repeat;background-position:center;");

    clabel = new QLabel(this);
    clabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/right_split_normal.svg); background-repeat:no-repeat;background-position:center;");

    rlabel = new QLabel(this);
    rlabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/max_split_normal.svg); background-repeat:no-repeat;background-position:center;");

    layout->addWidget(llabel);
    layout->addWidget(clabel);
    layout->addWidget(rlabel);
    llabel->installEventFilter(this);
    clabel->installEventFilter(this);
    rlabel->installEventFilter(this);
    layout->setContentsMargins(7, 14, 7, 2);
    setLayout(layout);

    shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 0);
    shadow->setColor(Qt::gray);
    shadow->setBlurRadius(10);
    this->setGraphicsEffect(shadow);

    QString qm = QString(":/splitmenu/translations/splitmenu_%1.qm").arg(QLocale::system().name());
    auto tran = new QTranslator(this);
    if (tran->load(qm)) {
        qApp->installTranslator(tran);
    } else {
        qCDebug(SPLIT_MENU) << "load " << qm << "failed";
    }
}

ChameleonSplitMenu::~ChameleonSplitMenu()
{
    if (tip_timer) {
        delete tip_timer;
        tip_timer = nullptr;
    }
    if (shadow) {
        delete shadow;
        shadow = nullptr;
    }
}

void ChameleonSplitMenu::enterEvent(QEvent *e)
{
    QWidget::enterEvent(e);
    stopTime();
}

void ChameleonSplitMenu::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    if (!m_MenuSt)
        startTime();
}

void ChameleonSplitMenu::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    QColor col_menu = m_color;
    painter.setBrush(QBrush(col_menu));

    QPainterPath painterPath;
    painterPath.addRoundedRect(QRect(5, 20, width() - 10, height() - 30), 14, 14);
    painter.drawPath(painterPath);

    int spot = (width() / 2) + 12;
    double w = M_PI / 25;
    int fx = spot -20;
    QPainterPath wave;
    wave.moveTo(fx, 20);
    for (int x = 0; x <= 50; x+=1) {
        double waveY = (double)(9 * sin(w * x + fx)) + 11;
        wave.lineTo(x + fx, waveY);
    }
    wave.lineTo(spot + 30, 20);
    painter.setBrush(QBrush(col_menu));
    painter.drawPath(wave);
}

bool ChameleonSplitMenu::eventFilter(QObject *obj, QEvent *event)
{
    QString str = "light";
    if (m_isDark) {
        str = "dark";
    }

    if (obj == llabel) {
        if (event->type() == QEvent::MouseButtonRelease) {
            llabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/left_split_hover.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            if (m_client) {
                KWinUtils::Window::setQuikTileMode(m_client, (int)QuickTileFlag::Left, true);
            }
            llabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/left_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            Hide();
        } else if (event->type() == QEvent::Enter) {
            llabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/left_split_hover.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            QPoint pos = m_pos;
            pos.setX(m_pos.x() - 70);
            pos.setY(m_pos.y() + 50);
            QToolTip::showText(pos, tr("Tile window to left of screen"));
        } else if (event->type() == QEvent::Leave) {
            llabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/left_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            QToolTip::hideText();
        }
        return false;
    } else if (obj == clabel) {
        if (event->type() == QEvent::MouseButtonRelease) {
            clabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/right_split_hover.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            if (m_client) {
                KWinUtils::Window::setQuikTileMode(m_client, (int)QuickTileFlag::Right, true);
            }
            clabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/right_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            Hide();
        } else if (event->type() == QEvent::Enter) {
            clabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/right_split_hover.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            QPoint pos = m_pos;
            pos.setX(m_pos.x() - 20);
            pos.setY(m_pos.y() + 50);
            QToolTip::showText(pos, tr("Tile window to right of screen"));
        } else if (event->type() == QEvent::Leave) {
            clabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/right_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str));
            QToolTip::hideText();
        }
        return false;
    } else if (obj == rlabel) {
        QString icon = "max";
        if (KWinUtils::Window::isFullMaximized(m_client)) {
            icon = "restore";
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            rlabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/%2_split_hover.svg); background-repeat:no-repeat;background-position:center;").arg(str).arg(icon));
            if (m_client) {
                KWinUtils::Window::setQuikTileMode(m_client, (int)QuickTileFlag::Maximize);
            }
            rlabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/%2_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str).arg(icon));
            Hide();
        } else if (event->type() == QEvent::Enter) {
            rlabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/%2_split_hover.svg); background-repeat:no-repeat;background-position:center;").arg(str).arg(icon));
            QPoint pos = m_pos;
            pos.setX(m_pos.x() + 30);
            pos.setY(m_pos.y() + 50);
            if (KWinUtils::Window::isFullMaximized(m_client)) {
                QToolTip::showText(pos, tr("Unmaximize"));
            } else {
                QToolTip::showText(pos, tr("Maximize"));
            }
        } else if (event->type() == QEvent::Leave) {
            rlabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/%2_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str).arg(icon));
            QToolTip::hideText();
        }
        return false;
    }

    return QWidget::eventFilter(obj, event);
}

void ChameleonSplitMenu::CheckTheme()
{

    QColor clr(255, 255, 255, 255);
    if (m_color == clr) {
        m_isDark = false;
    } else {
        m_isDark = true;
    }
}

void ChameleonSplitMenu::Show(QPoint pos, QColor color)
{
    if (m_isShow || !KWinUtils::Window::checkClientAllowToSplit(m_client))
        return;
    m_isShow = true;
    m_pos = pos;
    m_color = color;
    CheckTheme();
    pos.setX(m_pos.x() -75);
    QRect rect(pos, QSize(158, 85));
    setGeometry(rect);
    if (m_client) {
        QString str = "light";
        if (m_isDark) {
            str = "dark";
        }
        QString icon = "max";
        if (KWinUtils::Window::isFullMaximized(m_client)) {
            icon = "restore";
        }
        rlabel->setStyleSheet(QString("background-image:url(:/deepin/themes/deepin/%1/icons/%2_split_normal.svg); background-repeat:no-repeat;background-position:center;").arg(str).arg(icon));
    }

    show();
}

void ChameleonSplitMenu::Hide()
{
    m_isShow = false;
    hide();
}

bool ChameleonSplitMenu::isShow()
{
    return m_isShow;
}

void ChameleonSplitMenu::setShowSt(bool flag)
{
    m_MenuSt = flag;
}

void ChameleonSplitMenu::setEffect(WId id)
{
    m_client = KWinUtils::findClient(KWinUtils::Predicate::WindowMatch, id);
}

void ChameleonSplitMenu::startTime()
{
    if (!tip_timer) {
        tip_timer = new QTimer();
        tip_timer->setSingleShot(true);
        connect(tip_timer, &QTimer::timeout, [this] {
            Hide();
        });
        tip_timer->start(300);
    } else {
        tip_timer->start(300);
    }
}

void ChameleonSplitMenu::stopTime()
{
    if (tip_timer) {

        tip_timer->stop();
    }
}
