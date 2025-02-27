// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CHAMELEONBUTTON_H
#define CHAMELEONBUTTON_H

#include <KDecoration2/DecorationButton>
#include "chameleonsplitmenu.h"

class ChameleonButton : KDecoration2::DecorationButton
{
    Q_OBJECT
public:
    explicit ChameleonButton(KDecoration2::DecorationButtonType type, const QPointer<KDecoration2::Decoration> &decoration, QObject *parent = nullptr);
    virtual ~ChameleonButton();

    static DecorationButton *create(KDecoration2::DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent);

    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

protected slots:
    void onCompositorChanged(bool);

protected:
    void paint(QPainter *painter, const QRect &repaintRegion) override;

    KDecoration2::DecorationButtonType m_type;

    ChameleonSplitMenu *m_pSplitMenu = nullptr;
    QTimer *max_hover_timer = nullptr;

    QColor m_backgroundColor;
    KWin::EffectWindow *effect = nullptr;
    QTimer *max_timer = nullptr;
    bool m_isMaxAvailble = true;
};

#endif // CHAMELEONBUTTON_H
